#include "pixelia/engine/depth.h"

#include <algorithm>
#include <cstring>

#include "pixelia/engine/light.h"


namespace pixelia {

static_assert(DepthList::kCapacity <= 256, "Indizes der Liste sind 8 Bit");

bool DepthList::add(const Sprite& s, int fx, int fy, int px, int py, bool flip, const uint8_t* remap, int dz,
                    int shadow, bool baked) {
    if (n_ >= kCapacity) return false;
    DepthItem& it = items_[n_++];
    it.sprite = s;
    it.remap = remap;
    it.x = static_cast<int16_t>(fx - (flip ? s.width - 1 - px : px));
    it.y = static_cast<int16_t>(fy - py);
    it.foot_x = static_cast<int16_t>(fx);
    it.foot_y = static_cast<int16_t>(fy);
    it.depth = static_cast<int16_t>(fy + dz);
    it.shadow = static_cast<uint8_t>(shadow < 0 ? 0 : shadow > 255 ? 255 : shadow);
    it.flip = flip;
    it.baked = baked;
    it.figure = true;
    return true;
}

bool DepthList::add_at(const Sprite& s, int x, int y, int depth, bool flip, const uint8_t* remap, bool baked) {
    if (n_ >= kCapacity) return false;
    DepthItem& it = items_[n_++];
    it.sprite = s;
    it.remap = remap;
    it.x = static_cast<int16_t>(x);
    it.y = static_cast<int16_t>(y);
    it.foot_x = static_cast<int16_t>(x + s.width / 2);
    it.foot_y = static_cast<int16_t>(y + s.height - 1);
    it.depth = static_cast<int16_t>(depth);
    it.shadow = 0;
    it.flip = flip;
    it.baked = baked;
    it.figure = false;
    return true;
}

Rect DepthList::bounds(const DepthItem& it) {
    Rect r{it.x, it.y, it.sprite.width, it.sprite.height};
    if (it.shadow) {
        // Schatten: Zeile foot_y ± 1, Breite 2 × shadow + 1
        const int sx0 = it.foot_x - it.shadow, sx1 = it.foot_x + it.shadow + 1;
        const int sy0 = it.foot_y - 1, sy1 = it.foot_y + 2;
        const int x0 = r.x < sx0 ? r.x : sx0, y0 = r.y < sy0 ? r.y : sy0;
        const int x1 = r.x + r.w > sx1 ? r.x + r.w : sx1, y1 = r.y + r.h > sy1 ? r.y + r.h : sy1;
        r = {x0, y0, x1 - x0, y1 - y0};
    }
    return r;
}

void DepthList::sort() {
    // Einfügen nach (Tiefe, Reihenfolge): stabil, ohne Speicher anzufordern; die Listen sind klein
    for (int i = 0; i < n_; ++i) {
        const uint8_t v = static_cast<uint8_t>(i);
        const int d = items_[i].depth;
        int j = i;
        while (j > 0 && items_[sorted_[j - 1]].depth > d) {
            sorted_[j] = sorted_[j - 1];
            --j;
        }
        sorted_[j] = v;
    }
}

namespace {

void fill_clipped(Canvas& c, Rect r, const Rect& clip, uint8_t color) {
    const int x0 = r.x > clip.x ? r.x : clip.x, y0 = r.y > clip.y ? r.y : clip.y;
    const int x1 = r.x + r.w < clip.x + clip.w ? r.x + r.w : clip.x + clip.w;
    const int y1 = r.y + r.h < clip.y + clip.h ? r.y + r.h : clip.y + clip.h;
    if (x0 < x1 && y0 < y1) c.fill_rect(x0, y0, x1 - x0, y1 - y0, color);
}

void shade_clipped(Canvas& c, Rect r, const Rect& clip, const uint8_t* table) {
    const int x0 = std::max({r.x, clip.x, 0}), y0 = std::max({r.y, clip.y, 0});
    const int x1 = std::min({r.x + r.w, clip.x + clip.w, c.width()});
    const int y1 = std::min({r.y + r.h, clip.y + clip.h, c.height()});
    for (int y = y0; y < y1; ++y) {
        uint8_t* p = c.row(y);
        for (int x = x0; x < x1; ++x) p[x] = table[p[x]];
    }
}

}  // namespace

void DepthList::draw_item(Canvas& c, const DepthItem& it, const Rect& clip) const {
    if (it.shadow) {
        const int w = it.shadow;
        const Rect a{it.foot_x - w + 1, it.foot_y - 1, 2 * w - 1, 3}, b{it.foot_x - w, it.foot_y, 2 * w + 1, 1};
        if (shadow_table_) {
            // durchscheinend: Mittelzeile nur außerhalb von a, sonst würde sie doppelt abgedunkelt
            shade_clipped(c, a, clip, shadow_table_);
            shade_clipped(c, {b.x, b.y, 1, 1}, clip, shadow_table_);
            shade_clipped(c, {b.x + b.w - 1, b.y, 1, 1}, clip, shadow_table_);
        } else {
            fill_clipped(c, a, clip, shadow_color_);
            fill_clipped(c, b, clip, shadow_color_);
        }
    }
    if (!light_) {
        c.blit_clipped(it.sprite, it.x, it.y, it.flip, it.remap, clip);
        return;
    }
    if (!it.figure) {
        c.blit_clipped(it.sprite, it.x, it.y, it.flip, it.remap, clip);  // wie der Hintergrund: Licht beim Verdoppeln
        return;
    }
    const LightBlit lb{nullptr, light_base_, foot_level(c, it)};
    c.blit_clipped(it.sprite, it.x, it.y, it.flip, it.remap, clip, &lb);
}

int DepthList::foot_level(const Canvas& c, const DepthItem& it) const {
    // Mittel der Lichtkarte über das 4×4-Feld am Fußpunkt, gerundet auf eine Stufe
    const int x0 = std::clamp(it.foot_x & ~3, 0, c.width() - 4), y0 = std::clamp(it.foot_y & ~3, 0, c.height() - 4);
    int sum = 0;
    for (int y = y0; y < y0 + 4; ++y)
        for (int x = x0; x < x0 + 4; ++x) sum += light_[y * c.width() + x];
    return (sum / 16 + kLightUnit / 2) / kLightUnit;
}

void DepthList::copy_under_movers(Canvas& c, int movers) {
    CoarseLayers& L = *layers_;
    const int w = c.width(), h = c.height() < kCoarseHeight ? c.height() : kCoarseHeight;
    L.x0.fill(0xFF);
    L.x1.fill(0);
    // je Zeile die Hülle aller bewegten Teile (Bild und Schatten), auf gerade Grenzen erweitert
    for (int m = 0; m < movers; ++m) {
        const Rect r = bounds(items_[movers_[m]]);
        const int x0 = (r.x < 0 ? 0 : r.x) & ~1, x1r = r.x + r.w > w ? w : r.x + r.w;
        const int x1 = (x1r + 1) & ~1;
        const int y0 = r.y < 0 ? 0 : r.y, y1 = r.y + r.h > h ? h : r.y + r.h;
        if (x0 >= x1) continue;
        for (int y = y0; y < y1; ++y) {
            if (x0 < L.x0[y]) L.x0[y] = static_cast<uint8_t>(x0);
            if (x1 > L.x1[y]) L.x1[y] = static_cast<uint8_t>(x1 > w ? w : x1);
        }
    }
    for (int y = 0; y < h; ++y) {
        if (L.x0[y] >= L.x1[y]) {
            L.x0[y] = L.x1[y] = 0;
            continue;
        }
        std::memcpy(c.row(y) + L.x0[y], L.base + y * w + L.x0[y], L.x1[y] - L.x0[y]);
    }
}

void DepthList::draw(Canvas& c, bool all) {
    sort();
    int movers = 0;
    if (!all)
        for (int i = 0; i < n_; ++i)
            if (!items_[i].baked) movers_[movers++] = static_cast<uint8_t>(i);
    if (layers_ && layers_->base && !all) copy_under_movers(c, movers);
    for (int k = 0; k < n_; ++k) {
        const DepthItem& it = items_[sorted_[k]];
        if (all || !it.baked) {
            draw_item(c, it, {0, 0, c.width(), c.height()});
            continue;
        }
        // eingebacken: nur über bewegten Teilen neu zeichnen, die es überlappt
        const Rect r = bounds(it);
        for (int m = 0; m < movers; ++m) {
            const Rect mr = bounds(items_[movers_[m]]);
            if (r.overlaps(mr)) draw_item(c, it, mr);
        }
    }
}

}  // namespace pixelia
