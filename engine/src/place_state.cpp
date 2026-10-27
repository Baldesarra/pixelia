#include "pixelia/engine/place_state.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace pixelia {

namespace {

constexpr LightKindInfo kKinds[] = {
    {"glut", "Glut", 1, 0, false},
    {"kienspan", "Kienspan", 2, 20, true},
    {"talgkerze", "Talgkerze", 2, 150, true},
    {"wachskerze", "Wachskerze", 3, 180, true},
    {"kamin", "Kaminfeuer", 3, 0, true},
    {"laterne", "Laterne", 3, 150, true},
    {"oellampe", "Rüböllampe", 4, 150, true},
    {"fackel", "Fackel", 4, 45, true},
};
static_assert(sizeof(kKinds) / sizeof(kKinds[0]) == static_cast<int>(LightKind::kCount), "eine Zeile je Art");

// Lichtkreis je Stärke: Halbmesser (3/4-Sicht), Helligkeit
struct Reach {
    uint8_t rx, ry, strength;
};
constexpr Reach kReach[kMaxStrength + 1] = {{0, 0, 0}, {16, 11, 150}, {30, 21, 200}, {46, 31, 235}, {60, 40, 255}};

// Feuer: Hitze 0–1000; brennend steigt sie schnell, offen kühlt die Glut in gut drei Stunden aus, abgedeckt hält sie
// bei höchstens 400 die Nacht über (gut 12 Stunden bis kalt)
constexpr int kHot = 1000, kEmber = 150, kBanked = 400;
constexpr int kAwake = 5 * 60 + 30, kBed = 23 * 60;   // Hausregel Licht: wach von 5:30 bis 23 Uhr
constexpr int kCover = 22 * 60;                       // Herdfeuer: abdecken
constexpr int kDusk = 20;                             // Minuten nach Sonnenuntergang bzw. vor Sonnenaufgang

}  // namespace

const LightKindInfo& light_kind(LightKind k) {
    const int i = static_cast<int>(k);
    return kKinds[i >= 0 && i < static_cast<int>(LightKind::kCount) ? i : 0];
}

LightCircle light_circle(int x, int y, int strength) {
    const Reach& r = kReach[std::clamp(strength, 0, kMaxStrength)];
    LightCircle c{};
    c.x = static_cast<int16_t>(x);
    c.y = static_cast<int16_t>(y);
    c.rx = r.rx;
    c.ry = r.ry;
    c.strength = r.strength;
    return c;
}

int PlaceStates::declare(const char* place, const StateDef* defs, int n, const char* house, bool open_sky) {
    if (!place || n <= 0) return -1;
    int pi = -1;
    for (int k = 0; k < places_n_; ++k)
        if (std::strcmp(places_[k].id, place) == 0) pi = k;
    if (pi >= 0) {
        int count = 0;
        const int first = this->first(place, &count);
        for (int k = 0; k < std::min(n, count); ++k) {
            e_[first + k].def = &defs[k];
            reset(e_[first + k]);
        }
        places_[pi].house = house;
        places_[pi].sky = open_sky;
        return first;
    }
    if (places_n_ >= kMaxPlaces || n_ + n > kMax) return -1;
    pi = places_n_++;
    places_[pi] = Place{place, house, open_sky};
    const int first = n_;
    for (int k = 0; k < n; ++k) {
        Entry& e = e_[n_++];
        e = Entry{};
        e.def = &defs[k];
        e.place = static_cast<uint8_t>(pi);
        reset(e);
    }
    return first;
}

void PlaceStates::reset(Entry& e) const {
    const StateDef& d = *e.def;
    e.value = e.heat = e.frac = 0;
    e.on = e.covered = e.want = e.ruled = false;
    switch (d.kind) {
        case StateKind::Light:
            e.value = std::max<uint16_t>(1, light_kind(d.light).burn);
            e.on = d.rule != StateRule::Homecomer;  // wie bis 0.5.7: Lichter brennen, bis die Welt läuft
            break;
        case StateKind::Fill: e.value = static_cast<uint16_t>(d.capacity / 2); break;
        case StateKind::Work: e.value = static_cast<uint16_t>(std::min(65535, d.capacity * 60)); break;
        default: break;
    }
    e.phase = static_cast<uint8_t>(compute_phase(e));
}

int PlaceStates::find(std::string_view place, std::string_view id) const {
    for (int i = 0; i < n_; ++i)
        if (place == places_[e_[i].place].id && id == e_[i].def->id) return i;
    return -1;
}

int PlaceStates::first(std::string_view place, int* n) const {
    int first = -1, count = 0;
    for (int i = 0; i < n_; ++i)
        if (place == places_[e_[i].place].id) {
            if (first < 0) first = i;
            ++count;
        }
    if (n) *n = count;
    return first;
}

int PlaceStates::compute_phase(const Entry& e) const {
    const StateDef& d = *e.def;
    switch (d.kind) {
        case StateKind::Light: return e.on ? 1 : 0;
        case StateKind::Fire: return e.on ? 2 : e.heat >= kEmber ? 1 : 0;
        case StateKind::Wetness: return e.value < 100 ? 0 : e.value < 400 ? 1 : e.value < 750 ? 2 : 3;
        case StateKind::Fill: {
            if (d.capacity == 0) return 0;
            const int pct = e.value * 100 / d.capacity;
            return pct < 5 ? 0 : pct < 35 ? 1 : pct < 75 ? 2 : 3;
        }
        case StateKind::Work: return e.value == 0 ? 0 : e.value >= d.capacity * 60 ? 2 : 1;
    }
    return 0;
}

void PlaceStates::update_phase(int i) {
    Entry& e = e_[i];
    const int p = compute_phase(e);
    if (p == e.phase) return;
    e.phase = static_cast<uint8_t>(p);
    ++version_;
    if (!bus_) return;
    WorldEvent ev;
    ev.kind = EventKind::StateChanged;
    ev.place = places_[e.place].id;
    ev.a = static_cast<uint8_t>(i);
    ev.b = static_cast<uint8_t>(p);
    ev.day = day_;
    bus_->publish(ev);
}

int PlaceStates::strength(int i) const {
    const Entry& e = e_[i];
    switch (e.def->kind) {
        case StateKind::Light: return e.on ? light_kind(e.def->light).strength : 0;
        case StateKind::Fire:
            return e.phase == 2 ? light_kind(e.def->light).strength : e.phase == 1 ? light_kind(LightKind::Ember).strength : 0;
        default: return 0;
    }
}

bool PlaceStates::outside(int i) const {
    const Entry& e = e_[i];
    switch (e.def->kind) {
        case StateKind::Light: return e.on && light_kind(e.def->light).outside;
        case StateKind::Fire: return e.phase == 2 && light_kind(e.def->light).outside;
        default: return false;
    }
}

bool PlaceStates::any_light(std::string_view place) const {
    for (int i = 0; i < n_; ++i)
        if (place == places_[e_[i].place].id && strength(i) > 0) return true;
    return false;
}

int PlaceStates::outside_strength(std::string_view place) const {
    int sum = 0;
    for (int i = 0; i < n_; ++i)
        if (place == places_[e_[i].place].id && outside(i)) sum += strength(i);
    return std::min(sum, kMaxOutsideStrength);
}

int PlaceStates::describe(int i, char* buf, int n) const {
    if (i < 0 || i >= n_ || !buf || n <= 0) return 0;
    const Entry& e = e_[i];
    const StateDef& d = *e.def;
    char t[16];
    auto span = [&](int min) {
        if (min >= 60)
            std::snprintf(t, sizeof(t), "%d h %02d", min / 60, min % 60);
        else
            std::snprintf(t, sizeof(t), "%d min", min);
        return t;
    };
    switch (d.kind) {
        case StateKind::Light:
            if (!e.on) return std::snprintf(buf, n, "aus");
            return std::snprintf(buf, n, d.rule == StateRule::Homecomer ? "brennt für Heimkehrer, noch %s" : "brennt, noch %s",
                                 span(e.value));
        case StateKind::Fire:
            if (e.on) return std::snprintf(buf, n, "Feuer, Holz für %s", span(e.value));
            if (e.phase == 1) return std::snprintf(buf, n, e.covered ? "Glut, abgedeckt" : "Glut, kühlt ab");
            return std::snprintf(buf, n, "kalt");
        case StateKind::Wetness: {
            static constexpr const char* kWet[] = {"trocken", "feucht", "nass", "sehr nass"};
            return std::snprintf(buf, n, "Boden %s", kWet[std::min<int>(e.phase, 3)]);
        }
        case StateKind::Fill: return std::snprintf(buf, n, "Wasser: %d von %d Litern", e.value, d.capacity);
        case StateKind::Work:
            if (e.phase == 2) return std::snprintf(buf, n, "fertig");
            return std::snprintf(buf, n, "im Bau, %d %% fertig", d.capacity ? e.value * 100 / (d.capacity * 60) : 0);
    }
    return 0;
}

void PlaceStates::set_on(int i, bool on) {
    if (i < 0 || i >= n_) return;
    Entry& e = e_[i];
    switch (e.def->kind) {
        case StateKind::Light:
            if (on && e.value < std::max(1, light_kind(e.def->light).burn / 4))
                e.value = std::max<uint16_t>(1, light_kind(e.def->light).burn);  // neue Kerze, Lampe gefüllt
            e.on = on;
            break;
        case StateKind::Fire:
            if (on) {
                if (e.value < 30) e.value = 60;  // nachlegen
                e.covered = false;
                e.heat = static_cast<uint16_t>(std::max<int>(e.heat, kEmber));
            }
            e.on = on;
            break;
        default: return;
    }
    update_phase(i);
}

void PlaceStates::cover(int i, bool on) {
    if (i < 0 || i >= n_ || e_[i].def->kind != StateKind::Fire) return;
    Entry& e = e_[i];
    e.covered = on;
    if (on) {
        e.on = false;
        e.value = 0;
    }
    update_phase(i);
}

int PlaceStates::draw(int i, int liters) {
    if (i < 0 || i >= n_ || e_[i].def->kind != StateKind::Fill || liters <= 0) return 0;
    Entry& e = e_[i];
    const int got = std::min<int>(liters, e.value);
    e.value = static_cast<uint16_t>(e.value - got);
    update_phase(i);
    return got;
}

void PlaceStates::work(int i, int seconds) {
    if (i < 0 || i >= n_ || e_[i].def->kind != StateKind::Work || seconds <= 0) return;
    Entry& e = e_[i];
    e.value = static_cast<uint16_t>(std::min(e.def->capacity * 60, e.value + seconds));
    update_phase(i);
}

void PlaceStates::set_value(int i, int v) {
    if (i < 0 || i >= n_) return;
    Entry& e = e_[i];
    int hi = 65535;
    if (e.def->kind == StateKind::Wetness) hi = kWetMax;
    if (e.def->kind == StateKind::Fill) hi = e.def->capacity;
    if (e.def->kind == StateKind::Work) hi = e.def->capacity * 60;
    e.value = static_cast<uint16_t>(std::clamp(v, 0, hi));
    e.frac = 0;
    if (e.def->kind == StateKind::Light && e.value == 0) e.on = false;
    update_phase(i);
}

void PlaceStates::set_heat(int i, int v) {
    if (i < 0 || i >= n_ || e_[i].def->kind != StateKind::Fire) return;
    e_[i].heat = static_cast<uint16_t>(std::clamp(v, 0, kHot));
    update_phase(i);
}

void PlaceStates::step(int minute, const StateInput& in) {
    const bool day = minute >= in.sun.rise && minute < in.sun.set;
    for (int i = 0; i < n_; ++i) {
        Entry& e = e_[i];
        const StateDef& d = *e.def;
        const bool sky = places_[e.place].sky;
        switch (d.kind) {
            case StateKind::Light:
                if (e.on) {
                    if (e.value > 0) --e.value;
                    if (e.value == 0) e.on = false;  // abgebrannt
                }
                break;
            case StateKind::Fire:
                if (e.on) {
                    e.heat = static_cast<uint16_t>(std::min(kHot, e.heat + 100));
                    if (e.value > 0) --e.value;
                    if (e.value == 0) e.on = false;
                } else if (e.covered) {
                    if (e.heat > kBanked)
                        e.heat = static_cast<uint16_t>(std::max(kBanked, e.heat - 10));
                    else if (e.heat > 0 && ++e.frac >= 3) {
                        e.frac = 0;
                        --e.heat;
                    }
                } else {
                    e.heat = static_cast<uint16_t>(std::max(0, e.heat - 4));
                }
                break;
            case StateKind::Wetness: {
                int v16 = e.value * 16 + e.frac;
                if (sky) v16 += in.rain * 40 * 16 / 255;  // voller Regen: in 25 Minuten ganz nass
                if (!sky || in.rain < 20) v16 -= day ? in.dry_day : in.dry_night;
                v16 = std::clamp(v16, 0, kWetMax * 16);
                e.value = static_cast<uint16_t>(v16 / 16);
                e.frac = static_cast<uint16_t>(v16 % 16);
                break;
            }
            case StateKind::Fill:
                if (sky && d.rain > 0 && in.rain > 0) {
                    e.frac = static_cast<uint16_t>(e.frac + d.rain * in.rain / 255);  // in 1/60 Liter
                    e.value = static_cast<uint16_t>(std::min<int>(d.capacity, e.value + e.frac / 60));
                    e.frac %= 60;
                }
                break;
            case StateKind::Work: break;  // bleibt liegen
        }
        update_phase(i);
        if (d.rule != StateRule::None) apply_rule(i, minute, in);
    }
}

void PlaceStates::apply_rule(int i, int minute, const StateInput& in) {
    Entry& e = e_[i];
    const StateDef& d = *e.def;
    const bool dark = minute < in.sun.rise - kDusk || minute >= in.sun.set + kDusk;
    switch (d.rule) {
        case StateRule::Evening:
        case StateRule::Homecomer: {
            bool want = dark;
            if (d.rule == StateRule::Evening) {
                want = want && minute >= kAwake && minute < kBed;
            } else {
                const char* house = places_[e.place].house;
                const bool home = in.lead_house && house && std::strcmp(in.lead_house, house) == 0;
                want = want && in.lead_known && !home;
            }
            if (e.ruled && want == e.want) return;  // nur bei Wechseln: was jemand von Hand macht, bleibt
            e.ruled = true;
            e.want = want;
            set_on(i, want);
            return;
        }
        case StateRule::Hearth: {
            const bool window = (minute >= 6 * 60 && minute < 8 * 60) || (minute >= 11 * 60 + 30 && minute < 13 * 60) ||
                                (minute >= 17 * 60 && minute < kCover);
            const bool night = minute >= kCover || minute < 6 * 60;
            if (minute == kCover || (!e.ruled && night)) {
                if (!e.ruled) set_heat(i, 300);  // Start in der Nacht: abgedeckte Glut von gestern
                cover(i, true);
            } else if (window) {
                if (!e.on)
                    set_on(i, true);
                else if (e.value < 20)
                    e.value = static_cast<uint16_t>(e.value + 60);  // nachlegen
            }
            e.ruled = true;
            return;
        }
        case StateRule::None: return;
    }
}

PlaceStates::Saved PlaceStates::saved(int i) const {
    const Entry& e = e_[i];
    Saved s;
    s.value = e.value;
    s.heat = e.heat;
    s.frac = e.frac;
    s.bits = static_cast<uint8_t>((e.on ? 1 : 0) | (e.covered ? 2 : 0) | (e.want ? 4 : 0) | (e.ruled ? 8 : 0));
    s.phase = e.phase;
    return s;
}

void PlaceStates::restore(int i, const Saved& s) {
    if (i < 0 || i >= n_) return;
    Entry& e = e_[i];
    e.value = s.value;
    e.heat = s.heat;
    e.frac = s.frac;
    e.on = s.bits & 1;
    e.covered = s.bits & 2;
    e.want = s.bits & 4;
    e.ruled = s.bits & 8;
    e.phase = static_cast<uint8_t>(compute_phase(e));  // aus den Werten (die gespeicherte Stufe ist nur ein Hinweis)
    ++version_;
    ++restored_;
}

}  // namespace pixelia
