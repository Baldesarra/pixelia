// Diagnose-Schnittstelle über die serielle Konsole (UART0, 115200 Baud) und per WLAN (POST /api/cmd).
// Eine Zeile = ein Befehl. Gedacht für tools/monitor.ps1 -Send bzw. tools/netlog.ps1 -Send, funktioniert aber auch
// im Terminal. Befehle aus dem Netz laufen in derselben Task wie serielle (nacheinander, gleicher Stack), ihre
// Log-Zeilen gehen als Antwort zurück. Gefährliche Befehle (Liste kDangerous) brauchen dort eine Bestätigung.
// WLAN-Passwörter lassen sich über die Konsole bewusst nicht eintragen.

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <sys/stat.h>
#include <new>
#include <string>

#include "app.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/place_state.h"
#include "pixelia/engine/figure_art.h"
#include "pixelia/engine/stage.h"
#include "board/backlight.h"
#include "diag.h"
#include "input.h"
#include "logbuf.h"
#include "net.h"
#include "ota.h"
#include "settings.h"
#include "ui.h"
#include "web.h"
#include "driver/uart.h"
#include "esp_async_memcpy.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "github.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace app::console {

namespace {
constexpr const char* TAG = "cmd";
constexpr uart_port_t kUart = UART_NUM_0;
constexpr size_t kCaptureBytes = 16 * 1024;

// Befehle, die über das Netz nur mit Bestätigung laufen (sub = zweites Wort, nullptr = egal)
struct Danger {
    const char* cmd;
    const char* sub;
    const char* why;
};
constexpr Danger kDangerous[] = {
    {"reboot", nullptr, "startet Pixelia neu"},
    {"wifi", "forget", "löscht ein gespeichertes WLAN (ohne WLAN bleibt nur das Setup-WLAN am Gerät)"},
    {"wifi", "setup", "trennt Pixelia vom Heimnetz und startet den Setup-Modus"},
    {"wifi", "connect", "trennt die WLAN-Verbindung kurz"},
    {"flash", nullptr, "schreibt viele Sekunden lang in LittleFS (Flash-Last)"},
    {"pclk", nullptr, "stellt den Pixeltakt um, das Bild kann ausfallen"},
    {"timing", nullptr, "stellt das Panel-Timing um, das Bild kann ausfallen"},
    {"pol", nullptr, "stellt die Panel-Polaritäten um, das Bild kann ausfallen"},
    {"bounce", nullptr, "legt das Panel neu an"},
    {"clk", nullptr, "stellt die Taktquelle des Panels um"},
    {"buf", nullptr, "legt die Engine-Puffer neu an"},
    {"touchrst", nullptr, "setzt den Touch-Controller zurück"},
    {"log", "clear", "löscht den Log-Puffer"},
    {"coredump", "erase", "löscht den Absturzbericht"},
};

struct Job {
    std::string line;
    std::string out;
    bool known = false;
    SemaphoreHandle_t done = nullptr;
    ~Job() {
        if (done) vSemaphoreDelete(done);
    }
};
QueueHandle_t s_jobs = nullptr;            // std::shared_ptr<Job>* aus run_remote()
SemaphoreHandle_t s_remote_lock = nullptr;  // immer nur ein Befehl aus dem Netz gleichzeitig
bool s_remote = false;                      // gerade läuft ein Befehl aus dem Netz (nur Konsolen-Task)

const char* danger_of(const char* line) {
    char copy[64];
    std::snprintf(copy, sizeof(copy), "%s", line);
    char* save = nullptr;
    const char* cmd = strtok_r(copy, " \t", &save);
    const char* sub = strtok_r(nullptr, " \t", &save);
    if (!cmd) return nullptr;
    for (const Danger& d : kDangerous) {
        if (std::strcmp(cmd, d.cmd) != 0) continue;
        if (!d.sub || (sub && std::strcmp(sub, d.sub) == 0)) return d.why;
    }
    return nullptr;
}

void print_help() {
    ESP_LOGI(TAG, "Befehle:");
    ESP_LOGI(TAG, "  info | heap | scan | touch | touchrst | reboot");
    ESP_LOGI(TAG, "  log | log bench [n] | log clear | tasks | resets   Log-Puffer, Tasks, Reset-Historie");
    ESP_LOGI(TAG, "  coredump | coredump erase                      Absturzbericht zeigen bzw. löschen");
    ESP_LOGI(TAG, "  rot <0|180> | set | set bright <1-100>           Bild drehen, Einstellungen (gespeichert)");
    ESP_LOGI(TAG, "  set lp <1000-10000>                            Dauer des langen Drucks für die Info-Karte (ms)");
    ESP_LOGI(TAG, "  card | card off                                Info-Karte zeigen bzw. schließen");
    ESP_LOGI(TAG, "  wifi | wifi scan | wifi list | wifi forget <Name> | wifi setup | wifi connect");
    ESP_LOGI(TAG, "  ota | ntp | web | toast <Text>                 Update-Zustand, Uhrzeit, Webserver, Kurzmeldung");
    ESP_LOGI(TAG, "  tour [tap] | flash <Sekunden> | marks <n>      geführter Test (tap: Start per Tippen), Flash-Last");
    ESP_LOGI(TAG, "  pat <black|bars|frame|motion|touch|engine>   Testbild/Modus");
    ESP_LOGI(TAG, "  bl [<0-100>] | bl cycle | bl stop | blf <Hz>    Hintergrundbeleuchtung (ohne Wert: Kurve und Nachtmodus)");
    ESP_LOGI(TAG, "  blduty <0-4095> | bldmin <Pulsbreite>            Pulsbreite direkt; kleinste noch leuchtende speichern");
    ESP_LOGI(TAG, "  pclk <MHz>                                    Pixeltakt zur Laufzeit");
    ESP_LOGI(TAG, "  timing <hfp> <hpw> <hbp> <vfp> <vpw> <vbp>     Porches (Panel neu anlegen)");
    ESP_LOGI(TAG, "  pol <hsync_idle_low> <vsync_idle_low> <pclk_neg>  Polaritäten 0/1");
    ESP_LOGI(TAG, "  bounce <Zeilen> | clk <160|240>                Bounce-Buffer, Taktquelle");
    ESP_LOGI(TAG, "  swap <n> | sprites <n> | buf <int|psram>       Engine-Prototyp");
    ESP_LOGI(TAG, "  buf grob <int|psram>                           grober Puffer der Orte (Messung)");
    ESP_LOGI(TAG, "  ort [<Nr|Kennung> [Übergang]] | ort rundgang <s> | ort figuren <n>   Orte der Bühne");
    ESP_LOGI(TAG, "  kamera [standard|folge <Figur>|ort <Nr>] | karte [<Figur>|aus] | meldung [system] <Text> | uhr an|aus|ecke <n>|sekunden 0|1");
    ESP_LOGI(TAG, "  ort ausbau | ort baustelle | ort folgen 0/1 | ort pruefen 0/1   Testorte (Raum-Baukasten)");
    ESP_LOGI(TAG, "  bench [mem|licht|figuren]   Zeichnen messen: Verdoppeln, Kopie des Hintergrunds; mem: PSRAM-Durchsatz, DMA; licht: Lichtkarte; figuren: Baukasten");
    ESP_LOGI(TAG, "  time <HH:MM|real> | speed <Faktor>             Weltzeit stellen (real = echte Uhrzeit), Zeitraffer");
    ESP_LOGI(TAG, "  wetter [<Art> [%%] [sofort]] | wetter blitz | wetter wind links|rechts   Wetter (Art: sonne bewoelkt regen schnee nebel gewitter wind; übersteuert bis zur nächsten Wetterzeit)");
    ESP_LOGI(TAG, "  update abriss                                   nächster Download von GitHub trennt einmal (Weitermachen prüfen)");
    ESP_LOGI(TAG, "  welt [laenge <Tage> | auto]                     Weltkalender, Mond, Wetter-Simulation (auto = Simulation wieder an)");
    ESP_LOGI(TAG, "  welt speichern | figuren | chronik              Speicherstand jetzt schreiben, Figuren mit Ort, Chronik (seit 0.5.9)");
    ESP_LOGI(TAG, "  ort cmd <Befehl>   Befehl an Bühne/Ort, z. B. innenlicht um, ofen glut, nass 80, becken 50, regen fein, blitze 12 30");
    ESP_LOGI(TAG, "  ort cmd zustaende  Ortszustände ins Log (Herd, Kerze, Lampe, Becken, Nässe)");
    ESP_LOGI(TAG, "  ort cmd aufgaben   Aufgabenliste ins Log (holen, bringen, herstellen; wer, von wo nach wo)");
    ESP_LOGI(TAG, "  ort cmd projekte   Projekte ins Log; ort cmd projekt bett start|stufe <n>|fertig (Prüfbefehl)");
    ESP_LOGI(TAG, "  lager              Lagerplätze (Stand, Reichweite), Waren im Dorf und Projekte ins Log (wie die Lager-Karte)");
    ESP_LOGI(TAG, "  wirtschaft [<Datei> | bestand]   Protokoll-Dateien, eine Datei ins Log, Bestand aller Lagerplätze (Bilanz prüfen)");
    ESP_LOGI(TAG, "  perf <Sekunden> | perf off                     Leistungsmonitor");
    ESP_LOGI(TAG, "  assets [quelle eingebettet|littlefs|sd | kopieren littlefs|sd | messen | leeren | budget <KB>]   Grafik-Pakete");
    ESP_LOGI(TAG, "  sd | sd neu                                    microSD-Karte: Zustand, neu einhängen");
}

void log_timing(const board::display::Timing& t) {
    ESP_LOGI(TAG, "Timing: pclk %.2f MHz, H %u/%u/%u, V %u/%u/%u, pol %d/%d/%d, bounce %d, clk %s", t.pclk_hz / 1e6,
             t.hsync_front, t.hsync_pulse, t.hsync_back, t.vsync_front, t.vsync_pulse, t.vsync_back, t.hsync_idle_low,
             t.vsync_idle_low, t.pclk_active_neg, t.bounce_lines, t.pll240 ? "PLL240" : "PLL160");
}

void log_buffer_info() {
    const log::Info i = log::info();
    const log::Stats& s = i.stats;
    const double mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
    ESP_LOGI(TAG, "Log-Puffer: %lu Zeilen (Nr. %lu–%lu), %u von %u KB belegt, Start Nr. %u, beim Start %s (%lu Zeilen)",
             static_cast<unsigned long>(i.next - i.first), static_cast<unsigned long>(i.first),
             static_cast<unsigned long>(i.next ? i.next - 1 : 0), static_cast<unsigned>(i.used / 1024),
             static_cast<unsigned>(i.capacity / 1024), i.boot, i.origin, static_cast<unsigned long>(i.prev_lines));
    ESP_LOGI(TAG, "  RTC-Sicherung %u von %u Byte (ab Nr. %lu, %s), PSRAM-Puffer bei %p", static_cast<unsigned>(i.rtc_used),
             static_cast<unsigned>(i.rtc_capacity), static_cast<unsigned long>(i.rtc_first),
             i.mirror ? "zeilenweise im Probelauf" : "nur beim Neustart", i.address);
    ESP_LOGI(TAG, "  Hook: %lu Aufrufe, %lu im Puffer, %lu nur seriell, %lu gekürzt, %lu Fehler, %lu Warnungen",
             static_cast<unsigned long>(s.calls), static_cast<unsigned long>(s.stored),
             static_cast<unsigned long>(s.serial_only), static_cast<unsigned long>(s.truncated),
             static_cast<unsigned long>(s.errors), static_cast<unsigned long>(s.warnings));
    ESP_LOGI(TAG, "  Kosten pro Zeile: Formatieren %.1f Âµs, Puffer %.1f Âµs (max %.1f Âµs), seriell %.0f Âµs, gesamt max %.0f Âµs",
             s.calls ? s.fmt_cycles / mhz / s.calls : 0.0, s.stored ? s.store_cycles / mhz / s.stored : 0.0,
             s.store_max_cycles / mhz, s.calls ? s.uart_cycles / mhz / s.calls : 0.0, s.total_max_cycles / mhz);
}

// Messung: Puffer allein (ohne seriell), dann n echte Log-Zeilen über den Hook; Bildrate und Aussetzer dabei
// Speicher-Messung (0.5.2, „bench mem“): Wohin geht die Zeit beim Verdoppeln? Schreiben, Lesen, Kopie aus dem
// internen RAM und per DMA (GDMA, ohne Cache) in einen 96-KB-Puffer im PSRAM.
volatile uint32_t s_bench_sink = 0;
bool IRAM_ATTR bench_dma_done(async_memcpy_handle_t, async_memcpy_event_t*, void* arg) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(static_cast<SemaphoreHandle_t>(arg), &woken);
    return woken == pdTRUE;
}
void bench_memory() {
    constexpr int kRuns = 20, kChunk = 3200, kChunks = pixelia::kScreenPixels / kChunk;
    auto* fine = static_cast<uint8_t*>(heap_caps_aligned_alloc(64, pixelia::kScreenPixels, MALLOC_CAP_SPIRAM));
    auto* rows = static_cast<uint8_t*>(heap_caps_aligned_alloc(64, 2 * kChunk, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    SemaphoreHandle_t done = xSemaphoreCreateCounting(kChunks, 0);
    async_memcpy_config_t cfg = ASYNC_MEMCPY_DEFAULT_CONFIG();
    cfg.backlog = kChunks;
    cfg.dma_burst_size = 64;
    async_memcpy_handle_t mcp = nullptr;
    const esp_err_t dma_err = esp_async_memcpy_install(&cfg, &mcp);
    if (fine && rows && done) {
        for (int i = 0; i < 2 * kChunk; ++i) rows[i] = static_cast<uint8_t>(i * 3);
        auto timed = [&](auto&& f) {
            const int64_t t0 = esp_timer_get_time();
            for (int i = 0; i < kRuns; ++i) f(i);
            return (esp_timer_get_time() - t0) / kRuns;
        };
        const int64_t w = timed([&](int i) { std::memset(fine, i, pixelia::kScreenPixels); });
        const int64_t r = timed([&](int) {
            uint32_t s = 0;
            const auto* p = reinterpret_cast<const uint32_t*>(fine);
            for (int k = 0; k < pixelia::kScreenPixels / 4; k += 16) s += p[k];  // ein Wort je 64-Byte-Zeile
            s_bench_sink = s;
        });
        const int64_t c = timed([&](int) {
            for (int y = 0; y < pixelia::kScreenHeight; ++y)
                std::memcpy(fine + y * pixelia::kScreenWidth, rows + (y & 7) * pixelia::kScreenWidth,
                            pixelia::kScreenWidth);
        });
        const int64_t inv = timed([&](int) { esp_cache_msync(fine, pixelia::kScreenPixels, ESP_CACHE_MSYNC_FLAG_DIR_M2C); });
        int64_t d = -1;
        int fails = 0;
        if (dma_err == ESP_OK) {
            d = timed([&](int) {
                int ok = 0;
                for (int k = 0; k < kChunks; ++k) {
                    if (esp_async_memcpy(mcp, fine + k * kChunk, rows + (k & 1) * kChunk, kChunk, bench_dma_done,
                                         done) == ESP_OK)
                        ++ok;
                    else
                        ++fails;
                }
                for (int k = 0; k < ok; ++k) xSemaphoreTake(done, pdMS_TO_TICKS(50));
            });
        }
        ESP_LOGI(TAG, "PSRAM 96 KB: schreiben %" PRId64 " Âµs, lesen %" PRId64 " Âµs, Kopie aus internem RAM %" PRId64
                      " Âµs, DMA %" PRId64 " Âµs (%d Fehler, install %s), Cache verwerfen %" PRId64 " Âµs",
                 w, r, c, d, fails, esp_err_to_name(dma_err), inv);
    }
    if (mcp) esp_async_memcpy_uninstall(mcp);
    if (done) vSemaphoreDelete(done);
    heap_caps_free(fine);
    heap_caps_free(rows);
}

// Lichtkarte (seit 0.5.9 Teil 3, „bench licht“): Tabellen und ganze Karte für Lampe, Kerze und Herd wie im Testraum,
// dazu der Aufwand in Einheiten des Budgets je Schritt (Stage::kPrepareBudget). Auf Kern 0 wie „bench“
void bench_light() {
    auto* plan = static_cast<pixelia::LightPlan*>(heap_caps_malloc(sizeof(pixelia::LightPlan), MALLOC_CAP_SPIRAM));
    auto* map = static_cast<uint8_t*>(heap_caps_malloc(pixelia::kCoarsePixels, MALLOC_CAP_SPIRAM));
    if (plan && map) {
        new (plan) pixelia::LightPlan;
        const pixelia::LightCircle c[] = {pixelia::light_circle(70, 60, 4), pixelia::light_circle(120, 50, 3),
                                          pixelia::light_circle(40, 80, 3)};
        int64_t t0 = esp_timer_get_time();
        pixelia::plan_light_map(plan, c, 3, 3, pixelia::LightStyle::Fine);
        const int64_t tp = esp_timer_get_time() - t0;
        t0 = esp_timer_get_time();
        pixelia::plan_light_map(plan, c + 1, 2, 3, pixelia::LightStyle::Fine);  // Lampe aus: Tabellen nur kopiert
        pixelia::plan_light_map(plan, c, 3, 3, pixelia::LightStyle::Fine);      // wieder an: eine neu
        const int64_t tw = esp_timer_get_time() - t0;
        int cost = 0;
        uint8_t x0, x1;
        t0 = esp_timer_get_time();
        for (int y = 0; y < pixelia::kCoarseHeight; ++y) {
            cost += pixelia::light_row_cost(*plan, y, pixelia::kCoarseWidth);
            pixelia::build_light_row(map + y * pixelia::kCoarseWidth, pixelia::kCoarseWidth, y, *plan, &x0, &x1);
        }
        const int64_t tb = esp_timer_get_time() - t0;
        ESP_LOGI(TAG, "Lichtkarte (Lampe, Kerze, Herd): Tabellen %" PRId64 " µs (aus und an %" PRId64 " µs), Karte %"
                      PRId64 " µs, Aufwand %d (%d µs je 1000; Budget je Schritt %d ≈ %d µs)",
                 tp, tw, tb, cost, static_cast<int>(tb * 1000 / std::max(1, cost)), pixelia::Stage::kPrepareBudget,
                 static_cast<int>(tb * pixelia::Stage::kPrepareBudget / std::max(1, cost)));
    } else {
        ESP_LOGW(TAG, "bench licht: kein PSRAM frei");
    }
    heap_caps_free(plan);
    heap_caps_free(map);
}

void log_bench(int n) {
    const double mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
    for (bool rtc : {false, true}) {
        uint32_t avg = 0, mx = 0;
        const log::Stats s0 = log::info().stats;
        log::bench_store(n, rtc, &avg, &mx);
        const log::Stats s1 = log::info().stats;
        ESP_LOGI(TAG, "Messung Puffer allein%s: %d Zeilen, im Mittel %.2f µs (PSRAM %.2f µs, RTC %.2f µs), höchstens %.2f µs",
                 rtc ? " mit RTC-Sicherung (Probelauf)" : "", n, avg / mhz, (s1.main_cycles - s0.main_cycles) / mhz / n,
                 (s1.rtc_cycles - s0.rtc_cycles) / mhz / n, mx / mhz);
    }
    const log::Stats before = log::info().stats;
    const board::display::Stats d0 = board::display::stats(false);
    const int64_t t0 = esp_timer_get_time();
    for (int i = 0; i < n; ++i)
        ESP_LOGI(TAG, "Messzeile %d/%d über den Hook: seriell und Puffer, typische Länge mit Zahlen %d", i + 1, n, i * 37);
    const int64_t t1 = esp_timer_get_time();
    const board::display::Stats d1 = board::display::stats(false);
    const log::Stats after = log::info().stats;
    const uint32_t calls = after.calls - before.calls;
    const uint32_t stored = after.stored - before.stored;
    ESP_LOGI(TAG, "Messung Hook: %lu Zeilen in %.0f ms, pro Zeile Formatieren %.1f Âµs, Puffer %.1f Âµs, seriell %.0f Âµs",
             static_cast<unsigned long>(calls), (t1 - t0) / 1000.0,
             calls ? (after.fmt_cycles - before.fmt_cycles) / mhz / calls : 0.0,
             stored ? (after.store_cycles - before.store_cycles) / mhz / stored : 0.0,
             calls ? (after.uart_cycles - before.uart_cycles) / mhz / calls : 0.0);
    ESP_LOGI(TAG, "  Display währenddessen: %.1f Hz, %lu Bildtausch (%.1f fps), %lu Aussetzer",
             (d1.vsyncs - d0.vsyncs) * 1e6 / (t1 - t0), static_cast<unsigned long>(d1.swaps - d0.swaps),
             (d1.swaps - d0.swaps) * 1e6 / (t1 - t0), static_cast<unsigned long>(d1.desyncs - d0.desyncs));
}

// Gibt false zurück, wenn der Befehl unbekannt ist
bool handle(char* line) {
    char* argv[24] = {};
    int argc = 0;
    for (char* tok = std::strtok(line, " \t"); tok && argc < 24; tok = std::strtok(nullptr, " \t")) argv[argc++] = tok;
    if (argc == 0) return false;
    const char* cmd = argv[0];
    auto is = [&](const char* name) { return std::strcmp(cmd, name) == 0; };
    using board::display::Mode;
    bool known = true;

    if (is("help") || is("?")) {
        print_help();
    } else if (is("log")) {
        const char* sub = argc >= 2 ? argv[1] : "";
        if (std::strcmp(sub, "clear") == 0) {
            log::clear();
            ESP_LOGW(TAG, "Log-Puffer geleert");
        } else if (std::strcmp(sub, "bench") == 0) {
            log_bench(argc >= 3 ? std::max(1, std::min(1000, std::atoi(argv[2]))) : 100);
        } else {
            log_buffer_info();
        }
    } else if (is("tasks")) {
        diag::log_tasks();
    } else if (is("resets")) {
        diag::log_resets();
    } else if (is("coredump")) {
        if (argc >= 2 && std::strcmp(argv[1], "erase") == 0) diag::crash_erase();
        else diag::log_crash();
    } else if (is("info")) {
        sysinfo::log_all();
        log_timing(board::display::timing());
    } else if (is("heap")) {
        sysinfo::log_heap();
    } else if (is("scan")) {
        hwtest::scan();
    } else if (is("touchrst")) {
        hwtest::touch_reset_test();
    } else if (is("touch")) {
        hwtest::log_touch_status();
    } else if (is("tour2")) {
        hwtest::start_tour2();
    } else if (is("tour")) {
        hwtest::start_tour(!(argc >= 2 && std::strcmp(argv[1], "tap") == 0));
    } else if (is("flash") && argc >= 2) {
        hwtest::flash_stress(std::atoi(argv[1]));
    } else if (is("marks") && argc >= 2) {
        gfx::set_marks(std::atoi(argv[1]));
    } else if (is("wetter")) {
        if (argc == 1) {
            const gfx::EngineStats s = gfx::engine_stats(false);
            ESP_LOGI(TAG, "Wetter %s %d %%, Licht innen %s, %d Partikel",
                     pixelia::weather_name(static_cast<pixelia::WeatherKind>(s.weather)), s.weather_pct,
                     s.inner_light ? "an" : "aus", s.particles);
        } else {
            // „wetter blitz“ und „wetter wind links“ ohne Vorsilbe an die Bühne, sonst „wetter <Art> …“
            const bool plain = std::strcmp(argv[1], "blitz") == 0 || std::strcmp(argv[1], "wind") == 0;
            std::string cmd = plain ? "" : "wetter";
            for (int i = 1; i < argc; ++i) cmd += (cmd.empty() ? "" : " ") + std::string(argv[i]);
            if (!gfx::place_command(cmd.c_str())) ESP_LOGW(TAG, "Befehl nicht angenommen");
        }
    } else if (is("time") && argc >= 2 && std::strcmp(argv[1], "real") == 0) {
        gfx::set_real_time(true);
        ESP_LOGI(TAG, "Weltzeit folgt der echten Uhrzeit%s", timesync::valid() ? "" : " (sobald sie bekannt ist)");
    } else if (is("time") && argc >= 2) {
        int h = 0, m = 0;
        if (std::sscanf(argv[1], "%d:%d", &h, &m) >= 1 && h >= 0 && h < 24 && m >= 0 && m < 60) {
            gfx::set_time_of_day(h, m);
            ESP_LOGI(TAG, "Weltzeit %02d:%02d", h, m);
        } else {
            ESP_LOGW(TAG, "Uhrzeit als HH:MM angeben");
        }
    } else if (is("speed") && argc >= 2) {
        gfx::set_speed(static_cast<float>(std::atof(argv[1])));
        ESP_LOGI(TAG, "Zeitraffer %.1fx", gfx::speed());
    } else if (is("rot") && argc >= 2) {
        settings::set_rotation180(std::atoi(argv[1]) == 180);
    } else if (is("set")) {
        if (argc >= 3 && std::strcmp(argv[1], "bright") == 0) settings::set_brightness(std::atoi(argv[2]));
        else if (argc >= 3 && std::strcmp(argv[1], "rot") == 0) settings::set_rotation180(std::atoi(argv[2]) == 180);
        else if (argc >= 3 && std::strcmp(argv[1], "lp") == 0) settings::set_long_press_ms(std::atoi(argv[2]));
        settings::log_all();
    } else if (is("card")) {
        const bool off = argc >= 2 && std::strcmp(argv[1], "off") == 0;
        if (off) infocard::hide();
        else infocard::show();
        ESP_LOGI(TAG, "Info-Karte %s", off ? "geschlossen" : "gezeigt");
    } else if (is("wifi")) {
        const char* sub = argc >= 2 ? argv[1] : "";
        if (std::strcmp(sub, "scan") == 0) {
            static wifi::ScanEntry list[24];
            int n = wifi::scan(list, 24, true);
            for (int i = 0; i < n; ++i)
                ESP_LOGI(TAG, "  %4d dBm  Kanal %2u  %s%s%s", list[i].rssi, list[i].channel, list[i].ssid,
                         list[i].secure ? "" : " (offen)", list[i].saved ? "  [gespeichert]" : "");
        } else if (std::strcmp(sub, "list") == 0) {
            char names[wifi::kMaxSaved][33];
            int n = wifi::saved_networks(names, wifi::kMaxSaved);
            ESP_LOGI(TAG, "%d gespeicherte(s) WLAN (zuletzt benutztes zuerst):", n);
            for (int i = 0; i < n; ++i) ESP_LOGI(TAG, "  %d: %s", i + 1, names[i]);
        } else if (std::strcmp(sub, "forget") == 0 && argc >= 3) {
            // Namen mit Leerzeichen wieder zusammensetzen
            char name[33] = "";
            for (int i = 2; i < argc; ++i) {
                if (i > 2) std::strncat(name, " ", sizeof(name) - std::strlen(name) - 1);
                std::strncat(name, argv[i], sizeof(name) - std::strlen(name) - 1);
            }
            esp_err_t err = wifi::remove_network(name);
            ESP_LOGI(TAG, "WLAN „%s“: %s", name, err == ESP_OK ? "entfernt" : "nicht gespeichert");
        } else if (std::strcmp(sub, "setup") == 0) {
            wifi::force_setup();
        } else if (std::strcmp(sub, "connect") == 0) {
            wifi::reconnect();
        } else {
            wifi::Status s = wifi::status();
            ESP_LOGI(TAG, "WLAN %s, Netz „%s“, IP %s, %d dBm, Kanal %u, %d gespeichert", wifi::state_name(s.state), s.ssid,
                     s.ip[0] ? s.ip : "–", s.rssi, s.channel, s.saved);
            if (s.ap_active)
                ESP_LOGI(TAG, "Setup-WLAN „%s“ aktiv, %d Gerät(e), nächster Versuch in %d s", wifi::kApSsid, s.ap_clients,
                         s.retry_in_s);
            if (s.last_error[0]) ESP_LOGI(TAG, "Letzter Fehler: %s", s.last_error);
        }
    } else if (is("ota")) {
        ota::log_status();
    } else if (is("update") && argc >= 2 && std::strcmp(argv[1], "abriss") == 0) {
        github::test_break();  // seit 0.5.7: Weitermachen per HTTP-Range prüfen
        ESP_LOGI(TAG, "Nächster Download von GitHub trennt einmal nach 300 KB");
    } else if (is("toast") && argc >= 2) {
        // Kurzmeldung zum Prüfen der Anzeige; Wörter wieder zusammensetzen
        std::string text;
        for (int i = 1; i < argc; ++i) text += (i > 1 ? " " : "") + std::string(argv[i]);
        ui::toast(text, pixelia::ui::Tone::Info, 15);
    } else if (is("ntp")) {
        timesync::log_status();
        ESP_LOGI(TAG, "Weltzeit %s", gfx::real_time() ? "folgt der echten Uhrzeit" : "frei gestellt (time real = echte Zeit)");
    } else if (is("web")) {
        ESP_LOGI(TAG, "Webserver: http://%s.local, %d WebSocket-Verbindung(en)", wifi::kMdnsHost, web::ws_clients());
    } else if (is("reboot")) {
        if (s_remote) {
            // Antwort an den Aufrufer soll noch rausgehen
            ESP_LOGI(TAG, "Neustart in 1 s");
            ota::restart_soon(1000);
        } else {
            ESP_LOGI(TAG, "Neustart");
            gfx::save_world_now();  // seit 0.5.9: Speicherstand vorher schreiben
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();
        }
    } else if (is("pat") && argc >= 2) {
        Mode m;
        if (!board::display::mode_from_name(argv[1], &m)) {
            ESP_LOGW(TAG, "Unbekanntes Testbild: %s", argv[1]);
            return false;
        }
        if (m == Mode::Touch) hwtest::reset_corner_hits();
        board::display::set_mode(m);
        ESP_LOGI(TAG, "Testbild: %s", argv[1]);
    } else if (is("bl") && argc >= 2) {
        if (std::strcmp(argv[1], "cycle") == 0) {
            hwtest::backlight_cycle(true);
        } else if (std::strcmp(argv[1], "stop") == 0) {
            hwtest::backlight_cycle(false);
        } else {
            hwtest::backlight_cycle(false);
            board::backlight::set_percent(std::atoi(argv[1]));
            ESP_LOGI(TAG, "Helligkeit %d %%", board::backlight::percent());
        }
    } else if (is("bl") && argc == 1) {
        // seit 0.5.7: Kurve und Nachtmodus
        const settings::Values v = settings::get();
        ESP_LOGI(TAG, "Helligkeit %d %% = Pulsbreite %lu von %lu (kleinste %lu), Nachtmodus %s (%u %%), gerade %s",
                 board::backlight::percent(), static_cast<unsigned long>(board::backlight::duty()),
                 static_cast<unsigned long>(board::backlight::kMaxDuty),
                 static_cast<unsigned long>(board::backlight::min_duty()), v.night_on ? "an" : "aus", v.night_bright,
                 night::awake() ? "aufgehellt" : night::active() ? "Nacht" : "Tag");
    } else if (is("bldmin") && argc >= 2) {
        settings::set_backlight_min(std::atoi(argv[1]));
    } else if (is("blduty") && argc >= 2) {
        // Pulsbreite direkt (Messung: kleinste noch leuchtende Stufe finden); der Nachtmodus stellt sie bei der
        // nächsten Änderung zurück
        hwtest::backlight_cycle(false);
        board::backlight::set_duty(static_cast<uint32_t>(std::atoi(argv[1])));
        ESP_LOGI(TAG, "Pulsbreite %lu von %lu", static_cast<unsigned long>(board::backlight::duty()),
                 static_cast<unsigned long>(board::backlight::kMaxDuty));
    } else if (is("lager")) {
        // Lagerplätze mit Stand und Reichweite, Waren im Dorf, Projekte (seit 0.5.10 Teil 4b, wie die Lager-Karte)
        if (!gfx::place_command("lager")) ESP_LOGW(TAG, "Befehl nicht angenommen");
    } else if (is("wirtschaft")) {
        // Wirtschafts-Protokoll prüfen (seit 0.5.10 Teil 4c): Dateien, eine Datei ins Log, Bestand aller Lagerplätze.
        // Abholen und Bericht kommen mit 0.5.12
        static constexpr const char* kDir = "/data/wirtschaft";
        static char line[256], path[96];
        if (argc >= 2 && std::strcmp(argv[1], "bestand") == 0) {
            if (!gfx::place_command("bestand")) ESP_LOGW(TAG, "Befehl nicht angenommen");
        } else if (argc >= 2) {
            std::snprintf(path, sizeof(path), "%s/%.60s", kDir, argv[1]);
            FILE* f = std::fopen(path, "rb");
            if (!f) {
                ESP_LOGW(TAG, "%s fehlt", path);
            } else {
                int n = 0;
                while (std::fgets(line, sizeof(line), f)) {
                    line[std::strcspn(line, "\r\n")] = 0;
                    ESP_LOGI(TAG, "%s|%s", argv[1], line);
                    if (++n % 32 == 0) vTaskDelay(pdMS_TO_TICKS(20));  // Log-Puffer und WebSocket nicht fluten
                }
                std::fclose(f);
                ESP_LOGI(TAG, "%s: %d Zeilen", argv[1], n);
            }
        } else if (DIR* d = opendir(kDir)) {
            int n = 0;
            long sum = 0;
            while (dirent* e = readdir(d)) {
                struct stat sb;
                std::snprintf(path, sizeof(path), "%s/%.60s", kDir, e->d_name);
                if (stat(path, &sb) != 0) continue;
                ESP_LOGI(TAG, "  %-32s %6ld Byte", e->d_name, static_cast<long>(sb.st_size));
                ++n;
                sum += sb.st_size;
            }
            closedir(d);
            ESP_LOGI(TAG, "%s: %d Dateien, %.1f KB", kDir, n, sum / 1024.0);
        } else {
            ESP_LOGI(TAG, "%s gibt es noch nicht", kDir);
        }
    } else if (is("welt")) {
        // seit 0.5.7: Weltkalender, Mond, Wetter-Simulation; „welt laenge <Tage>“, „welt auto“ (Simulation wieder an)
        if (argc >= 3 && std::strcmp(argv[1], "laenge") == 0) {
            gfx::set_season_length(std::atoi(argv[2]));
        } else if (argc >= 2 && std::strcmp(argv[1], "auto") == 0) {
            gfx::place_command("wetter auto");
        } else if (argc >= 2 && std::strcmp(argv[1], "speichern") == 0) {
            ESP_LOGI(TAG, "%s", gfx::request_save() ? "Speicherstand wird geschrieben" : "Speichern läuft schon");
        } else if (argc >= 2 && (std::strcmp(argv[1], "figuren") == 0 || std::strcmp(argv[1], "chronik") == 0)) {
            // 6 KB: im PSRAM, nicht auf dem Stack der Konsole und nicht im internen RAM
            static auto* view = static_cast<gfx::WorldView*>(heap_caps_malloc(sizeof(gfx::WorldView), MALLOC_CAP_SPIRAM));
            gfx::WorldView& v = *view;
            if (!view || !gfx::world_view(view)) {
                ESP_LOGW(TAG, "Grafik-Task antwortet nicht");
            } else if (argv[1][0] == 'f') {
                for (int i = 0; i < v.figures; ++i) {
                    const gfx::FigureRow& f = v.fig[i];
                    char skills[96] = "";
                    for (int k = 0, n = 0; k < f.skill_n; ++k)
                        n += std::snprintf(skills + n, sizeof(skills) - n, "%s%s %s", k ? ", " : "",
                                           pixelia::skill_title(static_cast<pixelia::Skill>(f.skills[k].skill)),
                                           pixelia::skill_level_title(static_cast<pixelia::SkillLevel>(f.skills[k].level)));
                    ESP_LOGI(TAG, "%4u %-14s %2u J. %s%s%s%s · %s · %u Bez., %u Erinn.", f.id, f.name, f.age, f.place,
                             f.doing[0] ? ", " : "", f.doing, f.lead ? " (Hauptfigur)" : "", skills, f.relations,
                             f.memories);
                }
                ESP_LOGI(TAG, "%d Figuren, Besuche %d (%d laufen)", v.figures, v.visits, v.visits_open);
            } else {
                for (int i = v.chronicle - 1; i >= 0; --i)
                    ESP_LOGI(TAG, "Tag %d, %02d:%02d  %s", static_cast<int>(v.ch[i].day), v.ch[i].minute / 60,
                             v.ch[i].minute % 60, v.ch[i].text);
            }
        } else {
            const gfx::WorldInfo w = gfx::world_info();
            if (!w.started) {
                ESP_LOGI(TAG, "Weltzeit %s, Kalender noch nicht gestartet", w.valid ? "läuft" : "wartet auf die Uhrzeit");
            } else {
                ESP_LOGI(TAG, "Welttag %d, Weltjahr %d, %s Tag %d/%d%s (Jahreszeit %d Tage)%s", w.day, w.year,
                         pixelia::season_title(static_cast<pixelia::Season>(w.season)), w.day_in_season, w.season_days,
                         w.season_evening ? ", Jahreszeitenabend" : "", w.season_len, w.test ? ", Testuhr" : "");
                ESP_LOGI(TAG, "Sonne %02d:%02d–%02d:%02d, %s, Mond %s %d %%%s, Wetter %s %d %%%s", w.sunrise / 60,
                         w.sunrise % 60, w.sunset / 60, w.sunset % 60,
                         pixelia::day_part_title(static_cast<pixelia::DayPart>(w.day_part)), w.moon_title,
                         (w.moon_illum + 5) / 10, w.moon_visible ? " (sichtbar)" : "",
                         pixelia::weather_name(static_cast<pixelia::WeatherKind>(w.weather)), w.weather_pct,
                         w.weather_override ? " (übersteuert)" : "");
            }
            const gfx::SaveInfo s = gfx::save_info();
            const int64_t now = esp_timer_get_time();
            ESP_LOGI(TAG, "Speicherstand %lu, %.1f KB, %s, packen %lu µs, schreiben %lu ms, %d Figuren (%d unterwegs)%s%s",
                     static_cast<unsigned long>(s.generation), s.bytes / 1024.0,
                     s.at_us ? (std::to_string((now - s.at_us) / 1000000) + " s her").c_str()
                             : (s.loaded ? "geladen" : "noch keiner"),
                     static_cast<unsigned long>(s.pack_us), static_cast<unsigned long>(s.write_ms), s.figures, s.transit,
                     s.test ? ", Testuhr: speichert nicht" : s.waiting ? ", wartet auf die Uhrzeit" : "",
                     s.error[0] ? (std::string(", Fehler: ") + s.error).c_str() : "");
            if (s.loaded)
                ESP_LOGI(TAG, "Nachgeholt %lld min, übersprungen %lld min, offen %lld min", static_cast<long long>(s.caught_up),
                         static_cast<long long>(s.skipped), static_cast<long long>(s.catching_up));
        }
    } else if (is("blf") && argc >= 2) {
        esp_err_t err = board::backlight::set_frequency(static_cast<uint32_t>(std::atoi(argv[1])));
        ESP_LOGI(TAG, "PWM-Frequenz %lu Hz: %s", static_cast<unsigned long>(board::backlight::frequency()), esp_err_to_name(err));
    } else if (is("pclk") && argc >= 2) {
        auto hz = static_cast<uint32_t>(std::atof(argv[1]) * 1e6);
        esp_err_t err = board::display::set_pclk(hz);
        ESP_LOGI(TAG, "Pixeltakt %.2f MHz: %s (Bild %.2f Hz laut Timing)", hz / 1e6, esp_err_to_name(err),
                 1e6 / board::display::frame_period_us());
    } else if (is("timing") && argc >= 7) {
        board::display::Timing t = board::display::timing();
        t.hsync_front = static_cast<uint16_t>(std::atoi(argv[1]));
        t.hsync_pulse = static_cast<uint16_t>(std::atoi(argv[2]));
        t.hsync_back = static_cast<uint16_t>(std::atoi(argv[3]));
        t.vsync_front = static_cast<uint16_t>(std::atoi(argv[4]));
        t.vsync_pulse = static_cast<uint16_t>(std::atoi(argv[5]));
        t.vsync_back = static_cast<uint16_t>(std::atoi(argv[6]));
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("pol") && argc >= 4) {
        board::display::Timing t = board::display::timing();
        t.hsync_idle_low = std::atoi(argv[1]) != 0;
        t.vsync_idle_low = std::atoi(argv[2]) != 0;
        t.pclk_active_neg = std::atoi(argv[3]) != 0;
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("bounce") && argc >= 2) {
        board::display::Timing t = board::display::timing();
        t.bounce_lines = std::atoi(argv[1]);
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("clk") && argc >= 2) {
        board::display::Timing t = board::display::timing();
        t.pll240 = std::atoi(argv[1]) == 240;
        log_timing(t);
        gfx::request_reconfigure(t);
    } else if (is("swap") && argc >= 2) {
        board::display::set_swap_interval(std::atoi(argv[1]));
        ESP_LOGI(TAG, "Tauschintervall %d Bild(er)", board::display::swap_interval());
    } else if (is("sprites") && argc >= 2) {
        gfx::set_sprites(std::atoi(argv[1]));
        ESP_LOGI(TAG, "Sprites %d", std::atoi(argv[1]));
    } else if (is("kamera")) {
        // Kamera (seit 0.5.6): kamera | kamera standard | kamera folge <Figur> | kamera ort <Nr>
        if (argc >= 2 && std::strcmp(argv[1], "standard") == 0) gfx::camera_home();
        else if (argc >= 3 && std::strcmp(argv[1], "folge") == 0) gfx::camera_follow(static_cast<uint16_t>(std::atoi(argv[2])));
        else if (argc >= 3 && std::strcmp(argv[1], "ort") == 0) gfx::go_place(std::atoi(argv[2]));
        const gfx::CameraState c = gfx::camera_state();
        static const char* const kModes[] = {"Standard", "Frei", "Folgen"};
        ESP_LOGI(TAG, "Kamera: %s, Figur %u (%s), Ort %d, Rückkehr in %d s; Standard %d, Rückkehr nach %u s",
                 kModes[c.mode % 3], c.target, c.target_name, c.place, c.return_in_s, static_cast<int>(c.settings.home),
                 c.settings.return_s);
    } else if (is("karte")) {
        // Karte einer Figur öffnen (Messung): karte [Figur] | karte aus
        if (argc >= 2 && std::strcmp(argv[1], "aus") == 0) gfx::open_card(0);
        else gfx::open_card(argc >= 2 ? static_cast<uint16_t>(std::atoi(argv[1])) : 0xFFFF);
    } else if (is("meldung") && argc >= 2) {
        // Kurzmeldung: meldung <Text> (aus der Welt, höchstens 4 pro Stunde) | meldung system <Text>
        const bool sys = std::strcmp(argv[1], "system") == 0;
        std::string text;
        for (int i = sys ? 2 : 1; i < argc; ++i) text += (text.empty() ? "" : " ") + std::string(argv[i]);
        if (sys) ui::toast(text, pixelia::ui::Tone::Info, 8);
        else if (!ui::world_toast(text, pixelia::ui::Tone::Info, 8)) ESP_LOGW(TAG, "Meldung verworfen (Obergrenze pro Stunde)");
    } else if (is("uhr")) {
        // Pixel-Uhr: uhr an|aus | uhr ecke <0–3> | uhr sekunden 0|1
        const settings::Values v = settings::get();
        if (argc >= 2 && (std::strcmp(argv[1], "an") == 0 || std::strcmp(argv[1], "aus") == 0))
            settings::set_clock(argv[1][1] == 'n', v.clock_corner, v.clock_seconds);
        else if (argc >= 3 && std::strcmp(argv[1], "ecke") == 0)
            settings::set_clock(v.clock_on, std::atoi(argv[2]), v.clock_seconds);
        else if (argc >= 3 && std::strcmp(argv[1], "sekunden") == 0)
            settings::set_clock(v.clock_on, v.clock_corner, std::atoi(argv[2]) != 0);
    } else if (is("ort")) {
        if (argc >= 3 && std::strcmp(argv[1], "rundgang") == 0) {
            gfx::set_tour(std::atoi(argv[2]));
        } else if (argc >= 3 && std::strcmp(argv[1], "figuren") == 0) {
            gfx::set_figures(std::atoi(argv[2]));
        } else if (argc >= 3 && std::strcmp(argv[1], "folgen") == 0) {
            gfx::set_follow(std::atoi(argv[2]) != 0);
        } else if (argc >= 3 && std::strcmp(argv[1], "pruefen") == 0) {
            gfx::set_inspect(std::atoi(argv[2]) != 0);
        } else if (argc >= 2 && (std::strcmp(argv[1], "ausbau") == 0 || std::strcmp(argv[1], "baustelle") == 0)) {
            gfx::place_command(argv[1]);
        } else if (argc >= 3 && std::strcmp(argv[1], "cmd") == 0) {
            std::string cmd;
            for (int i = 2; i < argc; ++i) cmd += (i > 2 ? " " : "") + std::string(argv[i]);
            if (!gfx::place_command(cmd.c_str())) ESP_LOGW(TAG, "Befehl nicht angenommen");
        } else if (argc >= 2) {
            int n = gfx::find_place(argv[1]);
            if (n < 0 && argv[1][0] >= '0' && argv[1][0] <= '9') n = std::atoi(argv[1]);
            pixelia::Transition t;
            if (argc >= 3 && pixelia::parse_transition(argv[2], &t)) gfx::set_transition(t);
            if (n < 0 || n >= gfx::place_count()) ESP_LOGW(TAG, "Ort %s unbekannt", argv[1]);
            else gfx::go_place(n);
        }
        for (int i = 0; i < gfx::place_count(); ++i)
            ESP_LOGI(TAG, "%c %d %s (%s)", i == gfx::place() ? '*' : ' ', i, gfx::place_id(i), gfx::place_title(i));
        ESP_LOGI(TAG, "Übergang %s, Rundgang %d s, Figuren %d (0 = Standard), Blick folgt %d, Prüfansicht %d",
                 pixelia::transition_name(gfx::transition()), gfx::tour(), gfx::figures(), gfx::follow(),
                 gfx::inspect());
    } else if (is("buf") && argc >= 3 && std::strcmp(argv[1], "grob") == 0) {
        gfx::request_coarse(std::strcmp(argv[2], "psram") == 0);
    } else if (is("buf") && argc >= 2) {
        bool psram = std::strcmp(argv[1], "psram") == 0;
        gfx::request_buffers(psram);
        ESP_LOGI(TAG, "Engine-Puffer: %s", psram ? "PSRAM" : "intern");
    } else if (is("assets") || is("sd")) {
        assets::console(argc, argv);
    } else if (is("bench") && argc >= 2 && std::strcmp(argv[1], "licht") == 0) {
        bench_light();
    } else if (is("bench") && argc >= 2 && std::strcmp(argv[1], "figuren") == 0) {
        // Figuren-Baukasten (seit 0.5.9 Teil 4): gemessene Bilder seit dem Start, Aufwand je Bild fürs Budget
        const pixelia::FigureArtStats& st = pixelia::figure_art_stats();
        const double per = st.frames ? static_cast<double>(st.us) / st.frames : 0;
        ESP_LOGI(TAG, "Figuren-Baukasten: %lu Bilder, im Mittel %.0f µs (max %lld µs), erstes nach %.2f s, fertig nach %.2f s (zuletzt %.2f s)",
                 static_cast<unsigned long>(st.frames), per, static_cast<long long>(st.max_us), st.first_us / 1e6,
                 st.first_done_us / 1e6, st.done_us / 1e6);
        ESP_LOGI(TAG, "Budget: kFrameCost %d ≈ %.0f µs geschätzt (Vorarbeit %d je Schritt ≈ 1,5 ms); neu: ort cmd figuren neu", pixelia::FigureArt::kFrameCost,
                 pixelia::FigureArt::kFrameCost * 1500.0 / pixelia::Stage::kPrepareBudget, pixelia::Stage::kPrepareBudget);
    } else if (is("bench")) {
        // Messung fürs Zeichnen (seit 0.5.1, Grundlage für 0.5.2): Verdoppeln 200×120 → 400×240 und Kopie des
        // Hintergrundbilds (24 KB), beides PSRAM → PSRAM wie auf der Grafik-Task. Läuft auf Kern 0, die Grafik läuft
        // weiter (gleicher Speicherverkehr durch das Display).
        constexpr int kRuns = 50;
        auto* coarse = static_cast<uint8_t*>(heap_caps_malloc(pixelia::kCoarsePixels, MALLOC_CAP_SPIRAM));
        auto* bg = static_cast<uint8_t*>(heap_caps_malloc(pixelia::kCoarsePixels, MALLOC_CAP_SPIRAM));
        auto* fine = static_cast<uint8_t*>(heap_caps_malloc(pixelia::kScreenPixels, MALLOC_CAP_SPIRAM));
        if (coarse && bg && fine) {
            for (int i = 0; i < pixelia::kCoarsePixels; ++i) coarse[i] = bg[i] = static_cast<uint8_t>(i * 7);
            pixelia::Canvas c(fine, pixelia::kScreenWidth, pixelia::kScreenHeight);
            int64_t t0 = esp_timer_get_time();
            for (int i = 0; i < kRuns; ++i) pixelia::upscale2x(coarse, c);
            const int64_t up = esp_timer_get_time() - t0;
            t0 = esp_timer_get_time();
            for (int i = 0; i < kRuns; ++i) std::memcpy(coarse, bg, pixelia::kCoarsePixels);
            const int64_t cp = esp_timer_get_time() - t0;
            ESP_LOGI(TAG, "Verdoppeln 200×120 → 400×240: %" PRId64 " µs, Hintergrund kopieren (24 KB): %" PRId64
                          " Âµs (je Bild, PSRAM, Mittel aus %d)", up / kRuns, cp / kRuns, kRuns);
        } else {
            ESP_LOGW(TAG, "bench: kein PSRAM frei");
        }
        heap_caps_free(coarse);
        heap_caps_free(bg);
        heap_caps_free(fine);
        if (argc >= 2 && std::strcmp(argv[1], "mem") == 0) bench_memory();
    } else if (is("perf") && argc >= 2) {
        perf::set_interval(std::strcmp(argv[1], "off") == 0 ? 0 : std::atoi(argv[1]));
        ESP_LOGI(TAG, "Leistungsmonitor: %s", perf::interval() ? "an" : "aus");
    } else {
        ESP_LOGW(TAG, "Unbekannt: %s (help für Liste)", cmd);
        known = false;
    }
    return known;
}

// Befehl aus dem Netz ausführen; seine Log-Zeilen gehen als Antwort zurück
void run_job(Job& job) {
    static char* buf = static_cast<char*>(heap_caps_malloc(kCaptureBytes, MALLOC_CAP_SPIRAM));
    char line[256];
    std::snprintf(line, sizeof(line), "%s", job.line.c_str());
    ESP_LOGI(TAG, "> %s (per WLAN)", line);
    if (!buf) {
        job.known = handle(line);
    } else {
        log::Capture cap(buf, kCaptureBytes);
        s_remote = true;
        job.known = handle(line);
        s_remote = false;
        job.out.assign(buf, cap.size());
    }
    xSemaphoreGive(job.done);
}

void console_task(void*) {
    char line[256];
    size_t len = 0;
    for (;;) {
        uint8_t ch;
        // Kurz auf Zeichen warten, dazwischen Befehle aus dem Netz abarbeiten
        if (uart_read_bytes(kUart, &ch, 1, pdMS_TO_TICKS(20)) == 1) {
            if (ch == '\r' || ch == '\n') {
                if (len > 0) {
                    line[len] = 0;
                    ESP_LOGI(TAG, "> %s", line);
                    handle(line);
                    len = 0;
                }
            } else if ((ch == 8 || ch == 127) && len > 0) {
                --len;
            } else if (ch >= 32 && len < sizeof(line) - 1) {
                line[len++] = static_cast<char>(ch);
            }
        }
        std::shared_ptr<Job>* ref = nullptr;
        while (xQueueReceive(s_jobs, &ref, 0) == pdTRUE) {
            run_job(**ref);
            delete ref;
        }
    }
}
}  // namespace

void start() {
    // Nur Empfang über den Treiber; die Log-Ausgabe läuft unverändert weiter
    ESP_ERROR_CHECK(uart_driver_install(kUart, 1024, 0, 0, nullptr, 0));
    s_jobs = xQueueCreate(4, sizeof(std::shared_ptr<Job>*));
    s_remote_lock = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(console_task, "console", 4096, nullptr, 4, nullptr, kSystemCore);
}

Result run_remote(const char* line, bool confirmed, char* out, size_t out_len, char* why, size_t why_len,
                  int timeout_ms) {
    if (out_len) out[0] = 0;
    if (why_len) why[0] = 0;
    if (const char* reason = danger_of(line); reason && !confirmed) {
        std::snprintf(why, why_len, "%s", reason);
        return Result::NeedsConfirm;
    }
    if (!s_jobs || xSemaphoreTake(s_remote_lock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) return Result::Busy;
    auto job = std::make_shared<Job>();
    job->line = line;
    job->done = xSemaphoreCreateBinary();
    auto* ref = new std::shared_ptr<Job>(job);
    if (!job->done || xQueueSend(s_jobs, &ref, 0) != pdTRUE) {
        delete ref;
        xSemaphoreGive(s_remote_lock);
        return Result::Busy;
    }
    // Läuft der Befehl länger, arbeitet die Konsolen-Task ihn trotzdem zu Ende (Job gehört beiden Seiten)
    const bool finished = xSemaphoreTake(job->done, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
    xSemaphoreGive(s_remote_lock);
    if (!finished) return Result::Timeout;
    std::snprintf(out, out_len, "%s", job->out.c_str());
    return job->known ? Result::Done : Result::Unknown;
}

}  // namespace app::console
