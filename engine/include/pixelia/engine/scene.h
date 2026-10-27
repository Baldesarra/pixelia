#pragma once
// Schnittstelle für alles, was die Engine zeigt (Testszenen, später die Orte der Welten).
// Gerät und Simulator nutzen denselben Szenen-Code: Logik-Schritt, Zeichnen, Palette. Mehrere Szenen hält die
// Bühne (stage.h). Jede Szene hat ihren eigenen Zustand (Palette, später Kamera, Licht, Wetter).
// Regeln: tick() rechnet, render() zeichnet nur und ändert nichts (deshalb const); gleicher Startwert und gleiche
// Schritte ergeben dasselbe Bild, egal wie oft dazwischen gezeichnet wurde.

#include <memory>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/figure.h"
#include "pixelia/engine/rng.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

struct Environment;
namespace ui {
struct Card;
}

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
// und gibt den Reisenden im Zielort ab (arrive). Wer reist, legen die Orte fest; die Bühne reicht es durch.
struct Travel {
    const char* to = nullptr;  // Kennung des Zielortes
    uint8_t entry = 0;         // Ausgang des Zielortes, an dem man ankommt
    uint8_t from_exit = 0;     // benutzter Ausgang (kommt zurück, wenn es den Zielort nicht gibt)
    Transition transition = kDefaultTransition;
    bool follow = false;       // Hauptfigur: ihr folgt der Blick im Standardmodus (Kamera, camera.h)
    uint16_t who = 0;          // Kennung der Figur (seit 0.5.6 eindeutig über alle Orte, bleibt beim Reisen)
    uint8_t look = 0;          // Aussehen (Testorte: Farbreihe)
    uint16_t travel = 0;       // Reisezeit der Verbindung in Weltsekunden (seit 0.5.8; so lange ist man unterwegs)
};

// Was an einer Stelle zu sehen ist (seit 0.5.6, Touch): das vorderste Teil nach Tiefe, durchsichtige Punkte zählen
// nicht. Koordinaten fein (400 × 240).
struct Hit {
    enum class Kind : uint8_t { None, Figure, Part, Exit };
    Kind kind = Kind::None;
    uint16_t id = 0;     // Figur: Kennung; Teil: Nummer in der Raumliste; Ausgang: Nummer
    int16_t depth = 0;
    int16_t x = 0, y = 0;  // Fußpunkt
    int16_t h = 0;         // Höhe des Bildes
};

// Figuren (seit 0.5.9): Wer existiert und wo, steht im Weltzustand (figure.h, Environment::figures); ein Ort zeigt die
// Figuren, die dort sind. Seit 0.5.9 Teil 2 entscheidet sein Planer (decide, finish) ihre Tätigkeiten als Zeitblöcke
// (figure.h, Doing), die Bühne ruft ihn in Zeitreihenfolge auf und übergibt Figuren an Takt-Grenzen. Kennungen ab
// kStatistIds sind Statisten des Lastreglers (set_load, Messungen): Sie gehören nur der Darstellung des Ortes, gehen nie
// hinaus, buchen nichts und stehen in keinem Speicherstand.
inline constexpr uint16_t kStatistIds = 0xC000;
inline constexpr uint8_t kAnyEntry = 0xFF;  // Travel::entry: irgendwo am Ort auftauchen (Start, Laden)

// Figur in einer Liste (Weboberfläche: „Folge X“)
struct FigureInfo {
    uint16_t id = 0;
    bool lead = false;
    char name[24] = {};
};

// Wo ein Ort Himmel zeigt: Flächen (fein) und die Farben mit der Rolle Himmel (Grundfarben first … first+count−1,
// in jeder Lichtstufe k × base + i). Seit 0.5.8 gehört jede Fläche zu einer Gruppe (ein Fenster mit mehreren
// Scheiben; Gruppen von links nach rechts 0, 1, …), und der Ort kann eigene Mondfarben mitbringen: Dann blendet der
// Mond beim Ortswechsel mit dem Bild aus und ein, statt zu verschwinden.
struct SkyView {
    static constexpr int kMaxPanes = 16;
    Rect panes[kMaxPanes];
    uint8_t group[kMaxPanes] = {};
    int count = 0;
    int base = 80, first = 0, colors = 0;
    int moon = -1;  // erste von vier Grundfarben (hell, Meer, Schatten, Hof), Rolle fest; −1 = Farben der Oberfläche
};

class Scene {
public:
    virtual ~Scene() = default;
    virtual const char* name() const = 0;    // Kennung (Befehle, Dateinamen), ASCII
    virtual const char* title() const { return name(); }  // sichtbarer Name (Weboberfläche)
    // Gleicher Startwert = gleicher Ablauf (deterministisch)
    virtual void init(uint64_t seed) = 0;
    // Ein Logik-Schritt der Darstellung; die Uhr ist bereits weitergezählt. Seit 0.5.9 Teil 2 nur für betretene Orte
    // (sichtbar, Übergang, Vorbereitung: volle Stufe mit 30 Hz); was in der Welt passiert, rechnen decide, finish und
    // step in beiden Stufen gleich
    virtual void tick(const WorldClock& clock) = 0;

    // --- Stufen der Simulation (seit 0.5.9 Teil 2, SETTING.md Kapitel 15) ---
    // Planer: nächste Tätigkeit von Figur f ab at_us an diesem Ort (sie steht auf f.doing.gx/gy); Zufall nur aus rng
    // (je Figur und Entscheidung). Leave muss an einer Takt-Grenze enden. false = der Ort plant nicht (Figur steht)
    virtual bool decide(const FigureRecord& /*f*/, int64_t /*at_us*/, Rng& /*rng*/, Doing* /*out*/) { return false; }
    // Tätigkeit zu Ende (um d.end_us): Wirkung buchen (Becken schöpfen, am Bau arbeiten); Leave übergibt die Bühne
    virtual void finish(const FigureRecord& /*f*/, const Doing& /*d*/) {}
    // Weg durch Ausgang exit: Ziel, Eingang dort, Übergang, Reisezeit; false = gibt es nicht
    virtual bool exit_travel(int /*exit*/, Travel* /*out*/) const { return false; }
    // Wo jemand ankommt, der durch Eingang entry kommt (kAnyEntry: irgendwo frei; Start, Laden)
    virtual void entry_tile(uint8_t /*entry*/, Rng& /*rng*/, uint8_t* tx, uint8_t* ty) const { *tx = *ty = 0; }
    // An jeder Takt-Grenze, für alle Orte gleich (nach dem Planer): Regeln aus dem Ereignis-Bus, Raum nach den
    // Ortszuständen (im Bau, versperrte Kacheln)
    virtual void step(int64_t /*at_us*/) {}
    // Darstellung sofort an den Weltzustand angleichen (nach dem Laden eines Speicherstands; nur betretene Orte)
    virtual void refresh(int64_t /*now_us*/) {}
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
    // Wo der Ort Himmel zeigt (seit 0.5.7, sky.h): Flächen und Farben mit der Rolle Himmel; dort zeichnet die Bühne
    // den Mond. false = kein Himmel
    virtual bool sky_view(SkyView* /*out*/) const { return false; }
    // Umgebung (seit 0.5.3, environment.h: Wetter, Lichtquellen aller Orte); setzt die Bühne vor init()
    virtual void set_environment(Environment* /*env*/) {}
    // Wird sichtbar bzw. ist nicht mehr zu sehen (seit 0.5.1; z. B. Hintergrundbild vorzeichnen und freigeben).
    // Während eines Übergangs sind beide Orte „betreten“.
    virtual void enter() {}
    virtual void leave() {}
    // (bis 0.5.9 Teil 1 take_travel, arrive, clear_figures: seit Teil 2 übergibt die Bühne über den Weltzustand)
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

    // Touch und Karten (seit 0.5.6). hit: was an (x, y) fein zu sehen ist; nur sichtbare Orte mit geladenen Grafiken
    virtual Hit hit(int /*x*/, int /*y*/) const { return {}; }
    // Karte zu einem Treffer füllen (Figur: Name, Tätigkeit, Gedanke; Teil: Name, bei Türen card->place)
    virtual bool describe(const Hit& /*hit*/, ui::Card* /*card*/) const { return false; }
    // Figuren am Ort (Kennung, Name; laut Weltzustand, dazu Statisten); Hauptfigur (der der Blick im Standardmodus
    // folgt) immer zuerst
    virtual int figures(FigureInfo* /*out*/, int /*max*/) const { return 0; }
    // Wird die Figur gerade gezeigt? (seit 0.5.9 Teil 2 nur betretene Orte; Prüfung check_figures)
    virtual bool has_figure(uint16_t /*id*/) const { return false; }
    // Markierung einer Figur (Karte offen): Fußpunkt und Höhe fein; false = nicht hier
    virtual bool figure_mark(uint16_t /*id*/, int* /*x*/, int* /*y*/, int* /*h*/) const { return false; }
};

// Szene nach Namen anlegen; nullptr, wenn es sie nicht gibt
std::unique_ptr<Scene> make_scene(const char* name);
// Namen aller Szenen, durch Komma getrennt (für Hilfetexte)
const char* scene_names();

}  // namespace pixelia
