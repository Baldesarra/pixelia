#pragma once
// Farbumrechnung: Palette (RGB888) → RGB565 wie auf dem Gerät, und zurück für den Simulator

#include <cstdint>

#include "pixelia/engine/canvas.h"

namespace pixelia {

constexpr uint16_t to_rgb565(Color c) {
    return static_cast<uint16_t>(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3));
}

// RGB565 wieder auf 8 Bit pro Kanal (so sieht der Simulator genau die Farben des Displays)
constexpr Color from_rgb565(uint16_t v) {
    uint8_t r5 = v >> 11, g6 = (v >> 5) & 0x3F, b5 = v & 0x1F;
    return {static_cast<uint8_t>((r5 << 3) | (r5 >> 2)), static_cast<uint8_t>((g6 << 2) | (g6 >> 4)),
            static_cast<uint8_t>((b5 << 3) | (b5 >> 2))};
}

// Indiziertes Bild in RGB888 umrechnen, optional ganzzahlig vergrößert.
// device_colors: Farben wie auf dem Display (über RGB565). rgb muss (w*scale)*(h*scale)*3 Byte groß sein.
void indexed_to_rgb(const uint8_t* pixels, int w, int h, const Palette& palette, int scale, bool device_colors,
                    uint8_t* rgb);

}  // namespace pixelia
