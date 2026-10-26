// Gesten verteilen und Info-Karte mit Daten füllen (beides auf Kern 0, aus dem Touch-Task)

#include "input.h"

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <string>

#include "app.h"
#include "auth.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "github.h"
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

// Zurücksetzen am Gerät (Info-Karte → „Zurücksetzen …“): eigene Knopf-Logik statt Gesten, Knopf 3 s halten
using ResetButton = pixelia::ui::ResetButton;
constexpr int kResetSeconds = 30;  // ohne Berührung schließt der Bildschirm
bool s_reset_open = false;
pixelia::ui::ResetInfo s_reset;
bool s_reset_down = false;            // Finger liegt, seit der Bildschirm offen ist
ResetButton s_reset_first = ResetButton::None;  // Knopf beim Aufsetzen
uint32_t s_reset_t0 = 0;
bool s_swallow = false;  // Gesten bis zum Loslassen ignorieren (Finger lag noch beim Schließen)

uint32_t now_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

void reset_show() { ui::show_reset(s_reset, kResetSeconds); }

void reset_open() {
    infocard::hide();
    s_reset = {};
    s_reset.pin_set = auth::enabled();
    s_reset_open = true;
    s_reset_down = false;
    reset_show();
    ESP_LOGI(TAG, "Zurücksetzen am Gerät geöffnet (PIN %s)", s_reset.pin_set ? "an" : "aus");
}

void reset_close(const char* why) {
    s_reset_open = false;
    s_swallow = s_reset_down;
    ui::hide_reset();
    ESP_LOGI(TAG, "Zurücksetzen geschlossen: %s", why);
}

void reset_touch(int fingers, int x, int y, uint32_t t) {
    const int cx = x / 2, cy = y / 2;  // Panel 800×480 → Leinwand 400×240
    if (fingers > 0 && !s_reset_down) {
        s_reset_down = true;
        s_reset_first = pixelia::ui::reset_button_at(s_reset, cx, cy);
        s_reset_t0 = t;
        if (s_reset_first == ResetButton::Pin || s_reset_first == ResetButton::Factory) s_reset.held = s_reset_first;
        reset_show();  // zeigt den gehaltenen Knopf und verlängert die Anzeigedauer
    } else if (fingers > 0) {
        // Vom Knopf gerutscht: Halten abbrechen
        if (s_reset.held != ResetButton::None &&
            !pixelia::ui::reset_button_rect(s_reset.held).contains(cx, cy, 12)) {
            s_reset.held = ResetButton::None;
            s_reset.permille = 0;
            reset_show();
        }
    } else if (s_reset_down) {
        s_reset_down = false;
        if (s_reset_first == ResetButton::Cancel) return reset_close("Abbrechen");
        if (s_reset.held != ResetButton::None) {
            ESP_LOGI(TAG, "Zurücksetzen: Knopf nach %lu ms losgelassen (zu kurz)",
                     static_cast<unsigned long>(t - s_reset_t0));
            s_reset.held = ResetButton::None;
            s_reset.permille = 0;
            reset_show();
        }
    }
}

void reset_tick(uint32_t t) {
    if (!s_reset_open) return;
    if (!ui::reset_visible()) {
        s_reset_open = false;
        s_swallow = s_reset_down;
        ESP_LOGI(TAG, "Zurücksetzen geschlossen: Zeit abgelaufen");
        return;
    }
    if (s_reset.held == ResetButton::None) return;
    const int permille = static_cast<int>(std::min<uint32_t>(t - s_reset_t0, pixelia::ui::kResetHoldMs) * 1000 /
                                          pixelia::ui::kResetHoldMs);
    if (permille < 1000) {
        // In 5-%-Schritten neu zeichnen lassen
        if (permille / 50 != s_reset.permille / 50) {
            s_reset.permille = permille;
            reset_show();
        }
        return;
    }
    if (s_reset.held == ResetButton::Pin) {
        reset_close("PIN entfernt");
        auth::clear_on_device();
        ui::toast("PIN entfernt: Die Weboberfläche ist wieder ohne PIN erreichbar.", pixelia::ui::Tone::Success, 8);
    } else {
        s_reset_open = false;
        s_swallow = true;
        settings::factory_reset("Touch-Geste am Gerät");
    }
}

void configure_locked() {
    if (s_configured) return;
    pixelia::gesture::Config cfg;
    cfg.long_press_ms = settings::get().long_press_ms;
    cfg.tap_max_ms = 800;
    cfg.move_tolerance = 24;  // Panel-Pixel (≈ 2,5 mm), ein ruhig liegender Finger wandert weniger
    s_rec.set_config(cfg);
    s_configured = true;
}

// Gehört die Geste der Welt? (keine Systemanzeige im Weg)
bool world_free() {
    return !infocard::visible() && !ui::picture_visible() && !ui::update_visible() && !s_reset_open;
}

gfx::WorldInput world(gfx::WorldInput::Kind kind, const pixelia::gesture::Event& e) {
    return {kind, static_cast<int16_t>(e.x / 2), static_cast<int16_t>(e.y / 2), static_cast<int16_t>(e.dx / 2),
            static_cast<int16_t>(e.dy / 2)};
}

void dispatch(const pixelia::gesture::Event& e) {
    using pixelia::gesture::Type;
    if (s_reset_open || s_swallow) return;
    if (e.type == Type::LongPress) {
        if (ui::update_visible()) {
            ESP_LOGI(TAG, "Langer Druck bei %d/%d ignoriert (Update läuft)", e.x, e.y);
            return;
        }
        ESP_LOGI(TAG, "Langer Druck bei %d/%d (%lu ms, %lu Messwerte)", e.x, e.y,
                 static_cast<unsigned long>(e.duration_ms), static_cast<unsigned long>(s_samples));
        // Langer Druck = immer die Info-Karte; Folgen geht über den Knopf auf der Karte einer Figur (Wunsch des Users
        // 2026-10-22: Halten auf bewegten Figuren traf oft die falsche oder keine)
        infocard::show();
    } else if (e.type == Type::Swipe) {
        if (!world_free()) return;
        ESP_LOGI(TAG, "Wischen bei %d/%d um %d/%d (%lu ms)", e.x, e.y, e.dx, e.dy, static_cast<unsigned long>(e.duration_ms));
        gfx::world_input(world(gfx::WorldInput::Kind::Swipe, e));
    } else if (e.type == Type::Tap) {
        if (infocard::visible()) {
            if (pixelia::ui::info_reset_button().contains(e.x / 2, e.y / 2, 6)) {
                reset_open();
                return;
            }
            ESP_LOGI(TAG, "Tippen bei %d/%d (%lu ms) → Info-Karte geschlossen", e.x, e.y,
                     static_cast<unsigned long>(e.duration_ms));
            infocard::hide();
            return;
        }
        if (ui::picture_visible()) {
            ESP_LOGI(TAG, "Tippen bei %d/%d → Testbild beendet", e.x, e.y);
            ui::hide_picture();
            return;
        }
        // Sonst gehört das Tippen der Welt: Figur oder Teil → Karte (seit 0.5.6)
        if (world_free()) gfx::world_input(world(gfx::WorldInput::Kind::Tap, e));
    }
}

void process_locked(uint32_t t) {
    pixelia::gesture::Event e;
    while (s_rec.poll(&e)) dispatch(e);
    reset_tick(t);
    float p = s_rec.hold_progress(t);
    bool show = p >= kHoldIndicatorFrom && !s_reset_open && !s_swallow && !infocard::visible() && !ui::update_visible();
    if (show) ui::set_hold_progress(p);
    else if (s_hold_shown) ui::set_hold_progress(-1.0f);
    s_hold_shown = show;
}
}  // namespace

void on_touch(int fingers, int x, int y) {
    std::lock_guard<std::mutex> g(s_lock);
    configure_locked();
    const uint32_t t = now_ms();
    if (fingers > 0 && !s_rec.pressed()) {
        s_samples = 0;
        night::touched();  // nachts: eine Minute hell (seit 0.5.7)
    }
    if (fingers > 0) ++s_samples;
    if (s_reset_open) reset_touch(fingers, x, y, t);
    else if (fingers == 0) s_reset_down = false;
    s_rec.touch(fingers, x, y, t);
    process_locked(t);
    if (fingers == 0 && !s_reset_open) s_swallow = false;
}

void tick() {
    {
        std::lock_guard<std::mutex> g(s_lock);
        configure_locked();
        const uint32_t t = now_ms();
        s_rec.tick(t);
        process_locked(t);
    }
    if (gfx::take_info_request()) {
        ESP_LOGI(TAG, "Langer Druck ohne Figur → Info-Karte");
        infocard::show();
    }
    infocard::tick();
    // Hinweis auf eine neue Version (seit 0.5.6): einmal je Version und Start, nie automatisch installieren
    static int64_t next_check = 0;
    static std::string notified;
    const int64_t now = esp_timer_get_time();
    if (now >= next_check) {
        next_check = now + 5000000;
        const std::string v = github::available();
        if (!v.empty() && v != notified) {
            notified = v;
            ui::toast("Neue Version " + v + " verfügbar · Weboberfläche → Update", pixelia::ui::Tone::Info, 10);
            ESP_LOGI(TAG, "Hinweis am Display: Version %s verfügbar", v.c_str());
        }
    }
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
