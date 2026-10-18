// Gesten verteilen und Info-Karte mit Daten füllen (beides auf Kern 0, aus dem Touch-Task)

#include "input.h"

#include <cstdio>
#include <mutex>

#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "net.h"
#include "pixelia/engine/gesture.h"
#include "settings.h"
#include "ui.h"

namespace app::input {

namespace {
constexpr const char* TAG = "input";
// Halte-Anzeige erst ab einem Viertel der Zeit: normales Tippen und spätere Wischgesten bleiben ungestört
constexpr float kHoldIndicatorFrom = 0.25f;

std::mutex s_lock;
pixelia::gesture::Recognizer s_rec;
bool s_configured = false;
bool s_hold_shown = false;
uint32_t s_samples = 0;  // Touch-Daten im laufenden Druck (Diagnose: liefert der GT911 auch bei ruhigem Finger?)

uint32_t now_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

void configure_locked() {
    if (s_configured) return;
    pixelia::gesture::Config cfg;
    cfg.long_press_ms = settings::get().long_press_ms;
    cfg.tap_max_ms = 800;
    cfg.move_tolerance = 24;  // Panel-Pixel (≈ 2,5 mm), ein ruhig liegender Finger wandert weniger
    s_rec.set_config(cfg);
    s_configured = true;
}

void dispatch(const pixelia::gesture::Event& e) {
    using pixelia::gesture::Type;
    if (e.type == Type::LongPress) {
        if (ui::update_visible()) {
            ESP_LOGI(TAG, "Langer Druck bei %d/%d ignoriert (Update läuft)", e.x, e.y);
            return;
        }
        ESP_LOGI(TAG, "Langer Druck bei %d/%d (%lu ms, %lu Messwerte) → Info-Karte", e.x, e.y,
                 static_cast<unsigned long>(e.duration_ms), static_cast<unsigned long>(s_samples));
        infocard::show();
    } else if (e.type == Type::Tap) {
        if (infocard::visible()) {
            ESP_LOGI(TAG, "Tippen bei %d/%d (%lu ms) → Info-Karte geschlossen", e.x, e.y,
                     static_cast<unsigned long>(e.duration_ms));
            infocard::hide();
        }
        // Sonst gehört das Tippen später der Welt
    }
}

void process_locked(uint32_t t) {
    pixelia::gesture::Event e;
    while (s_rec.poll(&e)) dispatch(e);
    float p = s_rec.hold_progress(t);
    bool show = p >= kHoldIndicatorFrom && !infocard::visible() && !ui::update_visible();
    if (show) ui::set_hold_progress(p);
    else if (s_hold_shown) ui::set_hold_progress(-1.0f);
    s_hold_shown = show;
}
}  // namespace

void on_touch(int fingers, int x, int y) {
    std::lock_guard<std::mutex> g(s_lock);
    configure_locked();
    const uint32_t t = now_ms();
    if (fingers > 0 && !s_rec.pressed()) s_samples = 0;
    if (fingers > 0) ++s_samples;
    s_rec.touch(fingers, x, y, t);
    process_locked(t);
}

void tick() {
    {
        std::lock_guard<std::mutex> g(s_lock);
        configure_locked();
        const uint32_t t = now_ms();
        s_rec.tick(t);
        process_locked(t);
    }
    infocard::tick();
}

void set_long_press_ms(uint32_t ms) {
    std::lock_guard<std::mutex> g(s_lock);
    configure_locked();
    pixelia::gesture::Config cfg = s_rec.config();
    cfg.long_press_ms = ms;
    s_rec.set_config(cfg);
}

}  // namespace app::input

namespace app::infocard {

namespace {
int64_t s_next_refresh = 0;

pixelia::ui::InfoCardData collect() {
    using Mode = pixelia::ui::InfoCardData::Mode;
    const wifi::Status st = wifi::status();
    pixelia::ui::InfoCardData d;
    d.version = esp_app_get_description()->version;
    d.hostname = wifi::kHostname;
    d.mdns_host = wifi::kMdnsHost;
    if (st.state == wifi::State::Connected && st.ip[0]) {
        d.mode = Mode::Connected;
        d.ip = st.ip;
        d.ssid = st.ssid;
        d.rssi = st.rssi;
    } else if (st.ap_active) {
        d.mode = Mode::Setup;
        d.ap_ssid = wifi::kApSsid;
        d.ap_password = wifi::ap_password();
        d.ap_ip = wifi::kApIp;
        d.ap_clients = st.ap_clients;
    } else {
        d.mode = Mode::Offline;
        char buf[160];
        if (st.state == wifi::State::Off) std::snprintf(buf, sizeof(buf), "WLAN startet …");
        else if (st.ssid[0]) std::snprintf(buf, sizeof(buf), "Verbinde mit „%s“ …", st.ssid);
        else if (st.saved == 0) std::snprintf(buf, sizeof(buf), "Noch kein WLAN gespeichert");
        else std::snprintf(buf, sizeof(buf), "Suche gespeicherte WLANs …");
        d.status = buf;
        if (st.last_error[0]) d.status += std::string("\nZuletzt: ") + st.last_error;
    }
    return d;
}
}  // namespace

void show() {
    ui::show_info(collect(), kSeconds);
    s_next_refresh = esp_timer_get_time() + 2000000;
}

void hide() { ui::hide_info(); }

bool visible() { return ui::info_visible(); }

void tick() {
    const int64_t now = esp_timer_get_time();
    if (now < s_next_refresh || !visible()) return;
    s_next_refresh = now + 2000000;
    ui::refresh_info(collect());
}

}  // namespace app::infocard
