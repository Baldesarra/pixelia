#pragma once
// Testorte (seit 0.5.0, seit 0.5.1 auf Kacheln und Raum-Baukasten): neutrale Platzhalter im groben Raster 200×120
// zum Prüfen von Bühne, Übergängen, Figuren, Farbreihen, Tiefensortierung, Ausbaustufen und Ausgängen. Kein
// Setting-Inhalt: Platzhalter-Figuren in den Chibi-Maßen der Perspektive (Zelle 16×32, 28 Bildpunkte hoch, drei
// gezeichnete Ansichten, vier Richtungen) und einfache Möbel. Alles ist einmal mit einer grauen Platzhalter-Rampe
// gezeichnet und bekommt seine Farbe über Farbreihen. Grafiken liegen seit 0.5.5 als PNG in assets/testorte/ und kommen
// über den Grafik-Speicher (assets.h, Pakete grund, moebel, wiese, raum); die Orte selbst sind Daten (Kachelzeilen,
// Raumliste, Ausgänge). Lampe und Laterne haben Zustandsbilder aus/an nach ihrer Lichtquelle.
//
// Seit 0.5.8 liest die Klasse die Daten eines Ortes (PlaceDef, place.h): Wiese, Raum, Platz und Kammer stehen je in
// einer eigenen Datei in worlds/testdorf/orte/ (Kacheln, Raumliste, Verbindungen, Lichter, Fenster), die Ortsliste in
// testdorf.cpp. Der Baukasten (Katalog, Palette, Figur) ist für alle Testorte gleich und steht hier.
// Figuren laufen auf dem Kachelraster (Breitensuche um Möbel herum), benutzen freie Benutzungspunkte und wechseln
// über Ausgänge den Ort. Eine Figur ist die Begleiterin: Ihr folgt der Blick der Bühne durch Türen und Bildränder.
// Lastregler (set_load): Zahl der Figuren am Ort, 1–64, aufgefüllt mit Statisten; 0 = keine Statisten.
// Seit 0.5.9 gehören die Figuren der Welt (figure.h): Der Ort zeigt, wer laut Weltzustand hier ist; Namen kommen aus dem
// Datensatz. Statisten (Kennungen ab kStatistIds) gibt es nur für Messungen: Sie bleiben am Ort, buchen nichts und
// stehen in keinem Speicherstand.
// Seit 0.5.9 Teil 2 Stufen der Simulation (stage.h, world_sim.cpp): Der Planer (decide, finish) entscheidet für die
// Figuren der Welt Tätigkeiten als Zeitblöcke (umhergehen, ein Teil benutzen, hinausgehen; Gehzeit aus der Weglänge)
// und bucht ihre Wirkung (Becken schöpfen, am Bau arbeiten) an ihrem Ende, sichtbar oder nicht derselbe Code. Die
// Darstellung (tick, nur betreten) rechnet daraus, wo jede Figur gerade ist: mitten auf dem Weg, am Teil, wartend an der
// Tür; die Bilder der Animation kommen aus der Weltzeit. So zeigt ein Ort dasselbe, egal seit wann er zu sehen ist.
// Seit 0.5.3 Wetter (weather.h): Wiese und Platz draußen mit Partikeln und Schleiern, der Raum zeigt es in den
// Fenstern. Seit 0.5.8 Teil 2 Ortszustände (place_state.h) statt der Lichtquellen-Tafel: Lampe, Kerze und Ofen des
// Raumes, Laterne, Nässe und Becken des Platzes, Nässe der Wiese. Die Fenster der Wiese gehören zum Raum und leuchten
// nachts nur, wenn dort Licht mit Schein nach außen brennt (Befehl „innenlicht an|aus|um“). Der Ort zeigt die Zustände
// (Zustandsbilder, Lichtkreise nach der Stärke, nasser Boden dunkler, Pfützen) und erfährt Änderungen über den
// Ereignis-Bus (events.h); seine Lichtkarte entsteht seit 0.5.9 Teil 3 über mehrere Schritte (prepare, Budget der Bühne).
// Unter Tage (kPlaceUnderground): kein Wetter, kein Himmel, Licht immer wie nachts.
// Seit 0.5.10 Teil 2 Aufgaben (tasks.h, Testinhalt in waren.cpp): Wer keine Aufgabe hat, schaut ab und zu bis zur nächsten
// Takt-Grenze, ob es etwas zu tun gibt, und übernimmt dort eine (Gemeinschaftsaufgabe zuerst, dann die nächste; weiter
// als zwei Orte erst, wenn sie zwei Minuten wartet). Holen: über die Ausgänge Richtung Quell-Lager (Orts-Graph), dort
// in die Trage (mit Reservierung), dann ins Ziel. Herstellen: am Arbeitsplatz des Erzeugerlagers, je Arbeitsgang zwei
// Stück. Platzhalter bis 0.5.11 (Hunger): Jede Figur isst zweimal am Tag zu festen Zeiten einen Laib, wenn sie dann an
// einem Ort mit Brotvorrat ist.

#include <array>
#include <cstddef>
#include <cstdint>

#include "pixelia/engine/environment.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/rng.h"
#include "pixelia/engine/room.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/sprite.h"
#include "pixelia/engine/weather.h"

namespace pixelia {

class TestPlace : public Scene {
public:
    static constexpr int kMaxFigures = 64;
    static constexpr int kColors = 80;  // Grundfarben je Lichtstufe (drei Stufen, light.h)

    // index: Nummer des Ortes in der Welt (Kennungen der Figuren: (index + 1) × 1000 + laufende Nummer)
    TestPlace(const PlaceDef& def, int index);
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
    bool sky_view(SkyView* out) const override;  // Raum: Fensterglas (seit 0.5.7, Mond)
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
    // Planer (seit 0.5.9 Teil 2): wie bisher die Figuren am Ort, jetzt als Zeitblöcke in Weltzeit
    bool decide(const FigureRecord& f, int64_t at_us, Rng& rng, Doing* out) override;
    void finish(const FigureRecord& f, const Doing& d) override;
    bool exit_travel(int exit, Travel* out) const override;
    void entry_tile(uint8_t entry, Rng& rng, uint8_t* tx, uint8_t* ty) const override;
    void step(int64_t at_us) override;
    void refresh(int64_t now_us) override;
    int prepare(int budget) override;
    static int64_t tile_us(uint16_t id);  // Gehzeit je Kachel (je Figur etwas anders, 325–455 ms)
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
    // „dichte <%>“ (Partikel), „nebelbaenke <%>“. Seit 0.5.8 Teil 2: „ofen kalt|glut|feuer|abdecken“, „nass <%>“,
    // „becken <%>“
    bool command(const char* cmd) override;
    const PlacePalette& place_palette() const { return pal_; }
    const PlaceWeather& weather() const { return weather_; }
    bool outdoor() const { return def_.outdoor(); }
    const PlaceDef& def() const { return def_; }
    // Brennt das Licht dieses Ortes? Wiese: Licht mit Schein nach außen im Raum hinter den Fenstern
    bool lights_on() const;
    // Ortszustand dieses Ortes nach Kennung (Index in PlaceStates), −1 = keiner
    int state_index(const char* id) const;

    // Figuren am Ort laut Weltzustand, dazu die Statisten (seit 0.5.9 Teil 2 auch unsichtbar)
    int figure_count() const;
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
        bool statist;    // Statist des Lastreglers (seit 0.5.9): geht nie hinaus
        bool busy;       // benutzt gerade etwas
        uint16_t id;     // Kennung (seit 0.5.6)
        uint16_t here;   // Schritte am Ort (Statisten)
        Facing facing;
        Animator anim;   // Statisten; Figuren der Welt: Bild aus der Weltzeit (frame)
        uint8_t frame;   // gezeigtes Bild der Animation
        FigureLook look;  // Aussehen (seit 0.5.9 Teil 3, Figuren-Baukasten); ohne Ebenen die alte Testfigur
        // Figuren der Welt (seit 0.5.9 Teil 2): gezeigte Tätigkeit und ihr Weg (2 Bit je Schritt)
        uint32_t seq;
        int16_t path_n;  // Schritte, −1 = kein Weg (am Ende einfach dort)
        std::array<uint8_t, 24> path;
    };
    static constexpr int kPathMax = 96;

    void build_palette();
    void build_room();
    void apply_states();  // Lichtkreise, Zustandsbilder, Glühen (seit 0.5.8 Teil 2, vorher apply_lights)
    void apply_glow();
    void apply_wet();     // nasser Boden: Weg und Pflaster dunkler
    void read_events();
    uint32_t states_version() const;  // Stufenwechsel der eigenen Zustände und der Räume hinter den Fenstern
    void sync_building();  // im Bau nach den Projekten (Planer: Wege und Benutzungspunkte hängen daran)
    int build_image(const Project& p) const;  // Bau-Bild (Zustand) nach dem Zwischenstand
    bool random_free_tile(Rng& rng, int* tx, int* ty) const;
    void spawn_at(Figure& f, int tx, int ty, Facing facing, uint8_t row, bool guide, uint16_t id);
    bool any_free_tile(int* tx, int* ty) const;  // der Reihe nach (wenn der Zufall nichts findet)
    void figure_name(const Figure& f, char* out, size_t n) const;
    const Figure* figure_by_id(uint16_t id) const;
    // Bild einer Figur (seit 0.5.9 Teil 3): aus dem Baukasten, sonst die alte Testfigur mit Farbreihe
    struct FigureImage {
        Sprite sprite;
        bool flip;
        const uint8_t* remap;
        int pivot_x, pivot_y;
    };
    FigureImage figure_image(const Figure& f, Facing facing, int frame) const;
    FigureImage figure_image(const Figure& f) const { return figure_image(f, f.facing, f.frame); }
    // Aufgaben (seit 0.5.10 Teil 2, tasks.h): eigene Aufgabe weiterführen, an einer Takt-Grenze eine übernehmen, ablegen,
    // Mahlzeit (Platzhalter bis 0.5.11), auf die nächste Takt-Grenze warten; false = gewohnter Planer
    bool plan_work(const FigureRecord& f, int64_t at, Rng& rng, Doing* out);
    bool claim_task(const FigureRecord& f, int64_t at);
    // Projekte (seit 0.5.10 Teil 3, projects.h): PLATZHALTER bis 0.5.11 für die Wahl, wer daran arbeitet; hingehen und
    // Arbeitsgänge am Teil im Bau
    bool claim_project(const FigureRecord& f, int64_t at);
    bool build_work(const FigureRecord& f, const Project& p, int64_t at, Rng& rng, Doing* out);
    // zum Lagerplatz (an diesem Ort: an seinen Benutzungspunkt; sonst über den nächsten Ausgang Richtung seines Ortes)
    bool go_store(const FigureRecord& f, int store, DoingAim aim, int pieces, int64_t at, Rng& rng, Doing* out);
    bool go_place(const FigureRecord& f, int place, DoingAim aim, int64_t at, Rng& rng, Doing* out);
    void finish_work(const FigureRecord& f, const Doing& d);  // Buchung am Ende (Holen, Bringen, Herstellen, Essen)
    int producer_here(int part) const;     // Erzeugerlager dieses Ortes mit Arbeitsplatz part (sonst das erste)
    int food_here() const;                 // Hausvorrat mit Brot an diesem Ort, −1 = keiner
    int stow_here(int good) const;         // wohin eine Ware ohne Aufgabe an diesem Ort kommt, −1 = nirgends
    int exit_place(int exit) const;        // Ziel des Ausgangs als Nummer in der Welt, −1 = keins
    void statist_decide(Figure& f);
    bool use_taken(int part, int k, const Figure* self) const;
    bool use_taken_world(int part, int k, uint16_t self, int64_t at) const;
    void reached(int i);  // Statist i steht auf seinem Ziel
    void remove_figure(int i);
    void apply_load();
    void sync_figures(int64_t now);  // Darstellung = Figuren am Ort laut Weltzustand, an ihrer Stelle im Zeitblock
    // Lagerplätze Stück für Stück (seit 0.5.10 Teil 2b): Bestand laut Warenbuch auf die Stellplätze ihrer Teile
    void sync_stock(int64_t now);
    void sync_fills();  // Füllung der Lagerplätze und Materialstapel aus dem Warenbuch (ohne Zeit, auch beim Betreten)
    bool on_pile(int tx, int ty) const;
    const Project* project_of_part(int part) const;  // laufendes Projekt dieses Teils bzw. seines Materialstapels  // Kachel unter einem nicht versperrenden Lager (Materialstapel)
    int store_of_part(int part) const;  // Lagerplatz dieses Ortes mit Teil part (−1 = keiner)
    void show(Figure& v, const FigureRecord& r, int64_t now);
    void draw(Canvas& c, bool with_background, CoarseLayers* layers = nullptr) const;

    const PlaceDef& def_;
    int index_ = 0;
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
    Environment* env_ = nullptr;
    PlaceWeather weather_;
    int state_first_ = -1, state_n_ = 0;  // Ortszustände in der Umgebung
    int wet_state_ = -1, wet_level_ = -1;
    int reader_ = -1;                     // Leser am Ereignis-Bus
    bool states_dirty_ = true;
    bool entered_ = false;
    uint64_t seed_ = 0;
    // Rauch aus dem Schornstein (seit 0.5.8): nur zu sehen, eigener Zufall (ändert die Figuren nicht)
    struct Puff {
        int16_t x, y;  // feine Bildpunkte × 16
        int8_t vx;
        uint8_t age, life;
    };
    std::array<Puff, 24> smoke_{};
    int smoke_n_ = 0;
    int smoke_state_ = -1;
    uint32_t smoke_rng_ = 1;
    void tick_smoke();
    uint32_t enters_ = 0;
    uint64_t ticks_ = 0;
    uint16_t next_statist_ = 0;
    uint32_t restored_seen_ = 0;
    uint32_t states_seen_ = 0;  // Stand der Ortszustände, den die Darstellung zeigt (PlaceStates::version)
    std::array<int8_t, 16> exit_place_{};  // Ziele der Ausgänge (Nummer in der Welt), beim init() bestimmt
};

}  // namespace pixelia
