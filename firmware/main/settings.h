#pragma once
// Einstellungen im NVS (Projektplan 4.3, 5): bleiben bei Updates und Neustarts erhalten.
// WLAN-Zugangsdaten verwaltet der WLAN-Manager selbst (eigener Namensraum, Passwörter verlassen das Gerät nie).

#include <cstdint>

#include "esp_err.h"

namespace app::settings {

struct Values {
    bool rotate180 = false;   // Bild um 180° drehen (Aufstellort, Backlog 2026-10-18)
    uint8_t brightness = 100;  // Hintergrundbeleuchtung in Prozent (1–100)
    uint16_t long_press_ms = 3000;  // langer Druck aufs Display → Info-Karte (kLongPressMin–kLongPressMax)
};

inline constexpr int kLongPressMin = 1000;
inline constexpr int kLongPressMax = 10000;

// NVS-Partition öffnen (bei Formatwechsel neu anlegen) und Werte laden. Ganz am Anfang aufrufen.
void init();
Values get();

// Ändern, sofort anwenden und speichern
void set_rotation180(bool on);
void set_brightness(int percent);
void set_long_press_ms(int ms);

void log_all();

}  // namespace app::settings
