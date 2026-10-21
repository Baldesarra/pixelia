// Grafik-Task auf Kern 1: legt das Panel an (Interrupt landet so auf Kern 1) und lässt im Engine-Modus die Bühne
// mit ihren Orten laufen (gleicher Code wie im Simulator, siehe engine/stage.h und sim/).

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include "app.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pixelia/engine/color.h"
#include "pixelia/engine/memory.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/system_ui.h"
#include "net.h"
#include "ui.h"

namespace app::gfx {

namespace {
constexpr const char* TAG = "gfx";
constexpr size_t kBufBytes = pixelia::kScreenPixels;
constexpr uint64_t kSeed = 0x5049584C4941ULL;  // „PIXLIA“

enum class Request : uint8_t { None, Reconfigure, Buffers };

volatile Request s_request = Request::None;
board::display::Timing s_req_timing;
// Standard: Engine-Puffer im PSRAM (spart 192 KB internen RAM, Messung siehe docs/HARDWARE.md)
volatile bool s_req_psram = true;
volatile int s_sprites = 48;
volatile int s_marks = 0;
volatile int s_req_minute_of_day = -1;  // Uhrzeit stellen (vom Konsolen-Task, angewendet im Grafik-Task)
volatile float s_speed = 1.0f;

uint8_t* s_buf[2] = {nullptr, nullptr};
bool s_buf_psram = false;
pixelia::WorldClock s_clock;

// Bühne (seit 0.5.0): Orte stehen nach start() fest. Grobe Orte zeichnen in s_coarse, die zweite Szene einer
// Überblendung in s_coarse_aux, beide im PSRAM. Gemessen (Testort Wiese): intern 3,7 ms, PSRAM 4,2 ms pro Bild;
// intern kostet aber 24 KB des knappen internen RAMs (Update über HTTPS braucht ~27 KB). „buf grob int“ zum Messen.
pixelia::Stage s_stage;
uint8_t* s_coarse = nullptr;
uint8_t* s_coarse_aux = nullptr;
int s_landscape = 0;
std::atomic<int> s_place{0};       // sichtbarer Ort (schreibt der Grafik-Task)
std::atomic<int> s_req_place{-1};  // Wechsel angefordert (andere Tasks)
std::atomic<int> s_transition{static_cast<int>(pixelia::kDefaultTransition)};
std::atomic<int> s_tour_s{0};      // automatischer Rundgang, 0 = aus
std::atomic<int> s_figures{0};     // Figuren der Testorte, 0 = Standard
int s_sprites_applied = -1;
int s_figures_applied = -1;
uint32_t s_tour_ticks = 0;
std::atomic<int> s_req_cmd{0};     // Befehl an den sichtbaren Ort: 1 = ausbau, 2 = baustelle
std::atomic<bool> s_follow{true};  // Blick folgt der Begleiterin (Bühne, seit 0.5.1)
std::atomic<bool> s_inspect{false};
int s_shown_place = -1;            // zuletzt gemeldeter Ort (Wechsel durch Ausgänge ins Log)
std::atomic<int> s_req_coarse{-1};  // grober Puffer umziehen: 0 = intern, 1 = PSRAM (Messung, Befehl „buf grob“)

void move_coarse(bool psram) {
    auto* p = static_cast<uint8_t*>(
        heap_caps_malloc(pixelia::kCoarsePixels, psram ? MALLOC_CAP_SPIRAM : MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!p) {
        ESP_LOGW(TAG, "Grober Puffer: kein Platz im %s", psram ? "PSRAM" : "internen RAM");
        return;
    }
    heap_caps_free(s_coarse);
    s_coarse = p;
    s_stage.set_buffers(s_coarse, s_coarse_aux);
    ESP_LOGI(TAG, "Grober Puffer jetzt im %s", psram ? "PSRAM" : "internen RAM");
    app::sysinfo::log_heap();
}
uint16_t s_pal565[256];

portMUX_TYPE s_stats_lock = portMUX_INITIALIZER_UNLOCKED;
EngineStats s_stats{};

// Weltzeit folgt der echten Uhrzeit (NTP), bis jemand per Befehl Uhrzeit oder Zeitraffer verstellt
volatile bool s_follow_real = true;
int64_t s_next_real_sync = 0;

// Systemanzeigen: von allen Tasks geschrieben (s_ui), im Grafik-Task als Kopie benutzt (s_ui_local)
struct UiState {
    uint32_t version = 0;
    bool setup = false;
    pixelia::ui::SetupInfo setup_info;
    bool update = false;
    pixelia::ui::UpdateInfo update_info;
    int64_t update_until = 0;  // 0 = bis auf Weiteres
    bool message = false;
    pixelia::ui::MessageInfo message_info;
    int64_t message_until = 0;
    std::string toast;
    pixelia::ui::Tone toast_tone = pixelia::ui::Tone::Info;
    int64_t toast_until = 0;
    bool info = false;
    pixelia::ui::InfoCardData info_data;
    int64_t info_until = 0;
    bool reset = false;
    pixelia::ui::ResetInfo reset_info;
    int64_t reset_until = 0;
};
std::mutex s_ui_lock;
UiState s_ui;
UiState s_ui_local;
pixelia::ui::SetupScreen s_setup_screen;
pixelia::ui::InfoCard s_info_card;
uint16_t s_ui_pal565[16];
// Live-Vorschau: 0 = nichts angefordert, 1 = angefordert (Grafik-Task kopiert), 2 = Kopie fertig
std::atomic<int> s_snap_state{0};
uint8_t* s_snap_px = nullptr;  // PSRAM, beim ersten Abruf angelegt
uint8_t s_snap_pal[256 * 3];
uint32_t s_snap_frame = 0;
uint32_t s_snap_us = 0;
int64_t s_snap_at = 0;
pixelia::Palette s_ui_pal{};

// Halte-Anzeige in Promille, -1 = aus. Eigener Wert statt UiState: ändert sich bei jedem Touch-Abfrage-Takt
std::atomic<int> s_hold_permille{-1};

// Eigenes Testbild (ui::show_picture). Eigene Sperre statt s_ui_lock: Der Grafik-Task hält sie während der Kopie
// (~1 ms), freigegeben wird ein Puffer nur unter dieser Sperre, also nie während der Kopie.
std::mutex s_pic_lock;
uint8_t* s_pic_px = nullptr;  // PSRAM, nullptr = kein Testbild
pixelia::Palette s_pic_pal{};
int64_t s_pic_until = 0;
uint32_t s_pic_id = 0;          // zählt jedes neue Bild hoch (Schlüssel für die Puffer)
pixelia::Palette s_pic_shown{};  // Palette des zuletzt gezeichneten Testbilds (nur Grafik-Task)

// Testbild in den Hinterpuffer kopieren; false = keins (mehr) da
bool draw_picture(pixelia::Canvas& canvas) {
    std::lock_guard<std::mutex> g(s_pic_lock);
    if (!s_pic_px) return false;
    std::memcpy(canvas.pixels(), s_pic_px, kBufBytes);
    s_pic_shown = s_pic_pal;
    return true;
}

// Nummer des gültigen Testbilds (0 = keins); ein abgelaufenes wird hier freigegeben
uint32_t current_picture(int64_t now) {
    uint8_t* old = nullptr;
    uint32_t id = 0;
    {
        std::lock_guard<std::mutex> g(s_pic_lock);
        if (s_pic_px && now >= s_pic_until) {
            old = s_pic_px;
            s_pic_px = nullptr;
        } else if (s_pic_px) {
            id = s_pic_id;
        }
    }
    if (old) {
        heap_caps_free(old);
        ESP_LOGI(TAG, "Testbild abgelaufen, zurück zur Welt");
    }
    return id;
}

// Echte Uhrzeit in die Weltzeit übernehmen: große Abweichung (erster Abgleich, Sommerzeit) springt,
// kleine Gangunterschiede werden sanft nachgeführt, damit Bewegungen gleichmäßig bleiben
void follow_real_time(int64_t now) {
    if (!s_follow_real || now < s_next_real_sync) return;
    s_next_real_sync = now + 1000000;
    int64_t local;
    if (!timesync::local_us(&local)) return;
    int64_t diff = local - s_clock.world_us();
    if (std::llabs(diff) > 2000000) {
        bool first = s_clock.world_us() < 365LL * pixelia::WorldClock::kUsPerDay;
        s_clock.set_world_us(local);
        ESP_LOGI(TAG, "Weltzeit %s echte Uhrzeit: %02d:%02d", first ? "übernimmt die" : "springt auf die",
                 s_clock.hour(), s_clock.minute());
    } else {
        s_clock.set_world_us(s_clock.world_us() + diff / 8);
    }
}

// Ganzseitige Systemanzeige, die gerade gilt: 0 = keine (Welt läuft), 1 = Update, 2 = Meldung, 3 = Setup,
// 4 = Zurücksetzen, 5 = eigenes Testbild
int active_screen(int64_t now, bool picture) {
    const UiState& u = s_ui_local;
    if (u.update && (u.update_until == 0 || now < u.update_until)) return 1;
    if (u.reset && now < u.reset_until) return 4;
    if (u.message && now < u.message_until) return 2;
    if (picture) return 5;
    if (u.setup) return 3;
    return 0;
}

void draw_screen(pixelia::Canvas& canvas, int screen) {
    const UiState& u = s_ui_local;
    if (screen == 1) pixelia::ui::render_update(canvas, u.update_info);
    else if (screen == 2) pixelia::ui::render_message(canvas, u.message_info);
    else if (screen == 4) pixelia::ui::render_reset(canvas, u.reset_info);
    else s_setup_screen.render(canvas);
}

// Was zuletzt in welchem Puffer gezeichnet wurde (0 = Welt). Systemanzeigen sind unbewegt: Steht im Hinterpuffer
// schon dieselbe Anzeige, muss nichts neu gezeichnet werden (spart Rechenzeit und PSRAM-Bandbreite).
uint64_t s_buf_key[2] = {0, 0};

// Info-Karte und Halte-Anzeige über das fertige Bild legen (nicht über das Update)
void draw_overlays(pixelia::Canvas& canvas, int screen, bool info, int hold) {
    if (screen == 1) return;
    if (info) s_info_card.render(canvas);
    if (hold >= 0) pixelia::ui::draw_hold_indicator(canvas, hold / 1000.0f);
}

void free_buffers() {
    board::display::detach_buffers();
    // Zwei Bilder warten, damit der Interrupt die alten Puffer sicher nicht mehr liest
    vTaskDelay(pdMS_TO_TICKS(2 * board::display::frame_period_us() / 1000 + 5));
    for (auto& b : s_buf) {
        heap_caps_free(b);
        b = nullptr;
    }
}

bool alloc_buffers(bool psram) {
    uint32_t caps = psram ? (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    for (auto& b : s_buf) b = static_cast<uint8_t*>(heap_caps_aligned_alloc(4, kBufBytes, caps));
    if (!s_buf[0] || !s_buf[1]) {
        for (auto& b : s_buf) {
            heap_caps_free(b);
            b = nullptr;
        }
        return false;
    }
    s_buf_psram = psram;
    s_buf_key[0] = s_buf_key[1] = 0;
    std::memset(s_buf[0], 0, kBufBytes);
    std::memset(s_buf[1], 0, kBufBytes);
    return true;
}

bool ensure_engine_ready(bool psram) {
    if (s_buf[0] && s_buf_psram == psram) return true;
    if (s_buf[0]) free_buffers();
    if (!alloc_buffers(psram)) {
        ESP_LOGW(TAG, "2× %u Byte im %s nicht verfügbar", static_cast<unsigned>(kBufBytes), psram ? "PSRAM" : "internen RAM");
        if (psram || !alloc_buffers(true)) {
            ESP_LOGE(TAG, "Keine Engine-Puffer");
            return false;
        }
    }
    ESP_LOGI(TAG, "Engine-Puffer: 2× %u Byte im %s", static_cast<unsigned>(kBufBytes),
             s_buf_psram ? "PSRAM" : "internen RAM");
    app::sysinfo::log_heap();
    board::display::attach_buffers(s_buf[0], s_buf[1]);
    return true;
}

// Anfragen anderer Tasks zwischen den Logik-Schritten übernehmen: Ortswechsel, Rundgang, Lastregler, Befehle
void apply_place_requests(int ticks) {
    if (s_stage.active() != s_shown_place) {
        if (s_shown_place >= 0 && !s_stage.transitioning())
            ESP_LOGI(TAG, "Ort %s (Wechsel %u, Reisen %u)", s_stage.scene(s_stage.active()).name(),
                     static_cast<unsigned>(s_stage.transitions()), static_cast<unsigned>(s_stage.travels()));
        if (!s_stage.transitioning()) s_shown_place = s_stage.active();
    }
    if (const bool f = s_follow.load(std::memory_order_relaxed); f != s_stage.follow()) s_stage.set_follow(f);
    if (const bool v = s_inspect.load(std::memory_order_relaxed); v != s_stage.inspect()) s_stage.set_inspect(v);
    if (const int c = s_req_cmd.exchange(0); c > 0) {
        const char* cmd = c == 1 ? "ausbau" : "baustelle";
        const bool ok = s_stage.scene(s_stage.active()).command(cmd);
        ESP_LOGI(TAG, "Befehl %s an %s: %s", cmd, s_stage.scene(s_stage.active()).name(), ok ? "ok" : "unbekannt");
    }
    const auto t = static_cast<pixelia::Transition>(s_transition.load(std::memory_order_relaxed));
    const int req = s_req_place.exchange(-1);
    int to = -1;
    if (req >= 0) {
        to = req;
    } else if (const int tour = s_tour_s.load(std::memory_order_relaxed); tour > 0 && !s_stage.transitioning()) {
        s_tour_ticks += ticks;
        if (s_tour_ticks >= static_cast<uint32_t>(tour) * pixelia::kTicksPerSecond)
            to = (s_stage.active() + 1) % s_stage.count();
    }
    if (to >= 0 && to != s_stage.active()) {
        ESP_LOGI(TAG, "Ort %s → %s (%s)", s_stage.scene(s_stage.active()).name(), s_stage.scene(to).name(),
                 pixelia::transition_name(t));
        s_stage.go(to, t);
    }
    if (to >= 0) s_tour_ticks = 0;
    if (const int n = s_sprites; n != s_sprites_applied) {
        s_stage.scene(s_landscape).set_load(n);
        s_sprites_applied = n;
    }
    if (const int n = s_figures.load(std::memory_order_relaxed); n != s_figures_applied) {
        for (int i = 0; i < s_stage.count(); ++i)
            if (std::strncmp(s_stage.scene(i).name(), "testort-", 8) == 0) s_stage.scene(i).set_load(n);
        s_figures_applied = n;
    }
}

void gfx_task(void* arg) {
    auto* timing = static_cast<board::display::Timing*>(arg);
    ESP_ERROR_CHECK(board::display::init(*timing));
    delete timing;

    pixelia::ui::apply_palette(s_ui_pal);
    for (int i = 0; i < 16; ++i) s_ui_pal565[i] = pixelia::to_rgb565(s_ui_pal[pixelia::ui::kFirstIndex + i]);

    uint32_t last_frames = board::display::frame_count();
    uint32_t frame_rem = 0;
    for (;;) {
        Request req = s_request;
        if (req == Request::Reconfigure) {
            bool had = s_buf[0] != nullptr;
            if (had) free_buffers();
            board::display::reconfigure(s_req_timing);
            s_request = Request::None;
            if (had && board::display::mode() == board::display::Mode::Engine) ensure_engine_ready(s_buf_psram);
            continue;
        }
        if (req == Request::Buffers) {
            if (s_buf[0]) free_buffers();
            s_request = Request::None;
            if (board::display::mode() == board::display::Mode::Engine) ensure_engine_ready(s_req_psram);
            continue;
        }
        if (const int where = s_req_coarse.exchange(-1); where >= 0) move_coarse(where == 1);
        int minute = s_req_minute_of_day;
        if (minute >= 0) {
            s_clock.set_time_of_day(minute / 60, minute % 60);
            s_req_minute_of_day = -1;
        }
        s_clock.set_speed(s_speed);

        if (board::display::mode() != board::display::Mode::Engine) {
            vTaskDelay(pdMS_TO_TICKS(20));
            last_frames = board::display::frame_count();
            frame_rem = 0;
            continue;
        }
        if (!s_buf[0] && !ensure_engine_ready(s_req_psram)) {
            board::display::set_mode(board::display::Mode::Black);
            continue;
        }

        uint8_t* back = board::display::acquire_back_buffer(200);
        if (!back) continue;

        // Logik-Schritte aus der Zahl der angezeigten Panel-Bilder: im Normalfall genau ein Schritt pro
        // Engine-Bild, nach einem verpassten Bild zwei. Kein Verhaken mit einer zweiten Uhr möglich.
        int64_t t0 = esp_timer_get_time();
        int swap = board::display::swap_interval();
        uint32_t frames = board::display::frame_count();
        uint32_t elapsed = frames - last_frames + frame_rem;
        last_frames = frames;
        int ticks = static_cast<int>(elapsed / swap);
        frame_rem = elapsed % swap;
        if (ticks > 4) ticks = 4;  // Überlast: nicht endlos nachholen
        const int64_t step_us = static_cast<int64_t>(board::display::frame_period_us()) * swap;
        apply_place_requests(ticks);
        follow_real_time(t0);
        for (int i = 0; i < ticks; ++i) {
            s_clock.advance(step_us);
            s_stage.tick(s_clock);
        }
        s_place.store(s_stage.active(), std::memory_order_relaxed);

        // Systemanzeigen übernehmen (Kopie, damit andere Tasks nie auf den Grafik-Task warten müssen)
        bool ui_changed = false;
        {
            std::lock_guard<std::mutex> g(s_ui_lock);
            if (s_ui.version != s_ui_local.version) {
                s_ui_local = s_ui;
                ui_changed = true;
            }
        }
        if (ui_changed && s_ui_local.setup) s_setup_screen.set(s_ui_local.setup_info);
        if (ui_changed && s_ui_local.info) s_info_card.set(s_ui_local.info_data);

        int64_t t1 = esp_timer_get_time();
        pixelia::Canvas canvas(back, pixelia::kScreenWidth, pixelia::kScreenHeight);
        const uint32_t pic = current_picture(t1);
        const int screen = active_screen(t1, pic != 0);
        const int bi = back == s_buf[0] ? 0 : 1;
        const bool info = s_ui_local.info && t1 < s_ui_local.info_until;
        const int hold = s_hold_permille.load(std::memory_order_relaxed);
        if (screen == 0) {
            s_stage.render(canvas);
            const UiState& u = s_ui_local;
            if (!u.toast.empty() && t1 < u.toast_until) pixelia::ui::draw_toast(canvas, u.toast, u.toast_tone);
            draw_overlays(canvas, screen, info, hold);
            s_buf_key[bi] = 0;
        } else {
            // Halte-Anzeige in 1-%-Schritten: so oft muss eine unbewegte Anzeige höchstens neu gezeichnet werden.
            // Beim Testbild geht dessen Nummer mit ein, damit ein neues Bild beide Puffer neu füllt.
            const uint32_t version = s_ui_local.version + (screen == 5 ? pic * 0x9E3779B1u : 0);
            const uint64_t key = static_cast<uint64_t>(screen) << 40 |
                                 static_cast<uint64_t>(hold < 0 ? 0 : hold / 10 + 1) << 32 |
                                 static_cast<uint64_t>(info) << 31 | static_cast<uint64_t>(s_marks & 0x7F) << 24 |
                                 (version & 0x00FFFFFFu);
            if (s_buf_key[bi] != key) {
                if (screen != 5) draw_screen(canvas, screen);
                else if (!draw_picture(canvas)) canvas.clear(0);
                draw_overlays(canvas, screen, info, hold);
                s_buf_key[bi] = key;
            }
        }
        for (int i = 0; i < s_marks; ++i) canvas.fill_rect(4 + i * 10, 4, 6, 6, pixelia::ui::kWhite);
        // Indizes 240–255 gehören der System-Oberfläche (engine/system_ui.h)
        const pixelia::Palette& pal = screen == 5 ? s_pic_shown : s_stage.palette();
        for (int i = 0; i < pixelia::ui::kFirstIndex; ++i) s_pal565[i] = pixelia::to_rgb565(pal[i]);
        std::memcpy(&s_pal565[pixelia::ui::kFirstIndex], s_ui_pal565, sizeof(s_ui_pal565));
        int64_t t2 = esp_timer_get_time();
        // Live-Vorschau: fertiges Bild kopieren, bevor es getauscht wird (~1 ms PSRAM → PSRAM)
        if (s_snap_state.load(std::memory_order_acquire) == 1) {
            std::memcpy(s_snap_px, back, kBufBytes);
            for (int i = 0; i < 256; ++i) {
                const pixelia::Color c = i < pixelia::ui::kFirstIndex ? pal[i] : s_ui_pal[i];
                s_snap_pal[i * 3] = c.r;
                s_snap_pal[i * 3 + 1] = c.g;
                s_snap_pal[i * 3 + 2] = c.b;
            }
            s_snap_frame = s_stats.frames;
            s_snap_us = static_cast<uint32_t>(esp_timer_get_time() - t2);
            s_snap_state.store(2, std::memory_order_release);
        }
        board::display::present(s_pal565);

        uint32_t render_us = static_cast<uint32_t>(t2 - t1);
        portENTER_CRITICAL(&s_stats_lock);
        s_stats.frames++;
        s_stats.render_us_sum += render_us;
        if (render_us > s_stats.render_us_max) s_stats.render_us_max = render_us;
        s_stats.logic_us_sum += static_cast<uint64_t>(t1 - t0);
        s_stats.logic_ticks += ticks;
        s_stats.tick_hist[ticks < 2 ? ticks : 2]++;
        s_stats.world_minute = s_clock.hour() * 60 + s_clock.minute();
        s_stats.world_day = s_clock.day();
        portEXIT_CRITICAL(&s_stats_lock);
    }
}
}  // namespace

void start(const board::display::Timing& timing) {
    // Grafiken der Engine in den PSRAM, der interne RAM bleibt für Puffer, die jedes Bild beschrieben werden
    pixelia::set_asset_allocator([](size_t n) { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM); }, heap_caps_free);
    pixelia::add_standard_scenes(s_stage);
    s_stage.init(kSeed);  // Landschaft (Ort 0) bekommt genau kSeed, wie vor 0.5.0
    s_landscape = std::max(0, s_stage.find("scrolltest"));
    s_coarse = static_cast<uint8_t*>(heap_caps_malloc(pixelia::kCoarsePixels, MALLOC_CAP_SPIRAM));
    s_coarse_aux = static_cast<uint8_t*>(heap_caps_malloc(pixelia::kCoarsePixels, MALLOC_CAP_SPIRAM));
    s_stage.set_buffers(s_coarse, s_coarse_aux);
    ESP_LOGI(TAG, "Bühne: %d Orte, sichtbar %s; grober Puffer 2× %d Byte", s_stage.count(),
             s_stage.scene(s_stage.active()).name(), pixelia::kCoarsePixels);
    auto* arg = new board::display::Timing(timing);
    xTaskCreatePinnedToCore(gfx_task, "gfx", 6144, arg, 10, nullptr, kGfxCore);
}

void request_reconfigure(const board::display::Timing& timing) {
    s_req_timing = timing;
    s_request = Request::Reconfigure;
}

void request_buffers(bool psram) {
    s_req_psram = psram;
    s_request = Request::Buffers;
}

void set_sprites(int n) { s_sprites = n; }
void set_marks(int n) { s_marks = n; }
void request_coarse(bool psram) { s_req_coarse.store(psram ? 1 : 0); }

int place_count() { return s_stage.count(); }
const char* place_id(int i) { return i >= 0 && i < s_stage.count() ? s_stage.scene(i).name() : ""; }
const char* place_title(int i) { return i >= 0 && i < s_stage.count() ? s_stage.scene(i).title() : ""; }
int find_place(const char* id) { return s_stage.find(id); }
int place() { return s_place.load(std::memory_order_relaxed); }
void go_place(int index) {
    if (index >= 0 && index < s_stage.count()) s_req_place.store(index);
}
void set_transition(pixelia::Transition t) { s_transition.store(static_cast<int>(t)); }
pixelia::Transition transition() { return static_cast<pixelia::Transition>(s_transition.load()); }
void set_tour(int seconds) { s_tour_s.store(std::clamp(seconds, 0, 3600)); }
int tour() { return s_tour_s.load(); }
void set_figures(int n) { s_figures.store(std::clamp(n, 0, 64)); }
int figures() { return s_figures.load(); }
bool place_command(const char* cmd) {
    const int c = std::strcmp(cmd, "ausbau") == 0 ? 1 : std::strcmp(cmd, "baustelle") == 0 ? 2 : 0;
    if (c) s_req_cmd.store(c);
    return c != 0;
}
void set_follow(bool on) { s_follow.store(on); }
bool follow() { return s_follow.load(); }
void set_inspect(bool on) { s_inspect.store(on); }
bool inspect() { return s_inspect.load(); }
void reset_places() {
    s_tour_s.store(0);
    s_figures.store(0);
    s_follow.store(true);
    s_inspect.store(false);
    s_transition.store(static_cast<int>(pixelia::kDefaultTransition));
    s_req_place.store(s_landscape);
}
void set_time_of_day(int hour, int minute) {
    s_follow_real = false;
    s_req_minute_of_day = (hour % 24) * 60 + minute % 60;
}
void set_speed(float factor) {
    s_speed = factor > 0 ? factor : 1.0f;
    if (s_speed != 1.0f) s_follow_real = false;
}
float speed() { return s_speed; }
void set_real_time(bool on) {
    if (on) {
        s_speed = 1.0f;
        s_next_real_sync = 0;
    }
    s_follow_real = on;
}
bool real_time() { return s_follow_real; }

EngineStats engine_stats(bool reset_max) {
    portENTER_CRITICAL(&s_stats_lock);
    EngineStats s = s_stats;
    if (reset_max) s_stats.render_us_max = 0;
    portEXIT_CRITICAL(&s_stats_lock);
    s.buffers_in_psram = s_buf_psram;
    s.buffers_ready = s_buf[0] != nullptr;
    s.sprites = s_sprites;
    return s;
}

bool grab(Frame* out, int timeout_ms, int min_age_ms) {
    if (!s_snap_px) {
        s_snap_px = static_cast<uint8_t*>(heap_caps_malloc(kBufBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!s_snap_px) return false;
    }
    const int64_t now = esp_timer_get_time();
    bool fresh = false;
    if (s_snap_state.load(std::memory_order_acquire) != 2 || now - s_snap_at >= min_age_ms * 1000LL) {
        s_snap_state.store(1, std::memory_order_release);
        for (int waited = 0; s_snap_state.load(std::memory_order_acquire) != 2; waited += 5) {
            if (waited >= timeout_ms) {
                s_snap_state.store(0, std::memory_order_release);
                return false;
            }
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        s_snap_at = esp_timer_get_time();
        fresh = true;
    }
    *out = Frame{s_snap_px, s_snap_pal, s_snap_frame, s_snap_us, fresh};
    return true;
}

}  // namespace app::gfx

namespace app::ui {

namespace {
int64_t until(int seconds) { return esp_timer_get_time() + static_cast<int64_t>(seconds) * 1000000; }
}  // namespace

void show_setup(const pixelia::ui::SetupInfo& info) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (gfx::s_ui.setup && gfx::s_ui.setup_info == info) return;
    gfx::s_ui.setup = true;
    gfx::s_ui.setup_info = info;
    ++gfx::s_ui.version;
}

void hide_setup() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (!gfx::s_ui.setup) return;
    gfx::s_ui.setup = false;
    ++gfx::s_ui.version;
}

void show_update(const pixelia::ui::UpdateInfo& info) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.update = true;
    gfx::s_ui.update_info = info;
    // Fehlermeldung nach 12 s ausblenden, die alte Version läuft ja weiter
    gfx::s_ui.update_until = info.phase == pixelia::ui::UpdateInfo::Phase::Failed ? until(12) : 0;
    ++gfx::s_ui.version;
}

void hide_update() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.update = false;
    ++gfx::s_ui.version;
}

void show_message(const pixelia::ui::MessageInfo& info, int seconds) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.message = true;
    gfx::s_ui.message_info = info;
    gfx::s_ui.message_until = until(seconds);
    ++gfx::s_ui.version;
}

void hide_message() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.message = false;
    ++gfx::s_ui.version;
}

void show_reset(const pixelia::ui::ResetInfo& info, int seconds) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    // Auch ohne neue Daten Version erhöhen: Der Grafik-Task übernimmt die neue Anzeigedauer nur so
    gfx::s_ui.reset_until = until(seconds);
    gfx::s_ui.reset = true;
    gfx::s_ui.reset_info = info;
    ++gfx::s_ui.version;
}

void hide_reset() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (!gfx::s_ui.reset) return;
    gfx::s_ui.reset = false;
    ++gfx::s_ui.version;
}

bool reset_visible() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    return gfx::s_ui.reset && esp_timer_get_time() < gfx::s_ui.reset_until;
}

void toast(std::string_view text, pixelia::ui::Tone tone, int seconds) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.toast = text;
    gfx::s_ui.toast_tone = tone;
    gfx::s_ui.toast_until = until(seconds);
    ++gfx::s_ui.version;
}

void show_info(const pixelia::ui::InfoCardData& data, int seconds) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    gfx::s_ui.info = true;
    gfx::s_ui.info_data = data;
    gfx::s_ui.info_until = until(seconds);
    ++gfx::s_ui.version;
}

void refresh_info(const pixelia::ui::InfoCardData& data) {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (!gfx::s_ui.info || gfx::s_ui.info_data == data) return;
    gfx::s_ui.info_data = data;
    ++gfx::s_ui.version;
}

void hide_info() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    if (!gfx::s_ui.info) return;
    gfx::s_ui.info = false;
    ++gfx::s_ui.version;
}

bool info_visible() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    return gfx::s_ui.info && esp_timer_get_time() < gfx::s_ui.info_until;
}

bool update_visible() {
    std::lock_guard<std::mutex> g(gfx::s_ui_lock);
    const auto& u = gfx::s_ui;
    return u.update && (u.update_until == 0 || esp_timer_get_time() < u.update_until);
}

void set_hold_progress(float progress) {
    gfx::s_hold_permille.store(progress < 0 ? -1 : static_cast<int>(progress * 1000.0f), std::memory_order_relaxed);
}

void show_picture(uint8_t* pixels, const pixelia::Color* palette240, int seconds) {
    if (seconds < 1 || seconds > kPictureMaxSeconds) seconds = kPictureMaxSeconds;
    uint8_t* old;
    {
        std::lock_guard<std::mutex> g(gfx::s_pic_lock);
        old = gfx::s_pic_px;
        gfx::s_pic_px = pixels;
        // Indizes 240–255 gehören der System-Oberfläche und bleiben schwarz (das Bild benutzt sie nicht)
        gfx::s_pic_pal = {};
        std::memcpy(gfx::s_pic_pal.data(), palette240, pixelia::ui::kFirstIndex * sizeof(pixelia::Color));
        gfx::s_pic_until = until(seconds);
        if (++gfx::s_pic_id == 0) gfx::s_pic_id = 1;
    }
    heap_caps_free(old);
}

void hide_picture() {
    uint8_t* old;
    {
        std::lock_guard<std::mutex> g(gfx::s_pic_lock);
        old = gfx::s_pic_px;
        gfx::s_pic_px = nullptr;
    }
    heap_caps_free(old);
}

bool picture_visible() {
    std::lock_guard<std::mutex> g(gfx::s_pic_lock);
    return gfx::s_pic_px != nullptr;
}

}  // namespace app::ui
