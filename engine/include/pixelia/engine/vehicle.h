#pragma once
// Fahrzeuge als bewegliche Orte (seit 0.5.8, Schnittstelle; Zug, Draisine und Fuhrwerk kommen mit der Welt).
//
// - Ein Fahrzeug ist ein Ort (PlaceDef mit kPlaceVehicle) mit Fahrplan: Halte (Ort und Aufenthalt) und die Fahrzeit
//   bis zum nächsten Halt, im Kreis. Wo es gerade ist, ist eine reine Rechnung aus der Weltminute: Es fährt weiter,
//   wenn niemand hinschaut, springt mit der Uhr und steht auf Gerät und Simulator an derselben Stelle.
// - Verbindungen zum und vom Fahrzeug sind Daten wie alle anderen (room.h, Exit) mit der Art „Bahnhof“ und der
//   Bedingung „Fahrplan“ (kOpenTimetable, needs = Kennung des Fahrzeugs): Am Halt führt ein Ausgang ins Fahrzeug, im
//   Fahrzeug je Halt einer hinaus. Offen ist eine solche Verbindung nur, solange das Fahrzeug an diesem Halt steht
//   (exit_open in place.h). So sieht der Orts-Graph alle Wege, und Figuren steigen nur am stehenden Fahrzeug ein und aus.

#include <array>
#include <cstdint>
#include <string_view>

namespace pixelia {

struct VehicleStop {
    const char* place;  // Kennung des Ortes am Halt
    uint16_t dwell;     // Minuten Aufenthalt
    uint16_t travel;    // Minuten Fahrt bis zum nächsten Halt
};

struct VehicleDef {
    const char* id;     // Kennung = Kennung des Fahrzeug-Ortes
    const VehicleStop* stops;
    uint8_t stop_count;
    uint16_t start;     // Minute des Tages, zu der der Umlauf am ersten Halt beginnt (Tag 0)
};

struct VehiclePos {
    int stop = 0;          // Halt, an dem es steht, bzw. der zuletzt verlassene
    bool standing = true;
    int next = 0;          // nächster Halt
    int minutes = 0;       // seit Ankunft bzw. Abfahrt
    int left = 0;          // bis zur Abfahrt bzw. Ankunft
};

int vehicle_cycle(const VehicleDef& v);  // Minuten eines Umlaufs
VehiclePos vehicle_at(const VehicleDef& v, int64_t world_minute);

// Fahrzeuge der Welt (Umgebung): Orte melden ihr Fahrzeug beim Start an
class Vehicles {
public:
    static constexpr int kMax = 8;
    bool add(const VehicleDef* v);  // schon bekannt: true; false = voll
    const VehicleDef* find(std::string_view id) const;
    int count() const { return n_; }
    // Steht das Fahrzeug id gerade am Ort place?
    bool stands_at(std::string_view id, std::string_view place, int64_t world_minute) const;

private:
    std::array<const VehicleDef*, kMax> v_{};
    int n_ = 0;
};

}  // namespace pixelia
