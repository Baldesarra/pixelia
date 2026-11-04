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
#include "exper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logbuf.h"
#include "net.h"
#include "nvs.h"
#include "pixelia/engine/device_flow.h"
#include "ui.h"

namespace app::ota {

namespace {
constexpr const char* TAG = "ota";
constexpr const char* kNvs = "ota";

std::mutex s_lock;
Status s_status{};
pixelia::UpdateGate s_gate;  // B05: genau ein Update zur Zeit, nur der Besitzer schreibt
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

void set_error(const char* text) {
    std::lock_guard<std::mutex> g(s_lock);
    std::snprintf(s_status.last_error, sizeof(s_status.last_error), "%s", text);
}

// Selbsttest im Probelauf: Messen und Entscheiden in pixelia::TrialMachine (im Simulator geprüft, --test geraet),
// hier nur die SDK-Aufrufe. Erwartete Panelrate aus dem gerade aktiven Timing (Stromsparmodus 30 Hz, V07).
pixelia::TrialSample trial_sample() {
    pixelia::TrialSample s;
    s.now_us = esp_timer_get_time();
    s.vsyncs = board::display::stats(false).vsyncs;
    s.frames = gfx::engine_stats(false).frames;
    exper::active_rate(s.panel_hz, s.swap);
    const wifi::Status ws = wifi::status();
    s.net_ok = ws.state == wifi::State::Connected || ws.ap_active;
#if defined(PIXELIA_TEST_SELFTEST_FAIL)
    s.engine_fail = true;
#endif
    return s;
}

void publish_trial(const pixelia::TrialMachine& m) {
    std::lock_guard<std::mutex> g(s_lock);
    s_status.trial_elapsed_s = m.elapsed_s();
    s_status.trial_ok_s = m.ok_s();
    s_status.trial_phase = static_cast<uint8_t>(m.phase());
    s_status.trial_panel_hz = static_cast<float>(m.expected_panel());
    std::snprintf(s_status.trial_note, sizeof(s_status.trial_note), "%s", m.note());
}

void selftest_task(void*) {
    using pixelia::TrialMachine;
    const esp_app_desc_t* me = esp_app_get_description();
    char text[128];
    std::snprintf(text, sizeof(text), "Probelauf von %s: Selbsttest läuft (%d s)", me->version, kTrialSeconds);
    ui::toast(text, pixelia::ui::Tone::Info, 10);

    TrialMachine::Config cfg;
    cfg.need_s = kTrialSeconds;
    cfg.max_s = kTrialMaxSeconds;
    TrialMachine m(cfg);
    const pixelia::TrialSample first = trial_sample();
    m.start(first);
    const int64_t t0 = first.now_us;
    const uint32_t vs0 = first.vsyncs, fr0 = first.frames;
    ESP_LOGW(TAG, "Probelauf: Selbsttest läuft (mindestens %d s, höchstens %d s), Panel erwartet %.1f Hz, Engine-Bild %d×",
             kTrialSeconds, kTrialMaxSeconds, first.panel_hz, first.swap);
    // Messwerte des ganzen Tests fürs Log (V07: Panelrate, Engine-Bilder, Dauer)
    auto summary = [&](const char* what) {
        const pixelia::TrialSample s = trial_sample();
        const double sec = (s.now_us - t0) / 1e6;
        ESP_LOGW(TAG, "Selbsttest %s nach %.1f s: Panel %.1f Hz (erwartet %.1f, %lu Bilder), Engine %.1f/s (%lu Bilder), "
                      "gut %d s",
                 what, sec, (s.vsyncs - vs0) / sec, s.panel_hz, static_cast<unsigned long>(s.vsyncs - vs0),
                 (s.frames - fr0) / sec, static_cast<unsigned long>(s.frames - fr0), m.ok_s());
    };

    int elapsed = 0;
    while (!m.finished()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        test_misbehave(++elapsed);
        const pixelia::TrialSample s = trial_sample();
        const TrialMachine::Action a = m.step(s);
        publish_trial(m);
        if (a == TrialMachine::Action::MarkValid) {
            const esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
            m.confirm_result(err == ESP_OK, esp_timer_get_time(), err);
            publish_trial(m);
            if (m.phase() == TrialMachine::Phase::Passed) {
                summary("bestanden");
                ESP_LOGW(TAG, "Version %s gilt jetzt dauerhaft", me->version);
                {
                    std::lock_guard<std::mutex> g(s_lock);
                    s_status.trial = false;
                    s_status.trial_passed = true;
                }
                log::set_mirror(false);
                std::snprintf(text, sizeof(text), "Update auf %s erfolgreich", me->version);
                ui::toast(text, pixelia::ui::Tone::Success, 8);
            } else {
                // V08: gesund, aber nicht dauerhaft bestätigt – kein „erfolgreich“, neuer Versuch oder Hinweis
                ESP_LOGE(TAG, "Bestätigen fehlgeschlagen (%s, Versuch %d): %s", esp_err_to_name(err),
                         m.confirm_attempts(), m.note());
                if (m.phase() == TrialMachine::Phase::ConfirmFailed) {
                    summary("gesund, nicht bestätigt");
                    set_error("Nicht dauerhaft bestätigt: nach einem Neustart startet die vorige Version");
                    ui::show_message({"Update nicht bestätigt",
                                      std::string("Die neue Version läuft, ließ sich aber nicht dauerhaft bestätigen (") +
                                          esp_err_to_name(err) + ").\nNach einem Neustart startet die vorige Version.",
                                      pixelia::ui::Tone::Warning},
                                     20);
                }
            }
        } else if (a == TrialMachine::Action::Rollback) {
            char why[96];
            std::snprintf(why, sizeof(why), "Selbsttest nicht bestanden: %s", m.note());
            if (m.rollback_attempts() == 1) {
                summary("nicht bestanden");
                ESP_LOGE(TAG, "%s, zurück zur vorherigen Version", why);
                char sha[17];
                sha_hex(me->app_elf_sha256, sha);
                store_reason(sha, why);
                ui::show_message({"Selbsttest fehlgeschlagen", std::string(why) + "\nPixelia startet die vorherige Version.",
                                  pixelia::ui::Tone::Error},
                                 10);
                vTaskDelay(pdMS_TO_TICKS(3000));
            }
            const esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();  // kehrt nur bei Fehler zurück
            m.rollback_failed(esp_timer_get_time(), err);
            publish_trial(m);
            ESP_LOGE(TAG, "Rückfall fehlgeschlagen (%s, Versuch %d): %s", esp_err_to_name(err), m.rollback_attempts(),
                     m.note());
            if (m.phase() == TrialMachine::Phase::RollbackFailed) {
                set_error("Selbsttest nicht bestanden, Rückfall nicht möglich");
                ui::show_message({"Rückfall nicht möglich",
                                  std::string("Selbsttest nicht bestanden, die vorige Version lässt sich nicht starten (") +
                                      esp_err_to_name(err) + ").\nDiagnose auf der Seite System.",
                                  pixelia::ui::Tone::Error},
                                 30);
            }
        }
    }
    vTaskDelete(nullptr);
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

esp_err_t begin(const uint8_t* head, size_t len, size_t total, char* error, size_t error_len, const char* source,
                uint32_t& owner) {
    owner = 0;
    esp_app_desc_t ad;
    if (!check_header(head, len, &ad, error, error_len)) {
        set_error(error);
        return ESP_ERR_INVALID_ARG;
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (s_status.trial) {
            std::snprintf(error, error_len, "Der Probelauf der aktuellen Version läuft noch, bitte kurz warten");
            return ESP_ERR_INVALID_STATE;
        }
    }
    // B05: vor dem Löschen reservieren (kurze Sperre), damit kein zweites Update dieselbe Partition beschreibt
    const uint32_t id = s_gate.reserve();
    if (!id) {
        std::snprintf(error, error_len, "%s", s_gate.phase() == pixelia::UpdateGate::Phase::RebootPending
                                                  ? "Ein Update ist installiert, Neustart folgt"
                                                  : "Es läuft bereits ein Update");
        return ESP_ERR_INVALID_STATE;
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.busy = true;  // ab hier für Status und GitHub belegt, auch während des Löschens
        std::snprintf(s_status.source, sizeof(s_status.source), "%s", source);
        s_status.bytes = 0;
        s_status.total = static_cast<uint32_t>(total);
        std::snprintf(s_status.new_version, sizeof(s_status.new_version), "%s", ad.version);
        s_status.last_error[0] = 0;
    }
    auto fail = [&](esp_err_t err) {
        set_error(error);
        {
            std::lock_guard<std::mutex> g(s_lock);
            s_status.busy = false;
        }
        s_gate.release(id);
        return err;
    };
    s_target = esp_ota_get_next_update_partition(nullptr);
    if (!s_target || total > s_target->size) {
        std::snprintf(error, error_len, "Firmware zu groß für die App-Partition (%lu KB)",
                      static_cast<unsigned long>(s_target ? s_target->size / 1024 : 0));
        return fail(ESP_ERR_INVALID_SIZE);
    }
    // Nur den Bereich der neuen Firmware vorab löschen (in 64-KB-Blöcken, deutlich schneller als 4-KB-Sektoren
    // während des Schreibens). Das Display läuft dabei weiter, weil Programm und Daten im PSRAM liegen.
    const int64_t t_erase = esp_timer_get_time();
    esp_err_t err = esp_ota_begin(s_target, total, &s_handle);
    ESP_LOGI(TAG, "%u KB gelöscht in %lld ms", static_cast<unsigned>(total / 1024), (esp_timer_get_time() - t_erase) / 1000);
    if (err != ESP_OK) {
        s_handle = 0;
        std::snprintf(error, error_len, "Update kann nicht starten: %s", esp_err_to_name(err));
        return fail(err);
    }
    s_gate.start_writing(id);
    owner = id;
    s_ui_bytes = 0;
    ESP_LOGW(TAG, "Update auf %s (%u KB, gebaut %s %s, Quelle %s) nach %s", ad.version,
             static_cast<unsigned>(total / 1024), ad.date, ad.time, source, s_target->label);
    show_progress(UpdateInfo::Phase::Receiving);
    return ESP_OK;
}

esp_err_t write(uint32_t owner, const uint8_t* data, size_t len) {
    if (!s_gate.owns(owner)) return ESP_ERR_INVALID_STATE;
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

esp_err_t finish(uint32_t owner, char* error, size_t error_len) {
    if (!s_gate.owns(owner)) {
        std::snprintf(error, error_len, "Update gehört zu einem anderen Vorgang");
        return ESP_ERR_INVALID_STATE;
    }
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
        s_gate.release(owner);
        show_progress(UpdateInfo::Phase::Failed, error);
        ESP_LOGE(TAG, "Update fehlgeschlagen: %s", error);
        return err;
    }
    s_gate.done(owner);  // bis zum Neustart kein weiteres Update
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.busy = false;
        s_status.reboot_pending = true;
    }
    show_progress(UpdateInfo::Phase::Done);
    ESP_LOGW(TAG, "Update geprüft und als Startpartition gesetzt (%s), Neustart folgt", s_target->label);
    return ESP_OK;
}

void cancel(uint32_t owner, const char* why, bool by_user) {
    if (!s_gate.owns(owner)) return;  // nur der eigene Vorgang (B05)
    if (s_handle) esp_ota_abort(s_handle);
    s_handle = 0;
    set_error(why);
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_status.busy = false;
    }
    s_gate.release(owner);
    ESP_LOGE(TAG, "Update abgebrochen: %s", why);
    if (by_user) {
        ui::hide_update();
        ui::toast("Update abgebrochen", pixelia::ui::Tone::Info, 5);
        return;
    }
    show_progress(UpdateInfo::Phase::Failed, why);
}

void restart_soon(int ms) {
    static esp_timer_handle_t timer = nullptr;
    if (!timer) {
        esp_timer_create_args_t args{};
        args.callback = [](void*) {
            app::gfx::save_world_now();  // Weltkalender (seit 0.5.7), falls noch nicht gespeichert
            esp_restart();
        };
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
