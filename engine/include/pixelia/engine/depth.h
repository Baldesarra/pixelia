#pragma once
// Tiefensortierung nach Fußpunkt (seit 0.5.1): Figuren, Möbel und Deko eines Ortes gemeinsam sortiert.
//
// - Jedes Teil hat einen Fußpunkt. Wer weiter unten steht (größeres y), steht vorn und wird später gezeichnet; bei
//   gleicher Tiefe zählt die Reihenfolge des Hinzufügens (stabil, nichts flackert). Ein Tiefenversatz (dz) erlaubt
//   Ausnahmen, z. B. eine Figur, die auf einem Stuhl sitzt, vor der Lehne.
// - Eingebackene Teile (baked) stehen schon im Hintergrundbild des Ortes (room.h). Sie werden nur dort neu
//   gezeichnet, wo sie ein bewegtes Teil überdecken, beschnitten auf dessen Rechteck: So verschwindet eine Figur
//   hinter dem Tisch, und pro Bild kosten nur die bewegten Teile. Das Ergebnis ist pixelgleich mit „alles jedes Bild
//   zeichnen“ (Test im Simulator, --test stage).
// - Schatten: kleine Ellipse am Fußpunkt; gehört zum Teil und wird mit ihm verdeckt.
// Die Liste ist groß (einige KB) und gehört in den Grafik-Speicher, nicht auf den Stapel.

#include <array>
#include <cstdint>

#include "pixelia/engine/canvas.h"

namespace pixelia {

struct DepthItem {
    Sprite sprite;
    const uint8_t* remap;  // Farbreihe oder nullptr
    int16_t x, y;          // linke obere Ecke des Bildes
    int16_t foot_x, foot_y;
    int16_t depth;         // foot_y + dz
    uint8_t shadow;        // halbe Breite des Schattens, 0 = keiner
    bool flip;
    bool baked;
};

class DepthList {
public:
    static constexpr int kCapacity = 192;

    void clear() { n_ = 0; }
    int size() const { return n_; }
    void set_shadow_color(uint8_t c) { shadow_color_ = c; }
    // Fußpunkt (fx, fy) im Ort, Fußpunkt im Bild (px, py); false = Liste voll
    bool add(const Sprite& s, int fx, int fy, int px, int py, bool flip = false, const uint8_t* remap = nullptr,
             int dz = 0, int shadow = 0, bool baked = false);
    // Bild mit linker oberer Ecke (x, y) und Tiefe depth (Möbel: Unterkante der Stellfläche), ohne Schatten
    bool add_at(const Sprite& s, int x, int y, int depth, bool flip = false, const uint8_t* remap = nullptr,
                bool baked = false);
    // Zeichnet nach Tiefe. all = eingebackene Teile vollständig zeichnen (Vorzeichnen, Zeichnen ohne Hintergrund)
    void draw(Canvas& c, bool all = false);

private:
    void sort();
    static Rect bounds(const DepthItem& it);  // Bild und Schatten
    void draw_item(Canvas& c, const DepthItem& it, const Rect& clip) const;

    std::array<DepthItem, kCapacity> items_{};
    std::array<uint8_t, kCapacity> sorted_{};
    std::array<uint8_t, kCapacity> movers_{};
    int n_ = 0;
    uint8_t shadow_color_ = 0;
};

}  // namespace pixelia
