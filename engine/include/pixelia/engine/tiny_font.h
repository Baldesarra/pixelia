#pragma once
// Winzige 3×5-Prüfschrift für Beschriftungen in Testbildern und Kontaktbögen (Ziffern, : . - / Leerzeichen,
// Großbuchstaben). Die richtigen Pixel-Schriften mit Umlauten kommen in 0.5.x.

#include <cstdint>

namespace pixelia::tiny_font {

inline constexpr int kGlyphWidth = 3;
inline constexpr int kGlyphHeight = 5;
inline constexpr int kAdvance = 4;  // Glyphe + 1 Pixel Abstand

// 15 Bit: Zeile 0 oben in den Bits 14..12, links = höchstes Bit der Zeile. 0 für unbekannte Zeichen.
uint16_t glyph(char c);

// Breite eines Textes in Pixeln (vor dem Vergrößern)
int text_width(const char* text);

// Ruft plot(x, y) für jedes gesetzte Pixel auf (vergrößert um scale)
template <typename Plot>
void draw(const char* text, int x, int y, int scale, Plot&& plot) {
    for (const char* p = text; *p; ++p, x += kAdvance * scale) {
        uint16_t g = glyph(*p);
        for (int row = 0; row < kGlyphHeight; ++row)
            for (int col = 0; col < kGlyphWidth; ++col)
                if (g & (1u << (14 - row * 3 - col)))
                    for (int sy = 0; sy < scale; ++sy)
                        for (int sx = 0; sx < scale; ++sx) plot(x + col * scale + sx, y + row * scale + sy);
    }
}

}  // namespace pixelia::tiny_font
