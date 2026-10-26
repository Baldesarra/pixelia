// Einstellungen im NVS: ein Namensraum „pixelia“, ein Schlüssel pro Wert.

#include "settings.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>

#include "board/backlight.h"
#include "board/display.h"
#include "app.h"
#include "esp_log.h"
#include "input.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ota.h"
#include "ui.h"

namespace app::settings {

namespace {
constexpr const char* TAG = "set";
constexpr const char* kNamespace = "pixelia";

std::mutex s_lock;
Values s_values;
std::atomic<bool> s_frozen{false};

void save_u8(const char* key, uint8_t value) {
    if (s_frozen) return;
    nvs_handle_t h;
    if (nvs_open(kNamespace, NVS_READWRITE, &h) != ESP_OK) return;
    esp_err_t err = nvs_set_u8(h, key, value);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "Speichern von %s fehlgeschlagen: %s", key, esp_err_to_name(err));
}

void save_u16(const char* key, uint16_t value) {
    if (s_frozen) return;
    nvs_handle_t h;
    if (nvs_open(kNamespace, NVS_READWRITE, &h) != ESP_OK) return;
    esp_err_t err = nvs_set_u16(h, key, value);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "Speichern von %s fehlgeschlagen: %s", key, esp_err_to_name(err));
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
        nvs_close(h);
    }
    const Values& sv = s_values;
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
    save_u8("rot180", on ? 1 : 0);
    ESP_LOGI(TAG, "Bilddrehung %d° gespeichert", on ? 180 : 0);
}

void set_brightness(int percent) {
    auto v = static_cast<uint8_t>(std::clamp(percent, 1, 100));
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.brightness = v;
    }
    night::refresh();  // seit 0.5.7 setzt der Nachtmodus die Beleuchtung (nachts die Nacht-Helligkeit)
    save_u8("bright", v);
    ESP_LOGI(TAG, "Helligkeit %u %% gespeichert", v);
}

void set_night(bool on, int percent) {
    const auto p = static_cast<uint8_t>(std::clamp(percent, 1, 100));
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.night_on = on;
        s_values.night_bright = p;
    }
    night::refresh();
    save_u8("night", static_cast<uint8_t>((on ? 0x80 : 0) | p));
    ESP_LOGI(TAG, "Nachtmodus %s, Nacht-Helligkeit %u %% gespeichert", on ? "an" : "aus", p);
}

void set_backlight_min(int duty) {
    board::backlight::set_min_duty(static_cast<uint32_t>(std::max(duty, 1)));
    const auto d = static_cast<uint16_t>(board::backlight::min_duty());
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.bl_min = d;
    }
    night::refresh();
    save_u16("bl_min", d);
    ESP_LOGI(TAG, "Kleinste Pulsbreite %u von %lu gespeichert", d, static_cast<unsigned long>(board::backlight::kMaxDuty));
}

void set_long_press_ms(int ms) {
    auto v = static_cast<uint16_t>(std::clamp(ms, kLongPressMin, kLongPressMax));
    {
        std::lock_guard<std::mutex> g(s_lock);
        s_values.long_press_ms = v;
    }
    input::set_long_press_ms(v);
    save_u16("lp_ms", v);
    ESP_LOGI(TAG, "Langer Druck %u ms gespeichert", v);
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
    save_u8("clk", static_cast<uint8_t>((on ? 1 : 0) | c << 1 | (seconds ? 8 : 0)));
    ESP_LOGI(TAG, "Pixel-Uhr %s, Ecke %u, Sekunden %s gespeichert", on ? "an" : "aus", c, seconds ? "an" : "aus");
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
    save_u8("cam_home", v.cam_home);
    save_u16("cam_fig", v.cam_figure);
    save_u8("cam_plc", v.cam_place);
    save_u16("cam_ret", v.cam_return_s);
    ESP_LOGI(TAG, "Kamera: Standard %u (Figur %u, Ort %u), Rückkehr %u s gespeichert", v.cam_home, v.cam_figure,
             v.cam_place, v.cam_return_s);
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
    if (s_frozen) return true;
    nvs_handle_t h;
    if (nvs_open(kNamespace, NVS_READWRITE, &h) != ESP_OK) return true;
    esp_err_t err = nvs_set_str(h, "ui", json.c_str());
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "Farbwahl nicht gespeichert: %s", esp_err_to_name(err));
    else ESP_LOGI(TAG, "Farbwahl der Oberfläche gespeichert");
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

void log_all() {
    Values v = get();
    nvs_stats_t st{};
    nvs_get_stats(nullptr, &st);
    ESP_LOGI(TAG, "Einstellungen: Drehung %d°, Helligkeit %u %%, langer Druck %u ms | NVS %u von %u Einträgen belegt",
             v.rotate180 ? 180 : 0, v.brightness, v.long_press_ms, static_cast<unsigned>(st.used_entries),
             static_cast<unsigned>(st.total_entries));
}

}  // namespace app::settings
