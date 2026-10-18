#pragma once
// Netzwerk (Projektplan 4.12): WLAN-Manager mit bis zu 5 gespeicherten Netzen, Setup-Modus mit Access Point
// „Pixelia-Setup“ und Captive Portal, Hostname „Pixelia“, mDNS pixelia.local, NTP-Zeit (Europa/Berlin).
// Läuft komplett auf Kern 0.

#include <cstdint>

#include "esp_err.h"

namespace app::wifi {

inline constexpr int kMaxSaved = 5;
inline constexpr const char* kHostname = "Pixelia";      // im Router (DHCP)
inline constexpr const char* kMdnsHost = "pixelia";      // pixelia.local
inline constexpr const char* kApSsid = "Pixelia-Setup";
inline constexpr const char* kApIp = "192.168.4.1";
inline constexpr int kRetrySeconds = 120;                // Setup-Modus: neuer Versuch alle 2 Minuten

enum class State : uint8_t {
    Off,         // noch nicht gestartet
    Connecting,  // Verbindungsversuch läuft
    Connected,   // im Heimnetz
    Setup,       // Access Point aktiv, kein Heimnetz
};

struct Status {
    State state = State::Off;
    bool ap_active = false;
    int ap_clients = 0;
    char ssid[33] = "";        // verbundenes bzw. gerade versuchtes Netz
    int8_t rssi = 0;
    uint8_t channel = 0;
    char ip[16] = "";
    char last_error[64] = "";  // Ergebnis des letzten fehlgeschlagenen Versuchs
    int retry_in_s = -1;       // Setup-Modus: Sekunden bis zum nächsten Versuch
    int saved = 0;
    uint32_t connected_since_s = 0;  // Laufzeit beim Verbinden
};

struct ScanEntry {
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
    bool secure;
    bool saved;
};

void start();
Status status();
const char* state_name(State s);

// Namen der gespeicherten Netze, zuletzt benutztes zuerst (ohne Passwörter)
int saved_networks(char (*ssids)[33], int max);

// Speichert (oder aktualisiert) ein Netz und verbindet sofort damit. Passwort leer = offenes Netz.
esp_err_t add_network(const char* ssid, const char* password);
esp_err_t remove_network(const char* ssid);

// Letztes Scan-Ergebnis; fresh = neu scannen (blockiert bis zu timeout_ms)
int scan(ScanEntry* out, int max, bool fresh, int timeout_ms = 8000);

// Sofort neu verbinden bzw. den Setup-Modus erzwingen (Konsole, Weboberfläche)
void reconnect();
void force_setup();

}  // namespace app::wifi

namespace app::captive_dns {
// Beantwortet alle Namensanfragen im Setup-WLAN mit 192.168.4.1 (Captive Portal)
void start();
void stop();
bool running();
}  // namespace app::captive_dns

namespace app::timesync {
// Zeitzone setzen und NTP vorbereiten; Abgleich startet mit der ersten WLAN-Verbindung
void init();
void on_connected();
bool valid();  // echte Uhrzeit bekannt (NTP oder aus der Zeit vor einem Neustart)
// Lokale Zeit in µs seit 1970 (für die Weltzeit), false ohne gültige Uhrzeit
bool local_us(int64_t* out);
void log_status();
uint32_t sync_count();
}  // namespace app::timesync
