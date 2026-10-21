#include "pixelia/engine/canvas.h"

#include <algorithm>
#include <cstring>

namespace pixelia {

void Canvas::clear(uint8_t color) { std::memset(px_, color, static_cast<size_t>(w_) * h_); }

void Canvas::fill_rect(int x, int y, int w, int h, uint8_t color) {
    int x0 = std::max(x, 0), x1 = std::min(x + w, w_);
    int y0 = std::max(y, 0), y1 = std::min(y + h, h_);
    if (x0 >= x1) return;
    for (int yy = y0; yy < y1; ++yy) std::memset(row(yy) + x0, color, x1 - x0);
}

void Canvas::fill_row(int y, uint8_t color) { std::memset(row(y), color, w_); }

void Canvas::fill_row_pattern(int y, uint32_t pattern) {
    // Breite ist ein Vielfaches von 4 und die Zeilen sind 4-Byte-ausgerichtet
    auto* p = reinterpret_cast<uint32_t*>(row(y));
    for (int i = 0; i < w_ / 4; ++i) p[i] = pattern;
}

void Canvas::vline(int x, int y0, int y1, uint8_t color) {
    if (x < 0 || x >= w_) return;
    y0 = std::max(y0, 0);
    y1 = std::min(y1, h_);
    uint8_t* p = px_ + y0 * w_ + x;
    for (int y = y0; y < y1; ++y, p += w_) *p = color;
}

void Canvas::blit(const Sprite& s, int x, int y, bool flip_x, const uint8_t* remap) {
    blit_clipped(s, x, y, flip_x, remap, {0, 0, w_, h_});
}

void Canvas::blit_clipped(const Sprite& s, int x, int y, bool flip_x, const uint8_t* remap, const Rect& clip,
                          const LightBlit* light) {
    const int cx0 = std::max(clip.x, 0), cx1 = std::min(clip.x + clip.w, w_);
    const int cy0 = std::max(clip.y, 0), cy1 = std::min(clip.y + clip.h, h_);
    int sx0 = std::max(0, cx0 - x), sx1 = std::min(s.width, cx1 - x);
    int sy0 = std::max(0, cy0 - y), sy1 = std::min(s.height, cy1 - y);
    if (sx0 >= sx1 || sy0 >= sy1) return;
    // Quelle läuft beim Spiegeln rückwärts; Spiegeln und Farbreihe sind außerhalb der inneren Schleife entschieden
    const int n = sx1 - sx0;
    const int step = flip_x ? -1 : 1;
    for (int sy = sy0; sy < sy1; ++sy) {
        const uint8_t* src = s.pixels + sy * s.width + (flip_x ? s.width - 1 - sx0 : sx0);
        uint8_t* dst = row(y + sy) + x + sx0;
        if (light) {
            // im Licht (seit 0.5.2): Grundfarben auf die Stufe der Lichtkarte bzw. die feste Stufe heben
            const uint8_t* m = light->map ? light->map + (y + sy) * w_ + x + sx0 : nullptr;
            const int base = light->base, shift = light->level * base;
            for (int i = 0; i < n; ++i, src += step) {
                uint8_t c = *src;
                if (!c) continue;
                if (remap) c = remap[c];
                if (c < base) c = static_cast<uint8_t>(c + (m ? m[i] * base : shift));
                dst[i] = c;
            }
        } else if (remap) {
            for (int i = 0; i < n; ++i, src += step)
                if (const uint8_t c = *src) dst[i] = remap[c];
        } else {
            for (int i = 0; i < n; ++i, src += step)
                if (const uint8_t c = *src) dst[i] = c;
        }
    }
}

}  // namespace pixelia
