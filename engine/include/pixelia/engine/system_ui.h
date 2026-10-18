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

// Bausteine
void round_rect(Canvas& canvas, int x, int y, int w, int h, uint8_t color, int radius = 2);
void draw_qr(Canvas& canvas, const qr::Code& code, int x, int y, int scale, int quiet_modules);
uint8_t tone_color(Tone tone);

}  // namespace pixelia::ui
