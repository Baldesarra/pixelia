#pragma once
// Update von GitHub (seit 0.4.1): neue Versionen aus den Releases des öffentlichen Repos, Kanal Stabil/Vorschau,
// Installation per HTTPS direkt vom Gerät.
//
// - Auf Knopfdruck und (seit 0.4.2, abschaltbar) höchstens einmal täglich im Hintergrund: ohne Token erlaubt GitHub
//   60 Abfragen pro Stunde und IP. Kein Token auf dem Gerät. 403/429 wegen des Limits werden mit der Wartezeit
//   gemeldet. Die Hintergrundprüfung installiert nie, sie setzt nur den Hinweis (available(), Punkt an „System“).
// - Laufenden Download abbrechen (seit 0.4.2): die neue Partition bleibt ungültig, kein Neustart.
// - Ältere Versionen ab 0.4.0 (Downgrade, nur über die Seite Experimentell) kommen aus derselben Prüfung.
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
#include <cstdint>
#include <string>

namespace app::github {

inline constexpr const char* kRepo = "Baldesarra/pixelia";

enum class Channel : unsigned char { Stable, Preview };

void init();  // Kanal aus dem NVS
Channel channel();
void set_channel(Channel c);

// Asynchron starten; false mit Grund, wenn es gerade nicht geht (Setup-Modus, läuft schon …)
bool start_check(char* error, size_t error_len);
// older: Version aus der Liste der älteren Versionen (Downgrade)
bool start_install(const char* version, bool older, char* error, size_t error_len);
bool cancel_install();  // false, wenn kein Download läuft

void set_auto(bool on);   // Prüfung im Hintergrund (NVS upd_auto, Standard an)
std::string available();  // neuere Version laut letzter Prüfung (auch über Neustarts), leer = keine
uint32_t seq();           // Zähler: ändert sich mit jedem neuen Stand von state_json()

// Stand für GET /api/update: Kanal, letzte Prüfung, neuere Versionen mit Release-Notes, Limit, Fehler
std::string state_json();

}  // namespace app::github
