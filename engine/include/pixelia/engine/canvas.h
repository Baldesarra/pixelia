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

// Rechteck in Bildpunkten (Beschneiden, Überlappung)
struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool empty() const { return w <= 0 || h <= 0; }
    bool overlaps(const Rect& o) const { return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h; }
};

using Palette = std::array<Color, 256>;

// Grobes Bild aus zwei Schichten (seit 0.5.2): Die Bühne verdoppelt direkt aus dem unbewegten Hintergrund (base)
// und nimmt nur je Zeile im Bereich [x0, x1) den Arbeitspuffer mit den bewegten Teilen. Das spart die Kopie des
// ganzen Hintergrunds (24 KB PSRAM → PSRAM, 0,9 ms) in jedem Bild. Grenzen gerade, x0 == x1: Zeile ganz aus base.
struct CoarseLayers {
    const uint8_t* base = nullptr;  // nullptr = alles aus dem Arbeitspuffer
    std::array<uint8_t, kCoarseHeight> x0{}, x1{};
    // Licht (light.h): je grobem Bildpunkt die Stufen seiner vier feinen Bildpunkte (je 2 Bit, Raster schon beim
    // Vorzeichnen ausgerechnet), erst beim Verdoppeln angewendet, damit das Raster fein sein kann (2 × 2
    // Displaypunkte); je Zeile der Bereich [lx0, lx1) mit Licht. nullptr = kein Licht (z. B. tagsüber)
    const uint8_t* light = nullptr;
    std::array<uint8_t, kCoarseHeight> lx0{}, lx1{};
    uint8_t light_base = 0;  // Grundfarben je Lichtstufe
    // Schleier (seit 0.5.3, Wetter: Nebelbänke, Regenschleier, Wolkenschatten): Farbtabelle (Index → Index, z. B.
    // PlacePalette::tint_table) mit Anteil 0 … 16 je Feld von 4 × 4 groben Bildpunkten (veil_grid, kVeilW × kVeilH),
    // beim Verdoppeln fein gerastert nach dem Licht angewendet. nullptr = kein Schleier
    static constexpr int kVeilCell = 4;
    static constexpr int kVeilW = kCoarseWidth / kVeilCell, kVeilH = kCoarseHeight / kVeilCell;
    const uint8_t* veil_table = nullptr;
    const uint8_t* veil_grid = nullptr;
};

// Zeichnen im Licht (seit 0.5.2, light.h): Grundfarben 1 … base − 1 auf eine Lichtstufe heben, je Bildpunkt aus
// der Lichtkarte (map, Maße der Zeichenfläche) oder für das ganze Bild fest (map = nullptr, level)
struct LightBlit {
    const uint8_t* map = nullptr;
    int base = 0;
    int level = 0;
};

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
    // wie blit, zeichnet aber nur innerhalb von clip (Tiefensortierung: Möbel vor einer Figur neu zeichnen)
    void blit_clipped(const Sprite& s, int x, int y, bool flip_x, const uint8_t* remap, const Rect& clip,
                      const LightBlit* light = nullptr);

private:
    uint8_t* px_ = nullptr;
    int w_ = 0;
    int h_ = 0;
};

}  // namespace pixelia
