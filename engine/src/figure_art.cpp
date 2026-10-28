#include "pixelia/engine/figure_art.h"

#include <algorithm>
#include <cstring>

#include "pixelia/engine/memory.h"

namespace pixelia {

namespace {

constexpr int kFrames = kFigureViews * kFigurePoses;
constexpr int kMaxWidth = 32;   // Ebenen höchstens 32 × 48 (heute 24 × 36)
constexpr int kMaxHeight = 48;
// Arbeitsfläche beim Zusammensetzen (Grafik-Speicher, nicht der Stack): das Bild vor der Kontur und die Spalten je
// Quellzeile. Interner RAM brachte am Gerät nichts (0.5.9 Teil 5: 860 µs je Bild wie im PSRAM)
constexpr size_t kScratchImage = kMaxWidth * (kMaxHeight + FigureArt::kStatureTop);
constexpr size_t kScratchBytes = kScratchImage + kMaxWidth * kMaxHeight;
int64_t (*s_clock)() = nullptr;
FigureArtStats s_stats;

}  // namespace

void set_figure_art_clock(int64_t (*now_us)()) { s_clock = now_us; }
const FigureArtStats& figure_art_stats() { return s_stats; }
void reset_figure_art_stats() { s_stats = FigureArtStats{}; }

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
    if (sheet.frames < kFrames || sheet.width > kMaxWidth || sheet.height > kMaxHeight) return;
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
        if (s.frame >= 0 && s.look.part == look.part && s.look.row == look.row && s.look.build == look.build) return &s;
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
    const size_t bytes = static_cast<size_t>(width_) * (height_ + kStatureTop) * kFrames;
    if (!pick->pixels) pick->pixels = static_cast<uint8_t*>(asset_alloc(bytes));
    if (!pick->pixels) return;
    pick->look = look;
    pick->frame = 0;
    pick->used = ++tick_;
    pick->sheet = SpriteSheet{pick->pixels, width_, height_ + kStatureTop, kFrames, pivot_x_, pivot_y_ + kStatureTop};
}

const SpriteSheet* FigureArt::sheet(const FigureLook& look) const {
    const Slot* s = find(look);
    return s && s->frame == kFrames ? &s->sheet : nullptr;
}

int FigureArt::ready() const {
    return static_cast<int>(std::count_if(std::begin(slots_), std::end(slots_), [](const Slot& s) { return s.frame == kFrames; }));
}

// --- Statur (seit 0.5.9 Teil 5), wie tools/figure_art.py (stature_rows, stature_cols): Zeilen und Spalten im Inneren
// verdoppeln oder weglassen, so bleiben Lichtkante, Schatten und Kontur erhalten. Lage der Bänder vom Fußpunkt aus
namespace {

constexpr int kTorsoFromFoot = 14;  // Rumpf beginnt 14 Zeilen über dem Fußpunkt (Zeichenfläche: Y + 13)
constexpr int kLegsFromFoot = 5;    // Beine 5 Zeilen darüber

// Quellzeile je Zeile des fertigen Bildes (Höhe h + kStatureTop, Fußpunkt bleibt unten); -1 = leer
void stature_rows(const Stature& st, int h, int foot, int8_t* out) {
    int8_t src[64];
    int n = 0;
    const int torso_at = foot - kTorsoFromFoot + 4, legs_at = foot - kLegsFromFoot + 1;
    const int torso = st.height - st.stoop, legs = st.height;
    for (int y = 0; y < h && n < 60; ++y) {
        const int k = y == legs_at ? legs : y == torso_at ? torso : 0;
        // n > 0: diese und die folgenden Zeilen doppelt; n < 0: ab hier weglassen
        if (k > 0) {
            for (int j = 0; j < k && y + j < h; ++j) src[n++] = static_cast<int8_t>(y + j), src[n++] = static_cast<int8_t>(y + j);
            y += k - 1;
        } else if (k < 0) {
            y += -k - 1;
        } else {
            src[n++] = static_cast<int8_t>(y);
        }
    }
    int at_foot = 0;
    for (int i = 0; i < n; ++i)
        if (src[i] == foot) at_foot = i;
    const int off = at_foot - (foot + FigureArt::kStatureTop);
    for (int y = 0; y < h + FigureArt::kStatureTop; ++y) out[y] = y + off >= 0 && y + off < n ? src[y + off] : -1;
}

// Bauch: zusätzliche Spalten je Seite in Rumpfzeile k (0 = erste), rund, unten am stärksten
int belly_at(int k, int b) {
    if (!b || k < 2 || k >= 11) return 0;
    return std::min(b, k == 2 || k == 9 || k == 10 ? 1 : b);
}

// Spaltenabbildung einer Zeile: links ab la um nl, rechts ab ra um nr (je -1 … ), dann um shift nach rechts
struct ColMap {
    int la = 0, nl = 0, ra = 0, nr = 0, shift = 0;
    bool same = true;
    int src(int x) const {
        if (same) return x;
        x -= shift;
        if (x < 0) return -1;
        if (x < la - nl) return x + nl;
        if (x <= la) return la;
        if (x < ra) return x;
        if (x <= ra + nr) return ra;
        return x - nr;
    }
};

enum class Band : uint8_t { Head, Body, Tool };  // Kopf ganz (Haare, Kopfbedeckung), Körper, Werkzeug (starr an der Hand)

ColMap stature_cols(int view, int sy, const Stature& st, bool head, int cx, int foot) {
    ColMap m;
    if (st == Stature{}) return m;
    const bool side = view == 2;
    m.same = false;
    if (head) {  // Kopf unverändert, gebeugt von der Seite zwei vor
        m.la = m.ra = cx;
        m.shift = st.stoop && side ? 2 : 0;
        return m;
    }
    const int by = foot - kTorsoFromFoot, e = belly_at(sy - by, st.belly);
    if (side) {
        m.la = cx - 2, m.nl = st.width;  // Rücken
        m.ra = cx + 2, m.nr = e;         // Bauch nach vorn
        m.shift = st.stoop && sy < by + 3 ? 1 : 0;
    } else {
        m.la = cx - 3, m.ra = cx + 2;
        m.nl = m.nr = st.width + e;
    }
    return m;
}

}  // namespace

void FigureArt::compose_frame(Slot& s, int frame) {
    const int out_h = height_ + kStatureTop, n = width_ * out_h, in_n = width_ * height_;
    uint8_t* dst = s.pixels + static_cast<size_t>(frame) * n;
    const int view = frame / kFigurePoses, pose = frame % kFigurePoses;
    const int bob = pose == 1 || pose == 3 || pose == 4;  // Gehbilder und atmen: Kopf eine Zeile tiefer
    const int by = pivot_y_ - kTorsoFromFoot;
    const Stature& st = s.look.build;
    int8_t rows[kMaxHeight + FigureArt::kStatureTop];
    stature_rows(st, height_, pivot_y_, rows);
    // Spaltentabellen einmal je Bild: Kopf, Werkzeug (starr an der Hand), Körper je Quellzeile
    if (!scratch_) scratch_ = static_cast<uint8_t*>(asset_alloc(kScratchBytes));
    if (!scratch_) return;
    uint8_t* const img = scratch_;
    int8_t* const cols = reinterpret_cast<int8_t*>(scratch_ + kScratchImage);
    int8_t head[kMaxWidth], tool[kMaxWidth];
    auto fill = [&](int8_t* xs, const ColMap& m) {
        for (int x = 0; x < width_; ++x) {
            const int sx = m.src(x);
            xs[x] = static_cast<int8_t>(sx >= 0 && sx < width_ ? sx : -1);
        }
    };
    fill(head, stature_cols(view, by, st, true, pivot_x_, pivot_y_));
    fill(tool, stature_cols(view, by + 7, st, false, pivot_x_, pivot_y_));
    for (int sy = by; sy < height_; ++sy) fill(cols + sy * kMaxWidth, stature_cols(view, sy, st, false, pivot_x_, pivot_y_));
    // Ebenen in fester Reihenfolge (Körper, Beine, Oberteil, Haare, Kopfbedeckung, Werkzeug), zeilenweise mit Statur
    // und Farbreihe
    std::memset(img, 0, static_cast<size_t>(n));  // Zeilen über der Figur bleiben leer
    for (int l = 0; l < kLookLayers; ++l) {
        const int v = s.look.part[l];
        if (v < 1 || v > kVariants || !parts_[l][v - 1].pixels) continue;
        const uint8_t* src = parts_[l][v - 1].pixels + static_cast<size_t>(frame) * in_n;
        const int row = s.look.row[l];
        const uint8_t* map = rows_ && row > 0 && row < row_count_ ? rows_[row].table() : nullptr;
        const Band band = l == static_cast<int>(LookLayer::Hair) || l == static_cast<int>(LookLayer::Hat) ? Band::Head
                          : l == static_cast<int>(LookLayer::Tool)                                       ? Band::Tool
                                                                                                         : Band::Body;
        for (int y = 0; y < out_h; ++y) {
            const int sy = rows[y];
            if (sy < 0) continue;
            const int8_t* xs = band == Band::Tool                    ? tool
                               : band == Band::Head || sy < by + bob ? head
                                                                     : cols + sy * kMaxWidth;
            const uint8_t* srow = src + sy * width_;
            uint8_t* line = img + y * width_;
            for (int x = 0; x < width_; ++x)
                if (xs[x] >= 0)
                    if (const uint8_t c = srow[xs[x]]) line[x] = map ? map[c] : c;
        }
    }
    // Kontur nur außen: leere Punkte neben der Figur bekommen die dunkle Fassung des Nachbarn (unten, links,
    // rechts, oben zuerst wie in der Skizze); gelesen aus dem Puffer, geschrieben zeilenweise in den Grafik-Speicher
    const uint8_t* ink = outline_;
    uint8_t line[kMaxWidth];
    for (int y = 0; y < out_h; ++y) {
        const uint8_t* r = img + y * width_;
        for (int x = 0; x < width_; ++x) {
            uint8_t c = r[x];
            if (!c && ink) {
                if (y + 1 < out_h && r[x + width_]) c = r[x + width_];
                else if (x > 0 && r[x - 1]) c = r[x - 1];
                else if (x + 1 < width_ && r[x + 1]) c = r[x + 1];
                else if (y > 0 && r[x - width_]) c = r[x - width_];
                if (c) c = ink[c];
            }
            line[x] = c;
        }
        std::memcpy(dst + y * width_, line, static_cast<size_t>(width_));
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
