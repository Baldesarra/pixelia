#pragma once
// Umgebung der Orte (seit 0.5.3): was alle Orte einer Bühne gemeinsam sehen, das Wetter und die Lichtquellen.
// Die Bühne hält sie und gibt sie jedem Ort (Scene::set_environment); bis zum gemeinsamen Weltzustand (0.5.9) ist
// sie der einzige Weg, auf dem ein Ort etwas über einen anderen erfährt.
//
// Lichtquellen (LightBoard): Lampe, Kerze, Kamin je Ort mit Zustand an/aus. Der Zustand gehört zu den Ortsdaten und
// gilt auch, solange der Ort nicht zu sehen ist (unsichtbare Orte rechnen weiter). Ein Fenster am Außenort verweist
// auf den Raum dahinter (WindowLight): Nachts leuchtet es nach draußen nur, wenn dort mindestens eine Lichtquelle
// brennt. Außen- und Innenansicht lesen denselben Eintrag und zeigen so immer denselben Zustand. Warum ein Licht aus
// ist (kein Brennholz, kein Lampenöl, Bewohner schläft), entscheidet später die Simulation.

#include <array>
#include <cstdint>
#include <string_view>

#include "pixelia/engine/light.h"
#include "pixelia/engine/weather.h"

namespace pixelia {

class LightBoard {
public:
    static constexpr int kMaxPlaces = 16;
    static constexpr int kMaxSources = 16;  // je Ort

    // Ort mit n Lichtquellen anmelden (alle an); schon bekannt: nur die Zahl ändern. false = voll
    bool declare(std::string_view place, int sources);
    void set(std::string_view place, int source, bool on);
    void set_all(std::string_view place, bool on);
    bool on(std::string_view place, int source) const;
    bool any_on(std::string_view place) const;  // brennt dort mindestens ein Licht?
    int sources(std::string_view place) const;
    uint32_t version() const { return version_; }  // zählt jede Änderung (Orte bauen ihr Licht nur dann neu)

private:
    struct Entry {
        char place[24];
        uint8_t sources;
        uint16_t on;  // Bit je Lichtquelle
    };
    int find(std::string_view place) const;
    std::array<Entry, kMaxPlaces> e_{};
    int n_ = 0;
    uint32_t version_ = 0;
};

// Fenster nach draußen: Lichtkreis vor dem Fenster, gehört zum Raum dahinter (Kennung des Ortes)
struct WindowLight {
    LightCircle circle;
    const char* room;
};

struct Environment {
    Weather weather;
    LightBoard lights;
};

}  // namespace pixelia
