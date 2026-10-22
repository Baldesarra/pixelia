#pragma once
// Schnittstelle für alles, was die Engine zeigt (Testszenen, später die Orte der Welten).
// Gerät und Simulator nutzen denselben Szenen-Code: Logik-Schritt, Zeichnen, Palette. Mehrere Szenen hält die
// Bühne (stage.h). Jede Szene hat ihren eigenen Zustand (Palette, später Kamera, Licht, Wetter).
// Regeln: tick() rechnet, render() zeichnet nur und ändert nichts (deshalb const); gleicher Startwert und gleiche
// Schritte ergeben dasselbe Bild, egal wie oft dazwischen gezeichnet wurde.

#include <memory>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

struct Environment;

// Übergang beim Ortswechsel (Bühne, stage.h); hier, weil auch Ausgänge eines Ortes ihn tragen
enum class Transition : uint8_t {
    Cut,       // Schnitt: sofort
    Dissolve,  // Überblendung: grobe Bildpunkte wechseln in zufälliger Reihenfolge
    Pattern,   // Raster: wie Überblendung, in einem festen 4×4-Muster
    Fade,      // Abblenden über Schwarz
};
inline constexpr int kTransitionKinds = 4;
// Standard beim Ortswechsel: Abblenden (Entscheidung des Users 2026-10-21)
inline constexpr Transition kDefaultTransition = Transition::Fade;

// Weg durch einen Ausgang in einen anderen Ort (seit 0.5.1). Die Bühne fragt jeden Ort nach jedem Schritt danach
// und gibt den Reisenden im Zielort ab (arrive). Wer reist, legen die Orte fest (who); die Bühne reicht es durch.
struct Travel {
    const char* to = nullptr;  // Kennung des Zielortes
    uint8_t entry = 0;         // Ausgang des Zielortes, an dem man ankommt
    uint8_t from_exit = 0;     // benutzter Ausgang (kommt zurück, wenn es den Zielort nicht gibt)
    Transition transition = kDefaultTransition;
    bool follow = false;       // Blick folgt: ist der Ort gerade zu sehen, wechselt die Bühne mit
    uint16_t who = 0;
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual const char* name() const = 0;    // Kennung (Befehle, Dateinamen), ASCII
    virtual const char* title() const { return name(); }  // sichtbarer Name (Weboberfläche)
    // Gleicher Startwert = gleicher Ablauf (deterministisch)
    virtual void init(uint64_t seed) = 0;
    // Ein Logik-Schritt; die Uhr ist bereits weitergezählt
    virtual void tick(const WorldClock& clock) = 0;
    // Zeichnet in 400×240 (scale() == 1) oder ins grobe Raster 200×120 (scale() == 2)
    virtual void render(Canvas& canvas) const = 0;
    // Grobe Szenen mit Hintergrundbild (seit 0.5.2): zeichnen nur die bewegten Teile in den Arbeitspuffer und melden
    // den Hintergrund als untere Schicht (canvas.h, CoarseLayers). Ohne eigene Fassung: alles in den Puffer.
    virtual void render_layers(Canvas& work, CoarseLayers& out) const {
        out.base = nullptr;
        render(work);
    }
    virtual const Palette& palette() const = 0;
    // Lastregler für Messungen (z. B. Zahl der Sprites); Szenen ohne Regler ignorieren das
    virtual void set_load(int /*load*/) {}
    virtual int scale() const { return 1; }
    // Benutzte Paletteneinträge 0 … colors() − 1 (für die gemeinsame Palette während einer Überblendung)
    virtual int colors() const { return kSceneColors; }
    // Feine bewegte Teile (seit 0.5.3: Wetter-Partikel) nach dem Verdoppeln, auch während eines Übergangs. layers =
    // Schichten aus render_layers (Licht); remap = gemeinsame Palette einer Überblendung (nullptr = keine)
    virtual void render_fine(Canvas& /*fine*/, const CoarseLayers& /*layers*/, const uint8_t* /*remap*/) const {}
    // Feine Einblendungen der Szene (Text, Markierungen) in 400×240, nach dem Verdoppeln gezeichnet
    virtual void render_overlay(Canvas& /*fine*/) const {}
    // Umgebung (seit 0.5.3, environment.h: Wetter, Lichtquellen aller Orte); setzt die Bühne vor init()
    virtual void set_environment(Environment* /*env*/) {}
    // Wird sichtbar bzw. ist nicht mehr zu sehen (seit 0.5.1; z. B. Hintergrundbild vorzeichnen und freigeben).
    // Während eines Übergangs sind beide Orte „betreten“.
    virtual void enter() {}
    virtual void leave() {}
    // Reisende abgeben: true, solange noch jemand durch einen Ausgang geht (die Bühne fragt nach jedem Schritt)
    virtual bool take_travel(Travel* /*out*/) { return false; }
    // Jemand kommt an: t.entry ist der Ausgang dieses Ortes, an dem er auftaucht
    virtual void arrive(const Travel& /*t*/) {}
    // Prüfansicht: Stellflächen, Benutzungspunkte, Ausgänge einblenden (Seite Experimentell)
    virtual void set_inspect(bool /*on*/) {}
    // Kurzer Befehl für Prüfungen (Konsole, Seite Experimentell), z. B. „ausbau“, „baustelle“; false = unbekannt
    virtual bool command(const char* /*cmd*/) { return false; }
    // Grafik-Pakete (seit 0.5.5, assets.h): Namen der Pakete, die der Ort zum Zeichnen braucht (dauerhaft gültige
    // Texte). Die Bühne heftet sie an und betritt den Ort erst, wenn sie geladen sind; Grafiken also in enter()
    // auflösen. Rechnen (tick) darf keine Grafiken brauchen: unsichtbare Orte rechnen ohne geladene Pakete.
    virtual int packs(const char** /*out*/, int /*max*/) const { return 0; }
    // Nachbarorte (Kennungen, z. B. Ziele der Ausgänge): deren Pakete lädt die Bühne vorab
    virtual int neighbors(const char** /*out*/, int /*max*/) const { return 0; }
};

// Szene nach Namen anlegen; nullptr, wenn es sie nicht gibt
std::unique_ptr<Scene> make_scene(const char* name);
// Namen aller Szenen, durch Komma getrennt (für Hilfetexte)
const char* scene_names();

}  // namespace pixelia
