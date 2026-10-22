#pragma once
// Figuren-Bausteine (seit 0.5.0): Bildstreifen mit Fußpunkt, Blickrichtungen, Animationen und Farbreihen.
//
// - Ein Bildstreifen hält alle Bilder einer Figur gleicher Größe hintereinander (Bild für Bild je width × height
//   Indizes, Index 0 = durchsichtig), dazu den Fußpunkt, an dem die Figur steht (für Lage und später Tiefe).
// - Animationen zählen in Logik-Schritten: Sie laufen im Simulator und auf dem Gerät gleich und unabhängig davon,
//   wie oft gezeichnet wird.
// - Farbreihe = Umfärbe-Tabelle (256 Byte, Index → Index): Eine Grafik wird einmal mit einer Platzhalter-Rampe
//   gezeichnet, die Farbreihe ersetzt diese Rampe beim Zeichnen durch eine andere Rampe der Palette.

#include <array>
#include <cstddef>
#include <cstdint>

#include "pixelia/engine/canvas.h"

namespace pixelia {

struct SpriteSheet {
    const uint8_t* pixels = nullptr;
    int width = 0;   // eines Bildes
    int height = 0;
    int frames = 0;
    int pivot_x = 0;  // Fußpunkt im Bild
    int pivot_y = 0;
    // Zustandsbilder (seit 0.5.5): Bilder je Zustand = frames / states, Zustand für Zustand hintereinander
    // (z. B. Lampe aus, an). Die Bilder eines Zustands sind seine Animation.
    int states = 1;

    Sprite frame(int i) const {
        return {pixels + static_cast<size_t>(i) * width * height, width, height};
    }
    int frames_per_state() const { return states > 1 ? frames / states : frames; }
    // Bild f der Animation von Zustand s (s außerhalb: letzter Zustand)
    Sprite frame(int s, int f) const {
        const int per = frames_per_state();
        if (s >= states) s = states - 1;
        return frame((s < 0 ? 0 : s) * per + (per > 0 ? f % per : 0));
    }
};

// Blickrichtung. Vier Richtungen aus drei gezeichneten Ansichten: vorn, hinten, rechts (links = rechts gespiegelt)
enum class Facing : uint8_t { Down, Up, Left, Right };

struct View {
    uint8_t index;  // 0 = vorn, 1 = hinten, 2 = Seite
    bool flip;
};

constexpr View view_of(Facing f) {
    switch (f) {
        case Facing::Up: return {1, false};
        case Facing::Right: return {2, false};
        case Facing::Left: return {2, true};
        default: return {0, false};
    }
}

struct AnimFrame {
    uint8_t frame;  // Bild im Streifen
    uint8_t ticks;  // Dauer in Logik-Schritten (mindestens 1)
};

enum class AnimMode : uint8_t { Loop, Once, PingPong };

struct Animation {
    const AnimFrame* frames;
    uint8_t count;
    AnimMode mode;
};

class Animator {
public:
    // Neue Animation; dieselbe läuft ohne restart einfach weiter
    void play(const Animation* anim, bool restart = false);
    void tick();
    int frame() const { return anim_ && anim_->count ? anim_->frames[index_].frame : 0; }
    bool finished() const { return done_; }
    const Animation* animation() const { return anim_; }

private:
    const Animation* anim_ = nullptr;
    uint8_t index_ = 0;
    uint8_t left_ = 0;  // verbleibende Schritte des aktuellen Bildes
    int8_t dir_ = 1;
    bool done_ = false;
};

class Recolor {
public:
    Recolor();  // ohne Änderung (jeder Index bleibt)
    // Rampe [from, from + length) → [to, to + length); Index 0 bleibt durchsichtig
    Recolor& ramp(uint8_t from, uint8_t to, uint8_t length);
    Recolor& set(uint8_t from, uint8_t to);
    // Ganze Tabelle übernehmen (Farbreihe aus einem Asset-Paket, seit 0.5.5); Index 0 bleibt durchsichtig
    Recolor& load(const uint8_t* table);
    const uint8_t* table() const { return map_.data(); }
    uint8_t operator[](uint8_t i) const { return map_[i]; }

private:
    std::array<uint8_t, 256> map_;
};

}  // namespace pixelia
