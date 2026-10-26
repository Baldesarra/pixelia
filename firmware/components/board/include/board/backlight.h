#pragma once
// Hintergrundbeleuchtung per LEDC-PWM (12 Bit).
// 200 Hz ist geprüft (docs/HARDWARE.md): Der LED-Spannungswandler braucht nach jedem Einschalten Zeit zum
// Hochfahren. Ab 2 kHz bleibt das Display dunkel, 500 Hz–1 kHz pfeifen hörbar.
//
// Helligkeitskurve (seit 0.5.7): Das Auge empfindet Helligkeit nicht linear. Prozent werden deshalb mit dem
// Exponenten 2,2 auf die Pulsbreite abgebildet (50 % wirken wie die halbe Helligkeit), 1 % liegt auf der kleinsten
// noch sicher leuchtenden Pulsbreite (min_duty, am Gerät gemessen, Befehl „hell min“).

#include <cstdint>

#include "esp_err.h"

namespace board::backlight {

inline constexpr uint32_t kDefaultFrequencyHz = 200;
inline constexpr uint32_t kMaxDuty = (1u << 12) - 1;
inline constexpr uint32_t kDefaultMinDuty = 8;

esp_err_t init(uint32_t freq_hz = kDefaultFrequencyHz);
// 0..100 % nach der Helligkeitskurve (0 = aus)
void set_percent(int percent);
int percent();
uint32_t duty_for(int percent);
// Pulsbreite direkt (Messung der Kurve), 0 … kMaxDuty
void set_duty(uint32_t duty);
uint32_t duty();
void set_min_duty(uint32_t duty);
uint32_t min_duty();
esp_err_t set_frequency(uint32_t freq_hz);
uint32_t frequency();

}  // namespace board::backlight
