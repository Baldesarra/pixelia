#pragma once
// Updates (Projektplan 4.10): .bin-Upload mit Prüfung, A/B-Partitionen, Probelauf mit Selbsttest, Rückfall.
//
// - Vor dem Schreiben: gültiges ESP-App-Image, Chip ESP32-S3, passende Chip-Revision, Projektname „pixelia“.
// - Nach dem Schreiben prüft esp_ota_end() Prüfsumme und SHA-256, erst dann wird die neue Partition aktiv.
// - Die neue Version startet im Probelauf. Der Selbsttest verlangt ~60 s lang: Display liefert Bilder (Panel und
//   Engine), WLAN verbunden oder Setup-Modus aktiv, kein Absturz. Erst dann gilt sie dauerhaft.
// - Absturz, Hänger (Watchdog) oder Neustart im Probelauf: Der Bootloader startet die alte Version, die das
//   beim Start erkennt und meldet („Update auf X wurde zurückgerollt“ samt Grund und Absturzbericht).

#include <cstddef>
#include <cstdint>

#include "esp_app_desc.h"
#include "esp_err.h"

namespace app::ota {

inline constexpr int kTrialSeconds = 60;      // so lange muss die neue Version gesund laufen
inline constexpr int kTrialMaxSeconds = 120;  // danach gilt der Selbsttest als nicht bestanden

struct Status {
    char version[32];  // laufende Version
    char running[8];   // laufende Partition (ota_0/ota_1)
    // Upload
    bool busy;
    uint32_t bytes;
    uint32_t total;
    char new_version[32];
    char last_error[96];
    // Probelauf
    bool trial;
    int trial_elapsed_s;
    int trial_ok_s;
    char trial_note[64];
    bool trial_passed;  // in diesem Start bestanden
    // Rückfall auf die alte Version
    bool rollback;
    bool rollback_new;  // in diesem Start erkannt (für die Reset-Historie)
    char rollback_version[32];
    char rollback_reason[96];
};

void init();            // früh beim Start: Probelauf und Rückfall erkennen
void start_selftest();  // nach dem Start aller Dienste
Status status();
void ack_rollback();    // Hinweis auf den Rückfall gelesen

// Kopf einer .bin prüfen (mindestens kHeaderBytes). desc darf nullptr sein.
inline constexpr size_t kHeaderBytes = 24 + 8 + sizeof(esp_app_desc_t);
bool check_header(const uint8_t* head, size_t len, esp_app_desc_t* desc, char* error, size_t error_len);

// Upload in Stücken (vom Webserver); begin() bekommt die ersten kHeaderBytes
esp_err_t begin(const uint8_t* head, size_t len, size_t total, char* error, size_t error_len);
esp_err_t write(const uint8_t* data, size_t len);
esp_err_t finish(char* error, size_t error_len);
void cancel(const char* why);

// Neustart nach ms Millisekunden (Antwort an den Browser kann noch raus)
void restart_soon(int ms);

void log_status();

}  // namespace app::ota
