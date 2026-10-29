// Stufen der Simulation (seit 0.5.9 Teil 2, SETTING.md Kapitel 15; Entscheidungen des Users 2026-10-27): Tätigkeiten
// als Zeitblöcke mit exakten Weltzeiten, grob an jeder Takt-Grenze (ganze Weltsekunde) für alle Orte, voll dazwischen
// für die betretenen Orte; Übergaben nur an Takt-Grenzen, beide Stufen auf Kern 1.
//
// Warum beide Stufen dasselbe buchen:
// - Jede Entscheidung fällt zur Zeit ihres Blockendes (nicht dann, wenn sie gerechnet wird) und würfelt mit einem
//   eigenen Zufall je Figur und Entscheidung (doing_seed). Wann ein Ort sie rechnet, ändert nichts.
// - Ein Ort rechnet seine Blockenden in Zeitreihenfolge (gleichzeitig: nach Kennung) und sieht dabei nur seine eigenen
//   Figuren und Zustände. Orte berühren sich nur an Takt-Grenzen: Hinausgehen endet dort, Ankünfte kommen dort an,
//   Regeln lesen den Bus dort, und jeder Neuankömmling entscheidet erst an der nächsten Takt-Grenze.
// - Blockenden genau auf einer Takt-Grenze rechnet nur die Schleife der Takt-Grenzen; die volle Stufe rechnet
//   dazwischen nur, was echt davor liegt. Danach erst schreibt die Bühne Kalender, Himmel und Ortszustände fort
//   (Stage::tick): Beide Stufen sehen dieselbe Umgebung.

#include <algorithm>

#include "pixelia/engine/place.h"
#include "pixelia/engine/stage.h"

namespace pixelia {

namespace {

int64_t grid_floor(int64_t us) {
    const int64_t g = Stage::kGridUs;
    return (us >= 0 ? us / g : (us - g + 1) / g) * g;
}

int64_t grid_ceil(int64_t us) {
    const int64_t f = grid_floor(us);
    return f == us ? us : f + Stage::kGridUs;
}

}  // namespace

uint64_t Stage::doing_seed(uint16_t who, uint32_t seq) const {
    // splitmix64 über Startwert der Welt, Kennung und laufende Nummer: hängt nicht davon ab, wer vorher würfelte
    uint64_t x = seed_ ^ (0xD1B54A32D192ED03ULL * (who + 1ULL)) ^ (0x9E3779B97F4A7C15ULL * (seq + 1ULL));
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

void Stage::advance_grid(int64_t now) {
    last_grid_steps_ = 0;
    if (grid_ == INT64_MIN || now < grid_ - 2 * kGridUs || now - grid_ > kActivityCatchUp) reseat(now);
    // grobe Stufe: Takt-Grenze für Takt-Grenze, alle Orte
    for (int n = 0; n < kMaxGridPerTick && grid_ + kGridUs <= now; ++n) {
        const int64_t t = grid_ + kGridUs;
        deliver_transit(t);
        for (int i = 0; i < count_; ++i) plan(i, t);
        // Warenbuch (seit 0.5.10): Fächer der Reichweite, Besonderes für die Chronik; was mehrere Orte betrifft, ändert
        // sich nur hier
        env_.ledger.step(t, env_.calendar.date().day, &env_.events);
        // Aufgabenliste (seit 0.5.10 Teil 2): nach allen Planern, so sieht sie jeden Ort gleich weit; übernommen wird an
        // der nächsten Takt-Grenze
        env_.tasks.step(t, env_.calendar.date().day, env_.ledger, env_.figures, &env_.events);
        env_.events.deliver();  // Regeln sehen alles bis hierher, egal welcher Ort sichtbar ist
        for (int i = 0; i < count_; ++i) scenes_[i]->step(t);
        grid_ = t;
        ++last_grid_steps_;
    }
}

void Stage::advance_full(int64_t now) {
    // volle Stufe: die betretenen Orte bis jetzt, aber echt vor der nächsten Takt-Grenze (seit 0.5.10 nach Kalender und
    // Ortszuständen, siehe Stage::tick)
    if (grid_ == INT64_MIN) return;
    const int64_t until = std::min(now, grid_ + kGridUs - 1);
    for (int i = 0; i < count_; ++i)
        if (entered(i)) plan(i, until);
}

bool Stage::states_behind() const {
    // seit 0.5.10: holen die Ortszustände mehr als kActivityCatchUp nach (Laden, Sprung der Uhr), warten die Tätigkeiten;
    // danach setzen sie neu an (reseat). So kommt das Vergangene vor dem Jetzt ins Warenbuch
    if (!world_valid_ || state_minute_ == INT64_MIN) return false;
    const int64_t m = WorldClock::kUsPerMinute;
    const int64_t wm = world_us_ >= 0 ? world_us_ / m : (world_us_ - m + 1) / m;
    return wm - state_minute_ > kActivityCatchUp / m;
}

void Stage::reseat(int64_t now) {
    // Sprung der Uhr (oder erster Schritt): gleicher Ort, gleiche Kachel, an der nächsten Takt-Grenze neu entscheiden.
    // Wer auf einer Verbindung ist, kommt an der nächsten Takt-Grenze an (deliver_transit)
    if (grid_ != INT64_MIN) ++reseats_;
    grid_ = grid_floor(now);
    for (int k = 0; k < env_.figures.count(); ++k) {
        FigureRecord& f = env_.figures.at(k);
        if (f.where.kind != Whereabouts::Kind::Place) continue;
        Doing& d = f.doing;
        d.kind = DoingKind::Idle;
        d.sx = d.gx;
        d.sy = d.gy;
        d.steps = 0;
        d.start_us = d.walk_us = grid_;
        d.end_us = grid_ + kGridUs;
        d.since_us = grid_;
    }
}

void Stage::plan(int i, int64_t until) {
    const int place = place_of_[i];
    if (place < 0) return;
    FigureBook& book = env_.figures;
    for (int guard = 0; guard < 4096; ++guard) {
        FigureRecord* next = nullptr;
        for (int k = 0; k < book.count(); ++k) {
            FigureRecord& f = book.at(k);
            if (f.where.kind != Whereabouts::Kind::Place || f.where.place != place || f.doing.end_us > until) continue;
            if (!next || f.doing.end_us < next->doing.end_us ||
                (f.doing.end_us == next->doing.end_us && f.id < next->id))
                next = &f;
        }
        if (!next) return;
        run_doing(i, *next);
    }
}

void Stage::run_doing(int i, FigureRecord& f) {
    Doing& d = f.doing;
    const int64_t at = d.end_us;
    if (d.kind == DoingKind::Leave) {
        depart(i, f);
        return;
    }
    if (d.kind != DoingKind::None) scenes_[i]->finish(f, d);
    const uint32_t seq = d.seq + 1;
    Rng rng(doing_seed(f.id, seq));
    Doing next;
    ++decisions_;
    if (!scenes_[i]->decide(f, at, rng, &next)) {
        // der Ort plant nicht: die Figur bleibt stehen
        next = Doing{};
        next.kind = DoingKind::Idle;
        next.sx = next.gx = d.gx;
        next.sy = next.gy = d.gy;
        next.start_us = next.walk_us = at;
        next.end_us = INT64_MAX;
    }
    next.seq = seq;
    next.since_us = d.since_us;
    if (next.kind == DoingKind::Leave)
        next.end_us = grid_ceil(std::max(next.walk_us, at));  // Übergabe nur an einer Takt-Grenze
    else if (next.end_us <= at)
        next.end_us = at + kGridUs;  // immer vorwärts
    d = next;
}

void Stage::start_doing(FigureRecord& f, int scene, uint8_t entry, int64_t at) {
    // angekommen (oder Start, Laden): am Eingang stehen, an der nächsten Takt-Grenze entscheiden
    const uint32_t seq = f.doing.seq + 1;
    Rng rng(doing_seed(f.id, seq));
    uint8_t tx = 0, ty = 0;
    scenes_[scene]->entry_tile(entry, rng, &tx, &ty);
    Doing d;
    d.kind = DoingKind::Idle;
    d.seq = seq;
    d.sx = d.gx = tx;
    d.sy = d.gy = ty;
    d.start_us = d.walk_us = d.since_us = at;
    d.end_us = at == INT64_MIN ? INT64_MIN : grid_floor(at) + kGridUs;  // INT64_MIN: Start, reseat setzt die Zeit
    f.doing = d;
}

void Stage::depart(int i, FigureRecord& f) {
    // an der Takt-Grenze: in einem Schritt hinaus, auf die Verbindung oder gleich in den Zielort
    const int64_t at = f.doing.end_us;
    Travel t;
    const bool ok = scenes_[i]->exit_travel(f.doing.exit, &t);
    t.who = f.id;
    t.follow = f.lead();
    t.look = f.look.row[0];
    const int to = ok && t.to ? find(t.to) : -1;
    if (to < 0) {
        start_doing(f, i, f.doing.exit, at);  // Ziel gibt es nicht: am selben Ausgang zurück
        return;
    }
    if (t.travel > 0 && transit_n_ < kMaxTransit) {
        // auf der Verbindung: gehört keinem Ort, bis die Reisezeit um ist (eine Figur ist nie doppelt)
        const Transit tr{t, at, at + static_cast<int64_t>(t.travel) * kGridUs, static_cast<uint8_t>(i),
                         static_cast<uint8_t>(to)};
        transit_[transit_n_++] = tr;
        Whereabouts& w = f.where;
        w.kind = Whereabouts::Kind::Link;
        w.place = static_cast<uint8_t>(place_of_[to]);
        w.from = static_cast<uint8_t>(place_of_[i]);
        w.entry = t.entry;
        w.from_exit = t.from_exit;
        w.transition = static_cast<uint8_t>(t.transition);
        w.depart_us = tr.depart_us;
        w.arrive_us = tr.arrive_us;
        const uint32_t seq = f.doing.seq;  // unterwegs gibt es keine Tätigkeit, nur die laufende Nummer
        f.doing = Doing{};
        f.doing.seq = seq;
        const CalendarDate& d = env_.calendar.date();
        env_.visits.leave(t.who, d.day, d.minute);
        publish_travel(EventKind::Departed, i, to, t);
        return;
    }
    arrive_at(i, to, t, at);
}

void Stage::deliver_transit(int64_t at) {
    // Ankünfte von Verbindungen in der Reihenfolge des Aufbruchs; springt die Uhr zurück, kommt man gleich an
    for (int k = 0; k < transit_n_;) {
        const Transit& tr = transit_[k];
        if (at < tr.arrive_us && at >= tr.depart_us) {
            ++k;
            continue;
        }
        const Transit done = tr;
        for (int j = k + 1; j < transit_n_; ++j) transit_[j - 1] = transit_[j];
        --transit_n_;
        arrive_at(done.from, done.to, done.t, at);
    }
}

void Stage::arrive_at(int from, int to, const Travel& t, int64_t at) {
    ++travels_;
    set_where_place(t.who, to);  // Übergabe im Weltzustand: in einem Schritt
    if (FigureRecord* f = env_.figures.find(t.who)) start_doing(*f, to, t.entry, at);
    note_arrival(t.who, to);
    publish_travel(EventKind::Arrived, from, to, t);
    // der Blick folgt bei der Ankunft (bei Wegen mit Reisezeit bleibt er so lange am Ausgangsort)
    const bool followed = follow_who_ == 0 ? t.follow : follow_who_ != kNobody && t.who == follow_who_;
    if (followed && follow_ && from == active_ && from_ < 0 && pending_ < 0) start_transition(to, t.transition);
}

}  // namespace pixelia
