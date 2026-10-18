#pragma once
// Kleiner QR-Code-Erzeuger ohne Fremdbibliothek (ISO/IEC 18004): Byte-Modus, Version 1–10, alle vier
// Fehlerkorrekturstufen, automatische Maskenwahl. Reicht für WLAN-Zugangsdaten und Adressen auf dem Display.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pixelia::qr {

enum class Ecc : uint8_t { Low, Medium, Quartile, High };

struct Code {
    int version = 0;
    int size = 0;  // Module pro Seite (17 + 4 × Version)
    int mask = -1;
    std::vector<uint8_t> modules;  // size × size, 1 = dunkel

    bool dark(int x, int y) const { return modules[static_cast<size_t>(y) * size + x] != 0; }
};

// Kleinste passende Version wählen und kodieren. mask = -1: beste Maske nach den Strafpunkten der Norm.
// false, wenn die Daten nicht in Version 10 passen.
bool encode(std::string_view data, Ecc ecc, Code* out, int mask = -1);

// Text für den WLAN-QR-Code („WIFI:T:WPA;S:…;P:…;;“), Sonderzeichen werden maskiert
std::string wifi_payload(std::string_view ssid, std::string_view password);

}  // namespace pixelia::qr
