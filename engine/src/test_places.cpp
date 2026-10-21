#include "pixelia/engine/test_places.h"

#include <algorithm>
#include <cstring>
#include <new>

#include "pixelia/engine/font.h"
#include "pixelia/engine/memory.h"
#include "pixelia/engine/system_ui.h"

namespace pixelia {

namespace {

// Palette: beide Orte gleich aufgebaut, getönt wird am Ende (Wiese kühl, Raum warm)
constexpr uint8_t kOutline = 1;
constexpr uint8_t kGrass = 2;   // 2..5
constexpr uint8_t kPath = 6;    // 6..8
constexpr uint8_t kTuft = 9;
constexpr uint8_t kBloom = 10;  // 10..11
constexpr uint8_t kFloor = 12;  // 12..15
constexpr uint8_t kWall = 16;   // 16..19
constexpr uint8_t kRamp = 20;   // 20..23 graue Platzhalter-Rampe, wird per Farbreihe ersetzt
constexpr uint8_t kRows = 24;   // 24..47: sechs Farbreihen zu je 4 Farben
constexpr int kRowCount = 6;
constexpr uint8_t kSkin = 48;  // 48..50
constexpr uint8_t kHair = 51;  // 51..53
constexpr uint8_t kEye = 54;
constexpr uint8_t kShadow = 57;

constexpr int kFigW = 16, kFigH = 32, kPoses = 5;  // Posen: stehen, Schritt, Mitte, Schritt, atmen
constexpr int kBoxW = 16, kBoxH = 16;
constexpr int kTileCount = 10;
constexpr int kMapW = kCoarseWidth / 8, kMapH = kCoarseHeight / 8;

constexpr AnimFrame kWalkFrames[] = {{1, 6}, {2, 6}, {3, 6}, {2, 6}};
constexpr AnimFrame kIdleFrames[] = {{0, 54}, {4, 18}};
constexpr Animation kWalk{kWalkFrames, 4, AnimMode::Loop};
constexpr Animation kIdle{kIdleFrames, 2, AnimMode::Loop};

// Gemeinsame Grafiken beider Orte (einmal beim ersten init() gezeichnet, ~10 KB im Grafik-Speicher, memory.h)
struct Kit {
    uint8_t figure[3 * kPoses * kFigW * kFigH];
    uint8_t box[kBoxW * kBoxH];
    uint8_t tiles[kTileCount][64];
    Recolor rows[kRowCount];
};
Kit* s_kit = nullptr;

struct Paint {
    uint8_t* px;
    int w, h;
    void dot(int x, int y, uint8_t c) {
        if (x >= 0 && y >= 0 && x < w && y < h) px[y * w + x] = c;
    }
    void rect(int x, int y, int rw, int rh, uint8_t c) {
        for (int yy = y; yy < y + rh; ++yy)
            for (int xx = x; xx < x + rw; ++xx) dot(xx, yy, c);
    }
    // Umriss: jeder durchsichtige Punkt neben einem gezeichneten wird dunkel
    void outline(uint8_t c) {
        uint8_t copy[kFigW * kFigH];
        std::memcpy(copy, px, static_cast<size_t>(w) * h);
        auto solid = [&](int x, int y) { return x >= 0 && y >= 0 && x < w && y < h && copy[y * w + x] != 0; };
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (!copy[y * w + x] && (solid(x - 1, y) || solid(x + 1, y) || solid(x, y - 1) || solid(x, y + 1)))
                    px[y * w + x] = c;
    }
};

// Kopf 12 × 13, abgerundet (Einzug je Zeile)
constexpr int8_t kHeadInset[13] = {2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2};

// view: 0 vorn, 1 hinten, 2 Seite (Blick nach rechts); Fußpunkt (8, 30), 28 Bildpunkte hoch
void paint_figure(uint8_t* px, int view, int pose) {
    Paint p{px, kFigW, kFigH};
    std::memset(px, 0, kFigW * kFigH);
    const int lift = pose == 2 ? -1 : 0;  // Mitte des Schritts: Körper eine Zeile höher
    const int drop = pose == 4 ? 1 : 0;   // Atmen: Kopf sinkt eine Zeile

    // Beine bis Zeile 30
    const int top = 27 + lift;
    if (view == 2) {
        int front = 6, back = 6;
        if (pose == 1) front = 8, back = 4;
        if (pose == 3) front = 4, back = 8;
        p.rect(back, top, 3, 31 - top, kHair + 2);
        p.rect(front, top, 3, 31 - top, kHair + 1);
        p.rect(front, 30, 3, 1, kHair + 2);
    } else {
        const int l_end = pose == 1 ? 29 : 30, r_end = pose == 3 ? 29 : 30;
        p.rect(4, top, 3, l_end - top + 1, kHair + 1);
        p.rect(9, top, 3, r_end - top + 1, kHair + 1);
        p.rect(4, l_end, 3, 1, kHair + 2);
        p.rect(9, r_end, 3, 1, kHair + 2);
    }

    // Körper, Zeilen 16–26, in der Platzhalter-Rampe (hell links, dunkel rechts, Saum unten)
    const int by = 16 + lift;
    if (view == 2) {
        p.rect(5, by, 6, 1, kRamp + 1);
        p.rect(4, by + 1, 8, 10, kRamp + 1);
        p.rect(4, by + 1, 2, 10, kRamp);
        p.rect(10, by + 1, 2, 10, kRamp + 2);
        p.rect(4, by + 10, 8, 1, kRamp + 3);
        const int ax = pose == 1 ? 8 : pose == 3 ? 6 : 7;  // Arm schwingt mit
        p.rect(ax, by + 1, 2, 7, kRamp + 2);
        p.rect(ax, by + 8, 2, 1, kSkin + 1);
    } else {
        p.rect(4, by, 8, 1, kRamp + 1);
        p.rect(3, by + 1, 10, 10, kRamp + 1);
        p.rect(3, by + 1, 2, 10, kRamp);
        p.rect(11, by + 1, 2, 10, kRamp + 2);
        p.rect(3, by + 10, 10, 1, kRamp + 3);
        const int swing = pose == 1 ? -1 : pose == 3 ? 1 : 0;
        p.rect(2, by + 1, 1, 7 + swing, kRamp + 2);
        p.dot(2, by + 8 + swing, kSkin + 1);
        p.rect(13, by + 1, 1, 7 - swing, kRamp + 2);
        p.dot(13, by + 8 - swing, kSkin + 1);
    }

    // Kopf, Zeilen 3–15 (≈ 45 % der Figur)
    const int hy = 3 + lift + drop;
    auto head_row = [&](int r, uint8_t c) { p.rect(2 + kHeadInset[r], hy + r, 12 - 2 * kHeadInset[r], 1, c); };
    for (int r = 0; r < 13; ++r) head_row(r, kSkin);
    for (int r = 2; r < 11; ++r) p.dot(13, hy + r, kSkin + 1);
    p.rect(4, hy + 12, 8, 1, kSkin + 1);
    if (view == 0) {
        for (int r = 0; r < 4; ++r) head_row(r, kHair);
        p.rect(2, hy + 4, 2, 4, kHair);
        p.rect(12, hy + 4, 2, 4, kHair + 1);
        p.rect(5, hy + 4, 2, 1, kHair);
        p.rect(5, hy + 7, 1, 2, kEye);
        p.rect(10, hy + 7, 1, 2, kEye);
    } else if (view == 1) {
        for (int r = 0; r < 12; ++r) head_row(r, kHair);
        for (int r = 3; r < 12; ++r) p.dot(13, hy + r, kHair + 1);
        head_row(12, kHair + 1);
    } else {
        for (int r = 0; r < 4; ++r) head_row(r, kHair);
        p.rect(2, hy + 4, 5, 7, kHair);
        p.rect(3, hy + 11, 3, 1, kHair + 1);
        p.rect(11, hy + 7, 1, 2, kEye);
    }
    p.outline(kOutline);
}

// Kiste 16 × 16 in der Draufsicht von schräg oben: Deckel hell, Vorderseite mit Brettern; Fußpunkt (8, 15)
void paint_box(uint8_t* px) {
    Paint p{px, kBoxW, kBoxH};
    std::memset(px, 0, kBoxW * kBoxH);
    p.rect(1, 1, 14, 5, kRamp);
    p.rect(1, 6, 14, 8, kRamp + 1);
    p.rect(1, 6, 14, 1, kRamp + 2);
    p.rect(5, 7, 1, 7, kRamp + 2);
    p.rect(10, 7, 1, 7, kRamp + 2);
    p.rect(1, 14, 14, 1, kRamp + 3);
    p.outline(kOutline);
}

void paint_tiles(uint8_t (*t)[64]) {
    auto fill = [&](int i, uint8_t c) { std::memset(t[i], c, 64); };
    auto dot = [&](int i, int x, int y, uint8_t c) { t[i][y * 8 + x] = c; };
    // 0, 1: Gras im Schachbrett (Raster der Kacheln bleibt sichtbar), 2: Büschel, 3: Blüte, 4: Weg
    fill(0, kGrass + 1);
    dot(0, 1, 2, kGrass);
    dot(0, 5, 6, kGrass + 2);
    fill(1, kGrass + 2);
    dot(1, 2, 5, kGrass + 1);
    dot(1, 6, 1, kGrass + 3);
    std::memcpy(t[2], t[0], 64);
    dot(2, 2, 3, kTuft);
    dot(2, 4, 3, kTuft);
    dot(2, 3, 4, kTuft);
    dot(2, 3, 5, kGrass + 3);
    std::memcpy(t[3], t[0], 64);
    dot(3, 4, 3, kBloom);
    dot(3, 3, 4, kBloom);
    dot(3, 5, 4, kBloom);
    dot(3, 4, 4, kBloom + 1);
    dot(3, 4, 5, kGrass + 3);
    fill(4, kPath + 1);
    dot(4, 1, 1, kPath);
    dot(4, 5, 4, kPath);
    dot(4, 3, 6, kPath + 2);
    dot(4, 6, 2, kPath + 2);
    // 5, 6: Dielen (versetzte Stöße), 7: Wand, 8: Wand mit Fußleiste, 9: Wand unter der Decke
    for (int i = 5; i <= 6; ++i) {
        fill(i, kFloor + 1);
        for (int x = 0; x < 8; ++x) {
            dot(i, x, 0, kFloor);
            dot(i, x, 3, kFloor + 3);
            dot(i, x, 4, kFloor);
            dot(i, x, 7, kFloor + 3);
        }
        const int a = i == 5 ? 5 : 1, b = i == 5 ? 2 : 6;
        for (int y = 0; y < 3; ++y) dot(i, a, y, kFloor + 2);
        for (int y = 4; y < 7; ++y) dot(i, b, y, kFloor + 2);
    }
    fill(7, kWall + 1);
    for (int y = 0; y < 8; ++y) {
        dot(7, 3, y, kWall + 2);
        dot(7, 7, y, kWall);
    }
    std::memcpy(t[8], t[7], 64);
    for (int x = 0; x < 8; ++x) {
        dot(8, x, 5, kWall + 2);
        dot(8, x, 6, kWall + 3);
        dot(8, x, 7, kWall + 3);
    }
    std::memcpy(t[9], t[7], 64);
    for (int x = 0; x < 8; ++x) {
        dot(9, x, 0, kWall + 3);
        dot(9, x, 1, kWall + 3);
        dot(9, x, 2, kWall + 3);
        dot(9, x, 3, kWall + 2);
    }
}

const Kit& kit() {
    if (!s_kit) {
        void* mem = asset_alloc(sizeof(Kit));
        Kit* k = mem ? new (mem) Kit : new Kit;
        for (int v = 0; v < 3; ++v)
            for (int pose = 0; pose < kPoses; ++pose)
                paint_figure(k->figure + (v * kPoses + pose) * kFigW * kFigH, v, pose);
        paint_box(k->box);
        paint_tiles(k->tiles);
        for (int r = 0; r < kRowCount; ++r) k->rows[r].ramp(kRamp, static_cast<uint8_t>(kRows + r * 4), 4);
        s_kit = k;  // bleibt bis zum Ende (wird nie freigegeben)
    }
    return *s_kit;
}

constexpr SpriteSheet figure_sheet(const Kit& k) { return {k.figure, kFigW, kFigH, 3 * kPoses, 8, 30}; }

Color tint(Color c, int r256, int g256, int b256) {
    auto ch = [](int v, int k) { return static_cast<uint8_t>(std::min(255, v * k >> 8)); };
    return {ch(c.r, r256), ch(c.g, g256), ch(c.b, b256)};
}

}  // namespace

void TestPlace::build_palette() {
    Palette& p = palette_;
    p.fill({0, 0, 0});
    p[kOutline] = {28, 26, 36};
    const Color grass[4] = {{112, 150, 110}, {96, 134, 98}, {82, 118, 86}, {66, 100, 74}};
    const Color path[3] = {{186, 172, 138}, {166, 152, 120}, {142, 128, 100}};
    const Color floor[4] = {{164, 124, 90}, {146, 108, 78}, {126, 92, 66}, {104, 74, 54}};
    const Color wall[4] = {{206, 194, 178}, {186, 174, 158}, {160, 148, 134}, {128, 116, 106}};
    const Color ramp[4] = {{200, 200, 204}, {164, 164, 170}, {128, 128, 136}, {92, 92, 100}};
    // Farbreihen: rot, blau, gelb, lavendel, grün, türkis
    const Color rows[kRowCount][4] = {
        {{228, 106, 98}, {198, 80, 76}, {162, 60, 62}, {122, 46, 52}},
        {{106, 152, 224}, {80, 122, 198}, {62, 96, 162}, {48, 72, 122}},
        {{238, 206, 98}, {216, 176, 70}, {182, 142, 54}, {142, 106, 44}},
        {{178, 150, 222}, {150, 120, 198}, {124, 92, 168}, {96, 70, 132}},
        {{134, 202, 122}, {102, 172, 98}, {78, 140, 82}, {58, 106, 66}},
        {{110, 202, 198}, {82, 170, 172}, {62, 136, 142}, {46, 102, 110}},
    };
    for (int i = 0; i < 4; ++i) {
        p[kGrass + i] = grass[i];
        p[kFloor + i] = floor[i];
        p[kWall + i] = wall[i];
        p[kRamp + i] = ramp[i];
        for (int r = 0; r < kRowCount; ++r) p[kRows + r * 4 + i] = rows[r][i];
    }
    for (int i = 0; i < 3; ++i) p[kPath + i] = path[i];
    p[kTuft] = {130, 168, 118};
    p[kBloom] = {238, 234, 222};
    p[kBloom + 1] = {222, 180, 86};
    p[kSkin] = {238, 216, 192};
    p[kSkin + 1] = {216, 188, 162};
    p[kSkin + 2] = {182, 152, 130};
    p[kHair] = {84, 66, 60};
    p[kHair + 1] = {64, 50, 46};
    p[kHair + 2] = {46, 36, 34};
    p[kEye] = {32, 28, 38};
    p[kShadow] = kind_ == Kind::Field ? Color{70, 100, 78} : Color{100, 74, 56};
    // Licht des Ortes: Wiese kühl, Raum warm (zwei verschiedene Paletten für die Überblendung)
    for (int i = 1; i < kColors; ++i)
        p[i] = kind_ == Kind::Field ? tint(p[i], 246, 256, 268) : tint(p[i], 268, 248, 222);
}

void TestPlace::build_map() {
    for (int ty = 0; ty < kMapH; ++ty) {
        for (int tx = 0; tx < kMapW; ++tx) {
            uint8_t t;
            if (kind_ == Kind::Room) {
                t = ty == 0 ? 9 : ty < 3 ? 7 : ty == 3 ? 8 : (ty & 1) ? 6 : 5;
            } else if (ty == 10 || tx == 6) {
                t = 4;
            } else {
                const uint32_t r = rng_.below(100);
                t = r < 7 ? 2 : r < 10 ? 3 : static_cast<uint8_t>((tx + ty) & 1);
            }
            map_[ty][tx] = t;
        }
    }
}

int TestPlace::area_top() const { return kind_ == Kind::Field ? 34 : 58; }

void TestPlace::pick_target(Figure& f) {
    const int margin = kind_ == Kind::Field ? 10 : 14;
    f.tx = static_cast<int16_t>(rng_.range(margin, kCoarseWidth - margin));
    f.ty = static_cast<int16_t>(rng_.range(area_top(), kCoarseHeight - 4));
}

void TestPlace::spawn(Figure& f) {
    f = Figure{};
    pick_target(f);
    f.x = f.tx << 8;
    f.y = f.ty << 8;
    f.wait = static_cast<uint16_t>(rng_.range(1, 120));
    f.speed = static_cast<uint8_t>(rng_.range(150, 210));
    f.facing = static_cast<Facing>(rng_.below(4));
    f.anim.play(&kIdle);
}

void TestPlace::init(uint64_t seed) {
    kit();
    rng_.reseed(seed);
    build_palette();
    build_map();
    boxes_count_ = 0;
    if (kind_ == Kind::Room) {
        const int16_t xs[] = {26, 44, 62, 138, 156, 174};
        for (int i = 0; i < 6; ++i) boxes_[boxes_count_++] = {xs[i], static_cast<int16_t>(i == 1 || i == 4 ? 50 : 47),
                                                             static_cast<uint8_t>(i % kRowCount)};
    }
    count_ = 0;
    set_load(0);
    while (count_ < wanted_) {
        spawn(figures_[count_]);
        figures_[count_].row = static_cast<uint8_t>(count_ % kRowCount);
        ++count_;
    }
}

void TestPlace::set_load(int figures) {
    wanted_ = figures <= 0 ? (kind_ == Kind::Field ? 6 : 3) : std::min(figures, kMaxFigures);
}

void TestPlace::tick(const WorldClock& /*clock*/) {
    while (count_ < wanted_) {
        spawn(figures_[count_]);
        figures_[count_].row = static_cast<uint8_t>(count_ % kRowCount);
        ++count_;
    }
    if (count_ > wanted_) count_ = wanted_;

    for (int i = 0; i < count_; ++i) {
        Figure& f = figures_[i];
        if (f.wait > 0) {
            --f.wait;
            if (f.wait == 40 && rng_.below(3) == 0) f.facing = static_cast<Facing>(rng_.below(4));  // umschauen
            if (f.wait == 0) pick_target(f);
            f.anim.play(&kIdle);
        } else {
            // erst waagrecht, dann senkrecht: so passen die vier Blickrichtungen immer
            const int32_t gx = f.tx << 8, gy = f.ty << 8;
            if (f.x != gx) {
                const int32_t d = gx - f.x;
                const int32_t s = std::min<int32_t>(f.speed, d > 0 ? d : -d);
                f.x += d > 0 ? s : -s;
                f.facing = d > 0 ? Facing::Right : Facing::Left;
            } else if (f.y != gy) {
                const int32_t d = gy - f.y;
                const int32_t s = std::min<int32_t>(f.speed, d > 0 ? d : -d);
                f.y += d > 0 ? s : -s;
                f.facing = d > 0 ? Facing::Down : Facing::Up;
            } else {
                f.wait = static_cast<uint16_t>(rng_.range(40, 200));
            }
            f.anim.play(f.wait ? &kIdle : &kWalk);
        }
        f.anim.tick();
    }
}

void TestPlace::render(Canvas& c) const {
    const Kit& k = kit();
    for (int ty = 0; ty < kMapH; ++ty)
        for (int tx = 0; tx < kMapW; ++tx) {
            const uint8_t* t = k.tiles[map_[ty][tx]];
            for (int r = 0; r < 8; ++r) std::memcpy(c.row(ty * 8 + r) + tx * 8, t + r * 8, 8);
        }
    if (kind_ == Kind::Room) {
        c.fill_rect(0, 0, 3, kCoarseHeight, kWall + 3);
        c.fill_rect(kCoarseWidth - 3, 0, 3, kCoarseHeight, kWall + 3);
    }
    for (int i = 0; i < count_; ++i) {
        const int x = figures_[i].x >> 8, y = figures_[i].y >> 8;
        c.fill_rect(x - 4, y - 1, 9, 3, kShadow);
        c.fill_rect(x - 5, y, 11, 1, kShadow);
    }

    // Figuren und Kisten nach Fußpunkt: wer weiter unten steht, steht vorn
    struct Item {
        int y, i;
    };
    Item items[kMaxFigures + 6];
    int n = 0;
    for (int i = 0; i < count_; ++i) items[n++] = {figures_[i].y >> 8, i};
    for (int b = 0; b < boxes_count_; ++b) items[n++] = {boxes_[b].y, kMaxFigures + b};
    for (int a = 1; a < n; ++a)
        for (int b = a; b > 0 && (items[b].y < items[b - 1].y ||
                                  (items[b].y == items[b - 1].y && items[b].i < items[b - 1].i));
             --b)
            std::swap(items[b], items[b - 1]);

    const SpriteSheet sheet = figure_sheet(k);
    for (int j = 0; j < n; ++j) {
        const int i = items[j].i;
        if (i >= kMaxFigures) {
            const Box& b = boxes_[i - kMaxFigures];
            c.blit({k.box, kBoxW, kBoxH}, b.x - 8, b.y - 15, false, k.rows[b.row].table());
            continue;
        }
        const Figure& f = figures_[i];
        const View v = view_of(f.facing);
        c.blit(sheet.frame(v.index * kPoses + f.anim.frame()), (f.x >> 8) - sheet.pivot_x,
               (f.y >> 8) - sheet.pivot_y, v.flip, k.rows[f.row].table());
    }
}

void TestPlace::render_overlay(Canvas& fine) const {
    // Feine Beschriftung über dem groben Bild (Farben der System-Oberfläche)
    const char* t = title();
    fine.fill_rect(4, 4, font::text_width(t) + 8, 13, ui::kSurface);
    font::draw(fine, 8, 5, t, ui::kText);
}

}  // namespace pixelia
