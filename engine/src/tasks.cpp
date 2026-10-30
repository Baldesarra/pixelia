// Aufgabenliste der Gemeinschaft (seit 0.5.10 Teil 2): Holen und Bringen, Herstellen, Notbestand; siehe tasks.h.

#include "pixelia/engine/tasks.h"

#include <algorithm>
#include <cstdio>
#include <new>
#include <string_view>

#include "pixelia/engine/events.h"
#include "pixelia/engine/figure.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/memory.h"
#include "pixelia/engine/save.h"

namespace pixelia {

namespace {

const char* const kPhaseIds[] = {"offen", "holen", "bringen", "zurueck", "arbeiten", "fertig"};
static_assert(sizeof(kPhaseIds) / sizeof(kPhaseIds[0]) == static_cast<int>(TaskPhase::kCount));

// Regel eines Lagerplatzes für eine Ware, nullptr = keine
const StockRule* rule_of(const Ledger& l, int store, int good) {
    const StoreDef* d = l.store_def(store);
    if (!d || !d->rules) return nullptr;
    for (int r = 0; r < d->rule_count; ++r)
        if (l.find_good(d->rules[r].good) == good) return &d->rules[r];
    return nullptr;
}

}  // namespace

const char* task_phase_id(TaskPhase p) {
    const int i = static_cast<int>(p);
    return i >= 0 && i < static_cast<int>(TaskPhase::kCount) ? kPhaseIds[i] : "?";
}

TaskList::~TaskList() {
    if (t_) asset_free(t_);  // Task ist trivial zerstörbar
}

bool TaskList::ensure() {
    if (t_) return true;
    void* p = asset_alloc(sizeof(Task) * kMax);
    if (!p) return false;
    t_ = static_cast<Task*>(p);
    for (int i = 0; i < kMax; ++i) new (&t_[i]) Task();
    return true;
}

void TaskList::clear() {
    n_ = 0;
    next_ = 1;
    created_ = 0;
    orphans_ = true;
    ++version_;
}

const Task* TaskList::find(uint16_t id) const {
    for (int i = 0; i < n_; ++i)
        if (t_[i].id == id) return &t_[i];
    return nullptr;
}

Task* TaskList::of(uint16_t who) {
    if (!who) return nullptr;
    for (int i = 0; i < n_; ++i)
        if (t_[i].who == who && t_[i].active()) return &t_[i];
    return nullptr;
}

const Task* TaskList::of(uint16_t who) const { return const_cast<TaskList*>(this)->of(who); }

bool TaskList::claim(uint16_t id, uint16_t who, int64_t at_us) {
    if (!who || of(who)) return false;
    for (int i = 0; i < n_; ++i) {
        Task& t = t_[i];
        if (t.id != id) continue;
        if (t.phase != TaskPhase::Open) return false;
        t.who = who;
        t.phase = t.kind == TaskKind::Fetch ? TaskPhase::Fetch : TaskPhase::Work;
        t.claimed_us = at_us;
        ++version_;
        return true;
    }
    return false;
}

Task* TaskList::add(TaskKind kind, int good, int from, int to, int qty, int64_t at_us, uint8_t flags) {
    if (!ensure() || n_ >= kMax || qty <= 0) return nullptr;
    // nächste freie Kennung (nach kMaxId wieder von vorn); einsortieren, damit die Liste nach Kennung sortiert bleibt
    uint16_t id = next_;
    for (int guard = 0; guard <= kMax && find(id); ++guard) id = static_cast<uint16_t>(id >= kMaxId ? 1 : id + 1);
    next_ = static_cast<uint16_t>(id >= kMaxId ? 1 : id + 1);
    int at = n_;
    while (at > 0 && t_[at - 1].id > id) --at;
    for (int i = n_; i > at; --i) t_[i] = t_[i - 1];
    ++n_;
    Task& t = t_[at];
    t = Task{};
    t.id = id;
    t.kind = kind;
    t.flags = flags;
    t.good = static_cast<uint8_t>(good);
    t.from = static_cast<int16_t>(from);
    t.to = static_cast<int16_t>(to);
    t.qty = static_cast<uint16_t>(std::min(qty, 0xFFFF));
    t.created_us = at_us;
    ++version_;
    ++created_;
    return &t;
}

Task* TaskList::fetch(Ledger& l, int good, int from, int to, int qty, int64_t at_us, uint8_t flags) {
    // Menge am Quell-Lager reservieren; ist nichts frei, gibt es die Aufgabe nicht
    Task* t = add(TaskKind::Fetch, good, from, to, qty, at_us, flags);
    if (!t) return nullptr;
    t->res = l.reserve(from, good, t->qty, t->id);
    t->qty = static_cast<uint16_t>(l.reservation_left(t->res));
    if (t->res && t->qty) return t;
    l.release(t->res);
    const uint16_t id = t->id;
    for (int i = 0; i < n_; ++i)
        if (t_[i].id == id) {
            remove(i);
            break;
        }
    return nullptr;
}

const Task* TaskList::add_fetch(Ledger& l, int good, int from, int to, int qty, int64_t at_us, uint8_t flags) {
    return ensure() && from >= 0 && to >= 0 && from != to ? fetch(l, good, from, to, qty, at_us, flags) : nullptr;
}

int TaskList::incoming(int to, int good) const {
    int n = 0;
    for (int i = 0; i < n_; ++i) {
        const Task& t = t_[i];
        if (t.kind != TaskKind::Fetch || t.to != to || (good >= 0 && t.good != good)) continue;
        if (t.phase == TaskPhase::Open || t.phase == TaskPhase::Fetch) n += t.qty;
        if (t.phase == TaskPhase::Bring) n += t.carried;
    }
    return n;
}

void TaskList::cancel(uint16_t id) {
    for (int i = 0; i < n_; ++i)
        if (t_[i].id == id && t_[i].phase == TaskPhase::Open) {
            t_[i].phase = TaskPhase::Done;
            ++version_;
        }
}

void TaskList::remove(int i) {
    for (int k = i + 1; k < n_; ++k) t_[k - 1] = t_[k];
    --n_;
    ++version_;
}

int TaskList::active(TaskKind kind, int to, int good, int flags_mask, int flags) const {
    int n = 0;
    for (int i = 0; i < n_; ++i) {
        const Task& t = t_[i];
        n += t.active() && t.kind == kind && t.to == to && t.good == good && (t.flags & flags_mask) == flags;
    }
    return n;
}

void TaskList::step(int64_t at_us, int32_t day, Ledger& l, const FigureBook& book, EventBus* bus) {
    if (!ensure() || !l.ready()) return;
    // nach dem Laden (oder clear): Reservierungen von Aufgaben, die es nicht gibt, freigeben (Stand ohne Aufgabenliste)
    if (orphans_) {
        orphans_ = false;
        uint16_t h[96];
        const int k = l.holders(h, 96);
        for (int j = 0; j < k; ++j)
            if (h[j] <= kMaxId && !find(h[j])) l.release_holder(h[j]);
    }
    // 1. aufräumen: Fertiges weg (Rest der Reservierung frei); Figur gibt es nicht mehr → wieder offen
    for (int i = n_ - 1; i >= 0; --i) {
        Task& t = t_[i];
        if (t.who && t.active() && !book.find(t.who)) {
            if (t.phase == TaskPhase::Fetch || t.phase == TaskPhase::Work) {
                t.who = 0;
                t.phase = TaskPhase::Open;
                ++version_;
            } else {
                t.phase = TaskPhase::Done;  // Getragenes bleibt in der Trage (Bilanz stimmt)
            }
        }
        if (t.phase != TaskPhase::Done) continue;
        if (t.kind == TaskKind::Fetch) l.release_holder(t.id);
        remove(i);
    }
    // 2. Herstellen am Ziel beenden (Gemeinschaftsaufgabe: die Ware reicht wieder einen Tag); offenes Holen, das nicht
    //    mehr nötig ist (Vorrat wieder über low)
    for (int i = 0; i < n_; ++i) {
        Task& t = t_[i];
        if (t.kind == TaskKind::Produce && (t.phase == TaskPhase::Open || t.phase == TaskPhase::Work)) {
            const bool full = l.room(t.to, t.good) == 0;
            const bool enough = (t.flags & kTaskCommunity) ? l.level(t.good) != StockLevel::Emergency
                                                           : l.qty(t.to, t.good) >= t.qty;
            if (full || enough) {
                t.phase = TaskPhase::Done;
                ++version_;
            }
        } else if (t.kind == TaskKind::Fetch && t.phase == TaskPhase::Open && !(t.flags & kTaskProject)) {
            const StockRule* r = rule_of(l, t.to, t.good);
            if (!r || l.qty(t.to, t.good) >= r->low) {
                t.phase = TaskPhase::Done;
                ++version_;
            }
        }
    }
    // 3. neue Aufgaben aus den Regeln, Lagerplätze und Regeln in der Reihenfolge der Daten
    for (int s = 0; s < l.stores(); ++s) {
        const StoreDef* d = l.store_def(s);
        if (!d || !d->rules || d->kind == StoreKind::Site) continue;
        for (int k = 0; k < d->rule_count; ++k) {
            const StockRule& r = d->rules[k];
            const int g = l.find_good(r.good);
            if (g < 0) continue;
            const int q = l.qty(s, g);
            if (q >= r.low) continue;
            if (d->kind == StoreKind::Producer) {
                if (l.room(s, g) > 0 && !active(TaskKind::Produce, s, g, kTaskCommunity, 0))
                    add(TaskKind::Produce, g, -1, s, r.want, at_us, 0);
                continue;
            }
            const int src = d->source ? l.find_store(d->source) : -1;
            if (src < 0 || src == s || active(TaskKind::Fetch, s, g, 0, 0)) continue;
            int need = std::min(static_cast<int>(r.want) - q, l.room(s, g));
            need = std::min(need, Ledger::kHandCapacity / std::max<int>(1, l.good(g).weight));
            const int avail = l.available(src, g);
            // lohnt der Weg (mindestens die halbe Ladung)? Sonst warten; ist der Vorrat schon leer, holt jemand, was da ist
            if (need <= 0 || avail <= 0 || (avail * 2 < need && q > 0)) continue;
            fetch(l, g, src, s, std::min(need, avail), at_us, 0);
        }
    }
    // 4. Notbestand: Herstellen wird Gemeinschaftsaufgabe an jedem Erzeugerlager mit einer Regel für die Ware
    for (int g = 0; g < l.goods(); ++g) {
        if (l.level(g) != StockLevel::Emergency) continue;
        for (int s = 0; s < l.stores(); ++s) {
            const StoreDef* d = l.store_def(s);
            if (!d || d->kind != StoreKind::Producer || !rule_of(l, s, g) || l.room(s, g) == 0) continue;
            const bool started = active(TaskKind::Produce, s, g, kTaskCommunity, kTaskCommunity) > 0;
            int added = 0;
            for (int k = active(TaskKind::Produce, s, g, 0, 0); k < kHelpers; ++k, ++added)
                if (!add(TaskKind::Produce, g, -1, s, d->capacity, at_us, kTaskCommunity)) break;
            if (started || !added || !bus) continue;
            // Wunsch des Users 2026-10-29: Beginn einer Gemeinschaftsaufgabe steht in der Chronik
            WorldEvent ev;
            ev.kind = EventKind::Stock;
            ev.a = 3;
            ev.day = day;
            ev.place = d->place;
            std::snprintf(ev.text, sizeof(ev.text), "Gemeinschaftsaufgabe: alle machen %.20s", l.good(g).title);
            bus->publish(ev);
        }
    }
}

const char* TaskList::describe(int i, const Ledger& l, char* buf, int n) const {
    if (i < 0 || i >= n_) {
        if (n > 0) buf[0] = 0;
        return buf;
    }
    const Task& t = t_[i];
    std::snprintf(buf, n, "#%u %s%s %u/%u %s %s%s%s, Figur %u", static_cast<unsigned>(t.id), task_phase_id(t.phase),
                  (t.flags & kTaskCommunity) ? " (Gemeinschaft)" : (t.flags & kTaskProject) ? " (Projekt)" : "",
                  static_cast<unsigned>(t.kind == TaskKind::Fetch ? t.carried : l.qty(t.to, t.good)),
                  static_cast<unsigned>(t.qty), l.good(t.good).title, t.from >= 0 ? l.store_title(t.from) : "",
                  t.from >= 0 ? " → " : "", l.store_title(t.to), static_cast<unsigned>(t.who));
    return buf;
}

const char* TaskList::check(const Ledger& l, char* buf, int n) const {
    for (int i = 0; i < n_; ++i) {
        const Task& t = t_[i];
        if (!t.id || (i > 0 && t_[i - 1].id >= t.id)) {
            std::snprintf(buf, n, "Aufgabe %u: Kennungen nicht aufsteigend", static_cast<unsigned>(t.id));
            return buf;
        }
        if (t.phase == TaskPhase::Open ? t.who != 0 : t.active() && t.who == 0) {
            std::snprintf(buf, n, "Aufgabe %u: Phase %s mit Figur %u", static_cast<unsigned>(t.id), task_phase_id(t.phase),
                          static_cast<unsigned>(t.who));
            return buf;
        }
        for (int k = 0; k < i && t.who; ++k)
            if (t_[k].who == t.who && t_[k].active() && t.active()) {
                std::snprintf(buf, n, "Figur %u hat zwei Aufgaben", static_cast<unsigned>(t.who));
                return buf;
            }
        if (t.kind != TaskKind::Fetch) continue;
        if (l.reservation_left(t.res) > t.qty) {
            std::snprintf(buf, n, "Aufgabe %u: mehr reserviert als geholt wird", static_cast<unsigned>(t.id));
            return buf;
        }
        if ((t.phase == TaskPhase::Bring || t.phase == TaskPhase::Return) && l.qty(l.hand(t.who), t.good) < t.carried) {
            std::snprintf(buf, n, "Aufgabe %u: %u %s getragen, Trage von %u hat %d", static_cast<unsigned>(t.id),
                          static_cast<unsigned>(t.carried), l.good(t.good).id, static_cast<unsigned>(t.who),
                          l.qty(l.hand(t.who), t.good));
            return buf;
        }
    }
    return nullptr;
}

void TaskList::save(SaveWriter& w, const Ledger& l) const {
    char from[12], to[12];
    w.u16(next_);
    w.u16(static_cast<uint16_t>(n_));
    for (int i = 0; i < n_; ++i) {  // nach Kennung sortiert
        const Task& t = t_[i];
        const size_t r = w.record_begin();
        w.u16(t.id);
        w.u8(static_cast<uint8_t>(t.kind));
        w.u8(static_cast<uint8_t>(t.phase));
        w.u8(t.flags);
        w.str(l.good(t.good).id);
        w.str(t.from >= 0 ? l.store_key(t.from, from, sizeof(from)) : "");
        w.str(t.to >= 0 ? l.store_key(t.to, to, sizeof(to)) : "");
        w.u16(t.qty);
        w.u16(t.carried);
        w.u16(t.res);
        w.u16(t.who);
        w.i64(t.created_us);
        w.i64(t.claimed_us);
        w.record_end(r);
    }
}

int TaskList::load(SaveReader& r, uint16_t /*version*/, const Ledger& l) {
    if (!ensure()) return 0;
    const uint16_t next = r.u16();
    const int m = r.u16();
    int loaded = 0;
    for (int k = 0; k < m && r.ok(); ++k) {
        SaveReader x = r.record();
        Task t;
        t.id = x.u16();
        const uint8_t kind = x.u8(), phase = x.u8();
        t.flags = x.u8();
        const int g = l.find_good(x.view());
        const std::string_view from = x.view(), to = x.view();
        t.from = static_cast<int16_t>(from.empty() ? -1 : l.find_store(from));
        t.to = static_cast<int16_t>(to.empty() ? -1 : l.find_store(to));
        t.qty = x.u16();
        t.carried = x.u16();
        t.res = x.u16();
        t.who = x.u16();
        t.created_us = x.i64();
        t.claimed_us = x.i64();
        // Ware oder Lagerplatz gibt es nicht mehr: Aufgabe fällt weg (ihre Reservierung gibt step frei, Getragenes bleibt
        // in der Trage)
        if (!x.ok() || !t.id || t.id > kMaxId || find(t.id) || n_ >= kMax || g < 0 || t.to < 0 ||
            kind >= static_cast<uint8_t>(TaskKind::kCount) || phase >= static_cast<uint8_t>(TaskPhase::kCount))
            continue;
        t.kind = static_cast<TaskKind>(kind);
        t.phase = static_cast<TaskPhase>(phase);
        t.good = static_cast<uint8_t>(g);
        if (t.kind == TaskKind::Fetch && t.from < 0) continue;
        int at = n_;
        while (at > 0 && t_[at - 1].id > t.id) --at;
        for (int i = n_; i > at; --i) t_[i] = t_[i - 1];
        t_[at] = t;
        ++n_;
        ++loaded;
    }
    next_ = next && next <= kMaxId ? next : 1;
    orphans_ = true;
    ++version_;
    return loaded;
}

}  // namespace pixelia
