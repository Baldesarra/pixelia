#include "pixelia/engine/chronicle.h"

#include <algorithm>
#include <cstdio>
#include <new>

#include "pixelia/engine/figure.h"
#include "pixelia/engine/memory.h"

namespace pixelia {

namespace {
template <typename T>
T* make_ring(int n) {
    void* p = asset_alloc(sizeof(T) * n);
    if (!p) return nullptr;
    T* t = static_cast<T*>(p);
    for (int i = 0; i < n; ++i) new (&t[i]) T();
    return t;
}
}  // namespace

// --- Chronik ---

Chronicle::~Chronicle() {
    if (e_) asset_free(e_);
}

bool Chronicle::ensure() {
    if (!e_) e_ = make_ring<ChronicleEntry>(kMax);
    return e_ != nullptr;
}

void Chronicle::clear() {
    head_ = n_ = 0;
    total_ = 0;
}

void Chronicle::add(const ChronicleEntry& e) {
    if (!ensure()) return;
    e_[(head_ + n_) % kMax] = e;
    if (n_ < kMax)
        ++n_;
    else
        head_ = (head_ + 1) % kMax;
    ++total_;
}

const ChronicleEntry& Chronicle::at(int i) const { return e_[(head_ + i) % kMax]; }

bool Chronicle::note(const WorldEvent& e, const FigureBook& figures, int32_t day, int minute) {
    ChronicleEntry c;
    c.day = day;
    c.minute = static_cast<uint16_t>(minute);
    c.kind = ChronicleKind::Event;
    c.event = e.kind;
    c.a = e.a;
    c.b = e.b;
    c.who = e.who;
    c.other = e.other;
    auto name = [&](uint16_t id) {
        const FigureRecord* f = figures.find(id);
        return f ? f->name.data() : "?";
    };
    switch (e.kind) {
        case EventKind::SeasonEvening:
        case EventKind::SeasonBegins:
        case EventKind::First:
        case EventKind::Stock:  // seit 0.5.10: nur Besonderes (Wahl des Users), nicht jede Bewegung
            std::snprintf(c.text, sizeof(c.text), "%s", e.text);
            break;
        case EventKind::SkillUp:
            std::snprintf(c.text, sizeof(c.text), "%s: %s jetzt %s", name(e.who),
                          skill_title(static_cast<Skill>(e.a)), skill_level_title(static_cast<SkillLevel>(e.b)));
            break;
        case EventKind::BondChanged:
            std::snprintf(c.text, sizeof(c.text), "%s und %s: %s %s", name(e.who), name(e.other),
                          e.b ? "jetzt" : "nur noch", bond_title(static_cast<Bond>(e.a)));
            break;
        default:
            return false;
    }
    add(c);
    return true;
}

// --- Besuche ---

VisitLog::~VisitLog() {
    if (v_) asset_free(v_);
}

bool VisitLog::ensure() {
    if (!v_) v_ = make_ring<Visit>(kMax);
    return v_ != nullptr;
}

void VisitLog::clear() { head_ = n_ = 0; }

void VisitLog::add(const Visit& v) {
    if (!ensure()) return;
    v_[(head_ + n_) % kMax] = v;
    if (n_ < kMax)
        ++n_;
    else
        head_ = (head_ + 1) % kMax;
}

const Visit& VisitLog::at(int i) const { return v_[(head_ + i) % kMax]; }

int VisitLog::open(uint16_t who) const {
    for (int i = n_ - 1; i >= 0; --i)
        if (at(i).who == who && at(i).to == Visit::kOpen) return i;
    return -1;
}

void VisitLog::arrive(uint16_t who, uint8_t place, int32_t day, int minute) {
    Visit v;
    v.who = who;
    v.place = place;
    v.day = day;
    v.from = static_cast<uint16_t>(minute);
    add(v);
}

void VisitLog::leave(uint16_t who, int32_t day, int minute) {
    const int i = open(who);
    if (i < 0) return;
    Visit& v = v_[(head_ + i) % kMax];
    const int64_t m = static_cast<int64_t>(day - v.day) * 1440 + minute;
    v.to = static_cast<uint16_t>(std::clamp<int64_t>(m, v.from, Visit::kOpen - 1));
}

}  // namespace pixelia
