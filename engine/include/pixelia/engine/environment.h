#pragma once
// Umgebung der Orte (seit 0.5.3): was alle Orte einer Bühne gemeinsam sehen, das Wetter und die Lichtquellen.
// Die Bühne hält sie und gibt sie jedem Ort (Scene::set_environment); bis zum gemeinsamen Weltzustand (0.5.9) ist
// sie der einzige Weg, auf dem ein Ort etwas über einen anderen erfährt.
//
// Seit 0.5.8 statt der Lichtquellen-Tafel (LightBoard) die Ortszustände (place_state.h): Licht mit Brennstoff, Feuer und
// Glut, Nässe, Füllstand, angefangene Arbeit. Sie gehören zu den Ortsdaten und laufen weiter, solange der Ort nicht zu
// sehen ist. Ein Fenster am Außenort verweist auf den Raum dahinter (WindowLight): Nachts leuchtet es nach draußen,
// wenn dort Licht mit Schein nach außen brennt, umso größer, je mehr Licht (Summe der Stärken). Außen- und Innenansicht
// lesen denselben Eintrag und zeigen so immer denselben Zustand. Dazu der Ereignis-Bus (events.h) und die Fahrzeuge
// (vehicle.h).
//
// Seit 0.5.7 auch Kalender, Himmel (Sonne, Mond) und Wetter-Simulation: Die Bühne schreibt sie einmal je Weltminute
// fort (Stage::tick), die Orte lesen nur.
//
// Seit 0.5.9 ist die Umgebung der Weltzustand als Ganzes (SETTING.md Kapitel 15, Grundsatz 1): dazu die Figuren mit
// ihren Datensätzen und ihrem Aufenthalt (figure.h), die Chronik und der Besuchsverlauf (chronicle.h). Die Bühne
// speichert und lädt alles zusammen (Stage::save_state, save.h). Seit 0.5.10 das Warenbuch (ledger.h), seit Teil 2
// die Aufgabenliste (tasks.h), seit Teil 3 die Projekte (projects.h).

#include <array>
#include <cstdint>
#include <string_view>

#include "pixelia/engine/calendar.h"
#include "pixelia/engine/chronicle.h"
#include "pixelia/engine/events.h"
#include "pixelia/engine/figure.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/place_state.h"
#include "pixelia/engine/projects.h"
#include "pixelia/engine/sky.h"
#include "pixelia/engine/tasks.h"
#include "pixelia/engine/vehicle.h"
#include "pixelia/engine/weather.h"
#include "pixelia/engine/weather_sim.h"

namespace pixelia {

// Merker der Welt (seit 0.5.8): fertige Projekte, vorhandene Werkzeuge, stehende Fahrzeuge als Kennungen, bis
// Warenbuch und Projekte (0.5.10) sie liefern. Die Bedingungen der Verbindungen lesen sie (place.h)
class WorldFlags {
public:
    static constexpr int kMax = 16;
    void set(std::string_view key, bool on);
    bool has(std::string_view key) const;
    int count() const { return n_; }
    std::string_view key(int i) const { return k_[i].data(); }
    void clear() { *this = WorldFlags{}; }

private:
    std::array<std::array<char, 24>, kMax> k_{};
    int n_ = 0;
};

// Fenster nach draußen: Lichtkreis vor dem Fenster, gehört zum Raum dahinter (Kennung des Ortes)
struct WindowLight {
    LightCircle circle;
    const char* room;
};

struct Environment {
    Weather weather;
    PlaceStates states;  // seit 0.5.8 (vorher LightBoard lights)
    WorldCalendar calendar;
    Sky sky;
    WeatherSim weather_sim;
    WorldFlags flags;
    EventBus events;     // seit 0.5.8
    Vehicles vehicles;   // seit 0.5.8
    FigureBook figures;  // seit 0.5.9: alle Figuren mit Datensatz und Aufenthalt
    Chronicle chronicle;
    VisitLog visits;
    Ledger ledger;       // seit 0.5.10: Warenbuch für die ganze Welt
    TaskList tasks;      // seit 0.5.10 Teil 2: Aufgabenliste der Gemeinschaft (Holen und Bringen, Herstellen)
    ProjectList projects;  // seit 0.5.10 Teil 3: Projekte mit Teilaufgaben (Bett in der Kammer)
    int64_t world_minute = INT64_MIN;  // Weltminute (Tag × 1440 + Minute), INT64_MIN = Weltzeit läuft noch nicht
};

}  // namespace pixelia
