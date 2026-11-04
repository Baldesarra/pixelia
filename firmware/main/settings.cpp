// Einstellungen im NVS: ein Namensraum „pixelia“, ein Schlüssel pro Wert.

#include "settings.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>

#include "board/backlight.h"
#include "board/display.h"
#include "app.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "input.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ota.h"
#include "pixelia/engine/device_flow.h"
#include "ui.h"

namespace app::settings {

namespace {
constexpr const char* TAG = "set";
constexpr const char* kNamespace = "pixelia";

std::mutex s_lock;
Values s_values;
std::atomic<bool> s_frozen{false};

// B23 (seit 0.5.11): Alle Werte in einem Schreibvorgang mit Revision. Quittiert wird nur nach bestätigtem Commit;
// scheitert er, gilt der Wert sofort (RAM), wird aber erst durch einen neuen Versuch alle 30 s dauerhaft.
pixelia::SaveMark s_mark;
std::mutex s_write_lock;  // nur ein Schreiber zur Zeit (Web, Konsole, Zeitgeber)
std::atomic<int> s_fail_next{0};
esp_timer_handle_t s_retry = nullptr;
constexpr int64_t kRetryUs = 30 * 1000000LL;

esp_err_t write_all(const Values& v) {
    if (take_fault()) return ESP_FAIL;  // Prüfschalter „nvs fehler“
    nvs_handle_t h;
    esp_err_t err = nvs_open(kNamespace, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    // NVS schreibt unveränderte Werte nicht neu
    auto u8 = [&](const char* k, uint8_t x) { if (err == ESP_OK) err = nvs_set_u8(h, k, x); };
    auto u16 = [&](const char* k, uint16_t x) { if (err == ESP_OK) err = nvs_set_u16(h, k, x); };
    u8("rot180", v.rotate180 ? 1 : 0);
    u8("bright", v.brightness);
    u16("lp_ms", v.long_press_ms);
    u8("clk", static_cast<uint8_t>((v.clock_on ? 1 : 0) | v.clock_corner << 1 | (v.clock_seconds ? 8 : 0)));
    u8("cam_home", v.cam_home);
    u16("cam_fig", v.cam_figure);
    u8("cam_plc", v.cam_place);
    u16("cam_ret", v.cam_return_s);
    u8("night", static_cast<uint8_t>((v.night_on ? 0x80 : 0) | v.night_bright));
    u16("night_h", static_cast<uint16_t>(v.night_from << 8 | v.night_to));
    if (v.bl_min) u16("bl_min", v.bl_min);
    if (err == ESP_OK && v.ui[0]) {
        err = nvs_set_str(h, "ui", v.ui);
    } else if (err == ESP_OK) {
        const esp_err_t e = nvs_erase_key(h, "ui");
        if (e != ESP_ERR_NVS_NOT_FOUND) err = e;
    }
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

// Schreibt den aktuellen Stand; true = dauerhaft gespeichert (oder nichts offen)
bool persist() {
    if (s_frozen) return false;
    std::lock_guard<std::mutex> w(s_write_lock);
    if (!s_mark.dirty()) return true;
    const uint32_t rev = s_mark.begin();  // vor dem Lesen: spätere Änderungen bleiben offen
    const Values v = get();
    const esp_err_t err = write_all(v);
    if (err == ESP_OK) {
        s_mark.saved(rev);
        if (!s_mark.dirty() && s_retry) esp_timer_stop(s_retry);
        return true;
    }
    ESP_LOGW(TAG, "Einstellungen übernommen, aber nicht dauerhaft gespeichert (%s), neuer Versuch in 30 s",
             esp_err_to_name(err));
    if (s_retry) {
        esp_timer_stop(s_retry);
        esp_timer_start_once(s_retry, kRetryUs);
    }
    return false;
}

// Nach jeder Änderung: speichern und das Ergebnis ins Log
bool changed(const char* what) {
    s_mark.touch();
    const bool ok = persist();
    if (ok) ESP_LOGI(TAG, "%s gespeichert", what);
    else ESP_LOGW(TAG, "%s übernommen, noch nicht dauerhaft gespeichert", what);
    return ok;
}
}  // namespace

void init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS wird neu angelegt (%s)", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t h;
    if (nvs_open(kNamespace, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v;
        if (nvs_get_u8(h, "rot180", &v) == ESP_OK) s_values.rotate180 = v != 0;
        if (nvs_get_u8(h, "bright", &v) == ESP_OK) s_values.brightness = static_cast<uint8_t>(std::clamp<int>(v, 1, 100));
        uint16_t ms;
        if (nvs_get_u16(h, "lp_ms", &ms) == ESP_OK)
            s_values.long_press_ms = static_cast<uint16_t>(std::clamp<int>(ms, kLongPressMin, kLongPressMax));
        size_t len = sizeof(s_values.ui);
        if (nvs_get_str(h, "ui", s_values.ui, &len) != ESP_OK) s_values.ui[0] = 0;
        if (nvs_get_u8(h, "clk", &v) == ESP_OK) {
            s_values.clock_on = v & 1;
            s_values.clock_corner = v >> 1 & 3;
            s_values.clock_seconds = v >> 3 & 1;
        }
        if (nvs_get_u8(h, "cam_home", &v) == ESP_OK) s_values.cam_home = std::min<uint8_t>(v, 2);
        if (nvs_get_u8(h, "cam_plc", &v) == ESP_OK) s_values.cam_place = v;
        if (nvs_get_u16(h, "cam_fig", &ms) == ESP_OK) s_values.cam_figure = ms;
        if (nvs_get_u16(h, "cam_ret", &ms) == ESP_OK) s_values.cam_return_s = ms;
        if (nvs_get_u8(h, "night", &v) == ESP_OK) {
            s_values.night_on = v & 0x80;
            s_values.night_bright = static_cast<uint8_t>(std::clamp<int>(v & 0x7F, 1, 100));
        }
        if (nvs_get_u16(h, "bl_min", &ms) == ESP_OK) s_values.bl_min = ms;
        if (nvs_get_u16(h, "night_h", &ms) == ESP_OK) {
            s_values.night_from = static_cast<uint8_t>((ms >> 8) % 24);
            s_values.night_to = static_cast<uint8_t>((ms & 0xFF) % 24);
        }
        nvs_close(h);
    }
    esp_timer_create_args_t args{};
    args.callback = [](void*) {
        if (persist()) ESP_LOGI(TAG, "Einstellungen nach neuem Versuch dauerhaft gespeichert");
    };
    args.name = "einstellungen";
    esp_timer_create(&args, &s_retry);
    const Values& sv = s_values;
    night::set_hours(sv.night_from, sv.night_to);
    gfx::set_clock(sv.clock_on, sv.clock_corner, sv.clock_seconds);
    gfx::set_camera({static_cast<pixelia::CameraHome>(sv.cam_home), sv.cam_figure, static_cast<int8_t>(sv.cam_place),
                     sv.cam_return_s});
    log_all();
}

Values get() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_values;
}

void set_rotation180(bool on) {
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.rotate180 = on;
    }
    board::display::set_rotation180(on);
    changed(on ? "Bilddrehung 180°" : "Bilddrehung 0°");
}

void set_brightness(int percent) {
    auto v = static_cast<uint8_t>(std::clamp(percent, 1, 100));
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.brightness = v;
    }
    night::refresh();  // seit 0.5.7 setzt der Nachtmodus die Beleuchtung (nachts die Nacht-Helligkeit)
    char what[32];
    std::snprintf(what, sizeof(what), "Helligkeit %u %%", v);
    changed(what);
}

void set_night(bool on, int percent, int from, int to) {
    const auto p = static_cast<uint8_t>(std::clamp(percent, 1, 100));
    const auto f = static_cast<uint8_t>(((from % 24) + 24) % 24), t = static_cast<uint8_t>(((to % 24) + 24) % 24);
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.night_on = on;
        s_values.night_bright = p;
        s_values.night_from = f;
        s_values.night_to = t;
    }
    night::set_hours(f, t);
    char what[64];
    std::snprintf(what, sizeof(what), "Nachtmodus %s, %u–%u Uhr, Nacht-Helligkeit %u %%", on ? "an" : "aus", f, t, p);
    changed(what);
}

void set_backlight_min(int duty) {
    board::backlight::set_min_duty(static_cast<uint32_t>(std::max(duty, 1)));
    const auto d = static_cast<uint16_t>(board::backlight::min_duty());
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.bl_min = d;
    }
    night::refresh();
    char what[48];
    std::snprintf(what, sizeof(what), "Kleinste Pulsbreite %u von %lu", d, static_cast<unsigned long>(board::backlight::kMaxDuty));
    changed(what);
}

void set_long_press_ms(int ms) {
    auto v = static_cast<uint16_t>(std::clamp(ms, kLongPressMin, kLongPressMax));
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.long_press_ms = v;
    }
    input::set_long_press_ms(v);
    char what[32];
    std::snprintf(what, sizeof(what), "Langer Druck %u ms", v);
    changed(what);
}

void set_clock(bool on, int corner, bool seconds) {
    const auto c = static_cast<uint8_t>(std::clamp(corner, 0, 3));
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.clock_on = on;
        s_values.clock_corner = c;
        s_values.clock_seconds = seconds;
    }
    gfx::set_clock(on, c, seconds);
    char what[48];
    std::snprintf(what, sizeof(what), "Pixel-Uhr %s, Ecke %u, Sekunden %s", on ? "an" : "aus", c, seconds ? "an" : "aus");
    changed(what);
}

void set_camera(int home, int figure, int place, int return_s) {
    Values v;
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.cam_home = static_cast<uint8_t>(std::clamp(home, 0, 2));
        s_values.cam_figure = static_cast<uint16_t>(std::clamp(figure, 0, 0xFFFE));
        s_values.cam_place = static_cast<uint8_t>(std::clamp(place, 0, 15));
        s_values.cam_return_s = static_cast<uint16_t>(std::clamp(return_s, 0, 24 * 3600 / 2));
        v = s_values;
    }
    gfx::set_camera({static_cast<pixelia::CameraHome>(v.cam_home), v.cam_figure, static_cast<int8_t>(v.cam_place),
                     v.cam_return_s});
    char what[80];
    std::snprintf(what, sizeof(what), "Kamera: Standard %u (Figur %u, Ort %u), Rückkehr %u s", v.cam_home, v.cam_figure,
                  v.cam_place, v.cam_return_s);
    changed(what);
}

bool set_ui(const std::string& json) {
    if (json.size() > kUiMax) return false;
    for (char ch : json) {
        if (static_cast<unsigned char>(ch) < 32) return false;
    }
    {
        std::lock_guard<std::mutex> g(s_lock);
        if (json == s_values.ui) return true;
        std::memcpy(s_values.ui, json.c_str(), json.size() + 1);
    }
    changed("Farbwahl der Oberfläche");
    return true;
}

void factory_reset(const char* why) {
    if (s_frozen.exchange(true)) return;
    ESP_LOGW(TAG, "Werksreset (%s): Einstellungen, WLAN, Gerätenamen und PIN werden gelöscht", why);
    pixelia::ui::MessageInfo m;
    m.title = "Werksreset";
    m.body = "Einstellungen, WLAN-Netze und PIN sind gelöscht.\nPixelia startet neu im Setup-Modus …";
    m.tone = pixelia::ui::Tone::Warning;
    ui::hide_info();
    ui::hide_reset();
    ui::show_message(m, 30);
    // Reset-Historie und Startzähler („diag“) bleiben, ebenso der Rückfall-Grund des Updates („ota“)
    for (const char* ns : {kNamespace, "wifi", "clients", "auth"}) {
        nvs_handle_t h;
        if (nvs_open(ns, NVS_READWRITE, &h) != ESP_OK) continue;
        esp_err_t err = nvs_erase_all(h);
        if (err == ESP_OK) err = nvs_commit(h);
        nvs_close(h);
        if (err != ESP_OK) ESP_LOGE(TAG, "Namensraum %s nicht gelöscht: %s", ns, esp_err_to_name(err));
    }
    ota::restart_soon(2500);
}

bool frozen() { return s_frozen; }

bool pending() { return s_mark.dirty(); }

void fail_next(int n) { s_fail_next = n; }

bool take_fault() {
    int n = s_fail_next.load();
    while (n > 0 && !s_fail_next.compare_exchange_weak(n, n - 1)) {
    }
    return n > 0;
}

void log_all() {
    Values v = get();
    nvs_stats_t st{};
    nvs_get_stats(nullptr, &st);
    ESP_LOGI(TAG, "Einstellungen: Drehung %d°, Helligkeit %u %%, langer Druck %u ms | NVS %u von %u Einträgen belegt",
             v.rotate180 ? 180 : 0, v.brightness, v.long_press_ms, static_cast<unsigned>(st.used_entries),
             static_cast<unsigned>(st.total_entries));
}

}  // namespace app::settings
