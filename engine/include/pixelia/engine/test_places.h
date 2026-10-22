#pragma once
// Testorte (seit 0.5.0, seit 0.5.1 auf Kacheln und Raum-Baukasten): neutrale Platzhalter im groben Raster 200×120
// zum Prüfen von Bühne, Übergängen, Figuren, Farbreihen, Tiefensortierung, Ausbaustufen und Ausgängen. Kein
// Setting-Inhalt: Platzhalter-Figuren in den Chibi-Maßen der Perspektive (Zelle 16×32, 28 Bildpunkte hoch, drei
// gezeichnete Ansichten, vier Richtungen) und einfache Möbel. Alles ist einmal mit einer grauen Platzhalter-Rampe
// gezeichnet und bekommt seine Farbe über Farbreihen. Grafiken liegen seit 0.5.5 als PNG in assets/testorte/ und kommen
// über den Grafik-Speicher (assets.h, Pakete grund, moebel, wiese, raum); die Orte selbst sind Daten (Kachelzeilen,
// Raumliste, Ausgänge). Lampe und Laterne haben Zustandsbilder aus/an nach ihrer Lichtquelle.
//
// - testort-wiese: Wiese mit Hecke, Hauswand mit Tür (→ Raum) und Weg zum rechten Bildrand (→ Platz), kühles Licht
// - testort-raum: Raum mit Tapete, Fenstern, Schrank, Bett, Tisch, Stühlen, Teppich, Lampe; Ausgang unten (→ Wiese),
//   warmes Licht. Bett und Tisch haben zwei Ausbaustufen (Befehl „ausbau“), alle Möbel einen Zustand „im Bau“
//   (Befehl „baustelle“: halbfertiges Bett bzw. Bretterstapel)
// - testort-platz: gepflasterter Platz mit Bänken und Büschen, Ausgang am linken Bildrand (→ Wiese)
// Figuren laufen auf dem Kachelraster (Breitensuche um Möbel herum), benutzen freie Benutzungspunkte und wechseln
// über Ausgänge den Ort. Eine Figur ist die Begleiterin: Ihr folgt der Blick der Bühne durch Türen und Bildränder.
// Lastregler (set_load): Zahl der Figuren am Ort, 1–64; 0 = Standard des Ortes.
// Seit 0.5.3 Wetter (weather.h): Wiese und Platz draußen mit Partikeln und Schleiern, der Raum zeigt es in den
// Fenstern. Lichtquellen in der Umgebung (environment.h): die Lampe des Raumes, die Laterne des Platzes; die Fenster
// der Wiese gehören zum Raum und leuchten nachts nur, wenn dort Licht brennt (Befehl „innenlicht an|aus|um“).

#include <array>
#include <cstddef>
#include <cstdint>

#include "pixelia/engine/environment.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/rng.h"
#include "pixelia/engine/room.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/sprite.h"
#include "pixelia/engine/weather.h"

namespace pixelia {

class TestPlace : public Scene {
public:
    enum class Kind : uint8_t { Field, Room, Square };
    static constexpr int kMaxFigures = 64;
    static constexpr int kColors = 80;  // Grundfarben je Lichtstufe (drei Stufen, light.h)

    explicit TestPlace(Kind kind) : kind_(kind) {}
    // Orte liegen im Grafik-Speicher (auf dem Gerät PSRAM): je ~2,5 KB Figuren, der interne RAM bleibt frei
    static void* operator new(size_t bytes);
    static void operator delete(void* p);
    const char* name() const override;
    const char* title() const override;
    void init(uint64_t seed) override;
    void tick(const WorldClock& clock) override;
    void render(Canvas& canvas) const override;
    void render_layers(Canvas& work, CoarseLayers& out) const override;
    void render_fine(Canvas& fine, const CoarseLayers& layers, const uint8_t* remap) const override;
    void render_overlay(Canvas& fine) const override;
    void set_environment(Environment* env) override { env_ = env; }
    const Palette& palette() const override { return palette_; }
    void set_load(int figures) override;
    int scale() const override { return kCoarseScale; }
    int colors() const override { return pal_.used(); }
    void enter() override;
    void leave() override;
    // Grafik-Pakete (seit 0.5.5): „moebel“ (gemeinsam), dazu „wiese“ bzw. „raum“; Nachbarn = Ziele der Ausgänge
    int packs(const char** out, int max) const override;
    int neighbors(const char** out, int max) const override;
    bool take_travel(Travel* out) override;
    void arrive(const Travel& t) override;
    // Touch und Karten (seit 0.5.6): Figuren mit Kennung (Ort × 1000 + laufende Nummer, bleibt beim Reisen),
    // Hauptfigur = die Begleiterin (Platzhalter für Sykora); Gedanken sind Testtexte (echte ab 0.6.x)
    Hit hit(int x, int y) const override;
    bool describe(const Hit& hit, ui::Card* card) const override;
    int figures(FigureInfo* out, int max) const override;
    bool has_figure(uint16_t id) const override;
    bool figure_mark(uint16_t id, int* x, int* y, int* h) const override;
    void set_inspect(bool on) override { inspect_ = on; }
    // „ausbau“: nächste Ausbaustufe aller Teile mit Stufen; „baustelle“: Möbel im Bau bzw. wieder fertig.
    // Seit 0.5.2 zum Vergleichen: „stimmung n“ (Grundstimmung 0–2), „tageslicht n“ (0–2), „nacht n“ (Mindest-
    // helligkeit 0–256), „licht weich|hart“ (Ränder der Lichtkreise). Seit 0.5.3: „innenlicht an|aus|um“ (Lampe
    // des Raumes, wirkt auf Raum und Fenster der Wiese), „regen striche|tropfen|fein“, „schnee gemischt|klein|gross“,
    // „dichte <%>“ (Partikel), „nebelbaenke <%>“
    bool command(const char* cmd) override;
    const PlacePalette& place_palette() const { return pal_; }
    const PlaceWeather& weather() const { return weather_; }
    bool outdoor() const { return kind_ != Kind::Room; }
    // Brennt das Licht dieses Ortes? Wiese: das Licht im Raum hinter den Fenstern
    bool lights_on() const;

    int figure_count() const { return count_; }
    struct FigureView {
        uint16_t id;
        bool guide;
    };
    bool has_guide() const;
    const Room& room() const { return room_; }
    // Prüfung: zeichnet ohne Hintergrundbild (muss pixelgleich mit render() sein)
    void render_direct(Canvas& canvas) const { draw(canvas, false); }

private:
    enum class Goal : uint8_t { None, Wander, Use, Exit };
    struct Figure {
        int32_t x, y;    // Fußpunkt in groben Bildpunkten, 8.8 Festkomma
        uint8_t cx, cy;  // Kachel, auf die die Figur gerade geht oder auf der sie steht
        uint8_t gx, gy;  // Ziel-Kachel
        uint16_t wait;   // Schritte bis zur nächsten Entscheidung
        uint8_t speed;   // 1/256 Bildpunkt pro Schritt
        uint8_t row;     // Farbreihe (1–6)
        Goal goal;
        int8_t use_part, use_k, exit;
        int8_t dz;       // Tiefenversatz (Benutzungspunkt)
        bool guide;
        bool busy;       // benutzt gerade etwas
        uint16_t id;     // Kennung (seit 0.5.6)
        uint16_t here;   // Schritte am Ort (die Begleiterin geht erst nach einer Weile weiter)
        Facing facing;
        Animator anim;
    };

    void build_palette();
    void build_room();
    void apply_lights();
    void apply_glow();
    bool random_free_tile(int* tx, int* ty);
    void spawn_at(Figure& f, int tx, int ty, Facing facing, uint8_t row, bool guide, uint16_t id = 0);
    const Figure* figure_by_id(uint16_t id) const;
    Sprite figure_frame(const Figure& f, bool* flip) const;
    void decide(Figure& f);
    bool use_taken(int part, int k, const Figure* self) const;
    void reached(int i);  // Figur i steht auf ihrem Ziel
    void remove_figure(int i);
    void apply_load();
    void draw(Canvas& c, bool with_background, CoarseLayers* layers = nullptr) const;

    Kind kind_;
    Rng rng_;
    Palette palette_{};   // zur Tageszeit ausgerechnet (tick)
    PlacePalette pal_{};  // Grundfarben, Rollen, Farbrotation
    Daylight daylight_ = daylight_preset(0);
    uint8_t shadow_[256]{};  // Schatten durchscheinend (Farbtabelle)
    int mood_ = 0;
    LightStyle light_style_ = LightStyle::Fine;
    int light_inner_ = -1;  // −1 = Vorgabe des Ortes
    Room room_;
    std::array<Figure, kMaxFigures> figures_{};
    int count_ = 0;
    int wanted_ = 0;
    bool load_pending_ = false;
    bool inspect_ = false;
    std::array<Travel, 8> out_{};
    int out_count_ = 0;
    Environment* env_ = nullptr;
    PlaceWeather weather_;
    uint32_t lights_version_ = 0;
    uint64_t seed_ = 0;
    uint32_t enters_ = 0;
    uint64_t ticks_ = 0;
    uint16_t next_id_ = 0;
};

}  // namespace pixelia
