// Ablage für Grafiken (seit 0.5.5): LittleFS in der Partition „storage“ (/data), microSD über SPI2 (/sd) und die
// Lade-Task für den Grafik-Speicher der Engine (engine/assets.h).
//
// - Quelle: eingebettet (Standard, immer vorhanden), littlefs oder sd; fehlt eine Datei dort, kommt sie eingebettet.
//   Umschalten nur zum Messen (Befehl „assets quelle“), nicht gespeichert.
// - Lade-Task auf Kern 0 (Priorität 3): arbeitet die Anfragen der Bühne ab, während Kern 1 weiter zeichnet. Sie
//   hängt beim Start auch die SD-Karte ein (ohne Karte dauert der Versuch etwas; die Grafik läuft dabei schon).
// - Messung: „assets messen“ liest jedes Paket aus jeder Quelle und eine 256-KB-Messdatei und zählt dabei die
//   Display-Aussetzer (desync).
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "app.h"
#include "board/display.h"
#include "board/pins.h"
#include "driver/sdspi_host.h"
#include "esp_heap_caps.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "pixelia/engine/assets.h"
#include "pixelia/engine/png.h"
#include "sdmmc_cmd.h"

namespace app::assets {

namespace {

constexpr const char* TAG = "assets";
constexpr const char* kLfsMount = "/data";
constexpr const char* kLfsDir = "/data/assets";
constexpr const char* kSdMount = "/sd";
constexpr const char* kSdDir = "/sd/pixelia/assets";
constexpr const char* kBenchFile = "messung.pxa";
constexpr uint32_t kBenchBytes = 256 * 1024;
constexpr uint32_t kBudget = 2u << 20;  // Grafik-Speicher im PSRAM (Grundausstattung + Ortspakete)
constexpr uint32_t kLoaderStack = 6144;  // Lade-Task: Dateisysteme, Messung, Log mit Kommazahlen

pixelia::FileSource s_lfs_src("littlefs", kLfsDir);
pixelia::FileSource s_sd_src("sd", kSdDir);
std::atomic<bool> s_lfs{false};
std::atomic<bool> s_sd{false};
std::atomic<int> s_sd_state{0};  // 0 = noch nicht versucht, 1 = eingehängt, 2 = keine Karte
sdmmc_card_t* s_card = nullptr;
TaskHandle_t s_loader = nullptr;

uint64_t now_us() { return static_cast<uint64_t>(esp_timer_get_time()); }

void mount_littlefs() {
    esp_vfs_littlefs_conf_t conf{};
    conf.base_path = kLfsMount;
    conf.partition_label = "storage";
    conf.format_if_mount_failed = true;  // leer oder nach dem alten Flash-Test: neu anlegen
    const int64_t t0 = esp_timer_get_time();
    const esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LittleFS: %s", esp_err_to_name(err));
        return;
    }
    size_t total = 0, used = 0;
    esp_littlefs_info("storage", &total, &used);
    mkdir(kLfsDir, 0775);
    s_lfs = true;
    ESP_LOGI(TAG, "LittleFS: %u von %u KB belegt (%" PRId64 " ms)", static_cast<unsigned>(used >> 10),
             static_cast<unsigned>(total >> 10), (esp_timer_get_time() - t0) / 1000);
}

void mount_sd() {
    static bool bus = false;
    if (s_sd) return;
    const int64_t t0 = esp_timer_get_time();
    if (!bus) {
        spi_bus_config_t cfg{};
        cfg.mosi_io_num = board::kSdMosi;
        cfg.miso_io_num = board::kSdMiso;
        cfg.sclk_io_num = board::kSdClk;
        cfg.quadwp_io_num = -1;
        cfg.quadhd_io_num = -1;
        cfg.max_transfer_sz = 16 * 1024;
        const esp_err_t err = spi_bus_initialize(SPI2_HOST, &cfg, SDSPI_DEFAULT_DMA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "SD: SPI-Bus %s", esp_err_to_name(err));
            s_sd_state = 2;
            return;
        }
        bus = true;
    }
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;
    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = board::kSdCs;
    slot.host_id = SPI2_HOST;
    esp_vfs_fat_mount_config_t mc{};
    mc.format_if_mount_failed = false;  // nie eine fremde Karte löschen
    mc.max_files = 4;
    mc.allocation_unit_size = 16 * 1024;
    // ohne Karte melden die Treiber Fehler und einen Pin-Konflikt: leise, die Zeile unten reicht
    for (const char* tag : {"vfs_fat_sdmmc", "sdmmc_init", "sdmmc_common", "sdspi_host", "sdspi_transaction"})
        esp_log_level_set(tag, ESP_LOG_NONE);
    esp_log_level_set("gpio", ESP_LOG_ERROR);
    const esp_err_t err = esp_vfs_fat_sdspi_mount(kSdMount, &host, &slot, &mc, &s_card);
    esp_log_level_set("gpio", ESP_LOG_INFO);
    if (err != ESP_OK) {
        spi_bus_free(SPI2_HOST);  // spart internes RAM, „sd neu“ legt ihn wieder an
        bus = false;
        s_sd_state = 2;
        ESP_LOGI(TAG, "SD: keine Karte (%s, %" PRId64 " ms)", esp_err_to_name(err), (esp_timer_get_time() - t0) / 1000);
        return;
    }
    mkdir("/sd/pixelia", 0775);
    mkdir(kSdDir, 0775);
    s_sd = true;
    s_sd_state = 1;
    const uint64_t mb = static_cast<uint64_t>(s_card->csd.capacity) * s_card->csd.sector_size >> 20;
    ESP_LOGI(TAG, "SD: %s, %" PRIu64 " MB, Takt %d kHz (%" PRId64 " ms)", s_card->cid.name, mb,
             s_card->max_freq_khz, (esp_timer_get_time() - t0) / 1000);
}

void unmount_sd() {
    if (!s_sd) return;
    s_sd = false;
    esp_vfs_fat_sdcard_unmount(kSdMount, s_card);
    s_card = nullptr;
    s_sd_state = 0;
}

// Aufträge an die Lade-Task (Dateisystem-Arbeit braucht Stack; Konsole und Testläufe haben nur 3–4 KB)
enum class Job : uint8_t { CopyLfs, CopySd, Measure, FlashLoad, SdRetry };
struct JobReq {
    Job job;
    int arg;
    SemaphoreHandle_t done;  // nullptr = nicht warten
};
QueueHandle_t s_jobs = nullptr;

void log_last() {
    pixelia::AssetStore::PackInfo p;
    if (!pixelia::asset_store().last(&p)) return;
    if (p.state != pixelia::AssetStore::State::Ready) {
        ESP_LOGW(TAG, "Paket %s fehlt oder ist kaputt (Bilder fehlen)", p.name);
        return;
    }
    ESP_LOGI(TAG, "Paket %s aus %s: %.1f KB gepackt, %.1f KB im PSRAM, lesen %.2f ms, prüfen+entpacken %.2f ms, "
             "bereit nach %.2f ms", p.name, p.source, p.file_bytes / 1024.0, p.bytes / 1024.0, p.read_us / 1000.0,
             p.unpack_us / 1000.0, p.wait_us / 1000.0);
}

void copy_to(const char* label, const char* dir);
void measure_all();
void flash_load_impl(int seconds);

void run_job(const JobReq& r) {
    switch (r.job) {
        case Job::CopyLfs: copy_to("littlefs", kLfsDir); break;
        case Job::CopySd: copy_to("sd", kSdDir); break;
        case Job::Measure: measure_all(); break;
        case Job::FlashLoad: flash_load_impl(r.arg); break;
        case Job::SdRetry:
            unmount_sd();
            mount_sd();
            break;
    }
    if (r.done) xSemaphoreGive(r.done);
}

void loader_task(void*) {
    mount_sd();  // SD-Karte im Hintergrund: der Start wartet nicht auf sie
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
        while (pixelia::asset_store().work()) log_last();  // Pakete zuerst: die Bühne wartet vielleicht
        JobReq r;
        while (s_jobs && xQueueReceive(s_jobs, &r, 0) == pdTRUE) {
            run_job(r);
            while (pixelia::asset_store().work()) log_last();
        }
    }
}

void wake() {
    if (s_loader) xTaskNotifyGive(s_loader);
}

// Auftrag stellen; wait_s > 0 wartet höchstens so lange auf das Ende
bool post(Job job, int arg = 0, int wait_s = 0) {
    if (!s_jobs) return false;
    JobReq r{job, arg, wait_s > 0 ? xSemaphoreCreateBinary() : nullptr};
    if (xQueueSend(s_jobs, &r, 0) != pdTRUE) {
        if (r.done) vSemaphoreDelete(r.done);
        return false;
    }
    wake();
    if (!r.done) return true;
    const bool ok = xSemaphoreTake(r.done, pdMS_TO_TICKS(wait_s * 1000)) == pdTRUE;
    if (ok) vSemaphoreDelete(r.done);  // sonst gibt die Lade-Task ihn noch: dann bleibt er (selten, klein)
    return ok;
}

pixelia::AssetSource* source_by_name(const char* name) {
    if (std::strcmp(name, "eingebettet") == 0) return &pixelia::embedded_source();
    if (std::strcmp(name, "littlefs") == 0) return s_lfs ? &s_lfs_src : nullptr;
    if (std::strcmp(name, "sd") == 0) return s_sd ? &s_sd_src : nullptr;
    return nullptr;
}

// Messdatei: ein Paket mit einem Eintrag „Daten“ (256 KB, ungepackt, Inhalt aus einem festen Zufallsstrom)
bool write_bench(const char* dir) {
    char path[64];
    std::snprintf(path, sizeof(path), "%s/%s", dir, kBenchFile);
    std::FILE* f = std::fopen(path, "wb");
    if (!f) return false;
    static constexpr char kName[] = "messung";
    const uint16_t names = sizeof(kName);
    uint8_t* buf = static_cast<uint8_t*>(heap_caps_malloc(kBenchBytes, MALLOC_CAP_SPIRAM));
    if (!buf) {
        std::fclose(f);
        return false;
    }
    uint32_t x = 0x12345678u;
    for (uint32_t i = 0; i < kBenchBytes; ++i) {
        x = x * 1664525u + 1013904223u;
        buf[i] = static_cast<uint8_t>(x >> 24);
    }
    uint8_t dir_entry[pixelia::kPackEntryBytes] = {};
    const uint32_t id = pixelia::asset_id(kName);
    std::memcpy(dir_entry, &id, 4);
    dir_entry[4] = static_cast<uint8_t>(pixelia::AssetType::Data);
    std::memcpy(dir_entry + 12, &kBenchBytes, 4);
    std::memcpy(dir_entry + 16, &kBenchBytes, 4);
    uint32_t crc = pixelia::crc32(dir_entry, sizeof(dir_entry));
    crc = pixelia::crc32(reinterpret_cast<const uint8_t*>(kName), names, crc);
    crc = pixelia::crc32(buf, kBenchBytes, crc);
    uint8_t head[pixelia::kPackHeaderBytes] = {'P', 'X', 'A', '1', 1, 0};
    std::memcpy(head + 6, &names, 2);
    std::memcpy(head + 8, &kBenchBytes, 4);
    std::memcpy(head + 12, &kBenchBytes, 4);
    std::memcpy(head + 16, &crc, 4);
    bool ok = std::fwrite(head, 1, sizeof(head), f) == sizeof(head) &&
              std::fwrite(dir_entry, 1, sizeof(dir_entry), f) == sizeof(dir_entry) &&
              std::fwrite(kName, 1, names, f) == names && std::fwrite(buf, 1, kBenchBytes, f) == kBenchBytes;
    ok = std::fclose(f) == 0 && ok;
    heap_caps_free(buf);
    return ok;
}

// Eingebettete Pakete und die Messdatei in einen Ordner schreiben
void copy_to(const char* label, const char* dir) {
    int n = 0;
    const int64_t t0 = esp_timer_get_time();
    for (int i = 0; i < pixelia::kEmbeddedAssetCount; ++i) {
        const pixelia::EmbeddedAsset& a = pixelia::kEmbeddedAssets[i];
        char path[64];
        std::snprintf(path, sizeof(path), "%s/%s", dir, a.file);
        std::FILE* f = std::fopen(path, "wb");
        const bool ok = f && std::fwrite(a.data, 1, a.size, f) == a.size;
        if (f) std::fclose(f);
        if (!ok) {
            ESP_LOGE(TAG, "%s: %s nicht geschrieben", label, path);
            return;
        }
        ++n;
    }
    const bool bench = write_bench(dir);
    ESP_LOGI(TAG, "%s: %d Pakete%s nach %s geschrieben (%" PRId64 " ms)", label, n, bench ? " und Messdatei" : "",
             dir, (esp_timer_get_time() - t0) / 1000);
}

// Flash-Last wie bei Spielständen: Datei in LittleFS immer wieder schreiben (je 4 KB mit fsync, also Löschen und
// Schreiben im Flash), danach löschen. Bis 0.5.4 schrieb der Test roh in „storage“; das würde LittleFS zerstören.
void flash_load_impl(int seconds) {
    if (!s_lfs) {
        ESP_LOGE("flash", "LittleFS nicht eingehängt");
        return;
    }
    constexpr size_t kSector = 4096;
    constexpr int kSectors = 64;  // 256 KB je Durchgang
    constexpr const char* kPath = "/data/flashlast.bin";
    auto* buf = static_cast<uint32_t*>(heap_caps_malloc(kSector, MALLOC_CAP_SPIRAM));
    if (!buf) return;
    for (size_t i = 0; i < kSector / 4; ++i) buf[i] = 0xA5A50000u + i;
    const uint32_t desync0 = board::display::stats(false).desyncs;
    const int64_t t0 = esp_timer_get_time();
    const int64_t end = t0 + static_cast<int64_t>(seconds) * 1000000;
    size_t sectors = 0;
    bool ok = true;
    while (esp_timer_get_time() < end && ok) {
        std::FILE* f = std::fopen(kPath, "wb");
        ok = f != nullptr;
        for (int i = 0; ok && i < kSectors && esp_timer_get_time() < end; ++i) {
            ok = std::fwrite(buf, 1, kSector, f) == kSector && std::fflush(f) == 0 && fsync(fileno(f)) == 0;
            ++sectors;
        }
        if (f) std::fclose(f);
    }
    const double dt = (esp_timer_get_time() - t0) / 1e6;
    std::remove(kPath);
    heap_caps_free(buf);
    ESP_LOGI("flash", "%u Sektoren (%u KB) in %.1f s in LittleFS geschrieben: %s, Display-Aussetzer dabei: %" PRIu32,
             static_cast<unsigned>(sectors), static_cast<unsigned>(sectors * kSector / 1024), dt, ok ? "ok" : "Fehler",
             board::display::stats(false).desyncs - desync0);
}

void measure(pixelia::AssetSource& src, bool with_bench) {
    pixelia::AssetStore& store = pixelia::asset_store();
    const uint32_t desync0 = board::display::stats(false).desyncs;
    uint32_t files = 0, us = 0;
    for (int i = 0; i < pixelia::kEmbeddedAssetCount; ++i) {
        const char* file = pixelia::kEmbeddedAssets[i].file;
        const auto p = store.probe(src, file);
        if (!p.ok) {
            ESP_LOGW(TAG, "  %s: %s fehlt oder kaputt", src.name(), file);
            continue;
        }
        ++files;
        us += p.read_us + p.unpack_us;
        ESP_LOGI(TAG, "  %s: %-11s %6.1f KB lesen %6.2f ms, prüfen+entpacken %5.2f ms (%5.1f KB im PSRAM)",
                 src.name(), file, p.file_bytes / 1024.0, p.read_us / 1000.0, p.unpack_us / 1000.0, p.bytes / 1024.0);
    }
    if (with_bench) {
        const auto p = store.probe(src, kBenchFile);
        if (p.ok) {
            const double mbs = p.read_us ? p.file_bytes / static_cast<double>(p.read_us) : 0;  // Byte/µs = MB/s
            ESP_LOGI(TAG, "  %s: %s %6.1f KB lesen %6.2f ms (%.2f MB/s), prüfen %5.2f ms → Ort mit 200 KB ≈ %.0f ms",
                     src.name(), kBenchFile, p.file_bytes / 1024.0, p.read_us / 1000.0, mbs, p.unpack_us / 1000.0,
                     (p.read_us + p.unpack_us) * (200.0 * 1024 / p.file_bytes) / 1000.0);
        } else {
            ESP_LOGW(TAG, "  %s: Messdatei fehlt (erst „assets kopieren %s“)", src.name(), src.name());
        }
    }
    ESP_LOGI(TAG, "  %s: %" PRIu32 " Pakete in %.2f ms, Display-Aussetzer dabei: %" PRIu32, src.name(), files,
             us / 1000.0, board::display::stats(false).desyncs - desync0);
}

void measure_all() {
    ESP_LOGI(TAG, "Messung auf der Lade-Task (Kern 0), die Grafik läuft weiter:");
    measure(pixelia::embedded_source(), false);
    if (s_lfs) measure(s_lfs_src, true);
    if (s_sd) measure(s_sd_src, true);
    else ESP_LOGI(TAG, "  sd: keine Karte, nicht gemessen");
    ESP_LOGI(TAG, "Stack der Lade-Task: %u Byte frei", static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
}

const char* state_name(pixelia::AssetStore::State s) {
    switch (s) {
        case pixelia::AssetStore::State::Queued: return "wartet";
        case pixelia::AssetStore::State::Loading: return "lädt";
        case pixelia::AssetStore::State::Ready: return "geladen";
        case pixelia::AssetStore::State::Failed: return "fehlt";
        default: return "-";
    }
}

void print_status() {
    pixelia::AssetStore& store = pixelia::asset_store();
    const auto s = store.stats();
    ESP_LOGI(TAG, "Quelle %s (Rückfall eingebettet); LittleFS %s, SD %s; Stack der Lade-Task %u Byte frei", source(),
             s_lfs ? "bereit" : "fehlt",
             s_sd ? "bereit" : s_sd_state == 2 ? "keine Karte" : "noch nicht versucht",
             s_loader ? static_cast<unsigned>(uxTaskGetStackHighWaterMark(s_loader)) : 0u);
    ESP_LOGI(TAG, "Grafik-Speicher %.1f von %.0f KB (Höchststand %.1f KB), %d Pakete, %" PRIu32 " mal geladen, %" PRIu32
             " verdrängt, %" PRIu32 " fehlgeschlagen, %" PRIu32 " über Budget", s.used / 1024.0, s.budget / 1024.0,
             s.peak / 1024.0, s.packs, s.loads, s.evictions, s.failures, s.over_budget);
    pixelia::AssetStore::PackInfo list[pixelia::AssetStore::kMaxPacks];
    const int n = store.list(list, pixelia::AssetStore::kMaxPacks);
    for (int i = 0; i < n; ++i) {
        const auto& p = list[i];
        ESP_LOGI(TAG, "  %-8s %-8s %s%s %5.1f KB  aus %-11s lesen %.2f ms, entpacken %.2f ms, bereit nach %.2f ms (%u×)",
                 p.name, state_name(p.state), p.base ? "Grund" : p.pins ? "angeh." : "      ", "", p.bytes / 1024.0,
                 p.source[0] ? p.source : "-", p.read_us / 1000.0, p.unpack_us / 1000.0, p.wait_us / 1000.0, p.loads);
    }
}

}  // namespace

void start() {
    const size_t ram0 = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    mount_littlefs();
    pixelia::AssetStore& store = pixelia::asset_store();
    store.set_budget(kBudget);
    store.set_clock(now_us);
    store.set_sources(&pixelia::embedded_source(), nullptr);
    s_jobs = xQueueCreate(4, sizeof(JobReq));
    xTaskCreatePinnedToCore(loader_task, "assets", kLoaderStack, nullptr, 3, &s_loader, kSystemCore);
    store.set_async(wake);  // ab jetzt lädt die Lade-Task (die Grundausstattung lädt gfx::start sofort)
    ESP_LOGI(TAG, "Ablage bereit, internes RAM dafür %u KB",
             static_cast<unsigned>((ram0 - heap_caps_get_free_size(MALLOC_CAP_INTERNAL)) >> 10));
}

bool flash_load(int seconds) { return post(Job::FlashLoad, seconds, seconds + 30); }

bool littlefs_ready() { return s_lfs; }
bool sd_ready() { return s_sd; }

const char* source() {
    const pixelia::AssetSource* s = pixelia::asset_store().source();
    return s ? s->name() : "-";
}

Summary summary() {
    Summary out{};
    const auto s = pixelia::asset_store().stats();
    out.used = s.used;
    out.budget = s.budget;
    out.peak = s.peak;
    out.packs = s.packs;
    out.loads = s.loads;
    out.evictions = s.evictions;
    out.failures = s.failures;
    out.source = source();
    out.littlefs = s_lfs;
    out.sd = s_sd;
    pixelia::AssetStore::PackInfo p;
    if (pixelia::asset_store().last(&p)) {
        std::snprintf(out.last, sizeof(out.last), "%s", p.name);
        out.last_us = p.wait_us;
        out.last_source = p.source;
    }
    return out;
}

void console(int argc, char** argv) {
    pixelia::AssetStore& store = pixelia::asset_store();
    const char* sub = argc >= 2 ? argv[1] : "";
    if (std::strcmp(argv[0], "sd") == 0) {
        if (std::strcmp(sub, "neu") == 0) {
            ESP_LOGI(TAG, "SD wird neu eingehängt (Lade-Task)");
            post(Job::SdRetry, 0, 10);
            return;
        }
        if (s_sd && s_card) {
            const uint64_t mb = static_cast<uint64_t>(s_card->csd.capacity) * s_card->csd.sector_size >> 20;
            ESP_LOGI(TAG, "SD: %s, %" PRIu64 " MB, Takt %d kHz, eingehängt unter %s", s_card->cid.name, mb,
                     s_card->max_freq_khz, kSdMount);
        } else {
            ESP_LOGI(TAG, "SD: %s (Pins CS %d, MOSI %d, CLK %d, MISO %d; „sd neu“ versucht es noch einmal)",
                     s_sd_state == 2 ? "keine Karte" : "noch nicht versucht", board::kSdCs, board::kSdMosi,
                     board::kSdClk, board::kSdMiso);
        }
        return;
    }
    if (std::strcmp(sub, "quelle") == 0 && argc >= 3) {
        pixelia::AssetSource* src = source_by_name(argv[2]);
        if (!src) {
            ESP_LOGW(TAG, "Quelle %s gibt es nicht oder sie ist nicht bereit (eingebettet, littlefs, sd)", argv[2]);
            return;
        }
        store.set_sources(src, &pixelia::embedded_source());
        store.drop_unpinned();  // nächstes Laden kommt aus der neuen Quelle
        ESP_LOGI(TAG, "Quelle %s; nicht angeheftete Pakete freigegeben", src->name());
    } else if (std::strcmp(sub, "kopieren") == 0 && argc >= 3) {
        // auf der Lade-Task (Stack); die Konsole wartet, damit die Ausgabe in der Antwort steht
        if (std::strcmp(argv[2], "littlefs") == 0 && s_lfs) post(Job::CopyLfs, 0, 20);
        else if (std::strcmp(argv[2], "sd") == 0 && s_sd) post(Job::CopySd, 0, 20);
        else ESP_LOGW(TAG, "Ziel %s nicht bereit (littlefs, sd)", argv[2]);
        return;
    } else if (std::strcmp(sub, "messen") == 0) {
        post(Job::Measure, 0, 20);
        return;
    } else if (std::strcmp(sub, "leeren") == 0) {
        store.drop_unpinned();
        ESP_LOGI(TAG, "nicht angeheftete Pakete freigegeben");
    } else if (std::strcmp(sub, "budget") == 0 && argc >= 3) {
        store.set_budget(static_cast<uint32_t>(std::atoi(argv[2])) << 10);
        ESP_LOGI(TAG, "Budget %s KB (gilt beim nächsten Laden)", argv[2]);
    } else if (sub[0]) {
        ESP_LOGW(TAG, "assets [quelle eingebettet|littlefs|sd | kopieren littlefs|sd | messen | leeren | budget <KB>]");
        return;
    }
    print_status();
}

}  // namespace app::assets
