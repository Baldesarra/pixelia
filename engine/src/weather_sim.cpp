#include "pixelia/engine/weather_sim.h"

#include <algorithm>

#include "pixelia/engine/rng.h"

namespace pixelia {

namespace {

// Gewichte je Jahreszeit und Wetterzeit (morgens, mittags, abends, nachts) in der Reihenfolge von WeatherKind:
// Sonne, bewölkt, Regen, Schnee, Nebel, Gewitter, Wind
constexpr uint8_t kTable[4][4][kWeatherKinds] = {
    {{30, 30, 15, 0, 15, 0, 10}, {40, 28, 20, 0, 0, 4, 8}, {38, 30, 20, 0, 2, 3, 7}, {42, 32, 18, 0, 6, 0, 2}},   // Frühling
    {{52, 18, 8, 0, 14, 0, 8}, {52, 18, 10, 0, 0, 14, 6}, {50, 20, 10, 0, 0, 14, 6}, {60, 20, 10, 0, 6, 2, 2}},  // Sommer
    {{14, 30, 18, 0, 30, 0, 8}, {28, 32, 22, 0, 4, 1, 13}, {25, 32, 24, 0, 6, 0, 13}, {22, 32, 20, 0, 22, 0, 4}}, // Herbst
    {{18, 30, 5, 25, 20, 0, 2}, {25, 30, 6, 30, 5, 0, 4}, {22, 30, 6, 30, 8, 0, 4}, {25, 30, 4, 28, 12, 0, 1}},   // Winter
};
constexpr int kKeepPercent = 40;  // Wetterlage hält
constexpr int kChain = 8;         // Wetterzeiten Vorlauf der Kette

uint64_t mix(uint64_t seed, int64_t key, uint64_t salt) {
    uint64_t z = seed ^ (static_cast<uint64_t>(key) * 0x9E3779B97F4A7C15ULL) ^ salt;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Rand der Jahreszeit für Schnee: 1 = letzte zwei Tage des Herbstes, 2 = erste zwei Tage des Frühlings
int edge_of(const WorldCalendar& cal, int64_t day) {
    if (!cal.started()) return 0;
    int no;
    int64_t first, last;
    cal.season_at(day, &no, &first, &last);
    const Season s = static_cast<Season>(no & 3);
    if (s == Season::Autumn && day >= last - 1) return 1;
    if (s == Season::Spring && no > 0 && day <= first + 1) return 2;
    return 0;
}

}  // namespace

WeatherPick WeatherSim::roll(Season s, WeatherSlot slot, int edge, WeatherKind prev, uint64_t key) const {
    Rng rng(mix(seed_, static_cast<int64_t>(key), 0x57E77E5ULL));
    int w[kWeatherKinds];
    for (int k = 0; k < kWeatherKinds; ++k) w[k] = kTable[static_cast<int>(s)][static_cast<int>(slot)][k];
    if (edge == 1) w[static_cast<int>(WeatherKind::Snow)] += 8;
    if (edge == 2) w[static_cast<int>(WeatherKind::Snow)] += 6;
    if (prev == WeatherKind::Storm) w[static_cast<int>(WeatherKind::Rain)] += 20;  // nach dem Gewitter regnet es aus
    WeatherPick p;
    const int keep = prev == WeatherKind::Storm ? 15 : kKeepPercent;
    if (w[static_cast<int>(prev)] > 0 && static_cast<int>(rng.below(100)) < keep) {
        p.kind = prev;
    } else {
        int sum = 0;
        for (int v : w) sum += v;
        int r = static_cast<int>(rng.below(static_cast<uint32_t>(sum)));
        int k = 0;
        while (k < kWeatherKinds - 1 && r >= w[k]) r -= w[k++];
        p.kind = static_cast<WeatherKind>(k);
    }
    switch (p.kind) {
        case WeatherKind::Clear: p.strength = 255; break;
        case WeatherKind::Cloudy: p.strength = static_cast<uint8_t>(rng.range(140, 255)); break;
        case WeatherKind::Rain:
        case WeatherKind::Snow: {
            const uint32_t t = rng.below(100);  // leicht, mittel, kräftig
            p.strength = static_cast<uint8_t>(t < 50 ? rng.range(90, 150) : t < 85 ? rng.range(150, 210) : rng.range(210, 255));
            break;
        }
        case WeatherKind::Storm: p.strength = static_cast<uint8_t>(rng.range(200, 255)); break;
        default: p.strength = static_cast<uint8_t>(rng.range(130, 255)); break;
    }
    // Wind meist aus Westen (am Bild: nach links), einen Tag lang gleich
    Rng day_rng(mix(seed_, day_of_key(static_cast<int64_t>(key)), 0x3E57ULL));
    p.wind_dir = day_rng.below(100) < 70 ? -1 : 1;
    return p;
}

WeatherPick WeatherSim::slot(const WorldCalendar& cal, int64_t key) const {
    WeatherKind prev = WeatherKind::Cloudy;
    WeatherPick p;
    for (int64_t k = key - kChain; k <= key; ++k) {
        const int64_t day = day_of_key(k);
        p = roll(cal.season_of(day), slot_of_key(k), edge_of(cal, day), prev, static_cast<uint64_t>(k));
        prev = p.kind;
    }
    return p;
}

bool WeatherSim::burst(const WorldCalendar& cal, int64_t key, int* from, int* to) const {
    const WeatherSlot sl = slot_of_key(key);
    if (sl != WeatherSlot::Midday && sl != WeatherSlot::Evening) return false;
    const Season s = cal.season_of(day_of_key(key));
    const int chance = s == Season::Summer ? 18 : s == Season::Winter || sl == WeatherSlot::Evening ? 0 : 4;
    if (!chance) return false;
    const WeatherKind k = slot(cal, key).kind;
    if (k != WeatherKind::Clear && k != WeatherKind::Cloudy && k != WeatherKind::Wind) return false;  // schwül genug
    Rng rng(mix(seed_, key, 0xB0057ULL));
    if (static_cast<int>(rng.below(100)) >= chance) return false;
    *from = rng.range(30, 240);
    *to = *from + rng.range(45, 100);
    return true;
}

WeatherPick WeatherSim::at(const WorldCalendar& cal, int64_t day, int minute) const {
    const int64_t key = weather_slot_key(day, minute);
    WeatherPick p = slot(cal, key);
    int from, to;
    if (burst(cal, key, &from, &to)) {
        const int start = kSlotStart[static_cast<int>(slot_of_key(key))];
        const int in = ((minute - start) % 1440 + 1440) % 1440;
        if (in >= from && in < to) {
            p.kind = WeatherKind::Storm;
            p.strength = 230;
            p.burst = true;
        }
    }
    return p;
}

}  // namespace pixelia
