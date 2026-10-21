#include "pixelia/engine/depth.h"

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

}  // namespace

void DepthList::draw_item(Canvas& c, const DepthItem& it, const Rect& clip) const {
    if (it.shadow) {
        const int w = it.shadow;
        fill_clipped(c, {it.foot_x - w + 1, it.foot_y - 1, 2 * w - 1, 3}, clip, shadow_color_);
        fill_clipped(c, {it.foot_x - w, it.foot_y, 2 * w + 1, 1}, clip, shadow_color_);
    }
    c.blit_clipped(it.sprite, it.x, it.y, it.flip, it.remap, clip);
}

void DepthList::draw(Canvas& c, bool all) {
    sort();
    int movers = 0;
    if (!all)
        for (int i = 0; i < n_; ++i)
            if (!items_[i].baked) movers_[movers++] = static_cast<uint8_t>(i);
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
