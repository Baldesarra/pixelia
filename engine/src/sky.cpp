#include "pixelia/engine/sky.h"

#include <algorithm>
#include <cmath>

#include "pixelia/engine/system_ui.h"
#include "pixelia/engine/weather.h"

namespace pixelia {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRad = kPi / 180.0;
constexpr int kSolarNoon = 12 * 60 + 45;  // wie die Sonne im Kalender (calendar.cpp)

double norm360(double a) {
    a = std::fmod(a, 360.0);
    return a < 0 ? a + 360.0 : a;
}
}  // namespace

MoonInfo moon_at(int64_t unix_s, int minute) {
    // Julianisches Datum und Jahrhunderte seit J2000 (Meeus Kap. 48, Phasenwinkel mit den größten Gliedern)
    const double jd = static_cast<double>(unix_s) / 86400.0 + 2440587.5;
    const double t = (jd - 2451545.0) / 36525.0;
    const double d = norm360(297.8501921 + 445267.1114034 * t);   // mittlere Elongation
    const double m = norm360(357.5291092 + 35999.0502909 * t);    // mittlere Anomalie der Sonne
    const double mp = norm360(134.9633964 + 477198.8675055 * t);  // mittlere Anomalie des Mondes
    const double i = norm360(180.0 - d - 6.289 * std::sin(mp * kRad) + 2.100 * std::sin(m * kRad) -
                             1.274 * std::sin((2 * d - mp) * kRad) - 0.658 * std::sin(2 * d * kRad) -
                             0.214 * std::sin(2 * mp * kRad) - 0.110 * std::sin(d * kRad));
    MoonInfo r;
    const double elong = norm360(180.0 - i);  // 0 Neumond … 180 Vollmond … 360
    r.phase = elong / 360.0;
    r.illum = static_cast<int>(std::lround((1.0 + std::cos(i * kRad)) / 2.0 * 1000.0));
    r.waxing = elong < 180.0;
    // Lage: Durchgang im Süden je Phase später als die Sonne (Neumond mittags, Vollmond um Mitternacht)
    int ha = static_cast<int>(minute - kSolarNoon - std::lround(r.phase * 1440.0));
    ha = ((ha % 1440) + 1440 + 720) % 1440 - 720;  // −720 … 719 Minuten vom Durchgang
    if (std::abs(ha) < 360) {
        r.up = static_cast<int>(std::lround(std::cos(ha / 360.0 * kPi / 2) * 256));
        r.x = 128 + ha * 128 / 360;
    } else {
        r.up = 0;
        r.x = ha < 0 ? 0 : 256;
    }
    return r;
}

int moon_octant(const MoonInfo& m) { return static_cast<int>(std::floor(m.phase * 8 + 0.5)) % 8; }

const char* moon_phase_title(const MoonInfo& m) {
    constexpr const char* k[] = {"Neumond",          "zunehmende Sichel", "erstes Viertel", "zunehmender Mond",
                                 "Vollmond",         "abnehmender Mond",  "letztes Viertel", "abnehmende Sichel"};
    return k[moon_octant(m)];
}

void Sky::force(double phase, int up, int x) {
    forced_ = phase;
    forced_up_ = std::clamp(up, 0, 256);
    forced_x_ = std::clamp(x, 0, 256);
}

void Sky::update(const CalendarDate& date, int64_t unix_s, const WeatherState& w) {
    sun_ = date.sun;
    const int m = date.minute;
    // Dunkelheit nach den Sonnenzeiten: eine Stunde Dämmerung nach Untergang und vor Aufgang
    if (m >= sun_.set + 60 || m < sun_.rise - 60) dark_ = 256;
    else if (m >= sun_.set) dark_ = (m - sun_.set) * 256 / 60;
    else if (m < sun_.rise) dark_ = (sun_.rise - m) * 256 / 60;
    else dark_ = 0;
    moon_ = moon_at(unix_s, m);
    if (forced_ >= 0) {
        moon_.phase = forced_;
        moon_.illum = static_cast<int>(std::lround((1.0 - std::cos(forced_ * 2 * kPi)) / 2.0 * 1000.0));
        moon_.waxing = forced_ < 0.5;
        moon_.up = forced_up_;
        moon_.x = forced_x_;
    }
    // Klarer Himmel: Wolken und Nebel verdecken ihn, bei Regen und Schnee ist er weg
    const int cover = std::max<int>(w.cloud, w.fog);
    const int clear = w.rain > 40 || w.snow > 40 ? 0 : std::clamp((200 - cover) * 256 / 200, 0, 256);
    visible_ = moon_.up > 0 && dark_ >= 160 && clear >= 128 && moon_.illum > 20;
    light_ = moon_.illum * std::min(256, moon_.up * 3) / 1000 * dark_ / 256 * clear / 256;
}

void Sky::apply(Ambient& a, bool outdoor) const {
    if (a.lamp <= 0 || light_ <= 0) return;
    // Vollmond: Nacht um etwa ein Drittel heller, kühl (gemessen an der Mindesthelligkeit kDefaultNightFloor)
    const int k = light_ * std::min(a.lamp, 256) / (outdoor ? 256 : 512);
    a.r += 20 * k / 256;
    a.g += 26 * k / 256;
    a.b += 36 * k / 256;
}

void Sky::draw(Canvas& fine, const SkyView& view) const {
    if (!visible_ || view.count <= 0 || view.colors <= 0) return;
    const int n = view.count;
    // Lage: der Himmel läuft über alle Flächen von links (Osten) nach rechts (Westen)
    const int rx = moon_.x * n;
    const int idx = std::clamp(rx / 257, 0, n - 1);
    const int local = std::clamp(rx - idx * 256, 0, 256);
    const Rect& p = view.panes[idx];
    const int r = style_ == MoonStyle::Craters ? 7 : 5;
    const int span_x = std::max(0, p.w - 2 * r - 2), span_y = std::max(0, p.h - 2 * r - 2);
    const int cx = p.x + r + 1 + local * span_x / 256;
    const int cy = p.y + r + 1 + (256 - moon_.up) * span_y / 256;
    draw_disc(fine, cx, cy, view, p);
}

void Sky::draw_disc(Canvas& fine, int cx, int cy, const SkyView& view, const Rect& clip) const {
    const int r = style_ == MoonStyle::Craters ? 7 : 5;
    const double rr = (r + 0.5) * (r + 0.5);
    const double c = std::cos(moon_.phase * 2 * kPi);
    // Flecken (Meere) für das Aussehen mit Kratern: Lage relativ zur Mitte, Halbmesser²
    struct Spot {
        int x, y, r2;
    };
    constexpr Spot kSpots[] = {{-3, -3, 4}, {2, -2, 3}, {3, 1, 2}, {-4, 1, 4}, {-1, 4, 2}};
    const bool halo = style_ == MoonStyle::Halo && moon_.illum > 300;
    const int reach = r + (halo ? 3 : 0);
    const int x0 = std::max({cx - reach, clip.x, 0}), x1 = std::min({cx + reach, clip.x + clip.w - 1, fine.width() - 1});
    const int y0 = std::max({cy - reach, clip.y, 0}), y1 = std::min({cy + reach, clip.y + clip.h - 1, fine.height() - 1});
    for (int y = y0; y <= y1; ++y) {
        uint8_t* row = fine.row(y);
        const int dy = y - cy;
        for (int x = x0; x <= x1; ++x) {
            const uint8_t px = row[x];
            if (px == 0 || px >= kSceneColors) continue;
            const int i = px % view.base;
            if (i < view.first || i >= view.first + view.colors) continue;  // nur über Himmel bzw. Glas
            const int dx = x - cx;
            const double d2 = dx * dx + dy * dy;
            if (d2 > rr) {
                if (!halo) continue;
                // Hof: feines Raster, nach außen dünner, mit der Helligkeit des Mondes
                const double d = std::sqrt(d2) - (r + 0.5);
                const int level = d < 1.0 ? 6 : d < 2.0 ? 3 : d < 3.0 ? 1 : 0;
                if (dither4(x, y, level * moon_.illum / 1000)) row[x] = ui::kOutline;
                continue;
            }
            const double w = std::sqrt(std::max(0.0, rr - dy * dy));
            const bool lit = moon_.waxing ? dx + 0.5 > w * c : dx - 0.5 < -w * c;
            if (!lit) {
                if (style_ != MoonStyle::Plain) row[x] = ui::kSurfaceHigh;  // dunkle Seite, kaum sichtbar
                continue;
            }
            uint8_t col = ui::kText;
            if (style_ == MoonStyle::Craters)
                for (const Spot& s : kSpots)
                    if ((dx - s.x) * (dx - s.x) + (dy - s.y) * (dy - s.y) <= s.r2) col = ui::kTextDim;
            row[x] = col;
        }
    }
}

}  // namespace pixelia
