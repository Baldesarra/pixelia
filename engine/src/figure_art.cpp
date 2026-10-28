#include "pixelia/engine/figure_art.h"

#include <algorithm>
#include <cstring>

#include "pixelia/engine/memory.h"

namespace pixelia {

namespace {

constexpr int kFrames = kFigureViews * kFigurePoses;
constexpr int kMaxFramePixels = 4096;
int64_t (*s_clock)() = nullptr;
FigureArtStats s_stats;

}  // namespace

void set_figure_art_clock(int64_t (*now_us)()) { s_clock = now_us; }
const FigureArtStats& figure_art_stats() { return s_stats; }

FigureArt::~FigureArt() {
    for (Slot& s : slots_)
        if (s.pixels) asset_free(s.pixels);
    if (scratch_) asset_free(scratch_);
}

void FigureArt::clear() {
    for (Slot& s : slots_) {
        s.frame = -1;
        s.used = 0;
        s.look = FigureLook{};
    }
}

void FigureArt::set_part(LookLayer layer, int variant, const SpriteSheet& sheet) {
    const int l = static_cast<int>(layer);
    if (l < 0 || l >= kLookLayers || variant < 1 || variant > kVariants || !sheet.pixels) return;
    if (sheet.frames < kFrames || sheet.width * sheet.height > kMaxFramePixels) return;
    if (width_ == 0) {
        width_ = sheet.width;
        height_ = sheet.height;
        pivot_x_ = sheet.pivot_x;
        pivot_y_ = sheet.pivot_y;
    } else if (sheet.width != width_ || sheet.height != height_) {
        return;  // alle Ebenen gleich groß
    }
    parts_[l][variant - 1] = sheet;
    clear();
}

void FigureArt::set_rows(const Recolor* rows, int count, const uint8_t* outline) {
    rows_ = rows;
    row_count_ = count;
    outline_ = outline;
    clear();
}

bool FigureArt::any_part(const FigureLook& look) {
    return std::any_of(look.part.begin(), look.part.end(), [](uint8_t p) { return p != 0; });
}

FigureArt::Slot* FigureArt::find(const FigureLook& look) {
    for (Slot& s : slots_)
        if (s.frame >= 0 && s.look.part == look.part && s.look.row == look.row) return &s;
    return nullptr;
}

const FigureArt::Slot* FigureArt::find(const FigureLook& look) const {
    return const_cast<FigureArt*>(this)->find(look);
}

void FigureArt::want(const FigureLook& look) {
    if (!has_parts() || !any_part(look)) return;
    if (Slot* s = find(look)) {
        s->used = ++tick_;
        return;
    }
    // freier Platz, sonst der am längsten nicht gewünschte
    Slot* pick = &slots_[0];
    for (Slot& s : slots_) {
        if (s.frame < 0) {
            pick = &s;
            break;
        }
        if (s.used < pick->used) pick = &s;
    }
    const size_t bytes = static_cast<size_t>(width_) * height_ * kFrames;
    if (!pick->pixels) pick->pixels = static_cast<uint8_t*>(asset_alloc(bytes));
    if (!pick->pixels) return;
    pick->look = look;
    pick->frame = 0;
    pick->used = ++tick_;
    pick->sheet = SpriteSheet{pick->pixels, width_, height_, kFrames, pivot_x_, pivot_y_};
}

const SpriteSheet* FigureArt::sheet(const FigureLook& look) const {
    const Slot* s = find(look);
    return s && s->frame == kFrames ? &s->sheet : nullptr;
}

int FigureArt::ready() const {
    return static_cast<int>(std::count_if(std::begin(slots_), std::end(slots_), [](const Slot& s) { return s.frame == kFrames; }));
}

void FigureArt::compose_frame(Slot& s, int frame) {
    const int n = width_ * height_;
    uint8_t* dst = s.pixels + static_cast<size_t>(frame) * n;
    std::memset(dst, 0, static_cast<size_t>(n));
    // Ebenen in fester Reihenfolge (Körper, Beine, Oberteil, Haare, Kopfbedeckung, Werkzeug), je mit Farbreihe
    for (int l = 0; l < kLookLayers; ++l) {
        const int v = s.look.part[l];
        if (v < 1 || v > kVariants) continue;
        const SpriteSheet& part = parts_[l][v - 1];
        if (!part.pixels) continue;
        const int row = s.look.row[l];
        const uint8_t* map = rows_ && row > 0 && row < row_count_ ? rows_[row].table() : nullptr;
        const uint8_t* src = part.pixels + static_cast<size_t>(frame) * n;
        for (int i = 0; i < n; ++i)
            if (const uint8_t c = src[i]) dst[i] = map ? map[c] : c;
    }
    if (!outline_) return;
    // Kontur nur außen: leere Punkte neben der Figur bekommen die dunkle Fassung des Nachbarn (unten, links,
    // rechts, oben zuerst wie in der Skizze); gelesen wird aus einer Kopie (Grafik-Speicher, nicht der Stack)
    if (!scratch_) scratch_ = static_cast<uint8_t*>(asset_alloc(kMaxFramePixels));
    if (!scratch_) return;
    uint8_t* copy = scratch_;
    std::memcpy(copy, dst, static_cast<size_t>(n));
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x) {
            const int i = y * width_ + x;
            if (copy[i]) continue;
            uint8_t c = 0;
            if (y + 1 < height_ && copy[i + width_]) c = copy[i + width_];
            else if (x > 0 && copy[i - 1]) c = copy[i - 1];
            else if (x + 1 < width_ && copy[i + 1]) c = copy[i + 1];
            else if (y > 0 && copy[i - width_]) c = copy[i - width_];
            if (c) dst[i] = outline_[c];
        }
}

int FigureArt::prepare(int budget) {
    int used = 0;
    while (budget - used > 0) {
        // zuletzt Gewünschtes zuerst
        Slot* next = nullptr;
        for (Slot& s : slots_)
            if (s.frame >= 0 && s.frame < kFrames && (!next || s.used > next->used)) next = &s;
        if (!next) break;
        if (used > 0 && used + kFrameCost > budget) break;
        const int64_t t = s_clock ? s_clock() : 0;
        compose_frame(*next, next->frame++);
        used += kFrameCost;
        if (s_clock) {
            const int64_t now = s_clock(), d = now - t;
            ++s_stats.frames;
            s_stats.us += d;
            s_stats.max_us = std::max(s_stats.max_us, d);
            if (!s_stats.first_us) s_stats.first_us = t;
            bool pending = false;
            for (const Slot& s : slots_) pending = pending || (s.frame >= 0 && s.frame < kFrames);
            if (!pending) s_stats.done_us = now;
            if (!pending && !s_stats.first_done_us) s_stats.first_done_us = now;
        }
    }
    return used;
}

}  // namespace pixelia
