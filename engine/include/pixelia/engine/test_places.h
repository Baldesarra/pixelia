#pragma once
// Testorte (seit 0.5.0): neutrale Platzhalter im groben Raster 200×120 zum Prüfen von Bühne, Überblendung,
// Figuren, Animationen und Farbreihen. Kein Setting-Inhalt: Kacheln 8×8, Platzhalter-Figuren in den Chibi-Maßen
// der Perspektive (Zelle 16×32, 28 Bildpunkte hoch, drei gezeichnete Ansichten, vier Richtungen), Kisten.
// Figuren und Kisten sind einmal mit einer grauen Platzhalter-Rampe gezeichnet und bekommen ihre Farbe über
// Farbreihen. Die Grafiken entstehen beim Start im Code (Asset-Pipeline folgt in 0.5.4).
//
// - testort-wiese: Feld mit Wegen, kühles Licht, 6 Figuren
// - testort-raum: Raum mit Rückwand und Kisten, warmes Licht (andere Palette), 3 Figuren
// Lastregler (set_load): Zahl der Figuren, 1–64; 0 = Standard des Ortes.

#include <array>
#include <cstdint>

#include "pixelia/engine/rng.h"
#include "pixelia/engine/scene.h"
#include "pixelia/engine/sprite.h"

namespace pixelia {

class TestPlace : public Scene {
public:
    enum class Kind : uint8_t { Field, Room };
    static constexpr int kMaxFigures = 64;
    static constexpr int kColors = 64;

    explicit TestPlace(Kind kind) : kind_(kind) {}
    const char* name() const override { return kind_ == Kind::Field ? "testort-wiese" : "testort-raum"; }
    const char* title() const override { return kind_ == Kind::Field ? "Testort Wiese" : "Testort Raum"; }
    void init(uint64_t seed) override;
    void tick(const WorldClock& clock) override;
    void render(Canvas& canvas) const override;
    void render_overlay(Canvas& fine) const override;
    const Palette& palette() const override { return palette_; }
    void set_load(int figures) override;
    int scale() const override { return kCoarseScale; }
    int colors() const override { return kColors; }
    int figure_count() const { return count_; }

private:
    struct Figure {
        int32_t x, y;    // Fußpunkt in groben Bildpunkten, 8.8 Festkomma
        int16_t tx, ty;  // Ziel in ganzen Bildpunkten
        uint16_t wait;   // Schritte bis zum nächsten Weg
        uint8_t speed;   // 1/256 Bildpunkt pro Schritt
        uint8_t row;     // Farbreihe
        Facing facing;
        Animator anim;
    };
    struct Box {
        int16_t x, y;  // Fußpunkt
        uint8_t row;
    };

    void build_palette();
    void build_map();
    void spawn(Figure& f);
    void pick_target(Figure& f);
    int area_top() const;

    Kind kind_;
    Rng rng_;
    Palette palette_{};
    std::array<Figure, kMaxFigures> figures_{};
    std::array<Box, 6> boxes_{};
    int boxes_count_ = 0;
    int count_ = 0;
    int wanted_ = 0;
    uint8_t map_[kCoarseHeight / 8][kCoarseWidth / 8]{};  // Kachel-Nummern, 25 × 15
};

}  // namespace pixelia
