#include "board/backlight.h"

#include <algorithm>
#include <cmath>

#include "board/pins.h"
#include "driver/ledc.h"

namespace board::backlight {

namespace {
constexpr ledc_timer_t kTimer = LEDC_TIMER_0;
constexpr ledc_channel_t kChannel = LEDC_CHANNEL_0;
constexpr ledc_timer_bit_t kResolution = LEDC_TIMER_12_BIT;
constexpr double kGamma = 2.2;
int s_percent = 0;
uint32_t s_duty = 0;
uint32_t s_min_duty = kDefaultMinDuty;
uint32_t s_freq = 0;
}  // namespace

esp_err_t init(uint32_t freq_hz) {
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = kResolution;
    timer.timer_num = kTimer;
    timer.freq_hz = freq_hz;
    timer.clk_cfg = LEDC_AUTO_CLK;
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK) return err;

    ledc_channel_config_t ch = {};
    ch.gpio_num = kBacklight;
    ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ch.channel = kChannel;
    ch.timer_sel = kTimer;
    ch.duty = 0;
    ch.hpoint = 0;
    err = ledc_channel_config(&ch);
    if (err == ESP_OK) s_freq = freq_hz;
    return err;
}

uint32_t duty_for(int percent) {
    percent = std::clamp(percent, 0, 100);
    if (percent == 0) return 0;
    const double f = std::pow((percent - 1) / 99.0, kGamma);  // 1 % = kleinste Pulsbreite, 100 % = volle
    return s_min_duty + static_cast<uint32_t>(std::lround(f * (kMaxDuty - s_min_duty)));
}

void set_duty(uint32_t duty) {
    s_duty = std::min(duty, kMaxDuty);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, kChannel, s_duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, kChannel);
}

uint32_t duty() { return s_duty; }

void set_percent(int percent) {
    s_percent = std::clamp(percent, 0, 100);
    set_duty(duty_for(s_percent));
}

int percent() { return s_percent; }

void set_min_duty(uint32_t duty) {
    s_min_duty = std::clamp<uint32_t>(duty, 1, kMaxDuty / 4);
    if (s_freq) set_percent(s_percent);  // erst nach init()
}

uint32_t min_duty() { return s_min_duty; }

esp_err_t set_frequency(uint32_t freq_hz) {
    esp_err_t err = ledc_set_freq(LEDC_LOW_SPEED_MODE, kTimer, freq_hz);
    if (err == ESP_OK) s_freq = freq_hz;
    return err;
}

uint32_t frequency() { return s_freq; }

}  // namespace board::backlight
