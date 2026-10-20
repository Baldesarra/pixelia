#pragma once
// Update von GitHub (seit 0.4.1): neue Versionen aus den Releases des öffentlichen Repos, Kanal Stabil/Vorschau,
// Installation per HTTPS direkt vom Gerät.
//
// - Nur auf Anfrage (Knopf in der Oberfläche): ohne Token erlaubt GitHub 60 Abfragen pro Stunde und IP. Kein Token auf
//   dem Gerät. 403/429 wegen des Limits werden mit der Wartezeit gemeldet.
// - Die Antwort von /releases (mit den Release-Notes) landet im PSRAM und wird dort gelesen (kleiner Leser ohne
//   Baum, cJSON fehlt in ESP-IDF 6.1 und würde jeden Knoten im internen RAM anlegen).
// - Vorschau = Versionen X.Y.Z-beta.N (auf GitHub als Prerelease markiert). Vergleich wie version_key in
//   tools/release_notes.py: beta.N kommt vor der fertigen Version.
// - Installation: Datei pixelia-vX.Y.Z.bin über die Weiterleitung auf den Datei-Server von GitHub, Zertifikate aus
//   dem Bundle von ESP-IDF. Vor dem Schreiben: Kopf, Projektname und Version (wie im Release) und Größe; dann
//   ota::begin/write/finish mit allen Prüfungen, Probelauf, Selbsttest und Rückfall wie beim Upload.
// - Arbeit in einer eigenen Task auf Kern 0 (der Webserver bleibt frei, Fortschritt über /api/status).
// - Im Setup-Modus (eigenes WLAN, kein Internet) gesperrt.

#include <cstddef>
#include <string>

namespace app::github {

inline constexpr const char* kRepo = "Baldesarra/pixelia";

enum class Channel : unsigned char { Stable, Preview };

void init();  // Kanal aus dem NVS
Channel channel();
void set_channel(Channel c);

// Asynchron starten; false mit Grund, wenn es gerade nicht geht (Setup-Modus, läuft schon …)
bool start_check(char* error, size_t error_len);
bool start_install(const char* version, char* error, size_t error_len);

// Stand für GET /api/update: Kanal, letzte Prüfung, neuere Versionen mit Release-Notes, Limit, Fehler
std::string state_json();

}  // namespace app::github
