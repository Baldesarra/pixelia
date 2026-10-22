#pragma once
// Systemanzeigen auf dem Display: Setup-Modus mit QR-Codes, Update-Fortschritt, Meldungen und Kurzmeldungen.
// Plattformunabhängig, damit der Simulator dieselben Bilder zeigt wie das Gerät.
//
// Palettenregel: Die Indizes 240–255 gehören der System-Oberfläche. Szenen benutzen sie nicht; vor der Ausgabe
// schreibt apply_palette() dort die UI-Farben hinein. So lassen sich Kurzmeldungen über jede Welt legen.

#include <cstdint>
#include <string>
#include <string_view>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/qr.h"

namespace pixelia::ui {

inline constexpr int kFirstIndex = 240;

// Farben wie die dunkle Variante der Weboberfläche (Web-Flasher)
enum Color : uint8_t {
    kBg = 240,
    kSurface,
    kSurfaceHigh,
    kOutline,
    kTextDim,
    kText,
    kPrimary,
    kPrimaryDim,
    kOnPrimary,
    kSuccess,
    kWarning,
    kError,
    kQrDark,
    kQrLight,
    kShadow,
    kWhite,
};

enum class Tone : uint8_t { Info, Success, Warning, Error };

// UI-Farben in die Indizes 240–255 schreiben
void apply_palette(Palette& palette);

// Daten für den Setup-Bildschirm
struct SetupInfo {
    std::string ap_ssid = "Pixelia-Setup";
    std::string ap_password;
    std::string ap_url = "http://192.168.4.1";
    int clients = 0;            // am Setup-WLAN angemeldete Geräte
    int saved_networks = 0;     // gespeicherte Heim-WLANs
    int retry_seconds = -1;     // bis zum nächsten Verbindungsversuch, -1 = keiner geplant
    std::string connecting;     // WLAN, mit dem gerade verbunden wird (leer = keins)
    std::string last_error;     // Ergebnis des letzten Versuchs, z. B. „Passwort falsch?“

    bool operator==(const SetupInfo&) const = default;
};

class SetupScreen {
public:
    // Erzeugt die QR-Codes nur neu, wenn sich Zugangsdaten oder Adresse ändern
    void set(const SetupInfo& info);
    const SetupInfo& info() const { return info_; }
    void render(Canvas& canvas) const;

private:
    SetupInfo info_;
    qr::Code wifi_qr_;
    qr::Code url_qr_;
    std::string qr_key_;
};

// Daten für den Update-Bildschirm
struct UpdateInfo {
    enum class Phase : uint8_t { Receiving, Verifying, Done, Failed };
    Phase phase = Phase::Receiving;
    std::string from_version;
    std::string to_version;
    uint32_t bytes = 0;
    uint32_t total = 0;
    std::string message;  // Fehlertext bei Failed
};
void render_update(Canvas& canvas, const UpdateInfo& info);

// Ganzseitige Meldung (z. B. „Verbunden“ nach dem Setup)
struct MessageInfo {
    std::string title;
    std::string body;  // '\n' für Zeilenumbrüche
    Tone tone = Tone::Info;
};
void render_message(Canvas& canvas, const MessageInfo& info);

// Kurzmeldung unten im Bild, über der laufenden Welt
void draw_toast(Canvas& canvas, std::string_view text, Tone tone);

// Daten für die Info-Karte (langer Druck aufs Display): wie ist Pixelia gerade erreichbar?
struct InfoCardData {
    enum class Mode : uint8_t {
        Connected,  // im Heimnetz: IP, Namen, WLAN, Signal, QR-Code mit http://<IP>/
        Setup,      // Setup-Modus: Access Point, Passwort, QR-Code zum Beitreten
        Offline,    // weder noch (z. B. Verbindungsversuch läuft)
    };
    Mode mode = Mode::Connected;
    std::string version;               // Firmware
    std::string hostname = "Pixelia";  // Name im Router (DHCP)
    std::string mdns_host = "pixelia"; // ergibt pixelia.local, http://pixelia/ und pixelia.fritz.box
    // Heimnetz
    std::string ip;
    std::string ssid;
    int rssi = 0;
    // Setup-Modus
    std::string ap_ssid;
    std::string ap_password;
    std::string ap_ip;
    int ap_clients = 0;
    // Was gerade passiert, z. B. „Verbinde mit „Heimnetz“ …“ oder der letzte Fehler
    std::string status;

    bool operator==(const InfoCardData&) const = default;
};

class InfoCard {
public:
    // Erzeugt den QR-Code nur neu, wenn sich Adresse bzw. Zugangsdaten ändern
    void set(const InfoCardData& data);
    const InfoCardData& data() const { return data_; }
    // Als Karte über das aktuelle Bild zeichnen, der Rest wird abgedunkelt
    void render(Canvas& canvas) const;

private:
    InfoCardData data_;
    qr::Code qr_;
    std::string qr_key_;
};

// Rechteck in Leinwand-Koordinaten (400×240); margin vergrößert die Trefferfläche für Finger
struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool contains(int px, int py, int margin = 0) const {
        return px >= x - margin && px < x + w + margin && py >= y - margin && py < y + h + margin;
    }
};

// Knopf „Zurücksetzen …“ unten links auf der Info-Karte
Rect info_reset_button();

// Zurücksetzen am Gerät (Info-Karte → Zurücksetzen): PIN entfernen oder Werksreset, jeweils Knopf halten.
// Der Weg für einen vergessenen PIN (Projektplan 4.12): Wer das Display berühren kann, darf das.
inline constexpr int kResetHoldMs = 3000;
enum class ResetButton : int8_t { None = -1, Pin = 0, Factory = 1, Cancel = 2 };
struct ResetInfo {
    bool pin_set = false;                    // ohne PIN ist „PIN entfernen“ ausgegraut
    ResetButton held = ResetButton::None;    // gerade gehaltener Knopf
    int permille = 0;                        // Fortschritt des Haltens 0–1000

    bool operator==(const ResetInfo&) const = default;
};
Rect reset_button_rect(ResetButton button);
// Knopf an der Stelle (Leinwand-Koordinaten), ausgegraute Knöpfe zählen nicht
ResetButton reset_button_at(const ResetInfo& info, int x, int y);
void render_reset(Canvas& canvas, const ResetInfo& info);

// Halte-Anzeige während eines langen Drucks (progress 0..1), unten in der Mitte; seit 0.5.6 mit Text (auf einer
// Figur „Halten: Folgen“)
void draw_hold_indicator(Canvas& canvas, float progress, std::string_view label = "Halten für Info …");

// Signalstärke als 0–4 Balken (−55/−65/−75/−85 dBm)
int signal_bars(int rssi);
void draw_signal_bars(Canvas& canvas, int x, int baseline, int rssi);

// Bausteine
void round_rect(Canvas& canvas, int x, int y, int w, int h, uint8_t color, int radius = 2);
// Rahmen aus zwei abgerundeten Flächen: außen Kontur, innen Füllung
void framed(Canvas& canvas, int x, int y, int w, int h, uint8_t fill, uint8_t border, int radius = 3);
void draw_qr(Canvas& canvas, const qr::Code& code, int x, int y, int scale, int quiet_modules);
uint8_t tone_color(Tone tone);

}  // namespace pixelia::ui
