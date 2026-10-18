// Einstellungen im NVS: ein Namensraum „pixelia“, ein Schlüssel pro Wert.

#include "settings.h"

#include <algorithm>
#include <mutex>

#include "board/backlight.h"
#include "board/display.h"
#include "esp_log.h"
#include "input.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace app::settings {

namespace {
constexpr const char* TAG = "set";
constexpr const char* kNamespace = "pixelia";

std::mutex s_lock;
Values s_values;

void save_u8(const char* key, uint8_t value) {
    nvs_handle_t h;
    if (nvs_open(kNamespace, NVS_READWRITE, &h) != ESP_OK) return;
    esp_err_t err = nvs_set_u8(h, key, value);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    if (err != ESP_OK) ESP_LOGW(TAG, "Speichern von %s fehlgeschlagen: %s", key, esp_err_to_name(err));
}

void save_u16(const char* key, uint16_t value) {
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
        nvs_close(h);
    }
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
    board::backlight::set_percent(v);
    save_u8("bright", v);
    ESP_LOGI(TAG, "Helligkeit %u %% gespeichert", v);
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

void log_all() {
    Values v = get();
    nvs_stats_t st{};
    nvs_get_stats(nullptr, &st);
    ESP_LOGI(TAG, "Einstellungen: Drehung %d°, Helligkeit %u %%, langer Druck %u ms | NVS %u von %u Einträgen belegt",
             v.rotate180 ? 180 : 0, v.brightness, v.long_press_ms, static_cast<unsigned>(st.used_entries),
             static_cast<unsigned>(st.total_entries));
}

}  // namespace app::settings
