#pragma once
// Orte und Gebiete als Module (seit 0.5.8, SETTING.md Kapitel 15).
//
// - Ein Ort ist ein Bild und ein Modul-Baustein: Daten (PlaceDef) mit Kacheln, Raumliste, Verbindungen, Lichtern und
//   Fenstern. Ein Gebiet (AreaDef) ist eine Gruppe von Orten mit einer Fabrik, die aus den Daten eine Szene macht.
// - Neuer Ort: eigene Datei mit seinen Daten in worlds/<gebiet>/orte/ und eine Zeile in der Ortsliste des Gebiets.
//   Neues Gebiet: neuer Ordner in worlds/ und eine Zeile in worlds/gebiete.cpp. Jede .cpp in worlds/ wird mitgebaut.
// - Verbindungen sind Daten: die Ausgänge der Raumliste (room.h, Exit) mit Art, Reisezeit und Bedingungen. Daraus
//   entsteht der Orts-Graph (PlaceGraph): Er prüft, dass jedes Ziel existiert, jede Verbindung einen Rückweg hat und
//   von jedem Ort jeder andere erreichbar ist (Bühnen-Test „--test orte“).
// - Ein Ort greift nie direkt auf einen anderen zu; was Orte voneinander wissen, läuft über die Umgebung
//   (environment.h, bis 0.5.9 der Weltzustand kommt).

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

#include "pixelia/engine/environment.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/room.h"
#include "pixelia/engine/scene.h"

namespace pixelia {

enum PlaceFlag : uint8_t {
    kPlaceOutdoor = 1,      // draußen: Wetter mit Partikeln und Schleiern
    kPlaceUnderground = 2,  // unter Tage: kein Wetter, kein Himmel (Grube)
    kPlaceVehicle = 4,      // Fahrzeug: beweglicher Ort, seine Verbindungen wechseln mit dem Halt (vehicle.h folgt)
    kPlaceMeadow = 8,       // Gras zufällig streuen (Büschel, Blüten aus dem Startwert)
};

// Fensterglas des Ortes: zeigt den Himmel (innen) bzw. das Licht des Raums dahinter (außen)
enum class GlassRole : uint8_t { None, Sky, Glow };

// Lichtquelle eines Ortes: Lichtkreis und das Teil der Raumliste, das sie zeigt (Zustandsbild aus/an, −1 = keins).
// Quelle i des Ortes = Eintrag i in der Umgebung (LightBoard)
struct PlaceLight {
    LightCircle circle;
    int8_t part;
};

struct PlaceDef {
    const char* id;     // Kennung (Befehle, Speicherstand, Verbindungen), ASCII
    const char* title;  // sichtbarer Name
    uint8_t flags;      // PlaceFlag
    const RoomDef* room;
    const char* pack;   // eigenes Grafik-Paket zusätzlich zu den gemeinsamen, nullptr = keins
    uint8_t figures;    // Figuren beim Start
    bool guide;         // die Begleiterin (Hauptfigur) beginnt hier
    // Farbe und Licht
    uint16_t tint[3];   // Licht des Ortes (r, g, b; 256 = unverändert)
    uint16_t exposure;  // Umgebungslicht (Innenräume < 256)
    Color shadow;       // Schatten der Figuren; schwarz = wie in der Zeichenpalette
    GlassRole glass;
    const PlaceLight* lights;
    uint8_t light_count;
    const WindowLight* windows;  // Fenster nach draußen, gehören zum Raum dahinter
    uint8_t window_count;
    bool outdoor() const { return flags & kPlaceOutdoor; }
    bool underground() const { return flags & kPlaceUnderground; }
};

struct AreaDef {
    const char* id;
    const char* title;
    const PlaceDef* const* places;
    uint8_t count;
    std::unique_ptr<Scene> (*make)(const PlaceDef& def, int index);  // index = Nummer des Ortes in der Welt
};

// Die Welt (worlds/gebiete.cpp): alle Gebiete in fester Reihenfolge
int world_areas(const AreaDef* const** out);
const PlaceDef* find_place(std::string_view id);
int place_index(std::string_view id);  // Nummer über alle Gebiete, −1 = gibt es nicht
std::unique_ptr<Scene> make_place(std::string_view id);

// --- Verbindungen ---

// Bedingungen einer Verbindung (Exit::when): wann Figuren sie benutzen. Der Betrachter kann immer hin (Karte)
enum ExitWhen : uint8_t {
    kOpenDayOnly = 1,    // nur bei Tag (Dunkelheit nach den Sonnenzeiten)
    kOpenAfterBuild = 2,  // erst nach Bau (Exit::needs = Kennung des Projekts)
    kOpenWithTool = 4,    // Werkzeug nötig (Exit::needs = Kennung des Werkzeugs)
    kOpenTimetable = 8,   // Fahrzeug nach Fahrplan (steht der Zug da?)
};
const char* exit_kind_title(ExitKind k);  // Tür, Weg, Feldweg, Treppe, Bahnhof, Stolleneingang
// Ist die Verbindung für Figuren offen? why = Grund in einem Satz, wenn nicht (Karte, Info)
bool exit_open(const Exit& e, const Environment& env, const char** why = nullptr);

// Orts-Graph aus den Daten aller Orte
class PlaceGraph {
public:
    static constexpr int kMaxPlaces = 32;
    static constexpr int kMaxEdges = 128;
    struct Edge {
        uint8_t from, to;      // Orte
        uint8_t exit, entry;   // Ausgang im Ausgangsort, Ankunft im Zielort
        ExitKind kind;
        uint16_t travel;       // Reisezeit in Weltsekunden
        uint8_t when;
    };
    void build();  // aus world_areas()
    int places() const { return n_; }
    const PlaceDef& place(int i) const { return *defs_[i]; }
    int edges() const { return e_; }
    const Edge& edge(int i) const { return edges_[i]; }
    // Fehler in einem Satz (leer = alles gut): fehlendes Ziel, Ankunft ohne Ausgang, Verbindung ohne Rückweg, nicht
    // alle Orte erreichbar (in beide Richtungen)
    const char* check(char* buf, int n) const;
    // kürzester Weg (Zahl der Verbindungen) von a nach b, −1 = keiner
    int hops(int a, int b) const;

private:
    std::array<const PlaceDef*, kMaxPlaces> defs_{};
    std::array<Edge, kMaxEdges> edges_{};
    int n_ = 0, e_ = 0;
};

}  // namespace pixelia
