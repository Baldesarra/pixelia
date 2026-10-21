#include "pixelia/engine/weather.h"

#include <algorithm>
#include <cstdint>

#include "pixelia/engine/memory.h"

namespace pixelia {

namespace {

constexpr const char* kNames[kWeatherKinds] = {"sonne", "bewoelkt", "regen", "schnee", "nebel", "gewitter", "wind"};
constexpr const char* kTitles[kWeatherKinds] = {"Sonne", "Bewölkt", "Regen", "Schnee", "Nebel", "Gewitter", "Wind"};

// Dauer der Übergänge von 0 auf 255 in Logik-Schritten (30 je Sekunde)
constexpr int kCloudTicks = 20 * 30;
constexpr int kPrecipTicks = 12 * 30;
constexpr int kFogTicks = 20 * 30;
constexpr int kStormTicks = 8 * 30;
constexpr int kWindTicks = 6 * 30;
constexpr int32_t kFull = 255 << 8;

// Blitz: Helligkeit je Logik-Schritt, einfach bzw. mit Nachblitz
constexpr uint8_t kFlash[] = {255, 210, 110, 40};
constexpr uint8_t kFlash2[] = {255, 170, 50, 0, 0, 235, 160, 70, 20};

int32_t approach(int32_t v, int32_t goal, int ticks) {
    const int32_t step = kFull / ticks;
    if (v < goal) return std::min(goal, v + step);
    if (v > goal) return std::max(goal, v - step);
    return v;
}

int lerp(int a, int b, int f256) { return a + ((b - a) * f256 >> 8); }
Color lerp(Color a, Color b, int f256) {
    return {static_cast<uint8_t>(lerp(a.r, b.r, f256)), static_cast<uint8_t>(lerp(a.g, b.g, f256)),
            static_cast<uint8_t>(lerp(a.b, b.b, f256))};
}
uint8_t ch(int v) { return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v); }

// Grau gleicher Helligkeit, leicht kühl (bedeckter Himmel), mal k256
Color overcast(Color c, int k256) {
    const int lum = ((77 * c.r + 150 * c.g + 29 * c.b) >> 8) * k256 >> 8;
    return {ch(lum * 236 >> 8), ch(lum * 244 >> 8), ch(lum * 262 >> 8)};
}

// --- Rauschen für die Schleier: Gitterwerte gehasht, weich verbunden ---
uint32_t hash2(int32_t x, int32_t y, uint32_t salt) {
    uint32_t h = static_cast<uint32_t>(x) * 0x8DA6B343u ^ static_cast<uint32_t>(y) * 0xD8163841u ^ salt;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}

// Rauschen für ein ganzes Feld (CoarseLayers::kVeilW × kVeilH): Zelle (x, y) an (x0 + y·skew + x·dx, y0 + y·dy) in
// 1/256 Gitterzellen. Die Gitterwerte einer Zeile werden nur einmal gehasht (spart ~90 % gegenüber noise() je Zelle)
void noise_grid(uint8_t* out, int32_t x0, int32_t y0, int32_t dx, int32_t dy, int32_t skew, uint32_t salt) {
    constexpr int W = CoarseLayers::kVeilW, H = CoarseLayers::kVeilH, kMax = 64;
    int top[kMax], bot[kMax];
    int32_t row = INT32_MIN, first = INT32_MIN;
    for (int y = 0; y < H; ++y) {
        const int32_t Y = y0 + y * dy, xs = x0 + y * skew;
        const int32_t yi = Y >> 8, x_first = xs >> 8;
        int fy = Y & 255;
        fy = fy * fy * (768 - 2 * fy) >> 16;
        const int n = std::min(kMax, static_cast<int>(((xs + (W - 1) * dx) >> 8) - x_first) + 2);
        if (yi != row || x_first != first) {
            for (int k = 0; k < n; ++k) {
                top[k] = hash2(x_first + k, yi, salt) & 255;
                bot[k] = hash2(x_first + k, yi + 1, salt) & 255;
            }
            row = yi;
            first = x_first;
        }
        for (int x = 0; x < W; ++x) {
            const int32_t X = xs + x * dx;
            const int k = std::min(n - 2, static_cast<int>((X >> 8) - x_first));
            int fx = X & 255;
            fx = fx * fx * (768 - 2 * fx) >> 16;
            const int a = top[k] + ((top[k + 1] - top[k]) * fx >> 8), b = bot[k] + ((bot[k + 1] - bot[k]) * fx >> 8);
            out[y * W + x] = static_cast<uint8_t>(a + ((b - a) * fy >> 8));
        }
    }
}

// Schneeflocken pendeln: Versatz je Alter (1/256 Bildpunkt)
constexpr int16_t kWobble[16] = {0, 38, 70, 92, 100, 92, 70, 38, 0, -38, -70, -92, -100, -92, -70, -38};

constexpr int kWarmup = 120;  // Schritte Vorlauf beim Betreten (Schnee braucht am längsten)

}  // namespace

const char* weather_name(WeatherKind k) {
    const int i = static_cast<int>(k);
    return i >= 0 && i < kWeatherKinds ? kNames[i] : "?";
}

const char* weather_title(WeatherKind k) {
    const int i = static_cast<int>(k);
    return i >= 0 && i < kWeatherKinds ? kTitles[i] : "?";
}

bool parse_weather(std::string_view name, WeatherKind* out) {
    for (int i = 0; i < kWeatherKinds; ++i)
        if (name == kNames[i]) {
            *out = static_cast<WeatherKind>(i);
            return true;
        }
    return false;
}

// --- Weather ---

void Weather::init(uint64_t seed) {
    rng_.reseed(seed ^ 0x57E77E6ULL);
    kind_ = WeatherKind::Clear;
    strength_ = 255;
    goals(kind_, strength_, &goal_);
    cur_ = goal_;
    gust_ = gust_goal_ = 256 << 8;
    gust_wait_ = 0;
    flash_step_ = -1;
    flash_wait_ = -1;
    flashes_ = 0;
    drift_ = 0;
    const int8_t dir = s_.wind_dir;
    s_ = WeatherState{};
    s_.wind_dir = dir;
    publish();
}

void Weather::goals(WeatherKind kind, int strength, Channels* out) const {
    const int s = std::clamp(strength, 0, 255);
    Channels g{};
    switch (kind) {
        case WeatherKind::Clear: g.cloud = 55; g.wind = 30; break;  // ein paar Wolkenschatten ziehen durch
        case WeatherKind::Cloudy: g.cloud = s * 3 / 4; g.wind = 60; break;
        case WeatherKind::Rain: g.cloud = 255; g.rain = s; g.wind = 70; break;
        case WeatherKind::Snow: g.cloud = 235; g.snow = s; g.wind = 45; break;
        case WeatherKind::Fog: g.cloud = 150; g.fog = s; g.wind = 20; break;
        case WeatherKind::Storm: g.cloud = 255; g.rain = std::max(s, 160); g.storm = s; g.wind = 180; break;
        case WeatherKind::Wind: g.cloud = 100; g.wind = 150 + s * 105 / 255; break;
    }
    out->cloud = g.cloud << 8;
    out->rain = g.rain << 8;
    out->snow = g.snow << 8;
    out->fog = g.fog << 8;
    out->storm = g.storm << 8;
    out->wind = g.wind << 8;
}

void Weather::set(WeatherKind kind, int strength) {
    kind_ = kind;
    strength_ = std::clamp(strength, 0, 255);
    goals(kind_, strength_, &goal_);
}

void Weather::set_now(WeatherKind kind, int strength) {
    set(kind, strength);
    cur_ = goal_;
    publish();
}

void Weather::set_flash_seconds(int min_s, int max_s) {
    flash_min_ = std::clamp(min_s, 1, 600);
    flash_max_ = std::clamp(max_s, flash_min_, 600);
    flash_wait_ = -1;
}

void Weather::strike() {
    flash_step_ = 0;
    double_flash_ = rng_.below(3) == 0;
    ++flashes_;
}

bool Weather::settled() const {
    const int32_t storm = goal_.rain >= (128 << 8) ? goal_.storm : 0;
    return cur_.cloud == goal_.cloud && cur_.rain == goal_.rain && cur_.snow == goal_.snow && cur_.fog == goal_.fog &&
           cur_.wind == goal_.wind && cur_.storm == storm;
}

void Weather::tick() {
    const Channels& g = goal_;
    // Niederschlag erst, wenn die Wolken fast da sind; die Wolken bleiben, solange es noch regnet oder schneit
    const bool dense = cur_.cloud >= std::min<int32_t>(g.cloud, 220 << 8) - (8 << 8) && g.cloud >= (200 << 8);
    const int32_t precip = std::max(cur_.rain, cur_.snow);
    cur_.cloud = approach(cur_.cloud, precip > 0 ? std::max<int32_t>(g.cloud, 230 << 8) : g.cloud, kCloudTicks);
    cur_.rain = approach(cur_.rain, dense ? g.rain : 0, kPrecipTicks);
    cur_.snow = approach(cur_.snow, dense ? g.snow : 0, kPrecipTicks);
    cur_.storm = approach(cur_.storm, cur_.rain >= (128 << 8) ? g.storm : 0, kStormTicks);
    cur_.fog = approach(cur_.fog, g.fog, kFogTicks);
    cur_.wind = approach(cur_.wind, g.wind, kWindTicks);

    // Böen: alle 1,5–5 s ein neues Ziel zwischen 60 % und 120 % des Windes
    if (--gust_wait_ <= 0) {
        gust_goal_ = rng_.range(150, 300) << 8;
        gust_wait_ = rng_.range(45, 150);
    }
    gust_ += (gust_goal_ - gust_) / 24;

    // Blitze: bei vollem Gewitter zufällig alle flash_min_ … flash_max_ Sekunden; die Wartezeit läuft mit der
    // Stärke des Gewitters ab (halbes Gewitter = doppelt so lange)
    if (flash_step_ >= 0) {
        const int n = double_flash_ ? static_cast<int>(sizeof kFlash2) : static_cast<int>(sizeof kFlash);
        if (++flash_step_ >= n) flash_step_ = -1;
    }
    if (cur_.storm > 0) {
        if (flash_wait_ < 0) flash_wait_ = rng_.range(flash_min_ * 30, flash_max_ * 30) * 255;
        flash_wait_ -= cur_.storm >> 8;
        if (flash_wait_ <= 0) {
            strike();
            flash_wait_ = -1;
        }
    } else {
        flash_wait_ = -1;
    }
    publish();
    drift_ += static_cast<int64_t>(s_.wind) * s_.wind_dir * 2;
}

void Weather::publish() {
    s_.cloud = static_cast<uint8_t>(cur_.cloud >> 8);
    s_.rain = static_cast<uint8_t>(cur_.rain >> 8);
    s_.snow = static_cast<uint8_t>(cur_.snow >> 8);
    s_.fog = static_cast<uint8_t>(cur_.fog >> 8);
    s_.storm = static_cast<uint8_t>(cur_.storm >> 8);
    s_.wind = ch((cur_.wind >> 8) * (gust_ >> 8) >> 8);
    s_.flash = flash_step_ < 0 ? 0 : double_flash_ ? kFlash2[flash_step_] : kFlash[flash_step_];
}

void Weather::apply(Ambient& a, bool outdoor) const {
    const int c = s_.cloud, rain = s_.rain, snow = s_.snow, fog = s_.fog;
    // Wolken: nur der Anteil über der Nacht wird dunkler (die Nacht bleibt lesbar, Mindesthelligkeit)
    const int dark = (outdoor ? 74 : 100) * std::max(0, c - 60) / 195 + 24 * rain / 255;  // Schönwetterwolken: nur Schatten
    auto dim = [&](int v, int k) {
        constexpr int kNight = 96;
        return v > kNight ? v - ((v - kNight) * k >> 8) : v;
    };
    a.r = dim(a.r, dark);
    a.g = dim(a.g, dark);
    a.b = dim(a.b, dark * 13 / 16);  // bedeckt etwas kühler
    a.sat = a.sat * (256 - 96 * std::max(0, c - 60) / 195) >> 8;
    // Himmel (Fenster drinnen) wird grau
    const int sk = 220 * c / 255;
    a.sky_top = lerp(a.sky_top, overcast(a.sky_top, 236), sk);
    a.sky_bottom = lerp(a.sky_bottom, overcast(a.sky_bottom, 236), sk);
    // Dunst: Nebel, Regen, Schnee; Farbe im Licht der Tageszeit
    const Color base = snow > fog ? Color{226, 230, 238} : Color{206, 210, 216};
    const Color hc{ch(base.r * a.r >> 8), ch(base.g * a.g >> 8), ch(base.b * a.b >> 8)};
    if (outdoor) {
        a.haze = std::min(160, fog * 84 / 255 + rain * 26 / 255 + snow * 36 / 255);
        a.haze_color = hc;
    }
    const int sf = 210 * fog / 255;
    a.sky_top = lerp(a.sky_top, hc, sf);
    a.sky_bottom = lerp(a.sky_bottom, hc, sf);
    // Blitz: draußen alles hell und kühl, drinnen schwächer, im Fenster voll
    if (s_.flash) {
        const int f = s_.flash * (outdoor ? 256 : 80) >> 8;
        a.r += (380 - a.r) * f >> 8;
        a.g += (390 - a.g) * f >> 8;
        a.b += (440 - a.b) * f >> 8;
        a.sat = a.sat * (256 - (f >> 1)) >> 8;
        a.sky_top = lerp(a.sky_top, {236, 240, 255}, s_.flash);
        a.sky_bottom = lerp(a.sky_bottom, {244, 246, 255}, s_.flash);
    }
}

// --- ParticlePool ---

bool ParticlePool::init(int capacity) {
    release();
    p_ = static_cast<Particle*>(asset_alloc(sizeof(Particle) * static_cast<size_t>(capacity)));
    if (!p_) return false;
    cap_ = capacity;
    n_ = 0;
    return true;
}

void ParticlePool::release() {
    if (p_) asset_free(p_);
    p_ = nullptr;
    cap_ = n_ = 0;
}

// --- PlaceWeather ---

void PlaceWeather::configure(const WeatherLook& look, const PlacePalette& pal) {
    look_ = look;
    base_ = pal.base();
    pal.tint_table(look.fog, 150, fog_);
    pal.tint_table(look.veil, 110, veil_);
    pal.tint_table(look.shadow, 90, shadow_);
}

void PlaceWeather::enter(const Weather& w, uint64_t seed) {
    if (!pool_.ready() && !pool_.init(kMaxParticles)) return;
    pool_.clear();
    rng_.reseed(seed);
    rain_acc_ = snow_acc_ = leaf_acc_ = 0;
    peak_ = 0;
    for (int i = 0; i < kWarmup; ++i) tick(w);
}

void PlaceWeather::leave() { pool_.release(); }

void PlaceWeather::tick(const Weather& w) {
    if (!pool_.ready()) return;
    const WeatherState& s = w.state();
    wind_vx_ = s.wind * s.wind_dir;
    step();
    spawn(s);
    peak_ = std::max(peak_, pool_.size());
}

void PlaceWeather::step() {
    for (int i = 0; i < pool_.size();) {
        Particle& p = pool_[i];
        if (p.age < 255) ++p.age;
        bool gone = false;
        switch (p.kind) {
            case ParticleKind::Drop:
                p.x += p.vx;
                p.y += p.vy;
                if ((p.y >> 8) >= p.ground) {
                    if (look_.outdoor && rain_style_ != RainStyle::Fine) {
                        p.kind = ParticleKind::Splash;
                        p.age = 0;
                        p.y = p.ground << 8;
                    } else {
                        gone = true;
                    }
                }
                break;
            case ParticleKind::Splash:
                gone = p.age >= 4;
                break;
            case ParticleKind::Flake:
            case ParticleKind::BigFlake:
                p.x += p.vx + kWobble[(p.age + p.ground) >> 2 & 15];
                p.y += p.vy;
                gone = (p.y >> 8) >= p.ground;
                break;
            case ParticleKind::Leaf:
                // vom Wind getragen (Böen wirken sofort), flattert auf und ab
                p.x += wind_vx_ * p.ground >> 4;
                p.y += p.vy + kWobble[(p.age >> 1) & 15] * 2;
                gone = p.x < (-8 << 8) || p.x > ((kScreenWidth + 8) << 8) || p.y < (-8 << 8) ||
                       p.y > ((kScreenHeight + 8) << 8);
                break;
        }
        if (gone) {
            pool_.remove(i);
            continue;
        }
        ++i;
    }
}

void PlaceWeather::spawn(const WeatherState& s) {
    // Regen: bei voller Stärke ~22 Tropfen je Schritt, etwa 240 gleichzeitig
    const int rain_rate = rain_style_ == RainStyle::Fine ? 34 : rain_style_ == RainStyle::Drops ? 26 : 22;
    rain_acc_ += s.rain * rain_rate * density_ / 100;
    while (rain_acc_ >= 255) {
        rain_acc_ -= 255;
        Particle* p = pool_.add();
        if (!p) break;
        const bool slow = rain_style_ == RainStyle::Drops;
        p->kind = ParticleKind::Drop;
        p->age = 0;
        p->vy = static_cast<int16_t>(slow ? rng_.range(1100, 1400) : rng_.range(1700, 2100));
        p->vx = static_cast<int16_t>(wind_vx_ * 3 + rng_.range(-40, 40));
        const int fall = rng_.range(40, 110);  // Höhe über dem Aufschlag beim Entstehen
        p->ground = static_cast<int16_t>(rng_.range(0, kScreenHeight - 1));
        p->y = (p->ground - fall) << 8;
        // so versetzt, dass der Tropfen im Bild aufschlägt (Wind)
        p->x = (rng_.range(0, kScreenWidth - 1) << 8) - p->vx * (fall * 256 / p->vy);
    }
    // Schnee: ~1,6 Flocken je Schritt, langsam, pendelnd, vom Wind versetzt
    snow_acc_ += s.snow * 16 * density_ / 1000;
    while (snow_acc_ >= 255) {
        snow_acc_ -= 255;
        Particle* p = pool_.add();
        if (!p) break;
        const bool big = snow_style_ == SnowStyle::Large || (snow_style_ == SnowStyle::Mixed && rng_.below(4) == 0);
        p->kind = big ? ParticleKind::BigFlake : ParticleKind::Flake;
        p->age = 0;
        p->vy = static_cast<int16_t>(big ? rng_.range(270, 380) : rng_.range(150, 260));
        p->vx = static_cast<int16_t>(wind_vx_ * (big ? 6 : 5) / 4 + rng_.range(-30, 30));
        p->ground = static_cast<int16_t>(rng_.range(16, kScreenHeight - 1));
        const int y0 = -rng_.range(2, 30);
        p->y = y0 << 8;
        const int life = (p->ground - y0) * 256 / p->vy;
        p->x = (rng_.range(0, kScreenWidth - 1) << 8) - p->vx * life;
    }
    // Blätter im Wind (nur draußen, nicht bei starkem Niederschlag): vom Rand gegen den Wind
    if (look_.outdoor && s.wind > 90 && s.rain < 200 && s.snow < 120) {
        leaf_acc_ += (s.wind - 90) * density_ / 300;
        while (leaf_acc_ >= 255) {
            leaf_acc_ -= 255;
            Particle* p = pool_.add();
            if (!p) break;
            p->kind = ParticleKind::Leaf;
            p->age = static_cast<uint8_t>(rng_.below(32));
            p->ground = static_cast<int16_t>(rng_.range(48, 80));  // Faktor des Windes (16tel)
            p->vx = 0;
            p->vy = static_cast<int16_t>(rng_.range(-50, 50));
            p->x = (s.wind_dir > 0 ? -6 : kScreenWidth + 5) << 8;
            p->y = rng_.range(8, kScreenHeight - 8) << 8;
        }
    }
}

bool PlaceWeather::masked(uint8_t px) const {
    if (px >= kSceneColors) return false;
    const int i = px % base_ - look_.window_first;
    return i >= 0 && i < look_.window_count;
}

void PlaceWeather::layers(CoarseLayers& L, const Weather& w, uint64_t tick) const {
    L.veil_table = nullptr;
    L.veil_grid = nullptr;
    if (!look_.outdoor) return;  // drinnen zeigt sich das Wetter in den Fenstern (Palette, Partikel)
    const WeatherState& s = w.state();
    // Stärken der Schleier (0 … 256); es gilt der stärkste
    const int c = s.cloud;
    const int sh = c >= 24 ? std::min(256, (255 - c) * 4) * std::min(256, (c - 24) * 4) >> 8 : 0;
    const int shadow = sh * (255 - s.fog) / 255;
    const int fog = s.fog * fog_scale_ / 100;
    const int veil = s.rain > 110 ? (s.rain - 110) * 256 / 145 : 0;
    const int top = std::max({shadow, fog, veil});
    if (top < 8) return;
    constexpr int W = CoarseLayers::kVeilW, H = CoarseLayers::kVeilH;
    // Windversatz in 1/256 Zellen (eine Zelle = 8 feine Bildpunkte)
    const int64_t d = w.drift() / 8;
    const int32_t t = static_cast<int32_t>(tick % 0x40000000u);
    uint8_t* g = grid_;
    if (top == shadow) {
        // Wolkenschatten ziehen mit dem Wind; je dichter die Wolken, desto mehr Fläche (höchstens gut die Hälfte:
        // darüber dunkelt die Palette ab, und jeder beschattete Bildpunkt kostet Zeit)
        noise_grid(g, static_cast<int32_t>(-d / 26), 0, 256 / 13, 256 / 9, 0, 0xC10D);
        for (int i = 0; i < W * H; ++i)
            g[i] = static_cast<uint8_t>(std::clamp((std::min(c, 110) + 30 - g[i]) * 12 / 36, 0, 12) * shadow >> 8);
        L.veil_table = shadow_;
    } else if (top == fog) {
        // Nebelbänke: zwei Lagen, langsam mit dem Wind
        noise_grid(g, static_cast<int32_t>(-d / 44), t / 3, 256 / 11, 256 / 7, 0, 0xF06);
        noise_grid(noise_, static_cast<int32_t>(-d / 12), 0, 256 / 4, 256 / 4, 0, 0xF07);
        const int f = std::min(fog, 256);
        for (int i = 0; i < W * H; ++i) {
            const int n = (g[i] * 3 + noise_[i]) >> 2;
            g[i] = static_cast<uint8_t>(std::min((f * (24 + n * 232 / 255) >> 8) * 11 >> 8, 11));
        }
        L.veil_table = fog_;
    } else {
        // Regenschleier: Bahnen, die mit dem Wind über das Bild ziehen
        noise_grid(g, static_cast<int32_t>(-d / 2), -t * 12, 256 / 6, 256 / 14, -s.wind_dir * 40, 0x2A1);
        for (int i = 0; i < W * H; ++i)
            g[i] = static_cast<uint8_t>(std::clamp((g[i] - 120) * 6 / 70, 0, 6) * veil >> 8);
        L.veil_table = veil_;
    }
    L.veil_grid = grid_;
}

void PlaceWeather::draw(Canvas& fine, const CoarseLayers& L, const uint8_t* remap) const {
    if (!pool_.ready() || pool_.size() == 0) return;
    if (!look_.outdoor && remap) return;  // drinnen erkennt man das Glas nur an den eigenen Farben
    const uint8_t* lm = L.light;
    const int lb = L.light_base;
    auto put = [&](int x, int y, uint8_t c) {
        if (static_cast<unsigned>(x) >= kScreenWidth || static_cast<unsigned>(y) >= kScreenHeight) return;
        uint8_t* px = fine.row(y) + x;
        if (!look_.outdoor && !masked(*px)) return;
        int v = c;
        if (lm) v += (lm[(y >> 1) * kCoarseWidth + (x >> 1)] >> (2 * (((y & 1) << 1) | (x & 1))) & 3) * lb;
        *px = remap ? remap[v] : static_cast<uint8_t>(v);
    };
    const int len = rain_style_ == RainStyle::Streaks ? 5 : rain_style_ == RainStyle::Fine ? 3 : 2;
    for (int i = 0; i < pool_.size(); ++i) {
        const Particle& p = pool_[i];
        const int x = p.x >> 8, y = p.y >> 8;
        switch (p.kind) {
            case ParticleKind::Drop: {
                const int32_t slope = p.vy ? p.vx * 256 / p.vy : 0;  // waagrechter Versatz je Zeile, 8.8
                for (int k = 0; k < len; ++k) {
                    const uint8_t col = rain_style_ == RainStyle::Drops && k == 0 ? look_.splash : look_.rain;
                    put((p.x - slope * k) >> 8, y - k, col);
                }
                break;
            }
            case ParticleKind::Splash:
                if (p.age < 2) {
                    put(x - 1, y - 1, look_.splash);
                    put(x + 1, y - 1, look_.splash);
                } else {
                    put(x - 2, y - 1, look_.rain);
                    put(x + 2, y - 1, look_.rain);
                }
                break;
            case ParticleKind::Flake:
                put(x, y, look_.snow);
                break;
            case ParticleKind::BigFlake:
                put(x, y, look_.snow);
                put(x + 1, y, look_.snow);
                put(x, y + 1, look_.snow);
                put(x + 1, y + 1, look_.snow);
                break;
            case ParticleKind::Leaf: {
                // drei Bildpunkte, kippt alle vier Schritte (taumelt)
                const uint8_t col = look_.leaf[(i & 1)];
                put(x, y, col);
                put(x + 1, y, col);
                put(p.age & 4 ? x + 1 : x, p.age & 4 ? y + 1 : y - 1, col);
                break;
            }
        }
    }
}

}  // namespace pixelia
