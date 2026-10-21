#pragma once
// Zeichenfläche der Engine: 400×240 Pixel, 1 Byte pro Pixel = Index in die Palette.

#include <array>
#include <cstdint>

namespace pixelia {

inline constexpr int kScreenWidth = 400;
inline constexpr int kScreenHeight = 240;
inline constexpr int kScreenPixels = kScreenWidth * kScreenHeight;

// Grobes Raster der Orte (seit 0.5.0): 200×120 grobe Bildpunkte, die Bühne verdoppelt sie auf 400×240
// (am Display 4 × 4 Pixel je grobem Bildpunkt). Text und Einblendungen bleiben fein.
inline constexpr int kCoarseScale = 2;
inline constexpr int kCoarseWidth = kScreenWidth / kCoarseScale;
inline constexpr int kCoarseHeight = kScreenHeight / kCoarseScale;
inline constexpr int kCoarsePixels = kCoarseWidth * kCoarseHeight;

// Paletteneinträge der Szenen; 240–255 gehören der System-Oberfläche (system_ui.h)
inline constexpr int kSceneColors = 240;

struct Color {
    uint8_t r, g, b;
};

using Palette = std::array<Color, 256>;

// Sprite mit Palettenindizes, Index 0 = durchsichtig
struct Sprite {
    const uint8_t* pixels;
    int width;
    int height;
};

class Canvas {
public:
    Canvas() = default;
    Canvas(uint8_t* pixels, int width, int height) : px_(pixels), w_(width), h_(height) {}

    uint8_t* pixels() { return px_; }
    int width() const { return w_; }
    int height() const { return h_; }
    uint8_t* row(int y) { return px_ + y * w_; }

    void clear(uint8_t color);
    void fill_rect(int x, int y, int w, int h, uint8_t color);
    void fill_row(int y, uint8_t color);
    // Zeile mit einem 4-Pixel-Muster füllen (für Dithering)
    void fill_row_pattern(int y, uint32_t pattern);
    void vline(int x, int y0, int y1, uint8_t color);
    // remap: Farbreihe (256 Byte, Index → Index), nullptr = Farben wie gezeichnet
    void blit(const Sprite& s, int x, int y, bool flip_x = false, const uint8_t* remap = nullptr);

private:
    uint8_t* px_ = nullptr;
    int w_ = 0;
    int h_ = 0;
};

}  // namespace pixelia
