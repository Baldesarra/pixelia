#include "pixelia/engine/color.h"

namespace pixelia {

void indexed_to_rgb(const uint8_t* pixels, int w, int h, const Palette& palette, int scale, bool device_colors,
                    uint8_t* rgb) {
    Color lut[256];
    for (int i = 0; i < 256; ++i) lut[i] = device_colors ? from_rgb565(to_rgb565(palette[i])) : palette[i];
    if (scale < 1) scale = 1;
    const int out_w = w * scale;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = rgb + static_cast<size_t>(y) * scale * out_w * 3;
        uint8_t* p = row;
        for (int x = 0; x < w; ++x) {
            Color c = lut[pixels[y * w + x]];
            for (int s = 0; s < scale; ++s) {
                *p++ = c.r;
                *p++ = c.g;
                *p++ = c.b;
            }
        }
        // weitere Zeilen derselben Quellzeile kopieren
        for (int s = 1; s < scale; ++s) {
            uint8_t* dst = row + static_cast<size_t>(s) * out_w * 3;
            for (int i = 0; i < out_w * 3; ++i) dst[i] = row[i];
        }
    }
}

}  // namespace pixelia
