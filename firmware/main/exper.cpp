// Seite Experimentell: Schalter, Probe mit Rückfall für Panel-Timing und Stromsparmodus, Displaytests (exper.h).

#include "exper.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>

#include "app.h"
#include "board/pins.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "json.h"
#include "pixelia/engine/weather.h"
#include "nvs.h"
#include "settings.h"
#include "ui.h"

namespace app::exper {

namespace {
constexpr const char* TAG = "exp";
constexpr const char* kNvs = "pixelia";

using board::display::Mode;
using board::display::Timing;

// Gespeichertes Format (fest, unabhängig vom Aufbau von Timing)
struct Stored {
    uint8_t ver = 1;
    uint8_t eco = 0;
    uint8_t pol = 0;  // Bit 0 HSync low, Bit 1 VSync low, Bit 2 Takt fallend
    uint8_t pad = 0;
    uint32_t pclk = 0;
    uint16_t hp = 0, hb = 0, hf = 0, vp = 0, vb = 0, vf = 0;
};

std::mutex s_lock;
bool s_enabled = false;
bool s_safe_boot = false;  // Fluchtweg hat gegriffen: dieser Start läuft mit Standardwerten
Panel s_saved;             // zuletzt bestätigt
Panel s_active;            // läuft gerade
Panel s_prev;              // vor der Probe
int64_t s_trial_until = 0;  // 0 = keine Probe
int64_t s_test_until = 0;   // 0 = kein Testbild
esp_timer_handle_t s_trial_timer = nullptr;
esp_timer_handle_t s_test_timer = nullptr;
esp_timer_handle_t s_stable_timer = nullptr;

bool same(const Timing& a, const Timing& b) {
    return a.pclk_hz == b.pclk_hz && a.hsync_pulse == b.hsync_pulse && a.hsync_back == b.hsync_back &&
           a.hsync_front == b.hsync_front && a.vsync_pulse == b.vsync_pulse && a.vsync_back == b.vsync_back &&
           a.vsync_front == b.vsync_front && a.hsync_idle_low == b.hsync_idle_low &&
           a.vsync_idle_low == b.vsync_idle_low && a.pclk_active_neg == b.pclk_active_neg;
}
bool is_default(const Panel& p) { return !p.eco && same(p.timing, Timing{}); }

Timing effective(const Panel& p) {
    Timing t = p.timing;
    if (p.eco) t.pclk_hz /= 2;
    return t;
}

double hz_of(const Timing& t) {
    const double ht = board::kLcdWidth + t.hsync_pulse + t.hsync_back + t.hsync_front;
    const double vt = board::kLcdHeight + t.vsync_pulse + t.vsync_back + t.vsync_front;
    return t.pclk_hz / (ht * vt);
}

// Engine-Bilder pro Panel-Bild, damit die Welt bei ~30 fps bleibt (60 Hz → 2, 30 Hz → 1)
int swap_for(const Timing& t) { return std::clamp(static_cast<int>(std::lround(hz_of(t) / 30.0)), 1, 4); }

void apply(const Panel& p) {
    const Timing t = effective(p);
    s_active = p;
    board::display::set_swap_interval(swap_for(t));
    gfx::request_reconfigure(t);
    ESP_LOGI(TAG, "Panel: %.2f MHz, %.1f Hz, Engine-Bild %d× gezeigt%s", t.pclk_hz / 1e6, hz_of(t), swap_for(t),
             p.eco ? ", Stromsparmodus" : "");
}

Stored to_stored(const Panel& p) {
    Stored s;
    const Timing& t = p.timing;
    s.eco = p.eco;
    s.pol = (t.hsync_idle_low ? 1 : 0) | (t.vsync_idle_low ? 2 : 0) | (t.pclk_active_neg ? 4 : 0);
    s.pclk = t.pclk_hz;
    s.hp = t.hsync_pulse, s.hb = t.hsync_back, s.hf = t.hsync_front;
    s.vp = t.vsync_pulse, s.vb = t.vsync_back, s.vf = t.vsync_front;
    return s;
}

Panel from_stored(const Stored& s) {
    Panel p;
    Timing& t = p.timing;
    p.eco = s.eco;
    t.hsync_idle_low = s.pol & 1, t.vsync_idle_low = s.pol & 2, t.pclk_active_neg = s.pol & 4;
    t.pclk_hz = s.pclk;
    t.hsync_pulse = s.hp, t.hsync_back = s.hb, t.hsync_front = s.hf;
    t.vsync_pulse = s.vp, t.vsync_back = s.vb, t.vsync_front = s.vf;
    return p;
}

void nvs_put(const char* key, const void* blob, size_t len, int u8 = -1) {
    if (settings::frozen()) return;
    nvs_handle_t h;
    if (nvs_open(kNvs, NVS_READWRITE, &h) != ESP_OK) return;
    if (u8 >= 0) nvs_set_u8(h, key, static_cast<uint8_t>(u8));
    else if (blob) nvs_set_blob(h, key, blob, len);
    else nvs_erase_key(h, key);
    nvs_commit(h);
    nvs_close(h);
}

void save_panel(const Panel& p) {
    if (is_default(p)) {
        nvs_put("panel", nullptr, 0);
    } else {
        const Stored s = to_stored(p);
        nvs_put("panel", &s, sizeof(s));
    }
    s_saved = p;
}

const char* validate(const Panel& p) {
    const Timing& t = p.timing;
    if (t.pclk_hz < 8000000 || t.pclk_hz > 32000000) return "Pixeltakt muss zwischen 8 und 32 MHz liegen";
    for (uint16_t v : {t.hsync_pulse, t.hsync_back, t.hsync_front, t.vsync_pulse, t.vsync_back, t.vsync_front}) {
        if (v < 1 || v > 200) return "Sync- und Schwarzschulter-Werte müssen zwischen 1 und 200 liegen";
    }
    const double hz = hz_of(effective(p));
    if (hz < 24 || hz > 75) return "Bildwiederholung muss zwischen 24 und 75 Hz liegen";
    return nullptr;
}

void end_trial_locked(bool keep, const char* why) {
    if (!s_trial_until) return;
    s_trial_until = 0;
    esp_timer_stop(s_trial_timer);
    if (keep) {
        save_panel(s_active);
        ESP_LOGW(TAG, "Panel-Werte bestätigt und gespeichert");
        ui::toast("Panel-Werte gespeichert", pixelia::ui::Tone::Success, 4);
    } else {
        apply(s_prev);
        ESP_LOGW(TAG, "Probe beendet, vorige Panel-Werte zurück: %s", why);
        ui::toast("Panel-Werte zurückgesetzt", pixelia::ui::Tone::Warning, 5);
    }
}

void end_test_locked() {
    if (!s_test_until) return;
    s_test_until = 0;
    esp_timer_stop(s_test_timer);
    board::display::set_mode(Mode::Engine);
    board::display::set_touch_marker(-1, -1);
    ESP_LOGI(TAG, "Testbild beendet, Welt läuft wieder");
}

esp_timer_handle_t make_timer(const char* name, void (*cb)(void*)) {
    esp_timer_create_args_t a{};
    a.callback = cb;
    a.name = name;
    esp_timer_handle_t t = nullptr;
    esp_timer_create(&a, &t);
    return t;
}

void panel_json(Json& j, const char* key, const Panel& p) {
    const Timing& t = p.timing;
    const Timing e = effective(p);
    j.obj(key).num("pclk", t.pclk_hz / 1000).num("hp", t.hsync_pulse).num("hb", t.hsync_back).num("hf", t.hsync_front);
    j.num("vp", t.vsync_pulse).num("vb", t.vsync_back).num("vf", t.vsync_front);
    j.boolean("hlow", t.hsync_idle_low).boolean("vlow", t.vsync_idle_low).boolean("neg", t.pclk_active_neg);
    j.boolean("eco", p.eco).num("hz", hz_of(e), 2).num("swap", swap_for(e)).end_obj();
}
}  // namespace

void init() {
    s_trial_timer = make_timer("probe", [](void*) {
        std::lock_guard<std::mutex> g(s_lock);
        end_trial_locked(false, "keine Bestätigung");
    });
    s_test_timer = make_timer("testbild", [](void*) {
        std::lock_guard<std::mutex> g(s_lock);
        end_test_locked();
    });

    nvs_handle_t h;
    uint8_t on = 0, boots = 0;
    Stored st;
    size_t len = sizeof(st);
    bool has_panel = false;
    if (nvs_open(kNvs, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, "exp", &on);
        nvs_get_u8(h, "exp_boot", &boots);
        has_panel = nvs_get_blob(h, "panel", &st, &len) == ESP_OK && len == sizeof(st) && st.ver == 1;
        nvs_close(h);
    }
    std::lock_guard<std::mutex> g(s_lock);
    s_enabled = on != 0;
    if (has_panel) s_saved = from_stored(st);
    if (!s_enabled || validate(s_saved)) s_saved = Panel{};
    s_active = s_saved;
    if (is_default(s_saved)) return;

    // Fluchtweg: die beiden vorigen Starts mit eigenen Werten liefen keine kStableSeconds
    if (boots >= 2) {
        s_safe_boot = true;
        s_active = Panel{};
        nvs_put("exp_boot", nullptr, 0, 0);
        ESP_LOGW(TAG, "Zweimal kurz nach dem Start neu gestartet: Panel startet mit Standardwerten");
        return;
    }
    nvs_put("exp_boot", nullptr, 0, boots + 1);
    s_stable_timer = make_timer("exp-stabil", [](void*) {
        nvs_put("exp_boot", nullptr, 0, 0);
        ESP_LOGI(TAG, "Eigene Panel-Werte laufen seit %d s stabil", kStableSeconds);
    });
    esp_timer_start_once(s_stable_timer, kStableSeconds * 1000000ULL);
    ESP_LOGW(TAG, "Panel startet mit eigenen Werten (%s), Start %u von 2 vor dem Fluchtweg", s_saved.eco ? "Stromsparmodus" : "Timing",
             boots + 1);
}

bool enabled() {
    std::lock_guard<std::mutex> g(s_lock);
    return s_enabled;
}

void set_enabled(bool on) {
    std::lock_guard<std::mutex> g(s_lock);
    if (s_enabled == on) return;
    s_enabled = on;
    nvs_put("exp", nullptr, 0, on ? 1 : 0);
    ESP_LOGW(TAG, "Experimentell %s", on ? "an" : "aus");
    if (on) return;
    end_test_locked();
    ui::hide_picture();
    gfx::reset_places();
    s_trial_until = 0;
    esp_timer_stop(s_trial_timer);
    save_panel(Panel{});
    if (!is_default(s_active)) apply(Panel{});
}

Timing boot_timing() {
    std::lock_guard<std::mutex> g(s_lock);
    return effective(s_active);
}

int boot_swap() {
    std::lock_guard<std::mutex> g(s_lock);
    return swap_for(effective(s_active));
}

bool try_panel(const Panel& p, char* error, size_t error_len) {
    std::lock_guard<std::mutex> g(s_lock);
    const char* why = !s_enabled ? "Experimentell ist aus" : validate(p);
    if (why) {
        std::snprintf(error, error_len, "%s", why);
        return false;
    }
    if (!s_trial_until) s_prev = s_active;  // neue Werte während einer Probe: zurück geht es zum Stand davor
    end_test_locked();
    apply(p);
    s_trial_until = esp_timer_get_time() + kTrialSeconds * 1000000LL;
    esp_timer_stop(s_trial_timer);
    esp_timer_start_once(s_trial_timer, kTrialSeconds * 1000000ULL);
    ESP_LOGW(TAG, "Probe: ohne Bestätigung in %d s zurück", kTrialSeconds);
    return true;
}

bool confirm(char* error, size_t error_len) {
    std::lock_guard<std::mutex> g(s_lock);
    if (!s_trial_until) {
        std::snprintf(error, error_len, "Keine Probe aktiv (schon zurückgesetzt?)");
        return false;
    }
    end_trial_locked(true, nullptr);
    return true;
}

void revert(const char* why) {
    std::lock_guard<std::mutex> g(s_lock);
    end_trial_locked(false, why);
}

void reset_panel() {
    std::lock_guard<std::mutex> g(s_lock);
    s_trial_until = 0;
    esp_timer_stop(s_trial_timer);
    s_safe_boot = false;
    save_panel(Panel{});
    apply(Panel{});
}

bool start_test(Mode mode, char* error, size_t error_len) {
    std::lock_guard<std::mutex> g(s_lock);
    if (mode == Mode::Engine) {
        end_test_locked();
        return true;
    }
    // Gerätetests gehen seit 0.5.4 auch ohne Experimentell (Seite Gerätetests), nur nicht während einer Panel-Probe
    if (s_trial_until) {
        std::snprintf(error, error_len, "%s", "Erst die Probe der Panel-Werte abschließen");
        return false;
    }
    if (mode == Mode::Touch) hwtest::reset_corner_hits();
    board::display::set_mode(mode);
    s_test_until = esp_timer_get_time() + kTestSeconds * 1000000LL;
    esp_timer_stop(s_test_timer);
    esp_timer_start_once(s_test_timer, kTestSeconds * 1000000ULL);
    ESP_LOGI(TAG, "Testbild %s für %d s", board::display::mode_name(mode), kTestSeconds);
    return true;
}

std::string state_json() {
    const hwtest::TouchLive tl = hwtest::touch_live();
    std::lock_guard<std::mutex> g(s_lock);
    const int64_t now = esp_timer_get_time();
    Json j;
    j.obj().boolean("on", s_enabled).boolean("safeBoot", s_safe_boot);
    j.num("trialS", s_trial_until ? std::max<int64_t>(0, (s_trial_until - now + 999999) / 1000000) : -1);
    j.num("trialNeed", kTrialSeconds);
    panel_json(j, "panel", s_active);
    panel_json(j, "saved", s_saved);
    panel_json(j, "std", Panel{});
    const Mode m = board::display::mode();
    j.str("mode", board::display::mode_name(m));
    j.num("testS", s_test_until ? std::max<int64_t>(0, (s_test_until - now + 999999) / 1000000) : -1);
    j.boolean("pic", ui::picture_visible());
    // Orte (Vorschau): Liste, sichtbarer Ort, Übergang, Rundgang in s, Figuren der Testorte (0 = Standard)
    j.arr("places");
    for (int i = 0; i < gfx::place_count(); ++i)
        j.obj().str("id", gfx::place_id(i)).str("title", gfx::place_title(i)).end_obj();
    j.end_arr();
    j.num("place", gfx::place()).str("trans", pixelia::transition_name(gfx::transition()));
    j.num("tour", gfx::tour()).num("figures", gfx::figures());
    j.boolean("follow", gfx::follow()).boolean("inspect", gfx::inspect());
    // Testuhr (seit 0.5.2): Tageszeit der Welt, Zeitraffer, echte Uhrzeit
    j.num("minute", gfx::engine_stats(false).world_minute).num("speed", gfx::speed(), 0);
    j.boolean("real", gfx::real_time());
    // Wetter (seit 0.5.3): Art, Stärke, Licht im Testraum, Partikel am sichtbaren Ort
    const gfx::EngineStats es = gfx::engine_stats(false);
    j.str("weather", pixelia::weather_name(static_cast<pixelia::WeatherKind>(es.weather)));
    j.num("weatherPct", es.weather_pct).boolean("innen", es.inner_light).num("particles", es.particles);
    j.obj("touch").boolean("ok", tl.ok).boolean("down", tl.down).num("x", tl.x).num("y", tl.y);
    j.num("corners", tl.corners).num("n", tl.touches).end_obj();
    j.end_obj();
    return std::move(j.text());
}

}  // namespace app::exper
