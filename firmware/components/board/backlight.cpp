#include "board/backlight.h"

#include <algorithm>

#include "board/pins.h"
#include "driver/ledc.h"

namespace board::backlight {

namespace {
constexpr ledc_timer_t kTimer = LEDC_TIMER_0;
constexpr ledc_channel_t kChannel = LEDC_CHANNEL_0;
constexpr ledc_timer_bit_t kResolution = LEDC_TIMER_12_BIT;
constexpr uint32_t kMaxDuty = (1u << 12) - 1;
int s_percent = 0;
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

void set_percent(int percent) {
    s_percent = std::clamp(percent, 0, 100);
    uint32_t duty = (kMaxDuty * static_cast<uint32_t>(s_percent) + 50) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, kChannel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, kChannel);
}

int percent() { return s_percent; }

esp_err_t set_frequency(uint32_t freq_hz) {
    esp_err_t err = ledc_set_freq(LEDC_LOW_SPEED_MODE, kTimer, freq_hz);
    if (err == ESP_OK) s_freq = freq_hz;
    return err;
}

uint32_t frequency() { return s_freq; }

}  // namespace board::backlight
