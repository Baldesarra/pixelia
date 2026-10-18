#pragma once
// Hintergrundbeleuchtung per LEDC-PWM

#include <cstdint>

#include "esp_err.h"

namespace board::backlight {

esp_err_t init(uint32_t freq_hz = 20000);
// 0..100 %
void set_percent(int percent);
int percent();
esp_err_t set_frequency(uint32_t freq_hz);
uint32_t frequency();

}  // namespace board::backlight
