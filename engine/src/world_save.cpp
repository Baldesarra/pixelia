// Weltzustand der Bühne (seit 0.5.9): Startbesetzung, Übergaben im Weltzustand, Chronik und Besuche, Prüfung „jede
// Figur genau einmal“, Speicherstand (Format save.h) und Nachholen nach dem Laden.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

#include "pixelia/engine/place.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/tiles.h"

namespace pixelia {

namespace {

// Eigenschaften der Testfiguren (Gebiete ohne Figurendaten): nur Prüfinhalt, echte Figuren ab 0.6.0
constexpr const char* kTestTraits[] = {"fleissig", "neugierig", "ruhig", "gesellig", "stur", "geduldig"};
constexpr const char* kTestInterests[] = {"wetter", "holz", "brot", "musik", "tiere", "steine"};
constexpr Skill kTestSkills[] = {Skill::Baking, Skill::Joinery, Skill::Gardening, Skill::Fishing, Skill::Music,
                                 Skill::Smithing};

int64_t minute_of_us(int64_t us) {
    return us >= 0 ? us / WorldClock::kUsPerMinute : (us - WorldClock::kUsPerMinute + 1) / WorldClock::kUsPerMinute;
}

// Dauer in Worten: „5 Std. 12 Min.“, „2 Tage 3 Std.“
void duration_text(int64_t minutes, char* out, size_t n) {
    const int64_t d = minutes / 1440, h = minutes % 1440 / 60, m = minutes % 60;
    if (d > 0)
        std::snprintf(out, n, "%d %s %d Std.", static_cast<int>(d), d == 1 ? "Tag" : "Tage", static_cast<int>(h));
    else if (h > 0)
        std::snprintf(out, n, "%d Std. %d Min.", static_cast<int>(h), static_cast<int>(m));
    else
        std::snprintf(out, n, "%d Min.", static_cast<int>(m));
}

// Abschnitte des Speicherstands (Kennung, Version)
constexpr char kWorld[] = "WELT", kCal[] = "KAL ", kPlaces[] = "ORTE", kStates[] = "ZUST", kFlags[] = "MERK",
               kFigs[] = "FIGS", kChron[] = "CHRO", kVisits[] = "BESU",
               kDoings[] = "TAET";  // seit 0.5.9 Teil 2: Takt-Grenze und Tätigkeiten

}  // namespace

int Stage::scene_of(int place) const {
    if (place < 0) return -1;
    for (int i = 0; i < count_; ++i)
        if (place_of_[i] == place) return i;
    return -1;
}

int64_t Stage::catching_up() const {
    return state_minute_ != INT64_MIN && world_minute_ != INT64_MIN && world_minute_ > state_minute_
               ? world_minute_ - state_minute_
               : 0;
}

// --- Startbesetzung und Übergaben ---

void Stage::populate_figures() {
    // aus den Figurendaten der Gebiete; ohne Daten je Ort PlaceDef::figures Testfiguren (die erste eines Ortes mit
    // guide ist die Begleiterin, sie wohnt im ersten Haushalt des Gebiets), Zuhause = Haushalt des Startortes
    FigureBook& book = env_.figures;
    book.clear();
    const AreaDef* const* areas = nullptr;
    const int na = world_areas(&areas);
    int n = 0;
    for (int a = 0; a < na; ++a) {
        const AreaDef& area = *areas[a];
        if (area.figure_count > 0) {
            for (int k = 0; k < area.figure_count; ++k) {
                const FigureDef& d = area.figures[k];
                const int p = d.place ? place_index(d.place) : -1;
                book.add(d, static_cast<uint8_t>(p < 0 ? 0 : p));
            }
            continue;
        }
        const char* house = nullptr;
        for (int i = 0; i < area.count && !house; ++i) house = area.places[i]->house;
        for (int i = 0; i < area.count; ++i) {
            const PlaceDef& pd = *area.places[i];
            for (int k = 0; k < pd.figures; ++k) {
                const bool lead = pd.guide && k == 0;
                char name[24];
                if (lead)
                    std::snprintf(name, sizeof(name), "Begleiterin");
                else
                    std::snprintf(name, sizeof(name), "Testfigur %d", ++n);
                FigureDef d{};
                d.name = name;
                d.age = static_cast<uint8_t>(22 + (n * 7) % 47);
                d.role = "test";
                d.home = lead ? house : pd.house;
                d.chrono = static_cast<Chronotype>(n % 3);
                d.traits[0] = kTestTraits[n % 6];
                d.traits[1] = kTestTraits[(n + 2) % 6];
                d.interests[0] = {kTestInterests[n % 6], static_cast<uint8_t>(120 + n * 9 % 100)};
                d.skills[0] = {kTestSkills[n % 6], static_cast<SkillLevel>(1 + n % 3)};
                d.look.row[0] = static_cast<uint8_t>(1 + k % 4);  // Testfiguren: Farbreihe je Ort wie bis 0.5.8
                d.lead = lead;
                book.add(d, static_cast<uint8_t>(place_index(pd.id)));
            }
        }
    }
}

Travel Stage::travel_of(const FigureRecord& f, int to_scene) const {
    Travel t;
    t.to = scenes_[to_scene]->name();
    t.entry = f.where.entry;
    t.from_exit = f.where.from_exit;
    t.transition = static_cast<Transition>(std::min<int>(f.where.transition, kTransitionKinds - 1));
    t.follow = f.lead();
    t.who = f.id;
    t.look = f.look.row[0];
    const int64_t s = (f.where.arrive_us - f.where.depart_us) / 1000000;
    t.travel = static_cast<uint16_t>(std::clamp<int64_t>(s, 0, 0xFFFF));
    return t;
}

void Stage::adopt_figures() {
    // jede Figur an einem Ort bekommt eine Kachel (irgendwo frei) und entscheidet an der ersten Takt-Grenze; in der
    // Reihenfolge des Weltzustands, mit dem Zufall je Figur (seit 0.5.9 Teil 2, world_sim.cpp)
    FigureBook& book = env_.figures;
    for (int k = 0; k < book.count(); ++k) {
        FigureRecord& f = book.at(k);
        if (f.where.kind != Whereabouts::Kind::Place) continue;
        const int s = scene_of(f.where.place);
        if (s >= 0) start_doing(f, s, kAnyEntry, INT64_MIN);
    }
}

void Stage::rebuild_transit() {
    // Figuren auf Verbindungen in der Reihenfolge des Aufbruchs (gleichzeitig: nach Kennung)
    transit_n_ = 0;
    FigureBook& book = env_.figures;
    for (int k = 0; k < book.count(); ++k) {
        FigureRecord& f = book.at(k);
        if (f.where.kind != Whereabouts::Kind::Link) continue;
        const int to = scene_of(f.where.place), from = scene_of(f.where.from);
        if (to < 0 || from < 0 || transit_n_ >= kMaxTransit) {
            // Ziel nicht auf der Bühne (oder zu viele unterwegs): gleich angekommen
            const uint8_t entry = f.where.entry;
            f.where.kind = Whereabouts::Kind::Place;
            if (to >= 0) start_doing(f, to, entry, INT64_MIN);
            continue;
        }
        transit_[transit_n_++] = Transit{travel_of(f, to), f.where.depart_us, f.where.arrive_us,
                                         static_cast<uint8_t>(from), static_cast<uint8_t>(to)};
    }
    std::stable_sort(transit_.begin(), transit_.begin() + transit_n_, [](const Transit& a, const Transit& b) {
        return a.depart_us != b.depart_us ? a.depart_us < b.depart_us : a.t.who < b.t.who;
    });
}

void Stage::set_where_place(uint16_t who, int scene) {
    FigureRecord* f = env_.figures.find(who);
    if (!f) return;
    f->where = Whereabouts{};
    f->where.kind = Whereabouts::Kind::Place;
    f->where.place = static_cast<uint8_t>(place_of_[scene]);
}

void Stage::note_arrival(uint16_t who, int scene) {
    // Besuch: Ort eines fremden Haushalts betreten; innerhalb desselben Haushalts läuft der Besuch weiter
    const FigureRecord* f = env_.figures.find(who);
    if (!f) return;
    const CalendarDate& d = env_.calendar.date();
    const PlaceDef* here = find_place(scenes_[scene]->name());
    const char* house = here ? here->house : nullptr;
    const int open = env_.visits.open(who);
    if (open >= 0) {
        const PlaceDef* before = place_at(env_.visits.at(open).place);
        if (house && before && before->house && std::strcmp(before->house, house) == 0) return;
        env_.visits.leave(who, d.day, d.minute);
    }
    if (house && std::strcmp(f->home.data(), house) != 0)
        env_.visits.arrive(who, static_cast<uint8_t>(place_of_[scene]), d.day, d.minute);
}

void Stage::read_chronicle() {
    const CalendarDate& d = env_.calendar.date();
    for (WorldEvent e; env_.events.poll(chron_reader_, &e);) env_.chronicle.note(e, env_.figures, d.day, d.minute);
}

void Stage::note_wake() {
    // nach dem Laden, sobald die Weltzeit läuft: wie lange Pixelia aus war und was nachgeholt wird
    wake_pending_ = false;
    if (saved_minute_ == INT64_MIN || world_minute_ <= saved_minute_ + 1) return;
    const int64_t gap = world_minute_ - saved_minute_;
    char off[24], done[24];
    duration_text(gap, off, sizeof(off));
    ChronicleEntry c;
    c.kind = ChronicleKind::Wake;
    c.day = env_.calendar.date().day;
    c.minute = static_cast<uint16_t>(env_.calendar.date().minute);
    if (gap > kCatchUpMinutes) {
        duration_text(kCatchUpMinutes, done, sizeof(done));
        std::snprintf(c.text, sizeof(c.text), "Pixelia war %.14s aus, nachgeholt: %.14s", off, done);
    } else {
        std::snprintf(c.text, sizeof(c.text), "Pixelia war %.20s aus", off);
    }
    env_.chronicle.add(c);
}

const char* Stage::check_figures(char* buf, int n) const {
    // seit 0.5.9 Teil 2: Wo eine Figur ist, sagt allein ihr Feld where; gezeigt wird sie nur in einem betretenen Ort,
    // dort aber genau einmal (die Darstellung folgt dem Weltzustand in jedem Schritt)
    const FigureBook& book = env_.figures;
    for (int k = 0; k < book.count(); ++k) {
        const FigureRecord& f = book.at(k);
        int seen = 0, at = -1, moving = 0;
        for (int i = 0; i < count_; ++i)
            if (scenes_[i]->has_figure(f.id)) {
                ++seen;
                at = i;
            }
        for (int j = 0; j < transit_n_; ++j) moving += transit_[j].t.who == f.id;
        const char* err = nullptr;
        if (f.where.kind == Whereabouts::Kind::Nowhere) {
            err = "ist nirgends";
        } else if (f.where.kind == Whereabouts::Kind::Place) {
            const int s = scene_of(f.where.place);
            if (moving) err = "gehört einem Ort, ist aber unterwegs";
            else if (seen > 1) err = "ist doppelt im Bild";
            else if (seen && at != s) err = "ist im falschen Ort";
            else if (!seen && s >= 0 && entered(s)) err = "fehlt in ihrem Ort";
        } else if (seen || moving != 1) {
            err = seen ? "ist unterwegs und zugleich im Bild" : "ist unterwegs, aber nicht auf der Verbindung";
        }
        if (err) {
            std::snprintf(buf, n, "Figur %u (%s) %s (%d-mal im Bild, %d-mal unterwegs)", static_cast<unsigned>(f.id),
                          f.name.data(), err, seen, moving);
            return buf;
        }
    }
    for (int j = 0; j < transit_n_; ++j) {
        const FigureRecord* f = book.find(transit_[j].t.who);
        if (f && f->where.kind != Whereabouts::Kind::Link) {
            std::snprintf(buf, n, "Figur %u auf der Verbindung, laut Welt aber nicht", static_cast<unsigned>(f->id));
            return buf;
        }
    }
    return nullptr;
}

// --- Speicherstand ---

size_t Stage::save_state(uint8_t* buf, size_t cap, uint32_t generation) const {
    SaveWriter w(buf, cap);
    w.begin(generation, world_us_);

    w.chunk(kWorld, 1);
    w.u64(seed_);
    w.i64(state_minute_);
    w.u16(env_.figures.next_id());

    const CalendarState& cs = env_.calendar.state();
    w.chunk(kCal, 1);
    w.u8(cs.season_len);
    w.u16(cs.firsts);
    w.i32(cs.start_day);
    w.i32(cs.anchor_first);
    w.i32(cs.anchor_season);
    w.i32(cs.anchor_min_last);
    w.i32(cs.firsts_year);

    // Ortsliste: Nummer im Speicherstand = Nummer in der Welt beim Speichern; beim Laden über die Kennung neu
    const AreaDef* const* areas = nullptr;
    const int na = world_areas(&areas);
    int places = 0;
    for (int a = 0; a < na; ++a) places += areas[a]->count;
    w.chunk(kPlaces, 1);
    w.u16(static_cast<uint16_t>(places));
    for (int a = 0; a < na; ++a)
        for (int i = 0; i < areas[a]->count; ++i) w.str(areas[a]->places[i]->id);

    const PlaceStates& st = env_.states;
    w.chunk(kStates, 1);
    w.u16(static_cast<uint16_t>(st.count()));
    for (int i = 0; i < st.count(); ++i) {
        const size_t r = w.record_begin();
        const PlaceStates::Saved v = st.saved(i);
        w.u8(static_cast<uint8_t>(place_index(st.place(i))));
        w.str(st.def(i).id);
        w.u16(v.value);
        w.u16(v.heat);
        w.u16(v.frac);
        w.u8(v.bits);
        w.u8(v.phase);
        w.record_end(r);
    }

    w.chunk(kFlags, 1);
    w.u8(static_cast<uint8_t>(env_.flags.count()));
    for (int i = 0; i < env_.flags.count(); ++i) w.str(env_.flags.key(i));

    const FigureBook& book = env_.figures;
    w.chunk(kFigs, 1);
    w.u16(static_cast<uint16_t>(book.count()));
    for (int k = 0; k < book.count(); ++k) {
        const FigureRecord& f = book.at(k);
        const size_t r = w.record_begin();
        w.u16(f.id);
        w.u8(f.flags);
        w.u8(f.age);
        w.str(f.name.data());
        w.str(f.role.data());
        w.str(f.origin.data());
        w.str(f.goal.data());
        w.str(f.home.data());
        w.u8(static_cast<uint8_t>(f.chrono));
        w.u8(FigureRecord::kTraits);
        for (const auto& t : f.traits) w.str(t.data());
        w.u8(FigureRecord::kInterests);
        for (const auto& in : f.interests) {
            w.str(in.key.data());
            w.u8(in.weight);
        }
        w.u8(kLookLayers);
        for (int l = 0; l < kLookLayers; ++l) {
            w.u8(f.look.part[l]);
            w.u8(f.look.row[l]);
        }
        w.u8(static_cast<uint8_t>(f.where.kind));
        w.u8(f.where.place);
        w.u8(f.where.from);
        w.u8(f.where.entry);
        w.u8(f.where.from_exit);
        w.u8(f.where.transition);
        w.i64(f.where.depart_us);
        w.i64(f.where.arrive_us);
        int n = 0;
        for (const SkillState& s : f.skills) n += s.level != SkillLevel::Unknown || s.xp > 0;
        w.u8(static_cast<uint8_t>(n));
        for (int s = 0; s < kSkills; ++s) {
            const SkillState& x = f.skills[s];
            if (x.level == SkillLevel::Unknown && x.xp == 0) continue;
            w.str(skill_id(static_cast<Skill>(s)));
            w.u8(static_cast<uint8_t>(x.level));
            w.u16(x.xp);
        }
        n = 0;
        for (const Relation& x : f.relations) n += x.other != 0;
        w.u8(static_cast<uint8_t>(n));
        for (const Relation& x : f.relations) {
            if (!x.other) continue;
            w.u16(x.other);
            w.u8(static_cast<uint8_t>(x.bond));
            w.u8(x.flags);
            w.u16(x.points);
        }
        w.u8(static_cast<uint8_t>(f.memory_count()));
        for (const Memory& m : f.memories) {
            if (m.kind == MemoryKind::None) continue;
            w.u8(static_cast<uint8_t>(m.kind));
            w.u8(m.weight);
            w.u8(m.arg);
            w.u8(m.b);
            w.u16(m.who);
            w.u8(m.place);
            w.i32(m.day);
            w.u16(m.minute);
            w.u16(m.item);
        }
        n = 0;
        for (const auto& o : f.owns) n += o.key[0] != 0;
        w.u8(static_cast<uint8_t>(n));
        for (const auto& o : f.owns) {
            if (!o.key[0]) continue;
            w.str(o.key.data());
            w.u8(o.count);
            w.u8(o.state);
        }
        // seit 0.5.9 Teil 5: Statur hinten im Datensatz (ältere Leser überspringen sie, ältere Stände haben keine)
        w.u8(4);
        w.u8(static_cast<uint8_t>(f.look.build.height));
        w.u8(static_cast<uint8_t>(f.look.build.width));
        w.u8(f.look.build.belly);
        w.u8(f.look.build.stoop);
        w.record_end(r);
    }

    const Chronicle& ch = env_.chronicle;
    w.chunk(kChron, 1);
    w.u32(ch.total());
    w.u16(static_cast<uint16_t>(ch.count()));
    for (int i = 0; i < ch.count(); ++i) {
        const ChronicleEntry& e = ch.at(i);
        const size_t r = w.record_begin();
        w.i32(e.day);
        w.u16(e.minute);
        w.u8(static_cast<uint8_t>(e.kind));
        w.u8(static_cast<uint8_t>(e.event));
        w.u8(e.a);
        w.u8(e.b);
        w.u16(e.who);
        w.u16(e.other);
        w.str(e.text);
        w.record_end(r);
    }

    const VisitLog& vl = env_.visits;
    w.chunk(kVisits, 1);
    w.u16(static_cast<uint16_t>(vl.count()));
    for (int i = 0; i < vl.count(); ++i) {
        const Visit& v = vl.at(i);
        w.u16(v.who);
        w.u8(v.place);
        w.i32(v.day);
        w.u16(v.from);
        w.u16(v.to);
    }

    // Tätigkeiten (seit 0.5.9 Teil 2): letzte Takt-Grenze und der laufende Zeitblock jeder Figur (unterwegs zählt nur
    // die laufende Nummer: Zufall der nächsten Entscheidung). So geht nach einem Neustart jede Tätigkeit genau dort
    // weiter (fehlt der Abschnitt, entscheiden alle neu)
    w.chunk(kDoings, 1);
    w.i64(grid_);
    w.u16(static_cast<uint16_t>(book.count()));
    for (int k = 0; k < book.count(); ++k) {
        const FigureRecord& f = book.at(k);
        const Doing& d = f.doing;
        const size_t r = w.record_begin();
        w.u16(f.id);
        w.u8(static_cast<uint8_t>(d.kind));
        w.u8(d.part);
        w.u8(d.k);
        w.u8(d.exit);
        w.u8(d.sx);
        w.u8(d.sy);
        w.u8(d.gx);
        w.u8(d.gy);
        w.u16(d.steps);
        w.u32(d.seq);
        w.i64(d.start_us);
        w.i64(d.walk_us);
        w.i64(d.end_us);
        w.i64(d.since_us);
        w.record_end(r);
    }
    return w.finish();
}

bool Stage::load_state(const uint8_t* data, size_t n, LoadReport* report) {
    LoadReport rep;
    SaveHeader h;
    if (!save_check(data, n, &h, &rep.why)) {
        if (report) *report = rep;
        return false;
    }
    rep.generation = h.generation;
    rep.saved_us = h.world_us;

    // Ortsliste zuerst: Nummer im Speicherstand → Nummer in der Welt (−1 = gibt es nicht mehr). Große Teile auf dem
    // Heap: Geladen wird beim Start im Haupt-Task mit 3,5 KB Stack (ein Figuren-Datensatz allein hat 1,1 KB)
    std::vector<int16_t> remap(256, -1);
    auto fp = std::make_unique<FigureRecord>();
    int places = 0;
    {
        SaveChunks c(data, n);
        char tag[5];
        uint16_t version = 0;
        SaveReader d;
        while (c.next(tag, &version, &d))
            if (std::strcmp(tag, kPlaces) == 0) {
                places = std::min<int>(d.u16(), 255);
                for (int k = 0; k < places; ++k) remap[k] = static_cast<int16_t>(place_index(d.view()));
            }
    }
    auto place = [&](uint8_t i) -> uint8_t {
        if (i == 0xFF) return 0xFF;
        if (i >= places || remap[i] < 0) {
            ++rep.unknown_places;
            return 0xFF;
        }
        return static_cast<uint8_t>(remap[i]);
    };

    // Jeder Abschnitt ersetzt seinen Teil des Weltzustands; fehlt einer, bleibt der Teil beim Anfangswert (frisch
    // angelegte Welt nach init())
    bool figures = false;
    SaveChunks c(data, n);
    char tag[5];
    uint16_t version = 0;
    SaveReader d;
    while (c.next(tag, &version, &d)) {
        if (std::strcmp(tag, kWorld) == 0) {
            const uint64_t seed = d.u64();
            if (seed != seed_) {
                seed_ = seed;
                env_.weather.init(seed);
                env_.weather_sim.init(seed ^ 0x57E77E5157ULL);
            }
            state_minute_ = d.i64();
            const uint16_t next = d.u16();
            env_.figures.set_next_id(next);
        } else if (std::strcmp(tag, kCal) == 0) {
            CalendarState cs;
            cs.season_len = d.u8();
            cs.firsts = d.u16();
            cs.start_day = d.i32();
            cs.anchor_first = d.i32();
            cs.anchor_season = d.i32();
            cs.anchor_min_last = d.i32();
            cs.firsts_year = d.i32();
            // nur plausible Werte (die Rechnung läuft vom Anker aus): Start zwischen 1970 und 2500, Anker nahe am Start
            const bool sane = cs.season_len >= WorldCalendar::kMinSeasonDays &&
                              cs.season_len <= WorldCalendar::kMaxSeasonDays &&
                              (cs.start_day == -1 || (cs.start_day >= 0 && cs.start_day < 194000)) &&
                              std::abs(static_cast<int64_t>(cs.anchor_first) - cs.start_day) < 40000 &&
                              cs.anchor_season >= 0 && cs.anchor_season < 6000 &&
                              std::abs(static_cast<int64_t>(cs.anchor_min_last) - cs.anchor_first) < 40000;
            if (d.ok() && sane) env_.calendar.set_state(cs);
        } else if (std::strcmp(tag, kStates) == 0) {
            const int m = d.u16();
            for (int k = 0; k < m && d.ok(); ++k) {
                SaveReader r = d.record();
                const uint8_t p = place(r.u8());
                const std::string_view id = r.view();
                PlaceStates::Saved v;
                v.value = r.u16();
                v.heat = r.u16();
                v.frac = r.u16();
                v.bits = r.u8();
                v.phase = r.u8();
                if (!r.ok() || p == 0xFF) continue;
                const PlaceDef* pd = place_at(p);
                const int i = pd ? env_.states.find(pd->id, id) : -1;
                if (i < 0) continue;  // Zustand gibt es nicht mehr (oder der Ort ist nicht auf der Bühne)
                env_.states.restore(i, v);
                ++rep.states;
            }
        } else if (std::strcmp(tag, kFlags) == 0) {
            env_.flags.clear();
            const int m = d.u8();
            for (int k = 0; k < m && d.ok(); ++k) env_.flags.set(d.view(), true);
        } else if (std::strcmp(tag, kFigs) == 0) {
            // Figuren von den Verbindungen nehmen, dann die gespeicherten setzen (die Orte zeigen, was im Weltzustand
            // steht)
            figures = true;
            transit_n_ = 0;
            env_.figures.clear();
            const int m = d.u16();
            for (int k = 0; k < m && d.ok(); ++k) {
                SaveReader r = d.record();
                FigureRecord& f = *new (fp.get()) FigureRecord();  // an Ort und Stelle: kein Zwischenstück auf dem Stack
                f.id = r.u16();
                f.flags = r.u8();
                f.age = r.u8();
                r.str(f.name.data(), f.name.size());
                r.str(f.role.data(), f.role.size());
                r.str(f.origin.data(), f.origin.size());
                r.str(f.goal.data(), f.goal.size());
                r.str(f.home.data(), f.home.size());
                f.chrono = static_cast<Chronotype>(std::min<int>(r.u8(), 2));
                const int traits = r.u8();
                for (int t = 0; t < traits; ++t) {
                    char tmp[16];
                    r.str(tmp, sizeof(tmp));
                    if (t < FigureRecord::kTraits) set_text(f.traits[t], tmp);
                }
                const int interests = r.u8();
                for (int t = 0; t < interests; ++t) {
                    char tmp[16];
                    r.str(tmp, sizeof(tmp));
                    const uint8_t wgt = r.u8();
                    if (t >= FigureRecord::kInterests) continue;
                    set_text(f.interests[t].key, tmp);
                    f.interests[t].weight = wgt;
                }
                const int layers = r.u8();
                for (int l = 0; l < layers; ++l) {
                    const uint8_t part = r.u8(), row = r.u8();
                    if (l >= kLookLayers) continue;
                    f.look.part[l] = part;
                    f.look.row[l] = row;
                }
                f.where.kind = static_cast<Whereabouts::Kind>(std::min<int>(r.u8(), 2));
                f.where.place = place(r.u8());
                f.where.from = place(r.u8());
                f.where.entry = r.u8();
                f.where.from_exit = r.u8();
                f.where.transition = r.u8();
                f.where.depart_us = r.i64();
                f.where.arrive_us = r.i64();
                const int skills = r.u8();
                for (int s = 0; s < skills; ++s) {
                    const std::string_view id = r.view();
                    const uint8_t level = r.u8();
                    const uint16_t xp = r.u16();
                    Skill sk;
                    if (!parse_skill(id, &sk)) continue;
                    f.skills[static_cast<int>(sk)].level = static_cast<SkillLevel>(std::min<int>(level, 3));
                    f.skills[static_cast<int>(sk)].xp = xp;
                }
                const int rels = r.u8();
                for (int x = 0; x < rels; ++x) {
                    Relation rel;
                    rel.other = r.u16();
                    rel.bond = static_cast<Bond>(std::min<int>(r.u8(), 3));
                    rel.flags = r.u8();
                    rel.points = r.u16();
                    if (x < FigureRecord::kRelations) f.relations[x] = rel;
                }
                const int mems = r.u8();
                for (int x = 0; x < mems; ++x) {
                    Memory mem;
                    mem.kind = static_cast<MemoryKind>(r.u8());
                    mem.weight = r.u8();
                    mem.arg = r.u8();
                    mem.b = r.u8();
                    mem.who = r.u16();
                    mem.place = place(r.u8());
                    mem.day = r.i32();
                    mem.minute = r.u16();
                    mem.item = r.u16();
                    if (mem.kind >= MemoryKind::kCount) mem.kind = MemoryKind::Event;
                    if (x < FigureRecord::kMemories) f.memories[x] = mem;
                }
                const int owns = r.u8();
                for (int x = 0; x < owns; ++x) {
                    char tmp[16];
                    r.str(tmp, sizeof(tmp));
                    const uint8_t count = r.u8(), state = r.u8();
                    if (x >= FigureRecord::kPossessions) continue;
                    set_text(f.owns[x].key, tmp);
                    f.owns[x].count = count;
                    f.owns[x].state = state;
                }
                if (r.left()) {  // Statur (seit Teil 5); fehlt sie, bleibt die Standard-Statur
                    const int n = r.u8();
                    uint8_t b[4] = {};
                    for (int x = 0; x < n; ++x) {
                        const uint8_t v = r.u8();
                        if (x < 4) b[x] = v;
                    }
                    f.look.build = clamp_stature(Stature{static_cast<int8_t>(b[0]), static_cast<int8_t>(b[1]), b[2], b[3]});
                }
                if (f.id == 0) continue;
                // nie eine Figur verlieren: Ort unbekannt → in den ersten Ort der Welt
                if (f.where.kind == Whereabouts::Kind::Nowhere || f.where.place == 0xFF ||
                    (f.where.kind == Whereabouts::Kind::Link && f.where.from == 0xFF)) {
                    f.where.kind = Whereabouts::Kind::Place;
                    if (f.where.place == 0xFF) f.where.place = 0;
                }
                if (env_.figures.add_record(f)) ++rep.figures;
            }
        } else if (std::strcmp(tag, kChron) == 0) {
            env_.chronicle.clear();
            const uint32_t total = d.u32();
            const int m = d.u16();
            for (int k = 0; k < m && d.ok(); ++k) {
                SaveReader r = d.record();
                ChronicleEntry e;
                e.day = r.i32();
                e.minute = r.u16();
                e.kind = static_cast<ChronicleKind>(std::min<int>(r.u8(), 3));
                e.event = static_cast<EventKind>(r.u8());
                e.a = r.u8();
                e.b = r.u8();
                e.who = r.u16();
                e.other = r.u16();
                r.str(e.text, sizeof(e.text));
                if (!r.ok()) continue;
                env_.chronicle.add(e);
                ++rep.chronicle;
            }
            env_.chronicle.set_total(std::max<uint32_t>(total, env_.chronicle.total()));
        } else if (std::strcmp(tag, kVisits) == 0) {
            env_.visits.clear();
            const int m = d.u16();
            for (int k = 0; k < m && d.ok(); ++k) {
                Visit v;
                v.who = d.u16();
                v.place = place(d.u8());
                v.day = d.i32();
                v.from = d.u16();
                v.to = d.u16();
                if (d.ok()) env_.visits.add(v);
            }
        } else if (std::strcmp(tag, kPlaces) != 0 && std::strcmp(tag, kDoings) != 0) {
            ++rep.skipped_chunks;  // neuere Firmware: unbekannt, übersprungen
        }
    }

    // Figuren in ihre Orte, Verbindungen neu; Weltzeit und Wetter rechnen im nächsten Schritt neu, die Ortszustände
    // holen von state_minute_ aus nach
    if (figures) {
        adopt_figures();
        rebuild_transit();
        // Tätigkeiten (seit 0.5.9 Teil 2) nach den Figuren, egal wo der Abschnitt steht; ohne ihn (Stand aus Teil 1)
        // setzt der erste Schritt alle neu an
        grid_ = INT64_MIN;
        SaveChunks c2(data, n);
        while (c2.next(tag, &version, &d)) {
            if (std::strcmp(tag, kDoings) != 0) continue;
            const int64_t grid = d.i64();
            const int m = d.u16();
            for (int k = 0; k < m && d.ok(); ++k) {
                SaveReader r = d.record();
                const uint16_t id = r.u16();
                Doing x;
                x.kind = static_cast<DoingKind>(r.u8());
                x.part = r.u8();
                x.k = r.u8();
                x.exit = r.u8();
                x.sx = r.u8();
                x.sy = r.u8();
                x.gx = r.u8();
                x.gy = r.u8();
                x.steps = r.u16();
                x.seq = r.u32();
                x.start_us = r.i64();
                x.walk_us = r.i64();
                x.end_us = r.i64();
                x.since_us = r.i64();
                FigureRecord* f = env_.figures.find(id);
                const bool sane = r.ok() && x.kind < DoingKind::kCount && x.sx < kTilesX && x.gx < kTilesX &&
                                  x.sy < kTilesY && x.gy < kTilesY && x.start_us <= x.walk_us && x.walk_us <= x.end_us;
                if (!f || !sane) continue;
                if (f->where.kind != Whereabouts::Kind::Place || x.kind == DoingKind::None) {
                    f->doing.seq = x.seq;  // unterwegs (oder beim Laden gleich angekommen): nur die laufende Nummer
                    continue;
                }
                f->doing = x;
                ++rep.doings;
            }
            if (d.ok() && grid != INT64_MIN) grid_ = grid;
        }
        // ohne gespeicherte Tätigkeit: an der nächsten Takt-Grenze entscheiden
        for (int k = 0; grid_ != INT64_MIN && k < env_.figures.count(); ++k) {
            Doing& x = env_.figures.at(k).doing;
            if (x.end_us != INT64_MIN) continue;
            x.start_us = x.walk_us = x.since_us = grid_;
            x.end_us = grid_ + kGridUs;
        }
    }
    for (int k = 0; k < env_.figures.count(); ++k) rep.transit += env_.figures.at(k).where.kind == Whereabouts::Kind::Link;
    world_minute_ = INT64_MIN;
    sim_applied_ = false;
    override_key_ = kNoOverride;
    past_key_ = INT64_MIN;
    saved_minute_ = minute_of_us(h.world_us);
    world_us_ = h.world_us;
    for (int i = 0; figures && i < count_; ++i)
        if (entered(i)) scenes_[i]->refresh(world_us_);  // gezeigt wird ab sofort der geladene Stand
    wake_pending_ = true;
    caught_up_ = skipped_ = 0;
    rep.ok = true;
    if (report) *report = rep;
    return true;
}

}  // namespace pixelia
