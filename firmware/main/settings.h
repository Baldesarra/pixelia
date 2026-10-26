#pragma once
// Einstellungen im NVS (Projektplan 4.3, 5): bleiben bei Updates und Neustarts erhalten.
// WLAN-Zugangsdaten verwaltet der WLAN-Manager selbst (eigener Namensraum, Passwörter verlassen das Gerät nie).

#include <cstdint>
#include <string>

#include "esp_err.h"

namespace app::settings {

inline constexpr size_t kUiMax = 120;

struct Values {
    bool rotate180 = false;   // Bild um 180° drehen (Aufstellort, Backlog 2026-10-18)
    uint8_t brightness = 100;  // Hintergrundbeleuchtung in Prozent (1–100)
    uint16_t long_press_ms = 3000;  // langer Druck aufs Display → Info-Karte (kLongPressMin–kLongPressMax)
    // Farbwahl der Weboberfläche als JSON ({"mode":…,"seed":…,"style":…}), leer = Standard. Das Gerät prüft nur
    // Länge und Zeichen, die Oberfläche deutet den Inhalt (seit 0.3.2, vorher nur im Browser)
    char ui[kUiMax + 1] = "";
    // Pixel-Uhr (seit 0.5.6): Standard an, oben rechts, ohne Sekunden (Entscheidung des Users 2026-10-22).
    // Ecke 0 oben rechts, 1 oben links, 2 unten rechts, 3 unten links
    bool clock_on = true;
    uint8_t clock_corner = 0;
    bool clock_seconds = false;
    // Kamera (seit 0.5.6): Standard 0 Hauptfigur, 1 Figur, 2 Ort; Rückkehr in s, 0 = nie (Standard 10 min)
    uint8_t cam_home = 0;
    uint16_t cam_figure = 0;
    uint8_t cam_place = 0;
    uint16_t cam_return_s = 600;
    // Nachtmodus (seit 0.5.7): 22–6 Uhr auf die Nacht-Helligkeit (Standard an, 25 %, Entscheidung des Users)
    bool night_on = true;
    uint8_t night_bright = 25;
    // Kleinste noch leuchtende Pulsbreite der Hintergrundbeleuchtung (Helligkeitskurve, am Gerät gemessen)
    uint16_t bl_min = 8;
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
bool set_ui(const std::string& json);  // false = zu lang oder Steuerzeichen
void set_clock(bool on, int corner, bool seconds);
void set_camera(int home, int figure, int place, int return_s);
void set_night(bool on, int percent);
void set_backlight_min(int duty);

// Werksreset: Einstellungen, WLAN-Netze, Setup-Passwort, Gerätenamen und PIN löschen, dann Neustart.
// Startzähler und Reset-Historie (NVS „diag“) bleiben für die Diagnose erhalten. why landet im Log.
void factory_reset(const char* why);
// true, sobald der Werksreset läuft: Module schreiben dann nichts mehr ins NVS
bool frozen();

void log_all();

}  // namespace app::settings
