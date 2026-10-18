#pragma once
// Hintergrundbeleuchtung per LEDC-PWM (12 Bit).
// 200 Hz ist geprüft (docs/HARDWARE.md): Der LED-Spannungswandler braucht nach jedem Einschalten Zeit zum
// Hochfahren. Ab 2 kHz bleibt das Display dunkel, 500 Hz–1 kHz pfeifen hörbar.

#include <cstdint>

#include "esp_err.h"

namespace board::backlight {

inline constexpr uint32_t kDefaultFrequencyHz = 200;

esp_err_t init(uint32_t freq_hz = kDefaultFrequencyHz);
// 0..100 %
void set_percent(int percent);
int percent();
esp_err_t set_frequency(uint32_t freq_hz);
uint32_t frequency();

}  // namespace board::backlight
