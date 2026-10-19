#pragma once
// Optionaler PIN bzw. Passwort (Projektplan 4.12, Standard aus). Ist er gesetzt, verlangen Einstellungen, Aktionen,
// Updates, Werksreset, /api/cmd, das Löschen des Absturzberichts und Gerätenamen eine Anmeldung.
//
// - Im NVS (Namensraum „auth“) liegt nur ein gesalzener, vielfach gehashter Wert (SHA-256), nie der PIN selbst.
// - Die Oberfläche meldet sich einmal an (POST /api/login) und schickt danach ein Sitzungs-Token mit
//   (Kopf „X-Pixelia-Token“). Tokens leben nur im RAM: nach Neustart oder PIN-Wechsel neu anmelden.
// - Skripte (tools/*.ps1) schicken den PIN direkt im Kopf „X-Pixelia-Pin“ oder im Formularfeld „pin“.
// - Sperre nach 5 Fehlversuchen: 30 s, jeder weitere Fehlversuch verdoppelt bis höchstens 15 min.
// - Vergessen: nur am Gerät zurücksetzen (Info-Karte → „Zurücksetzen“, Knopf halten) oder per Werksreset.

#include <cstddef>

#include "esp_err.h"

namespace app::auth {

inline constexpr int kMinLen = 4;
inline constexpr int kMaxLen = 32;
inline constexpr int kTokenLen = 32;  // Hex-Zeichen

enum class Result { Ok, Wrong, Locked };

void init();
bool enabled();
// Sekunden bis zum Ende der Sperre (0 = nicht gesperrt), verbleibende Versuche vor der nächsten Sperre
int lock_seconds();
int attempts_left();

// PIN prüfen (zählt Fehlversuche). Bei Ok und token != nullptr: neues Sitzungs-Token (kTokenLen + 1 Byte)
Result check(const char* pin, char* token = nullptr);
bool token_valid(const char* token);
void logout(const char* token);

// Neuen PIN setzen (leer = aus). Meldet alle Sitzungen ab; mit token != nullptr gleich wieder angemeldet.
// ESP_ERR_INVALID_SIZE: Länge außerhalb kMinLen–kMaxLen, ESP_ERR_INVALID_ARG: Steuerzeichen
esp_err_t set(const char* pin, char* token = nullptr);

// Am Gerät (Touch-Geste): PIN entfernen, Sperre aufheben
void clear_on_device();

}  // namespace app::auth
