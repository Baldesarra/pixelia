#include "pixelia/engine/figure.h"

#include <algorithm>
#include <new>
#include <utility>

#include "pixelia/engine/memory.h"

namespace pixelia {

namespace {

struct SkillInfo {
    const char* id;
    const char* title;
};
constexpr SkillInfo kSkillInfo[kSkills] = {
    {"kochen", "Kochen"},       {"backen", "Backen"},           {"holzarbeit", "Holzarbeit"},
    {"holzfaellen", "Holzfällen"}, {"tischlern", "Tischlern"},  {"zimmern", "Zimmern"},
    {"mauern", "Mauern"},       {"schmieden", "Schmieden"},     {"reparieren", "Reparieren"},
    {"flechten", "Flechten"},   {"spinnen", "Spinnen"},         {"weben", "Weben"},
    {"naehen", "Nähen"},        {"gaertnern", "Gärtnern"},      {"ackerbau", "Ackerbau"},
    {"kraeuter", "Kräuterkunde"}, {"pilze", "Pilzkunde"},       {"tierpflege", "Tierpflege"},
    {"melken", "Melken"},       {"imkern", "Imkern"},           {"fischen", "Fischen"},
    {"jagen", "Jagen"},         {"bergbau", "Bergbau"},         {"musizieren", "Musizieren"},
    {"erzaehlen", "Erzählen"},  {"schreiben", "Lesen und Schreiben"}, {"bahntechnik", "Bahntechnik"},
};

// Gelingen in Promille je Stufe; innerhalb der Stufe gleitend zur nächsten (wer übt, wird besser)
constexpr int kChance[kSkillLevels + 1] = {150, 450, 800, 950, 980};

bool earlier(const Memory& a, const Memory& b) { return a.day != b.day ? a.day < b.day : a.minute < b.minute; }

}  // namespace

const char* skill_id(Skill s) { return s < Skill::kCount ? kSkillInfo[static_cast<int>(s)].id : "?"; }
const char* skill_title(Skill s) { return s < Skill::kCount ? kSkillInfo[static_cast<int>(s)].title : "?"; }
bool parse_skill(std::string_view id, Skill* out) {
    for (int i = 0; i < kSkills; ++i)
        if (id == kSkillInfo[i].id) {
            *out = static_cast<Skill>(i);
            return true;
        }
    return false;
}

const char* skill_level_title(SkillLevel l) {
    static const char* const k[] = {"unbekannt", "Anfänger", "geübt", "Meister"};
    return k[static_cast<int>(l) & 3];
}
const char* outcome_title(Outcome o) {
    static const char* const k[] = {"misslungen", "mit Makel", "gelungen"};
    return static_cast<int>(o) < 3 ? k[static_cast<int>(o)] : "?";
}
const char* bond_title(Bond b) {
    static const char* const k[] = {"fremd", "bekannt", "vertraut", "hilft"};
    return k[static_cast<int>(b) & 3];
}
const char* memory_kind_name(MemoryKind k) {
    static const char* const n[] = {"keine",     "erstes-mal", "fehlversuch", "stufe", "naeher",
                                    "entfernt", "besuch",     "gabe",        "ereignis"};
    return static_cast<int>(k) < static_cast<int>(MemoryKind::kCount) ? n[static_cast<int>(k)] : "?";
}
const char* doing_kind_name(DoingKind k) {
    static const char* const n[] = {"keine", "steht", "geht", "benutzt", "geht-hinaus"};
    return static_cast<int>(k) < static_cast<int>(DoingKind::kCount) ? n[static_cast<int>(k)] : "?";
}

const Relation* FigureRecord::relation(uint16_t other) const {
    for (const Relation& r : relations)
        if (r.other == other && other != 0) return &r;
    return nullptr;
}

int FigureRecord::memory_count() const {
    int n = 0;
    for (const Memory& m : memories) n += m.kind != MemoryKind::None;
    return n;
}

// --- FigureBook ---

FigureBook::~FigureBook() {
    if (f_) asset_free(f_);
}

bool FigureBook::ensure() {
    if (f_) return true;
    void* p = asset_alloc(sizeof(FigureRecord) * kMax);
    if (!p) return false;
    f_ = static_cast<FigureRecord*>(p);
    for (int i = 0; i < kMax; ++i) new (&f_[i]) FigureRecord();
    return true;
}

void FigureBook::clear() {
    // an Ort und Stelle neu (ein Zwischenstück hätte 1,1 KB auf dem Stack; geladen wird im Haupt-Task mit 3,5 KB)
    if (f_)
        for (int i = 0; i < kMax; ++i) new (&f_[i]) FigureRecord();
    n_ = 0;
    next_ = 1;
    ++version_;
}

FigureRecord* FigureBook::add(const FigureDef& def, uint8_t place, uint16_t id) {
    if (id == 0) id = next_;
    if (n_ >= kMax || id == 0 || find(id) || !ensure()) return nullptr;
    FigureRecord& f = *new (&f_[n_++]) FigureRecord();
    f.id = id;
    if (id >= next_) next_ = static_cast<uint16_t>(id + 1);
    f.flags = def.lead ? kFigLead : 0;
    f.age = def.age;
    set_text(f.name, def.name ? def.name : "");
    set_text(f.role, def.role ? def.role : "");
    set_text(f.origin, def.origin ? def.origin : "");
    set_text(f.goal, def.goal ? def.goal : "");
    set_text(f.home, def.home ? def.home : "");
    f.chrono = def.chrono;
    for (int i = 0; i < FigureRecord::kTraits; ++i)
        if (def.traits[i]) set_text(f.traits[i], def.traits[i]);
    for (int i = 0; i < FigureRecord::kInterests; ++i)
        if (def.interests[i].key) {
            set_text(f.interests[i].key, def.interests[i].key);
            f.interests[i].weight = def.interests[i].weight;
        }
    for (const auto& s : def.skills)
        if (s.level != SkillLevel::Unknown && s.skill < Skill::kCount) f.skills[static_cast<int>(s.skill)].level = s.level;
    f.look = def.look;
    f.where.kind = place == 0xFF ? Whereabouts::Kind::Nowhere : Whereabouts::Kind::Place;
    f.where.place = place;
    ++version_;
    return &f;
}

FigureRecord* FigureBook::add_record(const FigureRecord& r) {
    if (n_ >= kMax || r.id == 0 || find(r.id) || !ensure()) return nullptr;
    f_[n_] = r;
    if (r.id >= next_) next_ = static_cast<uint16_t>(r.id + 1);
    ++version_;
    return &f_[n_++];
}

FigureRecord* FigureBook::find(uint16_t id) {
    for (int i = 0; i < n_ && id; ++i)
        if (f_[i].id == id) return &f_[i];
    return nullptr;
}

const FigureRecord* FigureBook::find(uint16_t id) const {
    for (int i = 0; i < n_ && id; ++i)
        if (f_[i].id == id) return &f_[i];
    return nullptr;
}

uint16_t FigureBook::lead() const {
    for (int i = 0; i < n_; ++i)
        if (f_[i].lead()) return f_[i].id;
    return 0;
}

void FigureBook::publish(EventKind kind, uint16_t who, uint16_t other, uint8_t a, uint8_t b, int32_t day) {
    if (!bus_) return;
    WorldEvent e;
    e.kind = kind;
    e.who = who;
    e.other = other;
    e.a = a;
    e.b = b;
    e.day = day;
    if (const FigureRecord* f = find(who)) e.lead = f->lead();
    bus_->publish(e);
}

bool FigureBook::remember(uint16_t who, const Memory& m) {
    FigureRecord* f = find(who);
    if (!f || m.kind == MemoryKind::None) return false;
    Memory* slot = nullptr;
    for (Memory& x : f->memories) {
        if (x.kind == MemoryKind::None) {
            slot = &x;
            break;
        }
        // die unwichtigste, bei gleichem Gewicht die älteste
        if (!slot || x.weight < slot->weight || (x.weight == slot->weight && earlier(x, *slot))) slot = &x;
    }
    if (slot->kind != MemoryKind::None && slot->weight > m.weight) return false;
    *slot = m;
    ++version_;
    return true;
}

void FigureBook::level_up(FigureRecord& f, Skill s, int32_t day, int minute) {
    SkillState& st = f.skills[static_cast<int>(s)];
    st.level = static_cast<SkillLevel>(static_cast<int>(st.level) + 1);
    Memory m;
    m.kind = MemoryKind::SkillUp;
    m.arg = static_cast<uint8_t>(s);
    m.b = static_cast<uint8_t>(st.level);
    m.weight = static_cast<uint8_t>(120 + 40 * static_cast<int>(st.level));
    m.day = day;
    m.minute = static_cast<uint16_t>(minute);
    remember(f.id, m);
    publish(EventKind::SkillUp, f.id, 0, static_cast<uint8_t>(s), static_cast<uint8_t>(st.level), day);
}

bool FigureBook::learn(uint16_t who, Skill s, int xp, int32_t day, int minute) {
    FigureRecord* f = find(who);
    if (!f || s >= Skill::kCount || xp <= 0) return false;
    SkillState& st = f->skills[static_cast<int>(s)];
    bool up = false;
    int total = st.xp + xp;
    while (st.level != SkillLevel::Master && total >= kSkillNeed[static_cast<int>(st.level)]) {
        total -= kSkillNeed[static_cast<int>(st.level)];
        level_up(*f, s, day, minute);
        up = true;
    }
    st.xp = st.level == SkillLevel::Master ? 0 : static_cast<uint16_t>(total);
    ++version_;
    return up;
}

SkillTry FigureBook::try_skill(uint16_t who, Skill s, int difficulty, Rng& rng, int32_t day, int minute) {
    SkillTry out;
    FigureRecord* f = find(who);
    if (!f || s >= Skill::kCount) return out;
    const SkillState& st = f->skills[static_cast<int>(s)];
    const int l = static_cast<int>(st.level);
    int chance = kChance[l];
    if (kSkillNeed[l] > 0) chance += (kChance[l + 1] - kChance[l]) * st.xp / kSkillNeed[l];
    difficulty = std::clamp(difficulty, 0, 3);
    chance -= difficulty * (st.level == SkillLevel::Master ? 30 : 100);
    chance = std::clamp(chance, 50, 990);
    const int r = static_cast<int>(rng.below(1000));
    out.outcome = r < chance ? Outcome::Success : r < chance + (1000 - chance) / 2 ? Outcome::Flawed : Outcome::Failed;
    // erstes Gelingen und erster Fehlversuch bleiben in Erinnerung (Gretas erstes Brot ist zu dunkel)
    const MemoryKind kind = out.outcome == Outcome::Success ? MemoryKind::First : MemoryKind::Failure;
    bool known = false;
    for (const Memory& m : f->memories) known = known || (m.kind == kind && m.arg == static_cast<uint8_t>(s));
    if (!known) {
        Memory m;
        m.kind = kind;
        m.arg = static_cast<uint8_t>(s);
        m.b = static_cast<uint8_t>(out.outcome);
        m.weight = kind == MemoryKind::First ? 140 : 90;
        m.day = day;
        m.minute = static_cast<uint16_t>(minute);
        remember(who, m);
    }
    // aus Fehlern lernt man auch, nur weniger
    const int xp = (out.outcome == Outcome::Success ? 10 : out.outcome == Outcome::Flawed ? 7 : 4) + difficulty;
    out.level_up = learn(who, s, xp, day, minute);
    out.level = f->skills[static_cast<int>(s)].level;
    return out;
}

bool FigureBook::teach(uint16_t teacher, uint16_t learner, Skill s, int minutes, int32_t day, int minute) {
    const FigureRecord* t = find(teacher);
    FigureRecord* l = find(learner);
    if (!t || !l || teacher == learner || s >= Skill::kCount || minutes <= 0) return false;
    const SkillLevel tl = t->level(s);
    if (tl < SkillLevel::Practiced || tl <= l->level(s)) return false;  // zeigen kann nur, wer es besser kann
    bool shown = false;
    for (const Memory& m : l->memories)
        shown = shown || (m.kind == MemoryKind::First && m.arg == static_cast<uint8_t>(s) && m.who == teacher);
    if (!shown) {
        Memory m;
        m.kind = MemoryKind::First;
        m.arg = static_cast<uint8_t>(s);
        m.who = teacher;
        m.weight = 110;
        m.day = day;
        m.minute = static_cast<uint16_t>(minute);
        remember(learner, m);
    }
    learn(learner, s, minutes * (tl == SkillLevel::Master ? 2 : 1), day, minute);
    bond(teacher, learner, std::max(1, minutes / 10), day, minute);  // gemeinsame Arbeit verbindet
    return true;
}

Relation* FigureBook::relation_slot(FigureRecord& f, uint16_t other) {
    Relation* free_slot = nullptr;
    Relation* weakest = nullptr;
    for (Relation& r : f.relations) {
        if (r.other == other) return &r;
        if (r.other == 0) {
            if (!free_slot) free_slot = &r;
            continue;
        }
        // voll: die schwächste Bekanntschaft ohne Familie weicht
        if (r.flags & kRelFamily) continue;
        if (!weakest || r.bond < weakest->bond || (r.bond == weakest->bond && r.points < weakest->points)) weakest = &r;
    }
    Relation* slot = free_slot ? free_slot : weakest;
    if (!slot) return nullptr;
    *slot = Relation{};
    slot->other = other;
    return slot;
}

void FigureBook::bond_side(FigureRecord& f, uint16_t other, int points, int32_t day, int minute, bool tell) {
    Relation* r = relation_slot(f, other);
    if (!r) return;
    int p = r->points + points;
    int b = static_cast<int>(r->bond);
    const int before = b;
    while (b < kBonds - 1 && p >= kBondNeed[b]) p -= kBondNeed[b++];
    while (p < 0 && b > 0) p += kBondNeed[--b];
    if (b == kBonds - 1) p = std::min(p, 1000);
    r->points = static_cast<uint16_t>(std::max(0, p));
    r->bond = static_cast<Bond>(b);
    if (b == before) return;
    Memory m;
    m.kind = b > before ? MemoryKind::BondUp : MemoryKind::BondDown;
    m.who = other;
    m.b = static_cast<uint8_t>(b);
    m.weight = static_cast<uint8_t>(100 + 25 * b);
    m.day = day;
    m.minute = static_cast<uint16_t>(minute);
    remember(f.id, m);
    if (tell) publish(EventKind::BondChanged, f.id, other, static_cast<uint8_t>(b), b > before, day);
}

void FigureBook::bond(uint16_t a, uint16_t b, int points, int32_t day, int minute) {
    FigureRecord* fa = find(a);
    FigureRecord* fb = find(b);
    if (!fa || !fb || a == b || points == 0) return;
    bond_side(*fa, b, points, day, minute, true);
    bond_side(*fb, a, points, day, minute, false);  // ein Ereignis je Paar
    ++version_;
}

void FigureBook::set_relation_flag(uint16_t a, uint16_t b, uint8_t flag, bool on) {
    for (auto [x, y] : {std::pair{a, b}, std::pair{b, a}}) {
        FigureRecord* f = find(x);
        if (!f || x == y) continue;
        if (Relation* r = relation_slot(*f, y)) r->flags = on ? r->flags | flag : r->flags & ~flag;
    }
    ++version_;
}

}  // namespace pixelia
