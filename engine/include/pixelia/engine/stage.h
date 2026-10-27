#pragma once
// Bühne (seit 0.5.0): hält mehrere Szenen (Orte) gleichzeitig, eine davon ist sichtbar.
//
// - Jede Szene behält ihren eigenen Zustand (Palette, später Kamera, Licht, Wetter); die Bühne hat keinen
//   gemeinsamen Bildzustand außer dem laufenden Übergang.
// - Rechnen und Zeichnen sind getrennt: tick() rechnet alle Szenen, sichtbar oder nicht; render() zeichnet nur, was
//   zu sehen ist. Wie oft gezeichnet wird, ändert den Ablauf nicht (Determinismus-Test im Simulator, --test stage).
// - Ortswechsel per Sprung mit Übergang: go() wirkt im nächsten tick(), der Übergang zählt in Logik-Schritten.
// - Grobe Szenen (scale() == 2) zeichnen in 200×120 in einen Arbeitspuffer (auf dem Gerät PSRAM, siehe gfx.cpp);
//   die Bühne verdoppelt ins 400×240-Bild. Danach zeichnet die Szene ihre feinen Einblendungen, danach der Aufrufer
//   die System-Oberfläche.
// - Überblendung zweier Szenen mit verschiedenen Paletten: Während des Übergangs teilen sich beide eine gemeinsame
//   Palette (gleiche Farben zusammengelegt, Farbwechsel der Szenen laufen weiter). Brauchen beide zusammen mehr als
//   240 Farben oder sind beide fein, wird stattdessen über Schwarz geblendet.
// - Betreten (seit 0.5.1): Die Bühne meldet jedem Ort, ob er zu sehen ist (enter/leave); während eines Übergangs
//   sind beide Orte betreten. Räume zeichnen dabei ihr Hintergrundbild vor bzw. geben es frei.
// - Ausgänge (seit 0.5.1): Wer hinausgeht, kommt im Zielort an (seit 0.5.9 Teil 2 an der Takt-Grenze, über den
//   Weltzustand). Folgt der Blick (follow) und ist der Ort gerade zu sehen, wechselt die Bühne mit dem
//   Übergang des Ausgangs (Standard Abblenden). Gibt es den Zielort nicht, kommt der Reisende am selben Ausgang zurück.
// - Grafik-Pakete (seit 0.5.5, assets.h): Die Bühne heftet die Pakete des sichtbaren Ortes, des Ortes, von dem aus
//   gewechselt wird, und des angeforderten Ziels an und lädt die Pakete der Nachbarorte vorab. Ein Ort wird erst
//   betreten, wenn seine Pakete geladen sind. Abblenden beginnt sofort und wartet in der dunklen Mitte auf das Ziel;
//   Überblendung, Raster und Schnitt beginnen erst, wenn das Ziel geladen ist (bis dahin bleibt der alte Ort). Das
//   Rechnen aller Orte läuft davon unberührt weiter.
// - Seit 0.5.8 Teil 2: Ereignis-Bus (events.h) mit Zustellung zu Beginn jedes Schritts; die Bühne meldet Ankunft und
//   Aufbruch der Figuren, der Kalender seine Ereignisse. Ortszustände (place_state.h) schreibt die Bühne je Weltminute
//   fort (nach einem Sprung der Uhr Minute für Minute). Rechenspitzen verteilt: Überblendung und Raster betreten das
//   Ziel einen Schritt vor dem Mischen der Paletten (Vorbereitung), gemischt wird ein Durchgang je Schritt, bis einer
//   passt (so lange bleibt der alte Ort zu sehen), der Wetter-Vorlauf läuft über mehrere Schritte.
// - Seit 0.5.9 Weltzustand (environment.h): Die Figuren gehören der Welt (figure.h). Die Bühne legt die Startbesetzung
//   an, setzt jede Figur in ihren Ort und ist die Einzige, die eine Figur weitergibt: Aufbruch, Verbindung und Ankunft
//   ändern ihr Feld where in demselben Schritt, in dem der Ort sie abgibt bzw. aufnimmt (check_figures prüft das).
//   Chronik und Besuche trägt sie aus dem Ereignis-Bus ein. Speicherstand als Ganzes: save_state/load_state
//   (world_save.cpp, Format save.h); nach dem Laden holt sie die Ortszustände nach (bis 3 Tage, verteilt auf mehrere
//   Schritte, Wetter der verpassten Zeit aus der Wetter-Simulation).
// - Seit 0.5.9 Teil 2 Stufen der Simulation (world_sim.cpp, SETTING.md Kapitel 15, Entscheidung des Users 2026-10-27:
//   beide auf Kern 1): Tätigkeiten sind Zeitblöcke in Weltzeit (figure.h, Doing), die der Planer jedes Ortes
//   entscheidet (Scene::decide, finish). Grob: An jeder Takt-Grenze (ganze Weltsekunde) arbeitet die Bühne für alle Orte
//   die Blockenden bis dahin in Zeitreihenfolge ab (gleichzeitig: nach Kennung), dann die Übergaben (Hinausgehen endet
//   immer an einer Takt-Grenze, Ankünfte von Verbindungen ebenso) und die Regeln der Orte (Scene::step). Voll: Die
//   betretenen Orte arbeiten dazwischen in jedem Schritt die Blockenden bis jetzt ab und zeigen, wo jede Figur in ihrem
//   Block gerade ist (Scene::tick, nur betretene Orte). Weil sich Orte nur an Takt-Grenzen berühren und jede
//   Entscheidung zur Zeit ihres Blockendes fällt, bucht ein Ort sichtbar wie unsichtbar dasselbe (P10, --test speicher).
//   Springt die Uhr (zurück oder mehr als kActivityCatchUp vor), setzt die Bühne alle Figuren neu an (gleicher Ort, neue
//   Tätigkeit); bis dahin holt sie Tätigkeiten genau nach, höchstens kMaxGridPerTick Takt-Grenzen je Schritt.

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

#include "pixelia/engine/assets.h"
#include "pixelia/engine/canvas.h"
#include "pixelia/engine/environment.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

// Kennungen für Befehle und Weboberfläche: schnitt, ueberblendung, raster, abblenden
const char* transition_name(Transition t);
bool parse_transition(std::string_view name, Transition* out);

// 200×120 → 400×240 (jeder grobe Bildpunkt wird 2 × 2); mit layers direkt aus Hintergrund und bewegten Teilen
void upscale2x(const uint8_t* coarse, Canvas& fine, const CoarseLayers* layers = nullptr);
// Schichten zu einem Bild zusammenlegen: Hintergrund außerhalb der Bereiche in coarse kopieren (Prüfungen)
void flatten_layers(uint8_t* coarse, const CoarseLayers& layers);

class Stage {
public:
    static constexpr int kMaxScenes = 8;
    static constexpr int kDefaultTransitionTicks = 18;  // 0,6 s

    Stage();
    ~Stage();  // löst die Pakete der Orte
    Stage(const Stage&) = delete;
    Stage& operator=(const Stage&) = delete;

    int add(std::unique_ptr<Scene> scene);  // Index, −1 = voll
    // Grafik-Speicher (Standard: asset_store()); nullptr = Orte brauchen keine Pakete
    void set_assets(AssetStore* store);
    // Sind die Pakete von Szene i geladen (bzw. fehlgeschlagen)?
    bool scene_ready(int i) const;
    bool entered(int i) const { return i >= 0 && i < count_ && (entered_ & (1u << i)); }
    // Abblenden wartet in der dunklen Mitte auf das Ziel
    bool holding() const { return hold_; }
    uint32_t holds() const { return holds_; }  // Schritte, die ein Abblenden bisher gewartet hat (Messung)
    // Alle Szenen; Szene i bekommt einen eigenen, aus seed abgeleiteten Startwert (Szene 0 genau seed)
    void init(uint64_t seed);
    int count() const { return count_; }
    Scene& scene(int i) { return *scenes_[i]; }
    const Scene& scene(int i) const { return *scenes_[i]; }
    int find(std::string_view name) const;  // −1 = gibt es nicht

    // Sichtbare Szene; während eines Übergangs das Ziel
    int active() const { return active_; }
    // Sofort und ohne Übergang (Start, Tests); bricht einen laufenden Übergang ab
    void set_active(int index);
    // Wechsel anfordern; wirkt im nächsten tick() und gehört damit zum deterministischen Ablauf
    void go(int index, Transition t = kDefaultTransition);
    void set_transition_ticks(int ticks);
    int transition_ticks() const { return ticks_; }
    bool transitioning() const { return from_ >= 0; }
    int transition_from() const { return from_; }
    Transition transition() const { return trans_; }  // tatsächlich benutzt (Abblenden, wenn die Paletten nicht passen)
    int transition_step() const { return step_; }
    uint32_t transitions() const { return transitions_; }  // Wechsel seit init()

    // Blick folgt Reisenden mit follow (Standard an); aus: Orte tauschen Figuren, das Bild bleibt
    void set_follow(bool on) { follow_ = on; }
    bool follow() const { return follow_; }
    // Wem der Blick folgt (seit 0.5.6, Kamera): 0 = der Hauptfigur (Travel::follow), sonst der Figur mit dieser
    // Kennung; kNobody = niemandem (Modus „Frei“, fester Ort)
    static constexpr uint16_t kNobody = 0xFFFF;
    void set_follow_who(uint16_t id) { follow_who_ = id; }
    uint16_t follow_who() const { return follow_who_; }
    // Figuren (seit 0.5.6): Ort einer Figur (−1 = keiner, z. B. gerade unterwegs), Hauptfigur (0 = keine)
    int find_figure(uint16_t id) const;
    uint16_t lead() const;
    // Alle Figuren aller Orte; place[i] = Ort der Figur i (optional)
    int figures(FigureInfo* out, int* place, int max) const;

    // Touch (seit 0.5.6): Treffer am sichtbaren Ort (fein), nicht während eines Übergangs
    Hit hit(int x, int y) const;
    // Karte zum Treffer; bei Türen ergänzt die Bühne, wer im Ort dahinter ist
    bool describe(const Hit& hit, ui::Card* card) const;
    uint32_t travels() const { return travels_; }  // Reisen zwischen Orten seit init() (gezählt bei der Ankunft)
    // Stufen der Simulation (seit 0.5.9 Teil 2): Takt-Grenze in Weltzeit (ganze Sekunden)
    static constexpr int64_t kGridUs = 1000000;
    static constexpr int kMaxGridPerTick = 150;                    // Takt-Grenzen je Schritt (Zeitraffer 3600: 120)
    static constexpr int64_t kActivityCatchUp = 10 * 60 * kGridUs;  // Tätigkeiten genau nachholen bis 10 Weltminuten
    int64_t grid_us() const { return grid_; }  // zuletzt abgearbeitete Takt-Grenze, INT64_MIN = noch keine
    uint32_t decisions() const { return decisions_; }  // Entscheidungen der Planer seit init() (Messung)
    int last_grid_steps() const { return last_grid_steps_; }  // Takt-Grenzen im letzten Schritt
    // Vorarbeit der Darstellung (seit 0.5.9 Teil 3, Scene::prepare): Budget je Schritt in Einheiten von light_row_cost,
    // betretene Orte zuerst, dann die anderen der Reihe nach (einer nach dem anderen fertig). Am Gerät 0,26 µs je
    // Einheit („bench licht“): 6000 ≈ 1,5 ms, die Lichtkarte des Testraums (23 000) steht nach 4 Schritten
    static constexpr int kPrepareBudget = 6000;
    int last_prepare() const { return last_prepare_; }  // Aufwand im letzten Schritt
    uint32_t reseats() const { return reseats_; }  // Neuansätze nach Sprüngen der Uhr
    // Unterwegs auf einer Verbindung mit Reisezeit (seit 0.5.8): Figuren, die keinem Ort gehören, bis sie ankommen
    static constexpr int kMaxTransit = 16;
    int in_transit() const { return transit_n_; }
    const Travel& transit(int i) const { return transit_[i].t; }
    // Prüfansicht aller Orte (Stellflächen, Benutzungspunkte, Ausgänge)
    void set_inspect(bool on);
    bool inspect() const { return inspect_; }

    // Umgebung aller Orte (seit 0.5.3): Wetter und Lichtquellen; das Wetter rechnet tick() vor den Orten
    Environment& environment() { return env_; }
    const Environment& environment() const { return env_; }
    Weather& weather() { return env_.weather; }
    const Weather& weather() const { return env_.weather; }
    // Prüfbefehl: „wetter <art> [0–100] [sofort]“, „wind links|rechts“, „blitz“, „blitze <min s> [max s]“ wirken auf die Bühne,
    // alles andere geht an den sichtbaren Ort (Scene::command). false = unbekannt
    // Seit 0.5.7: „wetter <art>“ übersteuert die Wetter-Simulation bis zur nächsten Wetterzeit, „wetter auto“ gibt sie
    // frei; „mond a|b|c“ (Aussehen), „mond <Phase 0–100> [Höhe 0–100]“ bzw. „mond echt“ (Vergleichsbilder)
    bool command(const char* cmd);

    // Weltzeit (seit 0.5.7): Kalender, Himmel und Wetter-Simulation laufen erst, wenn die Uhr stimmt (Gerät: nach
    // NTP oder mit der Testuhr; Simulator: sofort). Ohne gestarteten Kalender beginnt Tag 1 beim ersten Schritt.
    void set_world_time(bool valid) { world_valid_ = valid; }
    bool world_time() const { return world_valid_; }
    // Wetter-Simulation an (Standard) oder Übersteuerung durch die Prüfbefehle aktiv (bis Wetterzeit override_key)
    bool weather_overridden() const { return override_key_ != kNoOverride; }
    void release_weather();  // Simulation wieder an
    // Kurzmeldungen der Welt (Jahreszeitenabend, Wechsel, Erstlinge) abholen; seit 0.5.8 ein Leser am Ereignis-Bus
    bool poll_event(WorldEvent* out) { return env_.events.poll(toast_reader_, out); }
    EventBus& events() { return env_.events; }

    void tick(const WorldClock& clock);
    void set_load(int load);  // Lastregler der sichtbaren Szene: so viele Figuren mit Statisten, 0 = keine Statisten

    // --- Weltzustand und Speicherstand (seit 0.5.9) ---
    struct LoadReport {
        bool ok = false;
        const char* why = "";      // Grund, wenn nicht geladen
        uint32_t generation = 0;
        int64_t saved_us = 0;      // Weltzeit beim Speichern
        int figures = 0, transit = 0, states = 0, chronicle = 0;
        int unknown_places = 0;    // Bezüge auf Orte, die es nicht mehr gibt (Figuren kommen in den ersten Ort)
        int skipped_chunks = 0;    // unbekannte Abschnitte (neuere Firmware)
        int doings = 0;            // laufende Tätigkeiten übernommen (seit 0.5.9 Teil 2; 0 = alle neu angesetzt)
    };
    // Weltzustand in buf schreiben, an einer Takt-Grenze (zwischen zwei tick()); Länge, 0 = Puffer zu klein
    size_t save_state(uint8_t* buf, size_t cap, uint32_t generation) const;
    // Speicherstand laden (nach init(), an einer Takt-Grenze); false = ungültig, dann bleibt die Welt, wie sie war
    bool load_state(const uint8_t* data, size_t n, LoadReport* report = nullptr);
    // Prüfung: jede Figur genau einmal (an ihrem Ort bzw. auf ihrer Verbindung), keine fremde; nullptr = alles gut
    const char* check_figures(char* buf, int n) const;
    // Nachholen nach dem Laden: noch offene Weltminuten der Ortszustände, bisher nachgeholt, übersprungen (länger aus
    // als kCatchUpMinutes)
    static constexpr int kCatchUpMinutes = 3 * 1440;  // Wunsch des Users 2026-10-27: bis 3 Tage
    static constexpr int kCatchUpPerTick = 240;       // Weltminuten je Schritt (Rechenzeit verteilen)
    int64_t catching_up() const;
    int64_t caught_up() const { return caught_up_; }
    int64_t skipped() const { return skipped_; }
    int64_t world_us() const { return world_us_; }  // Weltzeit des letzten Schritts
    uint64_t seed() const { return seed_; }
    int place_of(int scene) const { return scene >= 0 && scene < count_ ? place_of_[scene] : -1; }  // Nummer in der Welt
    int scene_of(int place) const;  // −1 = nicht auf der Bühne

    // Arbeitspuffer für grobe Szenen, je kCoarsePixels Byte: main für die sichtbare Szene, aux für die zweite
    // Szene während einer Überblendung. Ohne Puffer bleiben grobe Szenen schwarz.
    void set_buffers(uint8_t* main, uint8_t* aux);
    void use_own_buffers();  // Simulator und Tests
    void render(Canvas& fine) const;
    // Palette zum Bild von render(); 240–255 setzt der Aufrufer (System-Oberfläche)
    const Palette& palette() const;

private:
    void start_transition(int to, Transition t);
    void sync_entered();  // enter/leave nach active_ und from_ (nur Orte mit geladenen Paketen)
    void sync_pins();     // Pakete von active_, from_ und pending_ angeheftet, alle anderen gelöst
    void pin_scene(int i, bool on);
    void prefetch_neighbors();
    void prepare_scenes();  // Vorarbeit mit kPrepareBudget
    // Stufen der Simulation (world_sim.cpp)
    void advance(int64_t now);                     // Takt-Grenzen bis now, dann die betretenen Orte bis now
    void reseat(int64_t now);                      // nach einem Sprung der Uhr: alle neu ansetzen
    void plan(int scene, int64_t until);           // Blockenden des Ortes bis until (einschließlich)
    void run_doing(int scene, FigureRecord& f);    // ein Blockende: Wirkung, Übergabe oder nächste Tätigkeit
    void depart(int scene, FigureRecord& f);       // Hinausgehen an der Takt-Grenze
    void deliver_transit(int64_t at);              // Ankünfte von Verbindungen bis at
    void start_doing(FigureRecord& f, int scene, uint8_t entry, int64_t at);  // angekommen: Kachel, gleich entscheiden
    uint64_t doing_seed(uint16_t who, uint32_t seq) const;
    void arrive_at(int from, int to, const Travel& t, int64_t at);
    // Ein Durchgang mit Toleranz; last: ist alles belegt, die nächste Farbe nehmen (gelingt immer)
    bool merge_palettes(int from, int to, int tolerance, bool last);
    void update_palette();
    int visible() const;
    // transition: während eines Wechsels (Mond nur mit eigenen Mondfarben des Ortes, sie blenden mit dem Bild)
    void draw_scene(const Scene& s, Canvas& fine, bool transition) const;
    void draw_moon(const Scene& s, Canvas& fine, bool is_new) const;  // während einer Überblendung
    bool shows_new(int cx, int cy) const;  // Überblendung: zeigt der grobe Bildpunkt schon das Ziel?
    static bool owns_pixel(const void* ctx, int x, int y);
    void update_world(const WorldClock& clock);  // Kalender, Himmel, Wetter-Simulation (einmal je Weltminute)
    void step_states(int64_t world_minute);      // Ortszustände bis zu dieser Weltminute
    void publish_travel(EventKind kind, int from, int to, const Travel& t);
    void compose(Canvas& fine, const uint8_t* old_coarse, const uint8_t* new_coarse) const;
    // Weltzustand (seit 0.5.9)
    void populate_figures();  // Startbesetzung aus den Gebieten
    void adopt_figures();     // Figuren des Weltzustands in ihre Orte setzen (Start, Laden)
    void rebuild_transit();   // Figuren auf Verbindungen aus dem Weltzustand (Laden)
    Travel travel_of(const FigureRecord& f, int to_scene) const;
    void read_chronicle();
    void note_arrival(uint16_t who, int scene);
    void set_where_place(uint16_t who, int scene);
    void note_wake();

    std::array<std::unique_ptr<Scene>, kMaxScenes> scenes_{};
    int count_ = 0;
    Environment env_{};
    int active_ = 0;
    int from_ = -1;     // Szene, von der aus gewechselt wird (−1 = kein Übergang)
    int pending_ = -1;  // angeforderter Wechsel für den nächsten tick()
    int arming_ = -1;   // Vorbereitung: Ziel schon betreten, Paletten werden in den nächsten Schritten gemischt
    int merge_to_ = -1, merge_try_ = 0;  // Mischen: ein Durchgang je Schritt (Toleranz 0, 12, 30, 60)
    Transition pending_trans_ = Transition::Dissolve;
    Transition trans_ = Transition::Cut;
    int step_ = 0;
    int ticks_ = kDefaultTransitionTicks;
    uint32_t transitions_ = 0;
    uint32_t travels_ = 0;
    struct Transit {
        Travel t;
        int64_t depart_us, arrive_us;  // Weltzeit
        uint8_t from, to;              // Szenen
    };
    std::array<Transit, kMaxTransit> transit_{};
    int transit_n_ = 0;
    uint32_t entered_ = 0;  // Bit je Ort: betreten
    AssetStore* assets_ = nullptr;
    uint32_t pinned_ = 0;   // Bit je Ort: Pakete angeheftet
    int prefetched_ = -1;   // Ort, dessen Nachbarn zuletzt vorgeladen wurden
    int prepare_next_ = 0;  // Vorarbeit: unsichtbarer Ort, der zuletzt dran war
    int last_prepare_ = 0;
    bool hold_ = false;
    uint32_t holds_ = 0;
    bool follow_ = true;
    uint16_t follow_who_ = 0;
    bool inspect_ = false;
    // Weltzeit (seit 0.5.7)
    static constexpr int64_t kNoOverride = INT64_MIN, kPendingOverride = INT64_MIN + 1;
    bool world_valid_ = true;
    int64_t world_minute_ = INT64_MIN;  // zuletzt fortgeschriebene Weltminute (Tag × 1440 + Minute)
    int64_t override_key_ = kNoOverride;
    WeatherPick sim_pick_{};
    bool sim_applied_ = false;
    int64_t state_minute_ = INT64_MIN;  // Ortszustände fortgeschrieben bis hier
    int toast_reader_ = -1;
    // Weltzustand (seit 0.5.9)
    uint64_t seed_ = 0;
    int64_t world_us_ = 0;
    int chron_reader_ = -1;
    std::array<int16_t, kMaxScenes> place_of_{};
    bool wake_pending_ = false;            // nach dem Laden: Pause in die Chronik, sobald die Weltzeit läuft
    int64_t saved_minute_ = INT64_MIN;     // Weltminute im geladenen Speicherstand
    int64_t caught_up_ = 0, skipped_ = 0;
    int64_t past_key_ = INT64_MIN;         // Nachholen: Wetterzeit, für die past_rain_ gilt
    int past_rain_ = 0;
    // Stufen der Simulation (seit 0.5.9 Teil 2)
    int64_t grid_ = INT64_MIN;
    uint32_t decisions_ = 0, reseats_ = 0;
    int last_grid_steps_ = 0;
    // Gemeinsame Palette während einer Überblendung: Index der Szene → Platz, Platz → Quelle (Szene, Index)
    std::array<uint8_t, 256> map_from_{};
    std::array<uint8_t, 256> map_to_{};
    std::array<uint8_t, kSceneColors> slot_scene_{};
    std::array<uint8_t, kSceneColors> slot_index_{};
    std::array<uint8_t, 512> cell_head_{};          // Farbgitter beim Mischen: erster Platz je Zelle
    std::array<uint8_t, kSceneColors> cell_next_{};  // nächster Platz in derselben Zelle
    int slots_ = 0;
    Palette palette_{};
    uint8_t* main_ = nullptr;
    uint8_t* aux_ = nullptr;
    std::unique_ptr<uint8_t[]> own_;
    mutable std::array<uint8_t, kScreenWidth * 2> rows_{};  // zwei fertige Zeilen beim Zusammensetzen
    mutable CoarseLayers layers_{};                          // Schichten der sichtbaren groben Szene
    mutable CoarseLayers aux_layers_{};                      // der zweiten Szene während einer Überblendung
};

// Szenen von Gerät und Simulator in fester Reihenfolge: scrolltest (Landschaft), testort-wiese, testort-raum,
// testort-platz
void add_standard_scenes(Stage& stage);

}  // namespace pixelia
