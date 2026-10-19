// Updates: Upload mit Prüfung, Probelauf mit Selbsttest, Erkennen eines Rückfalls (siehe ota.h).

#include "ota.h"

#include <cstdio>
#include <cstring>
#include <mutex>

#include "app.h"
#include "esp_app_format.h"
#include "esp_bootloader_desc.h"
#include "esp_chip_info.h"
#include "esp_core_dump.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logbuf.h"
#include "net.h"
#include "nvs.h"
#include "ui.h"

namespace app::ota {

namespace {
constexpr const char* TAG = "ota";
constexpr const char* kNvs = "ota";

std::mutex s_lock;
Status s_status{};
esp_ota_handle_t s_handle = 0;
const esp_partition_t* s_target = nullptr;
uint32_t s_ui_bytes = 0;  // Stand der letzten Display-Aktualisierung

using pixelia::ui::UpdateInfo;

void sha_hex(const uint8_t* sha, char* out) {
    for (int i = 0; i < 8; ++i) std::snprintf(out + 2 * i, 3, "%02x", sha[i]);
}

// Merkt sich für die alte Version, warum der Probelauf gescheitert ist
void store_reason(const char* sha, const char* why) {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_str(h, "rb_sha", sha);
    nvs_set_str(h, "rb_why", why);
    nvs_set_u8(h, "rb_ack", 0);
    nvs_commit(h);
    nvs_close(h);
}

const char* reset_reason_for_rollback(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_PANIC: return "Absturz im Probelauf";
        case ESP_RST_TASK_WDT:
        case ESP_RST_INT_WDT:
        case ESP_RST_WDT: return "Hänger im Probelauf (Watchdog)";
        case ESP_RST_BROWNOUT: return "Unterspannung im Probelauf";
        case ESP_RST_POWERON: return "Strom während des Probelaufs unterbrochen";
        default: return "Neustart während des Probelaufs";
    }
}

void detect_rollback() {
    const esp_partition_t* inv = esp_ota_get_last_invalid_partition();
    if (!inv) return;
    esp_app_desc_t d;
    if (esp_ota_get_partition_description(inv, &d) != ESP_OK) return;
    char sha[17];
    sha_hex(d.app_elf_sha256, sha);

    char nvs_sha[17] = "", why[96] = "";
    uint8_t ack = 0;
    bool fresh = false;
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READONLY, &h) == ESP_OK) {
        size_t len = sizeof(nvs_sha);
        nvs_get_str(h, "rb_sha", nvs_sha, &len);
        len = sizeof(why);
        nvs_get_str(h, "rb_why", why, &len);
        nvs_get_u8(h, "rb_ack", &ack);
        nvs_close(h);
    }
    if (std::strcmp(nvs_sha, sha) != 0) {
        // Neuer Rückfall, die gescheiterte Version konnte nichts mehr speichern: Grund aus dem Resetgrund
        std::snprintf(why, sizeof(why), "%s", reset_reason_for_rollback(esp_reset_reason()));
        esp_core_dump_summary_t sum{};
        if (esp_core_dump_get_summary(&sum) == ESP_OK &&
            std::strncmp(reinterpret_cast<const char*>(sum.app_elf_sha256), sha, 8) == 0) {
            size_t n = std::strlen(why);
            std::snprintf(why + n, sizeof(why) - n, ", Task „%.15s“ bei 0x%08lx", sum.exc_task,
                          static_cast<unsigned long>(sum.exc_pc));
        }
        store_reason(sha, why);
        ack = 0;
        fresh = true;
    }
    std::lock_guard<std::mutex> g(s_lock);
    s_status.rollback = ack == 0;
    s_status.rollback_new = fresh;
    std::snprintf(s_status.rollback_version, sizeof(s_status.rollback_version), "%s", d.version);
    std::snprintf(s_status.rollback_reason, sizeof(s_status.rollback_reason), "%s", why);
}

#if defined(PIXELIA_TEST_SHIFT)
// Testvariante „crash-shift“: 64 KB Konstanten mehr verschieben den No-Init-Bereich im PSRAM um eine MMU-Seite
// (Zugriff über volatile, damit der Compiler das Feld nicht wegoptimiert)
const uint8_t kShiftBlock[64 * 1024] = {1};
#endif

// Testvarianten für den Rollback-Test (nur lokale Builds mit -DPIXELIA_TEST=…, siehe docs/HARDWARE.md)
void test_misbehave(int elapsed_s) {
#if defined(PIXELIA_TEST_SHIFT)
    if (elapsed_s == 1) {
        const volatile uint8_t* block = kShiftBlock;
        ESP_LOGW("test", "Testversion mit verschobenem Log-Puffer (Probe %u)", block[(elapsed_s * 4096) % sizeof(kShiftBlock)]);
    }
#endif
#if defined(PIXELIA_TEST_CRASH)
    if (elapsed_s == 20) {
        ESP_LOGE("test", "Testversion: absichtlicher Absturz im Probelauf");
        vTaskDelay(pdMS_TO_TICKS(100));
        *static_cast<volatile int*>(nullptr) = 1;
    }
#elif defined(PIXELIA_TEST_HANG)
    if (elapsed_s == 20) {
        ESP_LOGE("test", "Testversion: absichtlicher Hänger auf Kern 0 im Probelauf");
        vTaskDelay(pdMS_TO_TICKS(100));
        xTaskCreatePinnedToCore([](void*) { for (;;) {} }, "haenger", 2048, nullptr, configMAX_PRIORITIES - 2, nullptr,
                                kSystemCore);
    }
#else
    (void)elapsed_s;
#endif
}

void selftest_task(void*) {
    const esp_app_desc_t* me = esp_app_get_description();
    char text[96];
    std::snprintf(text, sizeof(text), "Probelauf von %s: Selbsttest läuft (%d s)", me->version, kTrialSeconds);
    ui::toast(text, pixelia::ui::Tone::Info, 10);
    ESP_LOGW(TAG, "Probelauf: Selbsttest läuft (mindestens %d s, höchstens %d s)", kTrialSeconds, kTrialMaxSeconds);

    uint32_t vs_prev = board::display::stats(false).vsyncs;
    uint32_t fr_prev = gfx::engine_stats(false).frames;
    int ok_s = 0;
    for (int elapsed = 1;; ++elapsed) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        test_misbehave(elapsed);
        uint32_t vs = board::display::stats(false).vsyncs;
        uint32_t fr = gfx::engine_stats(false).frames;
        bool panel_ok = vs - vs_prev >= 50;  // 60 Hz
        bool engine_ok = fr - fr_prev >= 20;  // 30 fps
        vs_prev = vs;
        fr_prev = fr;
        wifi::Status ws = wifi::status();
        bool net_ok = ws.state == wifi::State::Connected || ws.ap_active;
#if defined(PIXELIA_TEST_SELFTEST_FAIL)
        engine_ok = false;
#endif
        if (panel_ok && engine_ok) ++ok_s;
        const char* note = !panel_ok ? "Display liefert keine Bilder" : !engine_ok ? "Engine liefert keine Bilder"
                           : !net_ok ? "weder WLAN noch Setup-Modus" : "läuft";
        {
            std::lock_guard<std::mutex> g(s_lock);
            s_status.trial_elapsed_s = elapsed;
            s_status.trial_ok_s = ok_s;
            std::snprintf(s_status.trial_note, sizeof(s_status.trial_note), "%s", note);
        }
        if (elapsed >= kTrialSeconds && ok_s >= kTrialSeconds - 5 && net_ok) {
            esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGW(TAG, "Selbsttest bestanden nach %d s: Version %s gilt jetzt dauerhaft (%s)", elapsed, me->version,
                     esp_err_to_name(err));
            {
                std::lock_guard<std::mutex> g(s_lock);
                s_status.trial = false;
                s_status.trial_passed = true;
            }
            log::set_mirror(false);
            std::snprintf(text, sizeof(text), "Update auf %s erfolgreich", me->version);
            ui::toast(text, pixelia::ui::Tone::Success, 8);
            break;
        }
        if (elapsed >= kTrialMaxSeconds) {
            char why[96];
            std::snprintf(why, sizeof(why), "Selbsttest nicht bestanden: %s", note);
            ESP_LOGE(TAG, "%s, zurück zur vorherigen Version", why);
            char sha[17];
            sha_hex(me->app_elf_sha256, sha);
            store_reason(sha, why);
            ui::show_message({"Selbsttest fehlgeschlagen", std::string(why) + "\nPixelia startet die vorherige Version.",
                              pixelia::ui::Tone::Error},
                             10);
            vTaskDelay(pdMS_TO_TICKS(3000));
            esp_ota_mark_app_invalid_rollback_and_reboot();
            break;
        }
    }
    vTaskDelete(nullptr);
}

void set_error(const char* text) {
    std::lock_guard<std::mutex> g(s_lock);
    std::snprintf(s_status.last_error, sizeof(s_status.last_error), "%s", text);
}

void show_progress(UpdateInfo::Phase phase, const char* message = "") {
    Status st = status();
    UpdateInfo u;
    u.phase = phase;
    u.from_version = st.version;
    u.to_version = st.new_version;
    u.bytes = st.bytes;
    u.total = st.total;
    u.message = message;
    ui::show_update(u);
}
}  // namespace

void init() {
    const esp_app_desc_t* me = esp_app_get_description();
    const esp_partition_t* running = esp_ota_get_running_partition();
    {
        std::lock_guard<std::mutex> g(s_lock);
        std::snprintf(s_status.version, sizeof(s_status.version), "%s", me->version);
        std::snprintf(s_status.running, sizeof(s_status.running), "%.7s", running ? running->label : "?");
    }
    esp_ota_img_states_t state;
    if (running && esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
        {
            std::lock_guard<std::mutex> g(s_lock);
            s_status.trial = true;
        }
        // Stürzt die neue Version im Probelauf ab, startet die alte mit anderer Lage des Log-Puffers: letzte Zeilen
        // zusätzlich zeilenweise im RTC-Speicher sichern
        log::set_mirror(true);
    }
    detect_rollback();
    Status st = status();
    if (st.rollback) {
        ESP_LOGW(TAG, "Update auf %s wurde zurückgerollt: %s", st.rollback_version, st.rollback_reason);
        char text[160];
        std::snprintf(text, sizeof(text), "Update auf %.31s zurückgerollt: %.95s", st.rollback_version, st.rollback_reason);
        ui::toast(text, pixelia::ui::Tone::Warning, 20);
    }
    log_status();
}

void start_selftest() {
    if (!status().trial) return;
    xTaskCreatePinnedToCore(selftest_task, "selbsttest", 4096, nullptr, 3, nullptr, kSystemCore);
}

Status status() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_status;
}

void ack_rollback() {
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, "rb_ack", 1);
        nvs_commit(h);
        nvs_close(h);
    }
    std::lock_guard<std::mutex> g(s_lock);
    s_status.rollback = false;
}

bool check_header(const uint8_t* head, size_t len, esp_app_desc_t* desc, char* error, size_t error_len) {
    if (len < kHeaderBytes) {
        std::snprintf(error, error_len, "Datei zu kurz für eine Firmware");
        return false;
    }
    esp_image_header_t ih;
    std::memcpy(&ih, head, sizeof(ih));
    if (ih.magic != ESP_IMAGE_HEADER_MAGIC) {
        std::snprintf(error, error_len, "Keine ESP32-Firmware (Kennung 0x%02X statt 0xE9)", ih.magic);
        return false;
    }
    if (ih.chip_id != ESP_CHIP_ID_ESP32S3) {
        std::snprintf(error, error_len, "Firmware für einen anderen Chip (ID %u), hier läuft ein ESP32-S3",
                      static_cast<unsigned>(ih.chip_id));
        return false;
    }
    const uint8_t* desc_ptr = head + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
    esp_app_desc_t ad;
    std::memcpy(&ad, desc_ptr, sizeof(ad));
    if (ad.magic_word != ESP_APP_DESC_MAGIC_WORD) {
        if (desc_ptr[0] == ESP_BOOTLOADER_DESC_MAGIC_BYTE) {
            std::snprintf(error, error_len, "Das ist ein Factory-Image für den Web-Flasher. Für Updates die .bin ohne „factory“ nehmen.");
        } else {
            std::snprintf(error, error_len, "Keine App-Beschreibung im Image");
        }
        return false;
    }
    ad.project_name[sizeof(ad.project_name) - 1] = 0;
    ad.version[sizeof(ad.version) - 1] = 0;
    if (std::strcmp(ad.project_name, "pixelia") != 0) {
        std::snprintf(error, error_len, "Das ist keine Pixelia-Firmware (Projekt „%.31s“)", ad.project_name);
        return false;
    }
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    if (chip.revision < ih.min_chip_rev_full || (ih.max_chip_rev_full && chip.revision > ih.max_chip_rev_full)) {
        std::snprintf(error, error_len, "Firmware passt nicht zur Chip-Revision v%d.%d", chip.revision / 100,
                      chip.revision % 100);
        return false;
    }
    if (desc) *desc = ad;
    return true;
}

esp_err_t begin(const uint8_t* head, size_t len, size_t total, char* error, size_t error_len) {
    esp_app_desc_t ad;
    if (!check_header(head, len, &ad, error, error_len)) {
        set_error(error);
        return ESP_ERR_INVALID_ARG;
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (s_status.busy) {
            std::snprintf(error, error_len, "Es läuft bereits ein Update");
            return ESP_ERR_INVALID_STATE;
        }
        if (s_status.trial) {
            std::snprintf(error, error_len, "Der Probelauf der aktuellen Version läuft noch, bitte kurz warten");
            return ESP_ERR_INVALID_STATE;
        }
    }
    s_target = esp_ota_get_next_update_partition(nullptr);
    if (!s_target || total > s_target->size) {
        std::snprintf(error, error_len, "Firmware zu groß für die App-Partition (%lu KB)",
                      static_cast<unsigned long>(s_target ? s_target->size / 1024 : 0));
        set_error(error);
        return ESP_ERR_INVALID_SIZE;
    }
    // Nur den Bereich der neuen Firmware vorab löschen (in 64-KB-Blöcken, deutlich schneller als 4-KB-Sektoren
    // während des Schreibens). Das Display läuft dabei weiter, weil Programm und Daten im PSRAM liegen.
    const int64_t t_erase = esp_timer_get_time();
    esp_err_t err = esp_ota_begin(s_target, total, &s_handle);
    ESP_LOGI(TAG, "%u KB gelöscht in %lld ms", static_cast<unsigned>(total / 1024), (esp_timer_get_time() - t_erase) / 1000);
    if (err != ESP_OK) {
        std::snprintf(error, error_len, "Update kann nicht starten: %s", esp_err_to_name(err));
        set_error(error);
        return err;
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.busy = true;
        s_status.bytes = 0;
        s_status.total = static_cast<uint32_t>(total);
        std::snprintf(s_status.new_version, sizeof(s_status.new_version), "%s", ad.version);
        s_status.last_error[0] = 0;
    }
    s_ui_bytes = 0;
    ESP_LOGW(TAG, "Update auf %s (%u KB, gebaut %s %s) nach %s", ad.version, static_cast<unsigned>(total / 1024),
             ad.date, ad.time, s_target->label);
    show_progress(UpdateInfo::Phase::Receiving);
    return ESP_OK;
}

esp_err_t write(const uint8_t* data, size_t len) {
    esp_err_t err = esp_ota_write(s_handle, data, len);
    if (err != ESP_OK) return err;
    uint32_t bytes;
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.bytes += len;
        bytes = s_status.bytes;
    }
    if (bytes - s_ui_bytes >= 16 * 1024) {
        s_ui_bytes = bytes;
        show_progress(UpdateInfo::Phase::Receiving);
    }
    return ESP_OK;
}

esp_err_t finish(char* error, size_t error_len) {
    show_progress(UpdateInfo::Phase::Verifying);
    esp_err_t err = esp_ota_end(s_handle);  // prüft Prüfsumme und SHA-256 des Images
    s_handle = 0;
    if (err == ESP_OK) err = esp_ota_set_boot_partition(s_target);
    if (err != ESP_OK) {
        std::snprintf(error, error_len, "%s",
                      err == ESP_ERR_OTA_VALIDATE_FAILED ? "Image beschädigt: Prüfsumme stimmt nicht"
                                                         : esp_err_to_name(err));
        set_error(error);
        {
            std::lock_guard<std::mutex> g(s_lock);
            s_status.busy = false;
        }
        show_progress(UpdateInfo::Phase::Failed, error);
        ESP_LOGE(TAG, "Update fehlgeschlagen: %s", error);
        return err;
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.busy = false;
    }
    show_progress(UpdateInfo::Phase::Done);
    ESP_LOGW(TAG, "Update geprüft und als Startpartition gesetzt (%s), Neustart folgt", s_target->label);
    return ESP_OK;
}

void cancel(const char* why) {
    if (s_handle) esp_ota_abort(s_handle);
    s_handle = 0;
    set_error(why);
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (!s_status.busy) return;
        s_status.busy = false;
    }
    ESP_LOGE(TAG, "Update abgebrochen: %s", why);
    show_progress(UpdateInfo::Phase::Failed, why);
}

void restart_soon(int ms) {
    static esp_timer_handle_t timer = nullptr;
    if (!timer) {
        esp_timer_create_args_t args{};
        args.callback = [](void*) { esp_restart(); };
        args.name = "neustart";
        esp_timer_create(&args, &timer);
    }
    esp_timer_start_once(timer, static_cast<uint64_t>(ms) * 1000);
}

void log_status() {
    Status st = status();
    const esp_partition_t* boot = esp_ota_get_boot_partition();
    ESP_LOGI(TAG, "Version %s aus %s, nächster Start aus %s%s", st.version, st.running, boot ? boot->label : "?",
             st.trial ? ", PROBELAUF" : "");
    for (const char* label : {"ota_0", "ota_1"}) {
        const esp_partition_t* p = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
        if (!p) continue;
        esp_app_desc_t d;
        esp_ota_img_states_t s;
        bool has = esp_ota_get_partition_description(p, &d) == ESP_OK;
        const char* state = "–";
        if (esp_ota_get_state_partition(p, &s) == ESP_OK) {
            state = s == ESP_OTA_IMG_VALID            ? "gültig"
                    : s == ESP_OTA_IMG_PENDING_VERIFY ? "Probelauf"
                    : s == ESP_OTA_IMG_INVALID        ? "ungültig"
                    : s == ESP_OTA_IMG_ABORTED        ? "abgebrochen"
                    : s == ESP_OTA_IMG_NEW            ? "neu"
                                                      : "unbestimmt";
        }
        ESP_LOGI(TAG, "  %s: %s (%s)", label, has ? d.version : "leer", state);
    }
    if (st.rollback) ESP_LOGW(TAG, "  Rückfall: %s – %s", st.rollback_version, st.rollback_reason);
}

}  // namespace app::ota
