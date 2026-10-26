// Nachtmodus (seit 0.5.7, Entscheidung des Users 2026-10-26: sanft dimmen): Nachts (22–6 Uhr, dieselbe Nacht wie die
// gedimmte Pixel-Uhr, app::is_night) dimmt das Display ab 22 Uhr über 15 Minuten auf die Nacht-Helligkeit und ab 6 Uhr
// wieder hoch. Berühren hellt für eine Minute auf. Ohne bekannte Uhrzeit gibt es keine Nacht. Zeitgeber „nacht“ alle
// 250 ms auf Kern 0; die Hintergrundbeleuchtung wird nur bei einer Änderung neu gesetzt (Gerätetests stellen sie
// selbst und bleiben so ungestört).

#include <algorithm>
#include <atomic>

#include "app.h"
#include "board/backlight.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "net.h"
#include "settings.h"

namespace app::night {

namespace {
constexpr const char* TAG = "nacht";
constexpr int kFadeMinutes = 15;
constexpr int64_t kWakeUs = 60LL * 1000000;
std::atomic<int64_t> s_wake_until{0};
std::atomic<bool> s_active{false};
std::atomic<bool> s_refresh{true};
int s_current = -1;  // angewendete Helligkeit (Prozent)
esp_timer_handle_t s_timer = nullptr;

int lerp(int a, int b, int num, int den) { return a + (b - a) * num / den; }

// Helligkeit nach der Uhrzeit: Tag, Abblenden ab 22 Uhr, Nacht, Aufblenden ab 6 Uhr
int target(const settings::Values& v, int minute) {
    const int day = v.brightness, low = std::min<int>(v.night_bright, v.brightness);
    const int from = kNightFrom * 60, to = kNightTo * 60;
    if (minute >= from && minute < from + kFadeMinutes) return lerp(day, low, minute - from, kFadeMinutes);
    if (is_night(minute / 60)) return low;
    if (minute >= to && minute < to + kFadeMinutes) return lerp(low, day, minute - to, kFadeMinutes);
    return day;
}

void tick(void*) {
    const settings::Values v = settings::get();
    int64_t local = 0;
    const bool known = timesync::local_us(&local);
    int t = v.brightness;
    if (v.night_on && known) t = target(v, static_cast<int>(local / 60000000 % 1440));
    const bool night = t < v.brightness;
    if (night != s_active.load()) ESP_LOGI(TAG, "Nachtmodus %s", night ? "beginnt" : "endet");
    s_active.store(night);
    if (esp_timer_get_time() < s_wake_until.load()) t = v.brightness;
    if (s_refresh.exchange(false) || s_current < 0) {
        s_current = t;  // Einstellung geändert: sofort
    } else if (s_current < t) {
        s_current = std::min(t, s_current + 10);  // aufhellen schnell (Berühren)
    } else if (s_current > t) {
        s_current = std::max(t, s_current - 2);   // abdunkeln sanft
    } else {
        return;
    }
    board::backlight::set_percent(s_current);
}
}  // namespace

void init() {
    const esp_timer_create_args_t args = {.callback = tick, .arg = nullptr, .dispatch_method = ESP_TIMER_TASK,
                                          .name = "nacht", .skip_unhandled_events = true};
    if (esp_timer_create(&args, &s_timer) == ESP_OK) esp_timer_start_periodic(s_timer, 250 * 1000);
}

void refresh() { s_refresh.store(true); }

void touched() {
    if (s_active.load()) s_wake_until.store(esp_timer_get_time() + kWakeUs);
}

bool active() { return s_active.load(); }
bool awake() { return s_active.load() && esp_timer_get_time() < s_wake_until.load(); }

}  // namespace app::night
