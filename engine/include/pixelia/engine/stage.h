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
// - Ausgänge (seit 0.5.1): Nach jedem Schritt holt die Bühne die Reisenden aller Orte ab (take_travel) und gibt sie
//   im Zielort ab (arrive). Folgt der Blick (follow) und ist der Ort gerade zu sehen, wechselt die Bühne mit dem
//   Übergang des Ausgangs (Standard Abblenden). Gibt es den Zielort nicht, kommt der Reisende am selben Ausgang zurück.
// - Grafik-Pakete (seit 0.5.5, assets.h): Die Bühne heftet die Pakete des sichtbaren Ortes, des Ortes, von dem aus
//   gewechselt wird, und des angeforderten Ziels an und lädt die Pakete der Nachbarorte vorab. Ein Ort wird erst
//   betreten, wenn seine Pakete geladen sind. Abblenden beginnt sofort und wartet in der dunklen Mitte auf das Ziel;
//   Überblendung, Raster und Schnitt beginnen erst, wenn das Ziel geladen ist (bis dahin bleibt der alte Ort). Das
//   Rechnen aller Orte läuft davon unberührt weiter.

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
    uint32_t travels() const { return travels_; }  // Reisen zwischen Orten seit init()
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
    bool command(const char* cmd);

    void tick(const WorldClock& clock);
    void set_load(int load);  // Lastregler der sichtbaren Szene

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
    void deliver_travels();
    bool merge_palettes();
    bool merge_palettes(int tolerance, bool last);
    void update_palette();
    int visible() const;
    void draw_scene(const Scene& s, Canvas& fine) const;
    void compose(Canvas& fine, const uint8_t* old_coarse, const uint8_t* new_coarse) const;

    std::array<std::unique_ptr<Scene>, kMaxScenes> scenes_{};
    int count_ = 0;
    Environment env_{};
    int active_ = 0;
    int from_ = -1;     // Szene, von der aus gewechselt wird (−1 = kein Übergang)
    int pending_ = -1;  // angeforderter Wechsel für den nächsten tick()
    Transition pending_trans_ = Transition::Dissolve;
    Transition trans_ = Transition::Cut;
    int step_ = 0;
    int ticks_ = kDefaultTransitionTicks;
    uint32_t transitions_ = 0;
    uint32_t travels_ = 0;
    uint32_t entered_ = 0;  // Bit je Ort: betreten
    AssetStore* assets_ = nullptr;
    uint32_t pinned_ = 0;   // Bit je Ort: Pakete angeheftet
    int prefetched_ = -1;   // Ort, dessen Nachbarn zuletzt vorgeladen wurden
    bool hold_ = false;
    uint32_t holds_ = 0;
    bool follow_ = true;
    bool inspect_ = false;
    // Gemeinsame Palette während einer Überblendung: Index der Szene → Platz, Platz → Quelle (Szene, Index)
    std::array<uint8_t, 256> map_from_{};
    std::array<uint8_t, 256> map_to_{};
    std::array<uint8_t, kSceneColors> slot_scene_{};
    std::array<uint8_t, kSceneColors> slot_index_{};
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
