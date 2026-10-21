#include "pixelia/engine/light.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pixelia {

namespace {

constexpr int kDayUnits = 1440 * 256;              // Tageszeit in 1/256 Minuten
constexpr int64_t kUsPerUnit = 60000000 / 256;     // 234 375 µs
constexpr int64_t kUsPerDay = 24LL * 60 * 60000000;

int lerp(int a, int b, int f256) { return a + ((b - a) * f256 >> 8); }
Color lerp(Color a, Color b, int f256) {
    return {static_cast<uint8_t>(lerp(a.r, b.r, f256)), static_cast<uint8_t>(lerp(a.g, b.g, f256)),
            static_cast<uint8_t>(lerp(a.b, b.b, f256))};
}
int factor(uint8_t v) { return v + (v >> 7); }  // 0 … 255 → 0 … 256

// Stützpunkte a, b und Anteil f256 zwischen ihnen; über Mitternacht geht es vom letzten zum ersten weiter
template <typename K>
void bracket(const K* k, int n, int t, int* a, int* b, int* f256) {
    int i = n - 1;
    for (int j = 0; j < n; ++j)
        if (k[j].minute * 256 <= t) i = j;
    const int j = (i + 1) % n;
    int t0 = k[i].minute * 256, t1 = k[j].minute * 256;
    if (t1 <= t0) t1 += kDayUnits;
    if (t < t0) t += kDayUnits;
    *a = i;
    *b = j;
    *f256 = t1 > t0 ? std::clamp((t - t0) * 256 / (t1 - t0), 0, 256) : 0;
}

uint8_t ch(int v) { return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v); }

// Farbe unter Licht: erst Sättigung, dann je Kanal der Lichtfaktor
Color lit(Color c, int lr, int lg, int lb, int sat) {
    const int lum = (77 * c.r + 150 * c.g + 29 * c.b) >> 8;
    const int r = lum + ((c.r - lum) * sat >> 8), g = lum + ((c.g - lum) * sat >> 8), b = lum + ((c.b - lum) * sat >> 8);
    return {ch(r * lr >> 8), ch(g * lg >> 8), ch(b * lb >> 8)};
}

int dist2(Color a, Color b) {
    const int dr = a.r - b.r, dg = a.g - b.g, db = a.b - b.b;
    return 2 * dr * dr + 4 * dg * dg + 3 * db * db;
}

// --- Vorgaben des Tageslichts (Stützpunkte über den Tag) ---
// Himmel: Nacht, Dämmerung, Tag, Sonnenuntergang
constexpr SkyKey kSky[] = {
    {0, {12, 16, 44}, {30, 40, 84}},      {300, {12, 16, 44}, {30, 40, 84}},    {390, {84, 92, 168}, {250, 172, 142}},
    {480, {110, 168, 236}, {196, 222, 246}}, {1080, {110, 168, 236}, {196, 222, 246}},
    {1185, {92, 78, 168}, {252, 142, 92}}, {1260, {34, 32, 86}, {120, 76, 120}}, {1350, {12, 16, 44}, {30, 40, 84}},
};
// 0 natürlich: sanftes Gold am Morgen, Orange am Abend, Blau in der Nacht
constexpr LightKey kNatural[] = {
    {0, {40, 50, 92}, 140, 255},     {300, {40, 50, 92}, 140, 255},    {345, {92, 88, 132}, 170, 220},
    {390, {236, 172, 146}, 220, 110}, {480, {255, 238, 214}, 245, 0},   {660, {255, 255, 255}, 255, 0},
    {960, {255, 248, 232}, 255, 0},   {1110, {255, 206, 156}, 255, 0}, {1185, {222, 136, 118}, 225, 150},
    {1250, {116, 98, 148}, 175, 235}, {1320, {40, 50, 92}, 140, 255},
};
// 1 kräftig: rosa Morgen, roter Abend, violette Dämmerung, tiefblaue Nacht
constexpr LightKey kVivid[] = {
    {0, {30, 44, 104}, 150, 255},     {300, {30, 44, 104}, 150, 255},   {345, {120, 80, 150}, 190, 220},
    {390, {255, 150, 168}, 240, 110}, {480, {255, 232, 206}, 255, 0},   {660, {255, 255, 255}, 255, 0},
    {960, {255, 244, 222}, 255, 0},   {1110, {255, 180, 120}, 255, 0}, {1185, {255, 112, 92}, 240, 150},
    {1250, {150, 82, 172}, 200, 235}, {1320, {30, 44, 104}, 150, 255},
};
// 2 zurückhaltend: kaum Färbung, die Helligkeit trägt die Tageszeit
constexpr LightKey kSubtle[] = {
    {0, {52, 58, 82}, 120, 255},      {300, {52, 58, 82}, 120, 255},    {345, {110, 108, 124}, 160, 220},
    {390, {238, 222, 208}, 230, 110}, {480, {255, 248, 238}, 250, 0},   {660, {255, 255, 255}, 255, 0},
    {960, {255, 252, 244}, 255, 0},   {1110, {250, 226, 200}, 255, 0}, {1185, {218, 186, 170}, 220, 150},
    {1250, {116, 112, 132}, 160, 235}, {1320, {52, 58, 82}, 120, 255},
};

}  // namespace

Daylight::Daylight(const LightKey* keys, int n, const SkyKey* sky, int sky_n) {
    n_ = std::clamp(n, 0, kMaxKeys);
    sky_n_ = std::clamp(sky_n, 0, kMaxKeys);
    std::copy_n(keys, n_, keys_.begin());
    std::copy_n(sky, sky_n_, sky_.begin());
}

Ambient Daylight::at(int64_t us_of_day) const {
    Ambient a;
    int64_t us = us_of_day % kUsPerDay;
    if (us < 0) us += kUsPerDay;
    const int t = static_cast<int>(us / kUsPerUnit);
    int i, j, f;
    if (n_ > 0) {
        bracket(keys_.data(), n_, t, &i, &j, &f);
        const LightKey &p = keys_[i], &q = keys_[j];
        a.r = lerp(factor(p.tint.r), factor(q.tint.r), f);
        a.g = lerp(factor(p.tint.g), factor(q.tint.g), f);
        a.b = lerp(factor(p.tint.b), factor(q.tint.b), f);
        a.sat = lerp(factor(p.sat), factor(q.sat), f);
        a.lamp = lerp(factor(p.lamp), factor(q.lamp), f);
        const int avg = (a.r + a.g + a.b) / 3;
        if (avg > 0 && avg < floor_) {
            a.r = a.r * floor_ / avg;
            a.g = a.g * floor_ / avg;
            a.b = a.b * floor_ / avg;
        }
    }
    if (sky_n_ > 0) {
        bracket(sky_.data(), sky_n_, t, &i, &j, &f);
        a.sky_top = lerp(sky_[i].top, sky_[j].top, f);
        a.sky_bottom = lerp(sky_[i].bottom, sky_[j].bottom, f);
    }
    return a;
}

Daylight daylight_preset(int i) {
    constexpr int kSkyN = sizeof(kSky) / sizeof(kSky[0]);
    Daylight d = i == 1   ? Daylight(kVivid, sizeof(kVivid) / sizeof(kVivid[0]), kSky, kSkyN)
                 : i == 2 ? Daylight(kSubtle, sizeof(kSubtle) / sizeof(kSubtle[0]), kSky, kSkyN)
                          : Daylight(kNatural, sizeof(kNatural) / sizeof(kNatural[0]), kSky, kSkyN);
    d.set_floor(kDefaultNightFloor);
    return d;
}

const char* daylight_preset_name(int i) {
    constexpr const char* kNames[kDaylightPresets] = {"natuerlich", "kraeftig", "zurueckhaltend"};
    return i >= 0 && i < kDaylightPresets ? kNames[i] : "?";
}

void PlacePalette::set_levels(int levels) {
    levels_ = std::clamp(levels, 1, 3);
    base_ = kSceneColors / levels_;
}

void PlacePalette::set(int i, Color day) {
    if (i <= 0 || i >= base_) return;
    day_[i] = day;
    night_[i] = day;
    used_[i] = true;
}

void PlacePalette::set_glow(int i, Color night) {
    if (i <= 0 || i >= base_) return;
    night_[i] = night;
    role_[i] = ColorRole::Glow;
    used_[i] = true;
}

void PlacePalette::set_sky(int first, int count) {
    if (first <= 0 || count <= 0 || first + count > base_) return;
    sky_first_ = first;
    sky_count_ = count;
    for (int i = first; i < first + count; ++i) {
        role_[i] = ColorRole::Sky;
        used_[i] = true;
    }
}

void PlacePalette::set_fixed(int i) {
    if (i > 0 && i < base_) role_[i] = ColorRole::Fixed;
}

bool PlacePalette::add_cycle(const ColorCycle& c) {
    if (cycle_count_ >= kMaxCycles || c.count < 2 || c.first == 0 || c.first + c.count > base_ || c.ticks == 0)
        return false;
    cycles_[cycle_count_++] = c;
    return true;
}

void PlacePalette::apply(const Ambient& a, uint64_t tick, Palette& out) const {
    std::array<uint8_t, kSceneColors> src;
    for (int i = 0; i < base_; ++i) src[i] = static_cast<uint8_t>(i);
    for (int c = 0; c < cycle_count_; ++c) {
        const ColorCycle& cy = cycles_[c];
        const int off = static_cast<int>((tick / cy.ticks) % cy.count);
        for (int k = 0; k < cy.count; ++k) src[cy.first + k] = static_cast<uint8_t>(cy.first + (k + off) % cy.count);
    }
    const int ar = a.r * exposure_ >> 8, ag = a.g * exposure_ >> 8, ab = a.b * exposure_ >> 8;
    for (int i = 0; i < base_; ++i) {
        const int s = src[i];
        Color c = day_[s];
        switch (role_[i]) {
            case ColorRole::Fixed:
                break;
            case ColorRole::Sky: {
                const int f = sky_count_ > 1 ? (i - sky_first_) * 256 / (sky_count_ - 1) : 0;
                c = lerp(a.sky_top, a.sky_bottom, f);
                break;
            }
            case ColorRole::Glow:
                c = lerp(lit(c, ar, ag, ab, a.sat), night_[s], a.lamp);
                break;
            case ColorRole::Normal:
                for (int k = 0; k < levels_; ++k) {
                    const int lamp = levels_ > 1 ? a.lamp * k / (levels_ - 1) : 0;
                    const int sat = a.sat + ((256 - a.sat) * lamp >> 8);
                    out[k * base_ + i] = lit(c, ar + (factor(lamp_.r) * lamp >> 8), ag + (factor(lamp_.g) * lamp >> 8),
                                             ab + (factor(lamp_.b) * lamp >> 8), sat);
                }
                continue;
        }
        for (int k = 0; k < levels_; ++k) out[k * base_ + i] = c;
    }
}

int PlacePalette::nearest(Color c) const {
    int best = 1, bd = 1 << 30;
    for (int i = 1; i < base_; ++i) {
        if (!used_[i] || role_[i] != ColorRole::Normal) continue;
        const int d = dist2(c, day_[i]);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void PlacePalette::tint_table(Color target, int amount, uint8_t out[256]) const {
    for (int i = 0; i < 256; ++i) out[i] = static_cast<uint8_t>(i);
    for (int i = 1; i < base_; ++i) {
        if (!used_[i] || role_[i] != ColorRole::Normal) continue;
        const int j = nearest(lerp(day_[i], target, amount));
        for (int k = 0; k < levels_; ++k) out[k * base_ + i] = static_cast<uint8_t>(k * base_ + j);
    }
}

void PlacePalette::blend_table(uint8_t* out) const {
    for (int a = 0; a < base_; ++a)
        for (int b = 0; b < base_; ++b) {
            const bool both = used_[a] && used_[b] && role_[a] == ColorRole::Normal && role_[b] == ColorRole::Normal;
            out[a * base_ + b] = static_cast<uint8_t>(a == b || !both ? a : nearest(lerp(day_[a], day_[b], 128)));
        }
}

void build_light_map(uint8_t* map, int w, int h, const LightCircle* lights, int n, int levels) {
    std::memset(map, 0, static_cast<size_t>(w) * h);
    const int top = std::max(0, levels - 1) * kLightUnit;
    for (int l = 0; l < n; ++l) {
        const LightCircle& c = lights[l];
        if (!c.rx || !c.ry) continue;
        const float inner = c.inner / 255.0f;
        const int y0 = std::max(0, c.y - c.ry), y1 = std::min(h, c.y + c.ry + 1);
        const int x0 = std::max(0, c.x - c.rx), x1 = std::min(w, c.x + c.rx + 1);
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x) {
                const float dx = static_cast<float>(x - c.x) / c.rx, dy = static_cast<float>(y - c.y) / c.ry;
                const float d = std::sqrt(dx * dx + dy * dy);
                if (d >= 1.0f) continue;
                // innen voll, dann weich (smoothstep) bis zum Rand
                float f = d <= inner ? 1.0f : (1.0f - d) / (1.0f - inner);
                f = f * f * (3.0f - 2.0f * f);
                const int v = static_cast<int>(f * top * c.strength / 255.0f + 0.5f);
                uint8_t& m = map[y * w + x];
                if (v > m) m = static_cast<uint8_t>(v);
            }
    }
}

}  // namespace pixelia
