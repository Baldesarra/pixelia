#include "pixelia/engine/light.h"

#include "pixelia/engine/calendar.h"

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

// Tageszeit auf die Zeit der Stützpunkte abbilden (Aufgang 6:30, Untergang 19:45): Dämmerungen verschoben, Tag und
// Nacht gestreckt. t und Ergebnis in 1/256 Minuten.
static int sun_remap(int t, const SunTimes& sun) {
    constexpr int kRef[5] = {300, 480, 1110, 1350, 300 + 1440};
    const int a[5] = {sun.rise - 90, sun.rise + 90, sun.set - 75, sun.set + 165, sun.rise - 90 + 1440};
    if (!(a[0] < a[1] && a[1] < a[2] && a[2] < a[3] && a[3] < a[4])) return t;  // unsinnige Sonnenzeiten
    int m = t;
    if (m < a[0] * 256) m += kDayUnits;
    int k = 0;
    while (k < 3 && m >= a[k + 1] * 256) ++k;
    const int s0 = a[k] * 256, s1 = a[k + 1] * 256;
    const int r = kRef[k] * 256 + static_cast<int>(static_cast<int64_t>(m - s0) * (kRef[k + 1] - kRef[k]) * 256 / (s1 - s0));
    return r % kDayUnits;
}

Ambient Daylight::at(int64_t us_of_day, const SunTimes* sun) const {
    Ambient a;
    int64_t us = us_of_day % kUsPerDay;
    if (us < 0) us += kUsPerDay;
    int t = static_cast<int>(us / kUsPerUnit);
    if (sun) t = sun_remap(t, *sun);
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
    if (a.haze <= 0) return;
    // Dunst (Wetter): Lampenlicht scheint durch, je Stufe weniger gedämpft
    for (int i = 0; i < base_; ++i) {
        if (role_[i] == ColorRole::Fixed) continue;
        const int h = role_[i] == ColorRole::Glow ? a.haze * (256 - a.lamp / 2) >> 8 : a.haze;
        for (int k = 0; k < levels_; ++k) {
            Color& c = out[k * base_ + i];
            c = lerp(c, a.haze_color, h * (4 - k) >> 2);
        }
    }
}

void PlacePalette::set_glow_on(int i, bool on) {
    if (i <= 0 || i >= base_ || role_[i] == ColorRole::Sky || role_[i] == ColorRole::Fixed) return;
    role_[i] = on ? ColorRole::Glow : ColorRole::Normal;
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

void plan_light_map(LightPlan* plan, const LightCircle* lights, int n, int levels, LightStyle style) {
    static constexpr uint8_t kBayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    constexpr int kUnit = 64;  // Lichtwert einer Stufe
    // Wurzeln der Stützstellen einmal für alle (am Gerät kostet jede Wurzel einige hundert Takte)
    static const std::array<float, 257> kRoot = [] {
        std::array<float, 257> r{};
        for (int t = 0; t <= 256; ++t) r[t] = std::sqrt(t / 256.0f);
        return r;
    }();
    LightPlan& p = *plan;
    const int top = std::clamp(levels - 1, 0, 3) * kUnit;
    const bool same = top == p.top && style == p.style;
    // Abfall-Tabellen nach (Stärke, innen): gleiche Kreisart = gleiche Tabelle, nur kopieren. had[k] = Art, deren
    // Tabelle gerade auf Platz k liegt (~0 = keine)
    std::array<uint32_t, kMaxLightCircles> had;
    had.fill(~0u);
    if (same)
        for (int k = 0; k < p.n; ++k) had[k] = static_cast<uint32_t>(p.lights[k].strength << 8 | p.lights[k].inner);
    p.n = 0;
    for (int l = 0; l < std::min(n, kMaxLightCircles); ++l) {
        const LightCircle& c = lights[l];
        if (!c.rx || !c.ry) continue;
        const int k = p.n++;
        p.lights[k] = c;
        // d² = q / r mit r = rx²·ry² (< 2^32): r auf 16 Bit kürzen, dann reicht 32-Bit-Rechnung (q >> shift) · mul >> 16
        const uint32_t r = static_cast<uint32_t>(c.rx) * c.rx * c.ry * c.ry;
        int sh = 0;
        while ((r >> sh) >= 0x10000u) ++sh;
        p.shift[k] = static_cast<uint8_t>(sh);
        p.mul[k] = 0xFFFFFFFFu / (r >> sh);
        const uint32_t key = static_cast<uint32_t>(c.strength << 8 | c.inner);
        int from = had[k] == key ? k : -1;
        for (int j = 0; j < kMaxLightCircles && from < 0; ++j)
            if (had[j] == key) from = j;
        if (from >= 0) {
            if (from != k) p.falloff[k] = p.falloff[from];
        } else {
            // innen voll, dann weich (smoothstep) bis zum Rand; Stützstellen bei d² = t / 256, dazwischen linear
            const float inner = c.inner / 255.0f, out = 1.0f / std::max(1.0f / 255.0f, 1.0f - inner);
            const float scale = top * c.strength / 255.0f * 256.0f;
            for (int t = 0; t <= 256; ++t) {
                const float d = kRoot[t];
                float f = d <= inner ? 1.0f : (1.0f - d) * out;
                f = f * f * (3.0f - 2.0f * f);
                p.falloff[k][t] = static_cast<uint16_t>(f * scale + 0.5f);
            }
        }
        had[k] = key;
        // volles Licht, solange beide Stützstellen des Feldes voll sind: Felder 0 … full−1, also q < full · r / 256
        int full = 0;
        while (full < 256 && p.falloff[k][full + 1] == p.falloff[k][0]) ++full;
        p.inner_q[k] = static_cast<uint32_t>(static_cast<uint64_t>(full) * r / 256);
    }
    // Stufen der vier feinen Bildpunkte nach Lichtwert; das Raster hängt nur an (y & 3, x & 3) des groben Punktes
    if (same) return;
    p.top = top;
    p.style = style;
    const unsigned most = static_cast<unsigned>(top / kUnit);
    for (int pos = 0; pos < 16; ++pos) {
        const int py = pos >> 2, px = pos & 3;
        unsigned add[4];  // Schwelle je feinem Bildpunkt: Lichtwert + add, dann / 64
        for (int k = 0; k < 4; ++k) {
            const int fx = 2 * px + (k & 1), fy = 2 * py + (k >> 1);
            add[k] = style == LightStyle::Stepped  ? kUnit / 2
                     : style == LightStyle::Coarse ? kBayer[py][px] * 4 + 2
                                                   : kBayer[fy & 3][fx & 3] * 4 + 2;
        }
        for (unsigned v = 0; v <= kLightValueMax; ++v) {
            unsigned packed = 0;
            for (int k = 0; k < 4; ++k) packed |= std::min((v + add[k]) >> 6, most) << (2 * k);
            p.pack[pos][v] = static_cast<uint8_t>(packed);
        }
    }
}

int light_row_cost(const LightPlan& plan, int y, int w) {
    int cost = 16;  // leeren und Bereich
    for (int l = 0; l < plan.n; ++l) {
        const LightCircle& c = plan.lights[l];
        if (y >= c.y - c.ry && y <= c.y + c.ry) cost += std::max(0, std::min(w, c.x + c.rx + 1) - std::max(0, c.x - c.rx));
    }
    return cost;
}

void build_light_row(uint8_t* row, int w, int y, const LightPlan& plan, uint8_t* x0, uint8_t* x1) {
    // Zeile erst im internen RAM (Stack, höchstens 256 Byte), dann einmal in die Karte (PSRAM)
    uint8_t buf[256];
    w = std::min(w, 256);
    std::memset(buf, 0, static_cast<size_t>(w));
    int lo = w, hi = 0;
    for (int l = 0; l < plan.n; ++l) {
        const LightCircle& c = plan.lights[l];
        const int dy = y - c.y;
        if (dy < -c.ry || dy > c.ry) continue;
        // d² = q / r ganzzahlig mit q = dx²·ry² + dy²·rx², r = rx²·ry² (< 2^32); dx² wächst je Schritt um 2·dx + 1
        const uint32_t rx2 = static_cast<uint32_t>(c.rx) * c.rx, ry2 = static_cast<uint32_t>(c.ry) * c.ry;
        const uint32_t r = rx2 * ry2, qy = static_cast<uint32_t>(dy * dy) * rx2;
        if (qy >= r) continue;
        const int sh = plan.shift[l];
        const uint32_t mul = plan.mul[l], qin = plan.inner_q[l];
        const uint16_t* fall = plan.falloff[l].data();
        const uint32_t vin = (fall[0] + 128u) >> 8;  // innen voll
        const auto* pack = &plan.pack[(y & 3) << 2];
        auto put = [&](int x, uint32_t v) {
            const uint8_t p = pack[x & 3][v];
            if (!p) return;
            uint8_t& m = buf[x];
            if (m) {
                uint8_t out = 0;
                for (int k = 0; k < 8; k += 2) out |= static_cast<uint8_t>(std::max(m >> k & 3, p >> k & 3) << k);
                m = out;
            } else {
                m = p;
            }
            lo = std::min(lo, x);
            hi = std::max(hi, x + 1);
        };
        // spiegelgleich um c.x: Lichtwert je |dx| einmal, für links und rechts; dx² wächst je Schritt um 2·dx + 1
        const int reach = std::min<int>(c.rx, std::max<int>(c.x, w - 1 - c.x));
        uint32_t q = qy, step = ry2;  // q = dx²·ry² + dy²·rx² < r (sonst außerhalb)
        for (int dx = 0; dx <= reach && q < r; ++dx, q += step, step += 2 * ry2) {
            uint32_t v = vin;
            if (q >= qin) {
                const uint32_t u = ((q >> sh) * mul) >> 16;  // d² in 1/65536
                const uint32_t t = u >> 8, fr = u & 255;
                v = (fall[t] * (256 - fr) + fall[t + 1] * fr + 32768) >> 16;
                if (!v) break;  // weiter außen nur noch dunkler
            }
            const int xr = c.x + dx, xl = c.x - dx;
            if (xr >= 0 && xr < w) put(xr, v);
            if (dx && xl >= 0 && xl < w) put(xl, v);
        }
    }
    std::memcpy(row, buf, static_cast<size_t>(w));
    *x0 = static_cast<uint8_t>(lo < hi ? lo : 0);
    *x1 = static_cast<uint8_t>(lo < hi ? hi : 0);
}

int light_level_at(const uint8_t* map, int w, int h, int x, int y) {
    const int x0 = std::clamp(x & ~3, 0, w - 4), y0 = std::clamp(y & ~3, 0, h - 4);
    int sum = 0;
    for (int yy = y0; yy < y0 + 4; ++yy)
        for (int xx = x0; xx < x0 + 4; ++xx) {
            const uint8_t p = map[yy * w + xx];
            sum += (p & 3) + (p >> 2 & 3) + (p >> 4 & 3) + (p >> 6);
        }
    return (sum + 32) / 64;
}

}  // namespace pixelia
