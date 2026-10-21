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

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

enum class Transition : uint8_t {
    Cut,       // Schnitt: sofort
    Dissolve,  // Überblendung: grobe Bildpunkte wechseln in zufälliger Reihenfolge
    Pattern,   // Raster: wie Überblendung, in einem festen 4×4-Muster
    Fade,      // Abblenden über Schwarz
};
inline constexpr int kTransitionKinds = 4;
// Standard beim Ortswechsel: Abblenden (Entscheidung des Users 2026-10-21)
inline constexpr Transition kDefaultTransition = Transition::Fade;

// Kennungen für Befehle und Weboberfläche: schnitt, ueberblendung, raster, abblenden
const char* transition_name(Transition t);
bool parse_transition(std::string_view name, Transition* out);

// 200×120 → 400×240 (jeder grobe Bildpunkt wird 2 × 2)
void upscale2x(const uint8_t* coarse, Canvas& fine);

class Stage {
public:
    static constexpr int kMaxScenes = 8;
    static constexpr int kDefaultTransitionTicks = 18;  // 0,6 s

    int add(std::unique_ptr<Scene> scene);  // Index, −1 = voll
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
    bool merge_palettes();
    void update_palette();
    int visible() const;
    void draw_scene(const Scene& s, Canvas& fine) const;
    void compose(Canvas& fine, const uint8_t* old_coarse, const uint8_t* new_coarse) const;

    std::array<std::unique_ptr<Scene>, kMaxScenes> scenes_{};
    int count_ = 0;
    int active_ = 0;
    int from_ = -1;     // Szene, von der aus gewechselt wird (−1 = kein Übergang)
    int pending_ = -1;  // angeforderter Wechsel für den nächsten tick()
    Transition pending_trans_ = Transition::Dissolve;
    Transition trans_ = Transition::Cut;
    int step_ = 0;
    int ticks_ = kDefaultTransitionTicks;
    uint32_t transitions_ = 0;
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
};

// Szenen von Gerät und Simulator in fester Reihenfolge: scrolltest (Landschaft), testort-wiese, testort-raum
void add_standard_scenes(Stage& stage);

}  // namespace pixelia
