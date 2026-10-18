#include "pixelia/engine/scroll_test.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>

namespace pixelia {

namespace {

// Palettenbereiche
constexpr uint8_t kSky = 1;        // 1..32 Himmelsverlauf
constexpr uint8_t kFar = 33;       // 33..35 ferne Berge, 36 Schnee
constexpr uint8_t kSnow = 36;
constexpr uint8_t kNear = 37;      // 37..40 Hügel
constexpr uint8_t kGrass = 41;     // 41..44
constexpr uint8_t kDirt = 45;      // 45..48
constexpr uint8_t kStone = 49;     // 49..52
constexpr uint8_t kFlower = 53;    // 53..56
constexpr uint8_t kWater = 57;     // 57..72, Farbrotation
constexpr uint8_t kBird = 73;      // 73 Körper, 74 Flügel, 75 Schnabel
constexpr uint8_t kCloud = 77;     // 77 hell, 78 Schatten
constexpr uint8_t kStar = 79;
constexpr uint8_t kLitEnd = 80;    // 33..79 werden vom Tageslicht eingefärbt

constexpr int kSkyRows = 160;
constexpr int kFarBase = 150;
constexpr int kNearBase = 176;
constexpr int kGroundTop = 176;
constexpr int kWaterTop = 208;

constexpr uint8_t kBayer4[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

constexpr const char* kBirdArt[2][8] = {
    {"..2.......2.", "...2.....2..", "....2...2...", ".....111....", "..1111113...", ".....11.....",
     "............", "............"},
    {"............", "............", ".....111....", "..1111113...", "....2...2...", "...2.....2..",
     "..2.......2.", "............"},
};

// Tageslicht über 24 Stunden: Himmel oben, Himmel am Horizont, Licht auf der Landschaft (Faktor pro Kanal)
struct LightKey {
    float hour;
    Color top, horizon;
    float lr, lg, lb;
};
constexpr LightKey kDay[] = {
    {0.0f, {6, 8, 26}, {24, 30, 66}, 0.28f, 0.32f, 0.50f},
    {5.0f, {10, 14, 40}, {40, 40, 85}, 0.32f, 0.34f, 0.52f},
    {6.0f, {40, 55, 120}, {230, 140, 100}, 0.75f, 0.60f, 0.60f},
    {7.5f, {30, 70, 150}, {200, 200, 220}, 0.95f, 0.92f, 0.90f},
    {9.0f, {22, 52, 128}, {178, 214, 242}, 1.00f, 1.00f, 1.00f},
    {17.0f, {22, 52, 128}, {178, 214, 242}, 1.00f, 1.00f, 1.00f},
    {18.5f, {40, 60, 140}, {245, 160, 90}, 1.00f, 0.80f, 0.65f},
    {19.5f, {30, 30, 80}, {160, 80, 90}, 0.60f, 0.45f, 0.50f},
    {21.0f, {8, 10, 30}, {30, 32, 70}, 0.30f, 0.33f, 0.50f},
    {24.0f, {6, 8, 26}, {24, 30, 66}, 0.28f, 0.32f, 0.50f},
};

Color lerp(Color a, Color b, float t) {
    return {static_cast<uint8_t>(a.r + (b.r - a.r) * t), static_cast<uint8_t>(a.g + (b.g - a.g) * t),
            static_cast<uint8_t>(a.b + (b.b - a.b) * t)};
}

Color scale(Color c, float r, float g, float b) {
    return {static_cast<uint8_t>(std::min(255.0f, c.r * r)), static_cast<uint8_t>(std::min(255.0f, c.g * g)),
            static_cast<uint8_t>(std::min(255.0f, c.b * b))};
}

}  // namespace

void ScrollTest::init(uint64_t seed) {
    rng_.reseed(seed);
    build_palette();
    build_terrain();
    build_tiles();
    build_sprites();
    for (int i = 0; i < 32; ++i) bob_[i] = static_cast<int8_t>(std::lround(3.0 * std::sin(i * 6.2831853 / 32)));
    for (auto& s : stars_) {
        s.x = static_cast<int16_t>(rng_.below(kScreenWidth));
        s.y = static_cast<int16_t>(rng_.below(110));
        s.phase = static_cast<uint8_t>(rng_.below(256));
    }
    ticks_ = 0;
    cloud_x_ = far_x_ = near_x_ = ground_x_ = water_x_ = 0;
    apply_daylight(12.0);
}

void ScrollTest::build_palette() {
    Palette& p = base_palette_;
    p.fill({0, 0, 0});
    for (int i = 0; i < 32; ++i) p[kSky + i] = lerp({22, 52, 128}, {178, 214, 242}, i / 31.0f);
    p[kFar + 0] = {104, 122, 160};
    p[kFar + 1] = {88, 104, 142};
    p[kFar + 2] = {74, 88, 124};
    p[kSnow] = {236, 242, 250};
    p[kNear + 0] = {74, 142, 74};
    p[kNear + 1] = {60, 124, 64};
    p[kNear + 2] = {46, 104, 54};
    p[kNear + 3] = {34, 84, 46};
    p[kGrass + 0] = {98, 176, 72};
    p[kGrass + 1] = {78, 154, 60};
    p[kGrass + 2] = {60, 130, 52};
    p[kGrass + 3] = {44, 104, 44};
    p[kDirt + 0] = {150, 108, 70};
    p[kDirt + 1] = {128, 90, 58};
    p[kDirt + 2] = {106, 74, 48};
    p[kDirt + 3] = {84, 58, 40};
    p[kStone + 0] = {150, 150, 158};
    p[kStone + 1] = {124, 124, 134};
    p[kStone + 2] = {100, 100, 112};
    p[kStone + 3] = {76, 76, 90};
    p[kFlower + 0] = {230, 60, 60};
    p[kFlower + 1] = {250, 210, 60};
    p[kFlower + 2] = {250, 250, 250};
    p[kFlower + 3] = {170, 90, 220};
    for (int i = 0; i < 16; ++i) {
        float t = 0.5f + 0.5f * std::sin(i * 6.2831853f / 16);
        p[kWater + i] = lerp({20, 70, 140}, {110, 180, 230}, t);
    }
    p[kBird + 0] = {36, 36, 48};
    p[kBird + 1] = {84, 84, 104};
    p[kBird + 2] = {240, 176, 40};
    p[kCloud + 0] = {250, 250, 255};
    p[kCloud + 1] = {214, 224, 240};
    p[kStar] = {255, 252, 230};
    p[255] = {255, 255, 255};
    palette_ = p;
}

void ScrollTest::build_terrain() {
    // Periodische Höhenprofile (512 Spalten) aus Sinus-Summen mit zufälliger Phase
    const float tau = 6.2831853f;
    float p1 = rng_.below(1000) / 1000.0f * tau, p2 = rng_.below(1000) / 1000.0f * tau;
    float p3 = rng_.below(1000) / 1000.0f * tau, p4 = rng_.below(1000) / 1000.0f * tau;
    for (int x = 0; x < 512; ++x) {
        float u = x * tau / 512;
        float f = 42 + 18 * std::sin(3 * u + p1) + 10 * std::sin(7 * u + p2) + 4 * std::sin(19 * u + p3);
        far_[x] = static_cast<uint8_t>(std::clamp(f + rng_.range(-1, 1), 10.0f, 75.0f));
        float n = 24 + 9 * std::sin(5 * u + p4) + 5 * std::sin(11 * u + p1) + 2 * std::sin(29 * u + p2);
        near_[x] = static_cast<uint8_t>(std::clamp(n, 6.0f, 40.0f));
    }
    // Himmel: 32 Stufen über 160 Zeilen, Zwischenstufen per 4×4-Bayer-Dithering
    for (int y = 0; y < kScreenHeight; ++y) {
        float t = std::min(y, kSkyRows - 1) * 31.0f / (kSkyRows - 1);
        int s = static_cast<int>(t);
        int frac = static_cast<int>((t - s) * 16);
        uint32_t pat = 0;
        for (int i = 0; i < 4; ++i) {
            uint8_t c = kSky + std::min(s + (frac > kBayer4[y & 3][i] ? 1 : 0), 31);
            pat |= static_cast<uint32_t>(c) << (8 * i);
        }
        sky_rows_[y] = pat;
    }
    // Wasser: Wellenmuster, 128 Pixel breit und periodisch
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 128; ++x) {
            float w = std::sin(x * tau / 64 + y * 0.7f) * 3 + std::sin(x * tau / 32 - y * 0.45f) * 2;
            water_[y][x] = kWater + ((static_cast<int>(w + 8) + y / 2) & 15);
        }
    for (int i = 0; i < kClouds; ++i) {
        clouds_[i][0] = static_cast<int16_t>(i * 800 / kClouds + rng_.range(-40, 40));
        clouds_[i][1] = static_cast<int16_t>(rng_.range(8, 70));
    }
}

void ScrollTest::build_tiles() {
    // 0..2: Gras-Oberkanten, 3: Gras mit Blumen, 4: Erde, 5: Erde mit Steinen, 6: Stein, 7: Erde dunkel
    for (int t = 0; t < 8; ++t) {
        uint8_t* px = tiles_[t];
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x) {
                uint8_t c;
                if (t <= 3) {
                    int edge = 4 + ((x * 7 + t * 3) % 3);
                    c = y < edge ? kGrass + static_cast<uint8_t>(rng_.below(3)) : kDirt + static_cast<uint8_t>(rng_.below(3));
                    if (t == 3 && y == 2 && (x % 5) == 2) c = kFlower + static_cast<uint8_t>(rng_.below(4));
                } else if (t == 6) {
                    c = kStone + static_cast<uint8_t>(((x / 4 + y / 4) & 1) + rng_.below(2));
                } else {
                    c = kDirt + static_cast<uint8_t>(rng_.below(2)) + (t == 7 ? 2 : 0);
                    if (t == 5 && rng_.below(10) == 0) c = kStone + 1;
                }
                px[y * 16 + x] = c;
            }
    }
    for (int i = 0; i < 64; ++i) {
        map_[0][i] = static_cast<uint8_t>(rng_.below(4));
        map_[1][i] = static_cast<uint8_t>(4 + rng_.below(4));
    }
}

void ScrollTest::build_sprites() {
    for (int f = 0; f < 2; ++f)
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 12; ++x) {
                char ch = kBirdArt[f][y][x];
                bird_px_[f][y * 12 + x] = ch == '1' ? kBird : ch == '2' ? kBird + 1 : ch == '3' ? kBird + 2 : 0;
            }
    // Wolke: Vereinigung dreier Ellipsen, untere Hälfte schattiert
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 48; ++x) {
            auto in = [&](float cx, float cy, float rx, float ry) {
                float dx = (x - cx) / rx, dy = (y - cy) / ry;
                return dx * dx + dy * dy <= 1.0f;
            };
            bool inside = in(14, 10, 12, 6) || in(26, 7, 13, 7) || in(37, 10, 10, 5);
            cloud_px_[y * 48 + x] = inside ? (y > 10 ? kCloud + 1 : kCloud) : 0;
        }
    birds_.resize(kMaxSprites);
    for (auto& b : birds_) {
        b.x = rng_.range(-16, kScreenWidth) << 8;
        b.y = static_cast<int16_t>(rng_.range(6, 150));
        b.speed = static_cast<int16_t>(rng_.range(160, 420));
        b.phase = static_cast<uint8_t>(rng_.below(32));
        b.frame = 0;
    }
}

void ScrollTest::set_sprite_count(int n) { sprite_count_ = std::clamp(n, 0, kMaxSprites); }

void ScrollTest::apply_daylight(double hours) {
    // Stützstellen der Tageskurve finden und linear überblenden
    int k = 0;
    while (k + 1 < static_cast<int>(std::size(kDay)) - 1 && kDay[k + 1].hour <= hours) ++k;
    const LightKey& a = kDay[k];
    const LightKey& b = kDay[k + 1];
    float t = static_cast<float>((hours - a.hour) / (b.hour - a.hour));
    t = std::clamp(t, 0.0f, 1.0f);
    Color top = lerp(a.top, b.top, t), horizon = lerp(a.horizon, b.horizon, t);
    float lr = a.lr + (b.lr - a.lr) * t, lg = a.lg + (b.lg - a.lg) * t, lb = a.lb + (b.lb - a.lb) * t;

    for (int i = 0; i < 32; ++i) palette_[kSky + i] = lerp(top, horizon, i / 31.0f);
    int rot = static_cast<int>((ticks_ / 3) & 15);  // Farbrotation des Wassers
    for (int i = kFar; i < kLitEnd; ++i) {
        Color base = base_palette_[i];
        if (i >= kWater && i < kWater + 16) base = base_palette_[kWater + ((i - kWater + rot) & 15)];
        palette_[i] = scale(base, lr, lg, lb);
    }
    // Nacht: 0 bei vollem Tageslicht, 255 bei tiefer Nacht
    float night = std::clamp((0.85f - lg) / 0.5f, 0.0f, 1.0f);
    night_ = static_cast<uint8_t>(night * 255);
    palette_[kStar] = lerp(top, base_palette_[kStar], night);
    // Vögel schlafen nachts, bis auf ein paar
    visible_birds_ = static_cast<int>(sprite_count_ * (1.0f - 0.9f * night) + 0.5f);
}

void ScrollTest::tick(const WorldClock& clock) {
    ++ticks_;
    cloud_x_ += 32;    // 0,125 px pro Schritt
    far_x_ += 64;      // 0,25 px
    near_x_ += 192;    // 0,75 px
    ground_x_ += 512;  // 2 px
    water_x_ += 256;   // 1 px
    for (int i = 0; i < sprite_count_; ++i) {
        Bird& b = birds_[i];
        b.x += b.speed;
        if (b.x > (kScreenWidth + 16) << 8) {
            b.x = -16 << 8;
            b.y = static_cast<int16_t>(rng_.range(6, 150));
        }
        ++b.phase;
        b.frame = static_cast<uint8_t>(((ticks_ >> 2) + i) & 1);
    }
    apply_daylight(clock.hours());
}

void ScrollTest::render(Canvas& c) const {
    // Himmel
    for (int y = 0; y < kSkyRows; ++y) c.fill_row_pattern(y, sky_rows_[y]);

    // Sterne, leicht funkelnd
    if (night_ > 20) {
        for (const Star& s : stars_) {
            if (((ticks_ >> 3) + s.phase) % 13 == 0) continue;
            c.row(s.y)[s.x] = kStar;
        }
    }

    // Wolken (Welt 800 px breit, wiederholt)
    const Sprite cloud{cloud_px_, 48, 16};
    int coff = cloud_x_ >> 8;
    for (int i = 0; i < kClouds; ++i) {
        int x = ((clouds_[i][0] - coff) % 800 + 800) % 800 - 48;
        c.blit(cloud, x, clouds_[i][1]);
    }

    // Ferne Berge mit Schneekappen, Licht von links
    int foff = far_x_ >> 8;
    for (int x = 0; x < kScreenWidth; ++x) {
        int h = far_[(x + foff) & 511];
        int hr = far_[(x + foff + 1) & 511];
        int top = kFarBase - h;
        uint8_t col = h > hr ? kFar : (h == hr ? kFar + 1 : kFar + 2);
        int snow = h > 50 ? (h - 50) / 3 + 1 : 0;
        if (snow) c.vline(x, top, top + snow, kSnow);
        c.vline(x, top + snow, kNearBase, col);
    }

    // Nahe Hügel
    int noff = near_x_ >> 8;
    for (int x = 0; x < kScreenWidth; ++x) {
        int wx = x + noff;
        int h = near_[wx & 511];
        int top = kNearBase - h;
        c.vline(x, top, top + 2, kNear);
        c.vline(x, top + 2, kNearBase, kNear + 1 + ((wx * 13 >> 3) % 3 == 0 ? 1 : 0) + (h < 14 ? 1 : 0));
    }

    // Boden aus 16×16-Kacheln
    int goff = ground_x_ >> 8;
    for (int y = kGroundTop; y < kWaterTop; ++y) {
        int ty = (y - kGroundTop) >> 4, py = (y - kGroundTop) & 15;
        uint8_t* dst = c.row(y);
        int x = 0, wx = goff;
        while (x < kScreenWidth) {
            int tx = (wx >> 4) & 63, px = wx & 15;
            int n = std::min(16 - px, kScreenWidth - x);
            std::memcpy(dst + x, tiles_[map_[ty][tx]] + py * 16 + px, n);
            x += n;
            wx += n;
        }
    }

    // Wasser (Muster 128 px breit, jede Zeile leicht versetzt)
    int woff = water_x_ >> 8;
    for (int y = kWaterTop; y < kScreenHeight; ++y) {
        const uint8_t* src = water_[y - kWaterTop];
        uint8_t* dst = c.row(y);
        int x = 0, wx = woff + (y - kWaterTop) * 3;
        while (x < kScreenWidth) {
            int px = wx & 127;
            int n = std::min(128 - px, kScreenWidth - x);
            std::memcpy(dst + x, src + px, n);
            x += n;
            wx += n;
        }
    }

    // Vögel (nachts nur wenige)
    for (int i = 0; i < visible_birds_; ++i) {
        const Bird& b = birds_[i];
        const Sprite s{bird_px_[b.frame], 12, 8};
        c.blit(s, b.x >> 8, b.y + bob_[b.phase & 31]);
    }
}

}  // namespace pixelia
