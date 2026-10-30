// Projekte (seit 0.5.10 Teil 3): Teilaufgaben mit Material, Fähigkeit und Zwischenstand; siehe projects.h.

#include "pixelia/engine/projects.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>
#include <string_view>

#include "pixelia/engine/events.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/memory.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/tasks.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

namespace {

const char* const kKindIds[] = {"klein", "moebel", "notunterkunft", "raum", "ausbaustufe", "grossbau", "lok"};
static_assert(sizeof(kKindIds) / sizeof(kKindIds[0]) == static_cast<int>(ProjectKind::kCount));
const char* const kWaitIds[] = {"keiner", "material", "faehigkeit", "arbeitskraft", "wetter", "feierabend"};
static_assert(sizeof(kWaitIds) / sizeof(kWaitIds[0]) == kProjectWaits);

constexpr int kMaxSteps = 16;  // Teilaufgaben je Projekt
constexpr int kMaxGoods = 8;   // verschiedene Waren je Projekt (verbautes Material insgesamt)
constexpr int kXpSeconds = 600;  // 1 Erfahrung je 10 Arbeitsminuten

int64_t floor_div(int64_t a, int64_t b) { return a >= 0 ? a / b : (a - b + 1) / b; }

}  // namespace

const char* project_kind_id(ProjectKind k) {
    const int i = static_cast<int>(k);
    return i >= 0 && i < static_cast<int>(ProjectKind::kCount) ? kKindIds[i] : "?";
}

const char* project_wait_id(ProjectWait w) {
    const int i = static_cast<int>(w);
    return i >= 0 && i < kProjectWaits ? kWaitIds[i] : "?";
}

// Daten im Grafik-Speicher: Projekte und aufgelöste Kennungen der Daten
struct ProjectList::DefData {
    const ProjectDef* def = nullptr;
    int16_t place = -1, site = -1, source = -1;
    int8_t goods[kMaxSteps][Project::kMaxNeeds] = {};  // Ware je Schritt und Bedarf (−1 = unbekannt)
    int8_t slot[kMaxSteps][Project::kMaxNeeds] = {};   // … als Nummer unter den Waren des Projekts
    int8_t uniq[kMaxGoods] = {};                        // Waren des Projekts in der Reihenfolge der Daten
    uint8_t uniq_n = 0;
    uint8_t steps = 0;
};
struct ProjectList::Storage {
    Project p[ProjectList::kMax];
    DefData d[ProjectList::kMaxDefs];
};

ProjectList::~ProjectList() {
    if (s_) asset_free(s_);  // trivial zerstörbar
}

bool ProjectList::ensure() {
    if (s_) return true;
    void* mem = asset_alloc(sizeof(Storage));
    if (!mem) return false;
    s_ = new (mem) Storage;
    return true;
}

bool ProjectList::work_time(int64_t at_us) {
    const int64_t m = floor_div(at_us, WorldClock::kUsPerMinute);
    const int minute = static_cast<int>(m - floor_div(m, 1440) * 1440);
    return minute >= kWorkFrom && minute < kWorkTo;
}

void ProjectList::build(const Ledger& ledger) {
    const AreaDef* const* areas = nullptr;
    const int na = world_areas(&areas);
    const EconomyDef* defs[8];
    int n = 0;
    for (int a = 0; a < na && n < 8; ++a)
        if (areas[a]->economy) defs[n++] = areas[a]->economy;
    build(defs, n, ledger);
}

void ProjectList::build(const EconomyDef* const* defs, int n, const Ledger& l) {
    if (!ensure()) return;
    def_n_ = 0;
    for (int e = 0; e < n; ++e)
        for (int k = 0; defs[e] && k < defs[e]->project_count && def_n_ < kMaxDefs; ++k) {
            const ProjectDef& pd = defs[e]->projects[k];
            DefData& d = s_->d[def_n_++];
            d = DefData{};
            d.def = &pd;
            d.place = static_cast<int16_t>(pd.place ? place_index(pd.place) : -1);
            d.site = static_cast<int16_t>(pd.site ? l.find_store(pd.site) : -1);
            d.source = static_cast<int16_t>(pd.source ? l.find_store(pd.source) : -1);
            d.steps = static_cast<uint8_t>(std::min<int>(pd.step_count, kMaxSteps));
            for (int s = 0; s < d.steps; ++s)
                for (int k2 = 0; k2 < Project::kMaxNeeds; ++k2) {
                    d.goods[s][k2] = -1;
                    d.slot[s][k2] = -1;
                    if (k2 >= pd.steps[s].need_count) continue;
                    const int g = l.find_good(pd.steps[s].needs[k2].good);
                    if (g < 0) continue;
                    d.goods[s][k2] = static_cast<int8_t>(g);
                    int u = 0;
                    while (u < d.uniq_n && d.uniq[u] != g) ++u;
                    if (u == d.uniq_n && u < kMaxGoods) d.uniq[d.uniq_n++] = static_cast<int8_t>(g);
                    if (u < d.uniq_n) d.slot[s][k2] = static_cast<int8_t>(u);
                }
        }
    clear();
}

void ProjectList::clear() {
    n_ = 0;
    next_ = kFirstId;
    orphans_ = true;
    ++version_;
}

int ProjectList::find_def(const char* id) const {
    for (int d = 0; s_ && id && d < def_n_; ++d)
        if (std::strcmp(s_->d[d].def->id, id) == 0) return d;
    return -1;
}

const ProjectDef& ProjectList::def(int d) const { return *s_->d[d].def; }
const Project& ProjectList::at(int i) const { return s_->p[i]; }

Project* ProjectList::get(uint16_t id) {
    for (int i = 0; s_ && i < n_; ++i)
        if (s_->p[i].id == id) return &s_->p[i];
    return nullptr;
}

const Project* ProjectList::find(uint16_t id) const { return const_cast<ProjectList*>(this)->get(id); }

const Project* ProjectList::of(uint16_t who) const {
    for (int i = 0; who && s_ && i < n_; ++i) {
        const Project& p = s_->p[i];
        if (p.who == who && !p.done(s_->d[p.def].steps)) return &p;
    }
    return nullptr;
}

const Project* ProjectList::at_place(int place, int part) const {
    for (int i = 0; s_ && i < n_; ++i) {
        const Project& p = s_->p[i];
        const DefData& d = s_->d[p.def];
        if (d.place == place && d.def->part == part && !p.done(d.steps)) return &p;
    }
    return nullptr;
}

int ProjectList::place_of(const Project& p) const { return s_->d[p.def].place; }
int ProjectList::site_of(const Project& p) const { return s_->d[p.def].site; }

const ProjectStepDef* ProjectList::current(const Project& p) const {
    const DefData& d = s_->d[p.def];
    return p.step < d.steps ? &d.def->steps[p.step] : nullptr;
}

uint32_t ProjectList::step_total_s(const Project& p) const {
    const ProjectStepDef* s = current(p);
    return s ? std::max<uint32_t>(1, s->minutes * 60u) : 0;
}

bool ProjectList::can_work(const Project& p, const FigureRecord& f) const {
    const ProjectStepDef* s = current(p);
    if (!s) return false;
    if (s->level == SkillLevel::Unknown) return true;
    return f.skills[static_cast<int>(s->skill)].level >= s->level;
}

Project* ProjectList::add(int def, int64_t at_us) {
    if (!ensure() || n_ >= kMax || def < 0 || def >= def_n_) return nullptr;
    // nächste freie Kennung (nach 0xFFFF wieder ab kFirstId); einsortieren, die Liste bleibt nach Kennung sortiert
    uint16_t id = next_;
    for (int guard = 0; guard <= kMax && get(id); ++guard) id = static_cast<uint16_t>(id == 0xFFFF ? kFirstId : id + 1);
    next_ = static_cast<uint16_t>(id == 0xFFFF ? kFirstId : id + 1);
    int at = n_;
    while (at > 0 && s_->p[at - 1].id > id) --at;
    for (int i = n_; i > at; --i) s_->p[i] = s_->p[i - 1];
    ++n_;
    Project& p = s_->p[at];
    p = Project{};
    p.id = id;
    p.def = static_cast<uint8_t>(def);
    p.started_us = p.step_us = at_us;
    ++version_;
    return &p;
}

void ProjectList::start_new() {
    for (int d = 0; s_ && d < def_n_; ++d) {
        if (!s_->d[d].def->start) continue;
        bool running = false;
        for (int i = 0; i < n_ && !running; ++i) running = s_->p[i].def == d;
        if (!running) add(d, INT64_MIN);  // Beginn an der ersten Takt-Grenze
    }
}

void ProjectList::reset_step(Project& p) {
    p.begun = false;
    p.work_s = 0;
    p.used.fill(0);
    p.res.fill(0);
    p.person_s = 0;
    p.wait_s.fill(0);
}

int ProjectList::source_for(const DefData& d, const Ledger& l, int good) const {
    // die Quelle der Daten, wenn dort etwas frei ist; sonst das erste Gemeinschaftslager mit der Ware
    if (d.source >= 0 && l.available(d.source, good) > 0) return d.source;
    for (int s = 0; s < l.stores(); ++s)
        if (l.store_kind(s) == StoreKind::Community && s != d.site && l.available(s, good) > 0) return s;
    return -1;
}

bool ProjectList::begin(Project& p, Ledger& l) {
    // erst, wenn das ganze Material des Schritts frei an der Baustelle liegt; dann alles für ihn reservieren
    const DefData& d = s_->d[p.def];
    const ProjectStepDef* s = current(p);
    if (!s || d.site < 0) return false;
    for (int k = 0; k < s->need_count && k < Project::kMaxNeeds; ++k)
        if (d.goods[p.step][k] >= 0 && l.available(d.site, d.goods[p.step][k]) < s->needs[k].qty) return false;
    bool ok = true;
    for (int k = 0; k < s->need_count && k < Project::kMaxNeeds && ok; ++k) {
        const int g = d.goods[p.step][k];
        if (g < 0 || s->needs[k].qty == 0) continue;
        p.res[k] = l.reserve(d.site, g, s->needs[k].qty, p.id);
        ok = l.reservation_left(p.res[k]) == s->needs[k].qty;
    }
    if (!ok) {
        l.release_holder(p.id);
        p.res.fill(0);
        return false;
    }
    p.begun = true;
    ++version_;
    return true;
}

void ProjectList::fetch(Project& p, int64_t at_us, Ledger& l, TaskList& tasks) {
    // Material des laufenden und des nächsten Schritts (kAheadSteps) an die Baustelle: was fehlt, abzüglich dessen, was
    // schon dort liegt und was unterwegs ist. Je Ware höchstens kMaxTrips neue Aufgaben, je eine Korbladung
    const DefData& d = s_->d[p.def];
    if (d.site < 0) return;
    for (int u = 0; u < d.uniq_n; ++u) {
        const int g = d.uniq[u];
        int want = 0, own = 0;
        for (int s = p.step; s < d.steps && s <= p.step + kAheadSteps; ++s)
            for (int k = 0; k < d.def->steps[s].need_count && k < Project::kMaxNeeds; ++k) {
                if (d.goods[s][k] != g) continue;
                want += d.def->steps[s].needs[k].qty;
                if (s == p.step && p.begun) {
                    want -= p.used[k];
                    own += l.reservation_left(p.res[k]);
                }
            }
        // an der Baustelle: alles außer dem, was andere dort reserviert haben (zurückbringen)
        const int present = l.qty(d.site, g) - (l.reserved(d.site, g) - own);
        int short_by = want - present - tasks.incoming(d.site, g);
        if (short_by < 0) {
            // zu viel unterwegs (nach Prüfbefehlen): offene Aufgaben dieses Projekts streichen, neueste zuerst
            for (int i = tasks.count() - 1; i >= 0 && short_by < 0; --i) {
                const Task& t = tasks.at(i);
                if (t.phase != TaskPhase::Open || !(t.flags & kTaskProject) || t.to != d.site || t.good != g) continue;
                if (t.qty > -short_by) continue;
                short_by += t.qty;
                tasks.cancel(t.id);
            }
            continue;
        }
        const int basket = std::max(1, Ledger::kHandCapacity / std::max<int>(1, l.good(g).weight));
        for (int trip = 0; trip < kMaxTrips && short_by > 0; ++trip) {
            const int src = source_for(d, l, g);
            if (src < 0) break;
            const Task* t = tasks.add_fetch(l, g, src, d.site, std::min(short_by, basket), at_us, kTaskProject);
            if (!t) break;
            short_by -= t->qty;
        }
    }
}

void ProjectList::tidy(Project& p, int64_t at_us, Ledger& l, TaskList& tasks) {
    // fertig: was an der Baustelle übrig ist, zurück zur Quelle (sonst ins erste Gemeinschaftslager mit Platz); ist sie
    // leer und nichts mehr unterwegs, fällt das Projekt aus der Liste
    const DefData& d = s_->d[p.def];
    if (d.site >= 0) {
        uint8_t goods[16];
        uint16_t qtys[16];
        const int k = l.contents(d.site, goods, qtys, 16);
        for (int i = 0; i < k; ++i) {
            const int g = goods[i];
            const int free = l.available(d.site, g);
            if (free <= 0) continue;
            int dest = d.source >= 0 && l.room(d.source, g) > 0 ? d.source : -1;
            for (int s = 0; dest < 0 && s < l.stores(); ++s)
                if (l.store_kind(s) == StoreKind::Community && s != d.site && l.room(s, g) > 0) dest = s;
            if (dest < 0) continue;
            const int basket = std::max(1, Ledger::kHandCapacity / std::max<int>(1, l.good(g).weight));
            tasks.add_fetch(l, g, d.site, dest, std::min(free, basket), at_us, kTaskProject);
        }
        if (k > 0 || tasks.incoming(d.site, -1) > 0) return;
    }
    l.release_holder(p.id);
    const uint16_t id = p.id;
    for (int i = 0; i < n_; ++i)
        if (s_->p[i].id == id) {
            for (int j = i + 1; j < n_; ++j) s_->p[j - 1] = s_->p[j];
            --n_;
            ++version_;
            return;
        }
}

void ProjectList::finish_step(Project& p, int64_t at_us, int32_t day, Ledger& l, EventBus* bus) {
    const DefData& d = s_->d[p.def];
    const ProjectStepDef& s = d.def->steps[p.step];
    for (int k = 0; k < Project::kMaxNeeds; ++k)
        if (p.res[k]) l.release(p.res[k]);  // Rest (nur nach Prüfbefehlen nicht null)
    ProjectRecord r;
    r.def = d.def;
    r.step = &s;
    r.id = p.id;
    r.begin_us = p.step_us;
    r.end_us = at_us;
    r.planned_s = s.minutes * 60u;
    r.person_s = p.person_s;
    r.wait_s = p.wait_s;
    for (int k = 0; k < s.need_count && k < Project::kMaxNeeds; ++k) {
        if (d.goods[p.step][k] < 0) continue;
        r.goods[r.good_count] = static_cast<uint8_t>(d.goods[p.step][k]);
        r.qty[r.good_count++] = p.used[k];
    }
    if (on_record_) on_record_(listen_ctx_, r);
    if (bus) {
        // Chronik nur Besonderes (Wahl des Users): Teilaufgabe fertig, Projekt fertig
        WorldEvent ev;
        ev.kind = EventKind::Project;
        ev.a = 0;
        ev.b = p.step;
        ev.day = day;
        ev.place = d.def->place;
        std::snprintf(ev.text, sizeof(ev.text), "%.22s: %.24s fertig", d.def->title, s.title);
        bus->publish(ev);
    }
    ++p.step;
    reset_step(p);
    p.step_us = at_us;
    p.who = 0;
    p.leaving = false;
    ++version_;
    if (!p.done(d.steps)) return;
    p.done_us = at_us;
    ProjectRecord all;
    all.def = d.def;
    all.id = p.id;
    all.begin_us = p.started_us;
    all.end_us = at_us;
    for (int k = 0; k < d.steps; ++k) all.planned_s += d.def->steps[k].minutes * 60u;
    all.person_s = p.total_person_s;
    all.wait_s = p.total_wait_s;
    for (int u = 0; u < d.uniq_n; ++u) {
        all.goods[all.good_count] = static_cast<uint8_t>(d.uniq[u]);
        all.qty[all.good_count++] = p.built[u];
    }
    if (on_record_) on_record_(listen_ctx_, all);
    if (bus) {
        WorldEvent ev;
        ev.kind = EventKind::Project;
        ev.a = 1;
        ev.day = day;
        ev.place = d.def->place;
        std::snprintf(ev.text, sizeof(ev.text), "Projekt fertig: %.38s", d.def->title);
        bus->publish(ev);
    }
}

ProjectWait ProjectList::wait_of(const Project& p, int64_t at_us, const FigureBook& book, bool bad_weather) const {
    const DefData& d = s_->d[p.def];
    if (p.done(d.steps)) return ProjectWait::None;
    if (!p.begun) return ProjectWait::Material;
    bool anyone = false;
    for (int k = 0; k < book.count() && !anyone; ++k) anyone = can_work(p, book.at(k));
    if (!anyone) return ProjectWait::Skill;
    if (!work_time(at_us)) return ProjectWait::Rest;
    if (d.def->outdoor && bad_weather) return ProjectWait::Weather;
    return p.who ? ProjectWait::None : ProjectWait::Worker;
}

void ProjectList::step(int64_t at_us, int32_t day, Ledger& l, TaskList& tasks, const FigureBook& book, EventBus* bus,
                       bool bad_weather) {
    if (!ensure() || !l.ready()) return;
    if (orphans_) {
        // nach dem Laden (oder clear): Reservierungen von Projekten, die es nicht gibt, frei; begonnene Schritte, deren
        // Reservierung nicht mehr passt (Stand ohne Reservierungen), beginnen neu
        orphans_ = false;
        uint16_t h[96];
        const int k = l.holders(h, 96);
        for (int j = 0; j < k; ++j)
            if (h[j] >= kFirstId && !get(h[j])) l.release_holder(h[j]);
        for (int i = 0; i < n_; ++i) {
            Project& p = s_->p[i];
            const ProjectStepDef* s = current(p);
            if (s && !p.begun) l.release_holder(p.id);  // nicht begonnen: hält nichts (Stand ohne Projekte)
            if (!s || !p.begun) continue;
            bool ok = true;
            for (int n = 0; n < s->need_count && n < Project::kMaxNeeds; ++n)
                ok = ok && (s_->d[p.def].goods[p.step][n] < 0 ||
                            l.reservation_left(p.res[n]) == s->needs[n].qty - p.used[n]);
            if (ok) continue;
            l.release_holder(p.id);
            p.res.fill(0);
            p.begun = false;
            ++version_;
        }
    }
    for (int i = 0; i < n_; ++i) {
        Project& p = s_->p[i];
        const DefData& d = s_->d[p.def];
        if (p.started_us == INT64_MIN) p.started_us = at_us;
        if (p.step_us == INT64_MIN) p.step_us = at_us;
        const bool done = p.done(d.steps);
        // Wartezeit seit der letzten Takt-Grenze, nach dem Grund von dort (keiner = jemand arbeitet)
        if (!done && p.seen_us != INT64_MIN && at_us > p.seen_us) {
            const auto s = static_cast<uint32_t>(std::min<int64_t>((at_us - p.seen_us) / WorldClock::kUsPerSecond, 0x7FFFFFFF));
            p.wait_s[static_cast<int>(p.wait)] += s;
            p.total_wait_s[static_cast<int>(p.wait)] += s;
        }
        p.seen_us = at_us;
        if (done) {
            const uint16_t id = p.id;
            tidy(p, at_us, l, tasks);
            if (i < n_ && s_->p[i].id != id) --i;  // aus der Liste gefallen
            continue;
        }
        // Zuständigkeit: wer aufhört (oder nicht mehr da ist), gibt das Projekt frei
        if (p.who && (p.leaving || !book.find(p.who))) {
            p.who = 0;
            p.leaving = false;
            ++version_;
        }
        if (p.begun && p.work_s >= step_total_s(p)) finish_step(p, at_us, day, l, bus);
        if (p.done(d.steps)) {
            p.wait = ProjectWait::None;
            continue;
        }
        if (!p.begun) begin(p, l);
        fetch(p, at_us, l, tasks);
        p.wait = wait_of(p, at_us, book, bad_weather);
    }
}

bool ProjectList::claim(uint16_t id, const FigureRecord& f, int64_t at_us) {
    Project* p = get(id);
    if (!p || !p->begun || p->who || p->done(s_->d[p->def].steps) || p->work_s >= step_total_s(*p)) return false;
    if (of(f.id) || !can_work(*p, f) || !work_time(at_us) || p->wait == ProjectWait::Weather) return false;
    p->who = f.id;
    p->leaving = false;
    ++version_;
    return true;
}

int ProjectList::work(uint16_t id, uint16_t who, int seconds, int64_t at_us, int32_t day, Ledger& l,
                      FigureBook* figures) {
    Project* p = get(id);
    if (!p || !who || p->who != who || p->leaving || !p->begun || seconds <= 0) return 0;
    const ProjectStepDef* s = current(*p);
    const uint32_t total = step_total_s(*p);
    if (!s || p->work_s >= total) return 0;
    const DefData& d = s_->d[p->def];
    const int n = static_cast<int>(std::min<uint32_t>(static_cast<uint32_t>(seconds), total - p->work_s));
    const uint32_t before = p->person_s;
    p->work_s += n;
    p->person_s += n;
    p->total_person_s += n;
    // verbauen, so weit die Arbeit ist (aufgerundet: das erste Brett gleich beim ersten Arbeitsgang), aus der Reservierung
    BookingInfo info;
    info.at_us = at_us;
    info.who = who;
    info.reason = "bauen";
    info.holder = p->id;
    for (int k = 0; k < s->need_count && k < Project::kMaxNeeds; ++k) {
        const int g = d.goods[p->step][k];
        if (g < 0) continue;
        const uint32_t need = s->needs[k].qty;
        const uint32_t target = p->work_s >= total ? need : (need * p->work_s + total - 1) / total;
        if (target <= p->used[k]) continue;
        const int got = l.take(d.site, g, static_cast<int>(target - p->used[k]), Booking::Build, info, p->res[k]);
        p->used[k] = static_cast<uint16_t>(p->used[k] + got);
        if (d.slot[p->step][k] >= 0) p->built[d.slot[p->step][k]] = static_cast<uint16_t>(p->built[d.slot[p->step][k]] + got);
    }
    // lernen: 1 Erfahrung je 10 Arbeitsminuten am Schritt
    const int xp = static_cast<int>(p->person_s / kXpSeconds - before / kXpSeconds);
    if (figures && xp > 0 && s->level != SkillLevel::Unknown) {
        const int64_t m = floor_div(at_us, WorldClock::kUsPerMinute);
        figures->learn(who, s->skill, xp, day, static_cast<int>(m - floor_div(m, 1440) * 1440));
    }
    ++progress_;
    return n;
}

void ProjectList::leave(uint16_t id, uint16_t who) {
    if (Project* p = get(id); p && who && p->who == who) p->leaving = true;
}

bool ProjectList::test_start(int def, int64_t at_us, Ledger& l, TaskList& /*tasks*/) {
    if (!ensure() || def < 0 || def >= def_n_) return false;
    for (int i = n_ - 1; i >= 0; --i) {
        if (s_->p[i].def != def) continue;
        l.release_holder(s_->p[i].id);  // sein Material bleibt an der Baustelle, das neue Projekt nimmt es
        for (int j = i + 1; j < n_; ++j) s_->p[j - 1] = s_->p[j];
        --n_;
    }
    ++version_;
    return add(def, at_us) != nullptr;
}

bool ProjectList::test_jump(int def, int step, int64_t at_us, int32_t day, Ledger& l, TaskList& tasks, EventBus* bus) {
    if (!ensure() || def < 0 || def >= def_n_) return false;
    Project* p = nullptr;
    for (int i = 0; i < n_ && !p; ++i)
        if (s_->p[i].def == def && !s_->p[i].done(s_->d[def].steps)) p = &s_->p[i];
    if (!p && !test_start(def, at_us, l, tasks)) return false;
    for (int i = 0; i < n_ && !p; ++i)
        if (s_->p[i].def == def) p = &s_->p[i];
    const DefData& d = s_->d[def];
    step = std::clamp(step, 0, static_cast<int>(d.steps));
    BookingInfo info;
    info.at_us = at_us;
    info.reason = "pruefbefehl";
    info.holder = p->id;
    while (p->step < step) {
        // verbauen, was an der Baustelle liegt (nie mehr als der Bedarf); fehlt etwas, bleibt es ungebucht
        l.release_holder(p->id);
        p->res.fill(0);
        const ProjectStepDef& s = d.def->steps[p->step];
        for (int k = 0; k < s.need_count && k < Project::kMaxNeeds; ++k) {
            const int g = d.goods[p->step][k];
            if (g < 0 || s.needs[k].qty <= p->used[k]) continue;
            const int got = l.take(d.site, g, s.needs[k].qty - p->used[k], Booking::Build, info);
            p->used[k] = static_cast<uint16_t>(p->used[k] + got);
            if (d.slot[p->step][k] >= 0) p->built[d.slot[p->step][k]] = static_cast<uint16_t>(p->built[d.slot[p->step][k]] + got);
        }
        p->begun = true;
        p->work_s = step_total_s(*p);
        finish_step(*p, at_us, day, l, bus);
    }
    ++version_;
    return true;
}

const char* ProjectList::wait_text(const Project& p, const Ledger& l, char* buf, int n) const {
    if (n > 0) buf[0] = 0;
    const DefData& d = s_->d[p.def];
    const ProjectStepDef* s = current(p);
    if (!s) return buf;
    switch (p.wait) {
        case ProjectWait::Material: {
            // was an der Baustelle für den Schritt fehlt (höchstens zwei Waren)
            char a[32], b[32];
            int shown = 0;
            for (int k = 0; k < s->need_count && k < Project::kMaxNeeds && shown < 2; ++k) {
                const int g = d.goods[p.step][k];
                if (g < 0 || d.site < 0) continue;
                const int miss = s->needs[k].qty - l.available(d.site, g);
                if (miss <= 0) continue;
                good_count_text(l.good(g), miss, shown == 0 ? a : b, 32);
                ++shown;
            }
            if (shown == 0) std::snprintf(buf, n, "wartet auf Material");
            else if (shown == 1) std::snprintf(buf, n, "wartet auf %s", a);
            else std::snprintf(buf, n, "wartet auf %s und %s", a, b);
            break;
        }
        case ProjectWait::Skill:
            std::snprintf(buf, n, "wartet auf jemanden mit %s", skill_title(s->skill));
            break;
        case ProjectWait::Worker: std::snprintf(buf, n, "wartet, bis jemand Zeit hat"); break;
        case ProjectWait::Weather: std::snprintf(buf, n, "ruht wegen des Wetters"); break;
        case ProjectWait::Rest: std::snprintf(buf, n, "Feierabend, weiter um %d Uhr", kWorkFrom / 60); break;
        default: break;
    }
    return buf;
}

const char* ProjectList::describe(int i, const Ledger& l, char* buf, int n) const {
    if (!s_ || i < 0 || i >= n_) {
        if (n > 0) buf[0] = 0;
        return buf;
    }
    const Project& p = s_->p[i];
    const DefData& d = s_->d[p.def];
    const ProjectStepDef* s = current(p);
    if (!s) {
        std::snprintf(buf, n, "#%u %s: fertig (räumt auf)", static_cast<unsigned>(p.id), d.def->title);
        return buf;
    }
    int len = std::snprintf(buf, n, "#%u %s: Schritt %d/%d %s, %s, Arbeit %u/%u Min.", static_cast<unsigned>(p.id),
                            d.def->title, p.step + 1, d.steps, s->id, p.begun ? "begonnen" : "Material fehlt",
                            static_cast<unsigned>(p.work_s / 60), static_cast<unsigned>(s->minutes));
    for (int k = 0; k < s->need_count && k < Project::kMaxNeeds && len > 0 && len < n; ++k) {
        const int g = d.goods[p.step][k];
        if (g < 0) continue;
        len += std::snprintf(buf + len, n - len, ", %s %u/%u", l.good(g).id, static_cast<unsigned>(p.used[k]),
                             static_cast<unsigned>(s->needs[k].qty));
    }
    if (len > 0 && len < n)
        std::snprintf(buf + len, n - len, ", Figur %u, wartet: %s", static_cast<unsigned>(p.who), project_wait_id(p.wait));
    return buf;
}

const char* ProjectList::check(const Ledger& l, char* buf, int n) const {
    for (int i = 0; s_ && i < n_; ++i) {
        const Project& p = s_->p[i];
        if (p.id < kFirstId || (i > 0 && s_->p[i - 1].id >= p.id)) {
            std::snprintf(buf, n, "Projekt %u: Kennungen nicht aufsteigend", static_cast<unsigned>(p.id));
            return buf;
        }
        for (int k = 0; k < i && p.who; ++k)
            if (s_->p[k].who == p.who) {
                std::snprintf(buf, n, "Figur %u arbeitet an zwei Projekten", static_cast<unsigned>(p.who));
                return buf;
            }
        const ProjectStepDef* s = current(p);
        if (!s) continue;
        if (p.work_s > step_total_s(p)) {
            std::snprintf(buf, n, "Projekt %u: Arbeit über dem Schritt", static_cast<unsigned>(p.id));
            return buf;
        }
        for (int k = 0; k < s->need_count && k < Project::kMaxNeeds; ++k) {
            if (p.used[k] > s->needs[k].qty) {
                std::snprintf(buf, n, "Projekt %u: mehr verbaut als nötig", static_cast<unsigned>(p.id));
                return buf;
            }
            if (s_->d[p.def].goods[p.step][k] < 0) continue;
            const int left = l.reservation_left(p.res[k]);
            if (p.begun ? left != s->needs[k].qty - p.used[k] : left != 0) {
                std::snprintf(buf, n, "Projekt %u: Reservierung %d statt %d", static_cast<unsigned>(p.id), left,
                              p.begun ? s->needs[k].qty - p.used[k] : 0);
                return buf;
            }
        }
    }
    return nullptr;
}

void ProjectList::save(SaveWriter& w, const Ledger& l) const {
    w.u16(next_);
    w.u16(static_cast<uint16_t>(s_ ? n_ : 0));
    for (int i = 0; s_ && i < n_; ++i) {  // nach Kennung sortiert
        const Project& p = s_->p[i];
        const DefData& d = s_->d[p.def];
        const size_t r = w.record_begin();
        w.u16(p.id);
        w.str(d.def->id);
        w.u8(p.step);
        w.u8(static_cast<uint8_t>((p.begun ? 1 : 0) | (p.leaving ? 2 : 0)));
        w.u8(static_cast<uint8_t>(p.wait));
        w.u16(p.who);
        w.u32(p.work_s);
        w.i64(p.started_us);
        w.i64(p.step_us);
        w.i64(p.done_us);
        w.i64(p.seen_us);
        w.u32(p.person_s);
        w.u32(p.total_person_s);
        w.u8(static_cast<uint8_t>(kProjectWaits));
        for (int k = 0; k < kProjectWaits; ++k) {
            w.u32(p.wait_s[k]);
            w.u32(p.total_wait_s[k]);
        }
        // laufender Schritt: je Bedarf Ware, verbaut, Reservierung; verbaut insgesamt je Ware
        const ProjectStepDef* s = current(p);
        const int needs = s ? std::min<int>(s->need_count, Project::kMaxNeeds) : 0;
        w.u8(static_cast<uint8_t>(needs));
        for (int k = 0; k < needs; ++k) {
            const int g = d.goods[p.step][k];
            w.str(g >= 0 ? l.good(g).id : "");
            w.u16(p.used[k]);
            w.u16(p.res[k]);
        }
        w.u8(d.uniq_n);
        for (int u = 0; u < d.uniq_n; ++u) {
            w.str(l.good(d.uniq[u]).id);
            w.u16(p.built[u]);
        }
        w.record_end(r);
    }
}

int ProjectList::load(SaveReader& r, uint16_t /*version*/, const Ledger& l) {
    if (!ensure()) return 0;
    const uint16_t next = r.u16();
    const int m = r.u16();
    int loaded = 0;
    for (int k = 0; k < m && r.ok(); ++k) {
        SaveReader x = r.record();
        Project p;
        p.id = x.u16();
        char id[40];
        const std::string_view key = x.view();
        std::snprintf(id, sizeof(id), "%.*s", static_cast<int>(std::min<size_t>(key.size(), 39)), key.data());
        const int def = find_def(id);
        p.step = x.u8();
        const uint8_t flags = x.u8();
        p.begun = flags & 1;
        p.leaving = flags & 2;
        const uint8_t wait = x.u8();
        p.wait = wait < kProjectWaits ? static_cast<ProjectWait>(wait) : ProjectWait::None;
        p.who = x.u16();
        p.work_s = x.u32();
        p.started_us = x.i64();
        p.step_us = x.i64();
        p.done_us = x.i64();
        p.seen_us = x.i64();
        p.person_s = x.u32();
        p.total_person_s = x.u32();
        const int waits = x.u8();
        for (int w = 0; w < waits; ++w) {
            const uint32_t a = x.u32(), b = x.u32();
            if (w < kProjectWaits) {
                p.wait_s[w] = a;
                p.total_wait_s[w] = b;
            }
        }
        // Daten gibt es nicht mehr: Projekt fällt weg (seine Reservierungen gibt step frei)
        if (!x.ok() || def < 0 || p.id < kFirstId || get(p.id) || n_ >= kMax) continue;
        const DefData& d = s_->d[def];
        p.def = static_cast<uint8_t>(def);
        if (p.step > d.steps) p.step = d.steps;
        // Bedarfe nach Kennung der Ware zuordnen (Daten können sich geändert haben)
        const int needs = x.u8();
        for (int n = 0; n < needs; ++n) {
            const std::string_view good = x.view();
            const uint16_t used = x.u16(), res = x.u16();
            const int g = l.find_good(good);
            for (int k2 = 0; p.step < d.steps && g >= 0 && k2 < Project::kMaxNeeds; ++k2)
                if (d.goods[p.step][k2] == g) {
                    p.used[k2] = std::min<uint16_t>(used, d.def->steps[p.step].needs[k2].qty);
                    p.res[k2] = res;
                    break;
                }
        }
        const int uniq = x.u8();
        for (int u = 0; u < uniq; ++u) {
            const int g = l.find_good(x.view());
            const uint16_t q = x.u16();
            for (int v = 0; g >= 0 && v < d.uniq_n; ++v)
                if (d.uniq[v] == g) p.built[v] = q;
        }
        if (!x.ok()) continue;
        if (p.step < d.steps) p.work_s = std::min(p.work_s, std::max<uint32_t>(1, d.def->steps[p.step].minutes * 60u));
        int at = n_;
        while (at > 0 && s_->p[at - 1].id > p.id) --at;
        for (int i = n_; i > at; --i) s_->p[i] = s_->p[i - 1];
        s_->p[at] = p;
        ++n_;
        ++loaded;
    }
    next_ = next >= kFirstId ? next : kFirstId;
    orphans_ = true;
    ++version_;
    return loaded;
}

}  // namespace pixelia
