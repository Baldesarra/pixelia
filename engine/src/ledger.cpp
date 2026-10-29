// Warenbuch (seit 0.5.10): Bestände, Einzelstücke, Buchungen, Reservierungen, Reichweite und Bilanz; siehe ledger.h.

#include "pixelia/engine/ledger.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "pixelia/engine/memory.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

namespace {

constexpr int64_t kUsPerHour = 60 * WorldClock::kUsPerMinute;
constexpr uint8_t kSeenEmpty = 1, kSeenFull = 2;  // Besonderes schon gemeldet (Stock::flags)

const char* const kBookingIds[kBookingKinds] = {"herstellen", "sammeln",   "finden",      "eintauschen", "verbrauchen",
                                                "verbauen",   "verderben", "austauschen", "umlagern"};

uint64_t mix64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

int64_t floor_div(int64_t a, int64_t b) { return a >= 0 ? a / b : (a - b + 1) / b; }

}  // namespace

const char* booking_id(Booking b) {
    const int i = static_cast<int>(b);
    return i >= 0 && i < kBookingKinds ? kBookingIds[i] : "?";
}

const char* store_kind_title(StoreKind k) {
    switch (k) {
        case StoreKind::Producer: return "Erzeugerlager";
        case StoreKind::House: return "Hausvorrat";
        case StoreKind::Community: return "Gemeinschaftslager";
        case StoreKind::Site: return "Baustelle";
        case StoreKind::Hand: return "Trage";
    }
    return "?";
}

const char* good_count_text(const GoodDef& g, int n, char* out, int len) {
    const char* unit = n == 1 ? g.one : g.many;
    std::snprintf(out, len, "%d %s", n, unit ? unit : g.title);
    return out;
}

const char* stock_level_title(StockLevel l) {
    switch (l) {
        case StockLevel::Unknown: return "ohne Verbrauch";
        case StockLevel::Emergency: return "Notbestand";
        case StockLevel::Low: return "unter Soll";
        case StockLevel::Ok: return "gut";
        case StockLevel::High: return "über der Obergrenze";
    }
    return "?";
}

struct Ledger::Stock {
    uint8_t store = 0, good = 0;
    uint16_t qty = 0, reserved = 0;
    uint16_t in_day = 0, out_day = 0;  // laufender Welttag (Storage::today)
    uint8_t flags = 0;                 // kSeenEmpty, kSeenFull
    uint16_t out[kHours] = {};         // Abgänge je Weltstunde (Spalte = Stunde % 24)
    uint16_t key() const { return static_cast<uint16_t>(store << 8 | good); }
};

struct Ledger::Storage {
    const GoodDef* goods[kMaxGoods] = {};
    int good_n = 0;
    struct Slot {
        const StoreDef* def = nullptr;  // nullptr = Trage
        uint16_t figure = 0;
        int16_t place = -1;
        uint32_t version = 0;
    };
    Slot stores[kMaxStores];
    int store_n = 0;
    uint8_t store_seen[kMaxStores] = {};  // Lagerplatz voll gemeldet (kSeenFull)
    Stock stock[kMaxStock];  // sortiert nach Lagerplatz, Ware
    int stock_n = 0;
    Item items[kMaxItems];
    int item_n = 0;
    struct Res {
        uint16_t id = 0, holder = 0;
        uint8_t store = 0, good = 0;
        uint16_t qty = 0;
    };
    Res res[kMaxReservations];
    int res_n = 0;
    uint16_t next_res = 1;
    GoodTotals totals[kMaxGoods];
    int32_t sum[kMaxGoods] = {};          // Bestand je Ware im ganzen Dorf
    int32_t adjust[kMaxGoods] = {};       // Laden: Korrektur des Bilanz-Anfangs (verlorene Stück)
    uint16_t use[kMaxGoods][kHours] = {};  // Verbrauch je Weltstunde
    uint8_t level_seen[kMaxGoods] = {};    // Notbestand gemeldet
    int64_t hour = INT64_MIN;              // jüngste Stunde der Fächer
    int32_t today = INT32_MIN;             // Welttag von in_day/out_day
    const EconomyDef* defs[8] = {};
    int def_n = 0;
    uint16_t order[kMaxStock] = {};        // Arbeitsfeld zum Sortieren (Speicherstand)
};

Ledger::~Ledger() {
    if (s_) {
        s_->~Storage();
        asset_free(s_);
    }
}

bool Ledger::ensure() {
    if (s_) return true;
    void* p = asset_alloc(sizeof(Storage));
    if (!p) return false;
    s_ = new (p) Storage();
    return true;
}

// --- Aufbau ---

void Ledger::build() {
    const AreaDef* const* areas = nullptr;
    const int na = world_areas(&areas);
    const EconomyDef* defs[8];
    int n = 0;
    for (int a = 0; a < na && n < 8; ++a)
        if (areas[a]->economy) defs[n++] = areas[a]->economy;
    build(defs, n);
}

void Ledger::build(const EconomyDef* const* defs, int n) {
    if (!ensure()) return;
    s_->~Storage();
    new (s_) Storage();
    Storage& s = *s_;
    for (int k = 0; k < n && k < 8; ++k) {
        const EconomyDef& d = *defs[k];
        s.defs[s.def_n++] = defs[k];
        for (int g = 0; g < d.good_count && s.good_n < kMaxGoods; ++g)
            if (find_good(d.goods[g].id) < 0) s.goods[s.good_n++] = &d.goods[g];
        for (int i = 0; i < d.store_count && s.store_n < kMaxStores; ++i) {
            if (find_store(d.stores[i].id) >= 0) continue;
            Storage::Slot& slot = s.stores[s.store_n++];
            slot.def = &d.stores[i];
            slot.place = static_cast<int16_t>(place_index(d.stores[i].place));
        }
    }
    ++version_;
}

void Ledger::clear() {
    if (!s_) return;
    Storage& s = *s_;
    // Tragen weg (sie entstehen beim Laden bzw. beim ersten Tragen neu), Lagerplätze der Daten bleiben
    int keep = 0;
    while (keep < s.store_n && s.stores[keep].def) ++keep;
    s.store_n = keep;
    for (int i = 0; i < s.store_n; ++i) ++s.stores[i].version;
    s.stock_n = s.item_n = s.res_n = 0;
    s.next_res = 1;
    for (uint8_t& v : s.store_seen) v = 0;
    for (int g = 0; g < kMaxGoods; ++g) {
        s.totals[g] = GoodTotals{};
        s.sum[g] = 0;
        s.adjust[g] = 0;
        s.level_seen[g] = 0;
        for (uint16_t& u : s.use[g]) u = 0;
    }
    s.hour = INT64_MIN;
    s.today = INT32_MIN;
    ++version_;
}

void Ledger::start_stock(int32_t day) {
    if (!s_) return;
    for (int k = 0; k < s_->def_n; ++k) {
        const EconomyDef& d = *s_->defs[k];
        for (int i = 0; i < d.start_count; ++i) {
            const StartStock& st = d.start[i];
            const int store = find_store(st.store), g = find_good(st.good);
            if (store < 0 || g < 0) continue;
            // als Anfang der Bilanz, nicht als Zugang: Zähler danach zurück auf den Anfang
            BookingInfo info;
            info.reason = "startvorrat";
            int got = 0;
            if (good(g).unique) {
                for (int q = 0; q < st.qty; ++q) {
                    info.at_us = q;  // eigene Kennung je Stück
                    got += add_item(store, g, 0, st.condition, Booking::Find, info) != 0;
                }
                for (int q = 0; q < s_->item_n; ++q)
                    if (s_->items[q].good == g && s_->items[q].store == store) s_->items[q].day = day;
            } else {
                got = add(store, g, st.qty, Booking::Find, info);
            }
            GoodTotals& t = s_->totals[g];
            t.moved[static_cast<int>(Booking::Find)] -= got;
            t.start += got;
        }
    }
}

// --- Waren und Lagerplätze ---

int Ledger::goods() const { return s_ ? s_->good_n : 0; }
const GoodDef& Ledger::good(int g) const { return *s_->goods[g]; }

int Ledger::find_good(std::string_view id) const {
    if (!s_) return -1;
    for (int g = 0; g < s_->good_n; ++g)
        if (id == s_->goods[g]->id) return g;
    return -1;
}

int Ledger::stores() const { return s_ ? s_->store_n : 0; }
const StoreDef* Ledger::store_def(int s) const { return s_ && s >= 0 && s < s_->store_n ? s_->stores[s].def : nullptr; }

StoreKind Ledger::store_kind(int s) const {
    const StoreDef* d = store_def(s);
    return d ? d->kind : StoreKind::Hand;
}

const char* Ledger::store_key(int s, char* out, int n) const {
    if (const StoreDef* d = store_def(s)) return d->id;
    std::snprintf(out, n, "@%05u", static_cast<unsigned>(hand_owner(s)));
    return out;
}

const char* Ledger::store_title(int s) const {
    const StoreDef* d = store_def(s);
    return d ? d->title : "Trage";
}

int Ledger::find_store(std::string_view key) const {
    if (!s_) return -1;
    if (!key.empty() && key[0] == '@') {
        unsigned v = 0;
        for (size_t i = 1; i < key.size(); ++i) {
            if (key[i] < '0' || key[i] > '9') return -1;
            v = v * 10 + static_cast<unsigned>(key[i] - '0');
        }
        return v > 0 && v <= 0xFFFF ? hand(static_cast<uint16_t>(v)) : -1;
    }
    for (int i = 0; i < s_->store_n; ++i)
        if (s_->stores[i].def && key == s_->stores[i].def->id) return i;
    return -1;
}

int Ledger::store_place(int s) const { return s_ && s >= 0 && s < s_->store_n ? s_->stores[s].place : -1; }
uint16_t Ledger::hand_owner(int s) const {
    return s_ && s >= 0 && s < s_->store_n && !s_->stores[s].def ? s_->stores[s].figure : 0;
}

int Ledger::hand(uint16_t figure) const {
    if (!s_ || !figure) return -1;
    for (int i = 0; i < s_->store_n; ++i)
        if (!s_->stores[i].def && s_->stores[i].figure == figure) return i;
    return -1;
}

int Ledger::make_hand(uint16_t figure) {
    if (const int h = hand(figure); h >= 0 || !s_ || !figure) return h;
    if (s_->store_n >= kMaxStores) return -1;
    Storage::Slot& slot = s_->stores[s_->store_n];
    slot = Storage::Slot{};
    slot.figure = figure;
    return s_->store_n++;
}

// --- Bestand ---

Ledger::Stock* Ledger::entry(int store, int good, bool create) {
    if (!s_ || store < 0 || store >= s_->store_n || good < 0 || good >= s_->good_n) return nullptr;
    Storage& s = *s_;
    const uint16_t key = static_cast<uint16_t>(store << 8 | good);
    int lo = 0, hi = s.stock_n;
    while (lo < hi) {
        const int mid = (lo + hi) / 2;
        if (s.stock[mid].key() < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < s.stock_n && s.stock[lo].key() == key) return &s.stock[lo];
    if (!create || s.stock_n >= kMaxStock) return nullptr;
    for (int i = s.stock_n; i > lo; --i) s.stock[i] = s.stock[i - 1];
    s.stock[lo] = Stock{};
    s.stock[lo].store = static_cast<uint8_t>(store);
    s.stock[lo].good = static_cast<uint8_t>(good);
    ++s.stock_n;
    return &s.stock[lo];
}

const Ledger::Stock* Ledger::entry(int store, int good) const {
    return const_cast<Ledger*>(this)->entry(store, good, false);
}

int Ledger::qty(int store, int good) const {
    const Stock* e = entry(store, good);
    return e ? e->qty : 0;
}

int Ledger::reserved(int store, int good) const {
    const Stock* e = entry(store, good);
    return e ? e->reserved : 0;
}

int Ledger::weight_of(int store) const {
    int w = 0;
    for (int i = 0; i < s_->stock_n; ++i) {
        const Stock& e = s_->stock[i];
        if (e.store == store) w += e.qty * s_->goods[e.good]->weight;
    }
    return w;
}

int Ledger::load(int store) const {
    if (!s_ || store < 0 || store >= s_->store_n) return 0;
    if (!s_->stores[store].def) return weight_of(store);
    int n = 0;
    for (int i = 0; i < s_->stock_n; ++i)
        if (s_->stock[i].store == store) n += s_->stock[i].qty;
    return n;
}

int Ledger::capacity(int store) const {
    if (!s_ || store < 0 || store >= s_->store_n) return 0;
    const StoreDef* d = s_->stores[store].def;
    return d ? d->capacity : kHandCapacity;
}

int Ledger::room(int store, int good) const {
    if (!s_ || store < 0 || store >= s_->store_n || good < 0 || good >= s_->good_n) return 0;
    const int left = std::max(0, capacity(store) - load(store));
    if (s_->stores[store].def) return left;
    const int w = std::max<int>(1, s_->goods[good]->weight);
    return left / w;
}

int Ledger::total(int good) const { return s_ && good >= 0 && good < s_->good_n ? s_->sum[good] : 0; }

int Ledger::contents(int store, uint8_t* goods, uint16_t* qtys, int max) const {
    if (!s_) return 0;
    int n = 0;
    for (int i = 0; i < s_->stock_n && n < max; ++i) {
        const Stock& e = s_->stock[i];
        if (e.store != store || e.qty == 0) continue;
        goods[n] = e.good;
        qtys[n] = e.qty;
        ++n;
    }
    return n;
}

// --- Buchungen ---

int64_t Ledger::hour_of(int64_t at_us) const { return floor_div(at_us, kUsPerHour); }

void Ledger::bucket(Stock& e, int64_t at_us, int n) {
    if (n <= 0) return;
    const int64_t h = hour_of(at_us);
    if (s_->hour == INT64_MIN || h > s_->hour) roll(at_us);
    if (h <= s_->hour - kHours) return;  // zu alt für die Fächer
    uint16_t& b = e.out[static_cast<int>((h % kHours + kHours) % kHours)];
    b = static_cast<uint16_t>(std::min(0xFFFF, b + n));
}

void Ledger::used(int good, int64_t at_us, int n) {
    if (n <= 0) return;
    const int64_t h = hour_of(at_us);
    if (s_->hour == INT64_MIN || h > s_->hour) roll(at_us);
    if (h <= s_->hour - kHours) return;
    uint16_t& b = s_->use[good][static_cast<int>((h % kHours + kHours) % kHours)];
    b = static_cast<uint16_t>(std::min(0xFFFF, b + n));
}

void Ledger::touch(int store) {
    ++version_;
    if (s_ && store >= 0 && store < s_->store_n) ++s_->stores[store].version;
}

uint32_t Ledger::store_version(int store) const {
    return s_ && store >= 0 && store < s_->store_n ? s_->stores[store].version : 0;
}

void Ledger::notify(Booking kind, int good, int from, int to, int n, uint32_t item, const BookingInfo& info) {
    if (!on_booking_) return;
    BookingRecord r;
    r.info = info;
    r.kind = kind;
    r.good = static_cast<uint8_t>(good);
    r.from = static_cast<int16_t>(from);
    r.to = static_cast<int16_t>(to);
    r.qty = static_cast<uint16_t>(n);
    r.item = item;
    on_booking_(listen_ctx_, r);
}

int Ledger::add(int store, int good, int n, Booking kind, const BookingInfo& info) {
    if (!s_ || n <= 0 || !booking_in(kind)) return 0;
    if (good >= 0 && good < s_->good_n && s_->goods[good]->unique) {
        int got = 0;
        BookingInfo i2 = info;
        for (int k = 0; k < n; ++k) {
            got += add_item(store, good, 0, 1000, kind, i2) != 0;
            ++i2.at_us;  // eigene Kennung je Stück
        }
        return got;
    }
    n = std::min(n, room(store, good));
    Stock* e = n > 0 ? entry(store, good, true) : nullptr;
    if (!e) return 0;
    n = std::min(n, 0xFFFF - e->qty);
    e->qty = static_cast<uint16_t>(e->qty + n);
    e->in_day = static_cast<uint16_t>(std::min(0xFFFF, e->in_day + n));
    s_->sum[good] += n;
    s_->totals[good].moved[static_cast<int>(kind)] += n;
    touch(store);
    notify(kind, good, -1, store, n, 0, info);
    return n;
}

int Ledger::take(int store, int good, int n, Booking kind, const BookingInfo& info, uint16_t res) {
    if (!s_ || n <= 0 || !booking_out(kind)) return 0;
    Stock* e = entry(store, good, false);
    if (!e) return 0;
    Storage::Res* r = nullptr;
    if (res)
        for (int i = 0; i < s_->res_n; ++i)
            if (s_->res[i].id == res && s_->res[i].store == store && s_->res[i].good == good) r = &s_->res[i];
    const int free = e->qty - e->reserved;
    n = std::min(n, free + (r ? r->qty : 0));
    if (n <= 0) return 0;
    if (s_->goods[good]->unique) {
        // Einzelstücke: die mit der kleinsten Kennung (unabhängig von der Reihenfolge im Speicher)
        int got = 0;
        for (int k = 0; k < n; ++k) {
            uint32_t best = 0;
            for (int i = 0; i < s_->item_n; ++i) {
                const Item& it = s_->items[i];
                if (it.store == store && it.good == good && (!best || it.id < best)) best = it.id;
            }
            if (!best) break;
            // Reservierung zuerst verbrauchen
            if (r && r->qty > 0) {
                --r->qty;
                --e->reserved;
            }
            got += take_item(best, kind, info);
        }
        return got;
    }
    const int from_res = r ? std::min<int>(n, r->qty) : 0;
    if (r) {
        r->qty = static_cast<uint16_t>(r->qty - from_res);
        e->reserved = static_cast<uint16_t>(e->reserved - from_res);
    }
    e->qty = static_cast<uint16_t>(e->qty - n);
    e->out_day = static_cast<uint16_t>(std::min(0xFFFF, e->out_day + n));
    bucket(*e, info.at_us, n);
    s_->sum[good] -= n;
    s_->totals[good].moved[static_cast<int>(kind)] += n;
    used(good, info.at_us, n);
    touch(store);
    notify(kind, good, store, -1, n, 0, info);
    return n;
}

int Ledger::move(int from, int to, int good, int n, const BookingInfo& info, uint16_t res) {
    if (!s_ || n <= 0 || from == to) return 0;
    Stock* e = entry(from, good, false);
    if (!e || to < 0 || to >= s_->store_n) return 0;
    Storage::Res* r = nullptr;
    if (res)
        for (int i = 0; i < s_->res_n; ++i)
            if (s_->res[i].id == res && s_->res[i].store == from && s_->res[i].good == good) r = &s_->res[i];
    n = std::min(n, e->qty - e->reserved + (r ? r->qty : 0));
    n = std::min(n, room(to, good));
    if (n <= 0) return 0;
    if (s_->goods[good]->unique) {
        int got = 0;
        for (int k = 0; k < n; ++k) {
            uint32_t best = 0;
            for (int i = 0; i < s_->item_n; ++i) {
                const Item& it = s_->items[i];
                if (it.store == from && it.good == good && (!best || it.id < best)) best = it.id;
            }
            if (!best) break;
            Stock* src = entry(from, good, false);
            if (r && r->qty > 0 && src) {
                --r->qty;
                --src->reserved;
            }
            got += move_item(best, to, info);
        }
        return got;
    }
    Stock* d = entry(to, good, true);
    if (!d) return 0;
    e = entry(from, good, false);  // nach dem Einfügen kann sich die Lage verschoben haben
    const int from_res = r ? std::min<int>(n, r->qty) : 0;
    if (r) {
        r->qty = static_cast<uint16_t>(r->qty - from_res);
        e->reserved = static_cast<uint16_t>(e->reserved - from_res);
    }
    n = std::min(n, 0xFFFF - d->qty);
    e->qty = static_cast<uint16_t>(e->qty - n);
    e->out_day = static_cast<uint16_t>(std::min(0xFFFF, e->out_day + n));
    bucket(*e, info.at_us, n);
    d->qty = static_cast<uint16_t>(d->qty + n);
    d->in_day = static_cast<uint16_t>(std::min(0xFFFF, d->in_day + n));
    s_->totals[good].moved[static_cast<int>(Booking::Move)] += n;
    touch(from);
    touch(to);
    notify(Booking::Move, good, from, to, n, 0, info);
    return n;
}

void Ledger::demand(int good, int store, int want, int got, const BookingInfo& info) {
    if (!s_ || good < 0 || good >= s_->good_n || want <= 0) return;
    GoodTotals& t = s_->totals[good];
    t.want += want;
    t.got += std::clamp(got, 0, want);
    if (!on_demand_) return;
    DemandRecord r;
    r.at_us = info.at_us;
    r.good = static_cast<uint8_t>(good);
    r.store = static_cast<int16_t>(store);
    r.want = static_cast<uint16_t>(want);
    r.got = static_cast<uint16_t>(std::clamp(got, 0, want));
    r.who = info.who;
    r.reason = info.reason;
    on_demand_(listen_ctx_, r);
}

// --- Einzelstücke ---

uint32_t Ledger::add_item(int store, int good, uint16_t owner, uint16_t condition, Booking kind,
                          const BookingInfo& info) {
    if (!s_ || !booking_in(kind) || good < 0 || good >= s_->good_n || !s_->goods[good]->unique) return 0;
    if (s_->item_n >= kMaxItems || room(store, good) < 1) return 0;
    Stock* e = entry(store, good, true);
    if (!e || e->qty == 0xFFFF) return 0;
    // Kennung aus Zeit, Figur, Ware und Lagerplatz: hängt nicht davon ab, wer vorher etwas anlegte
    uint32_t id = static_cast<uint32_t>(mix64(static_cast<uint64_t>(info.at_us) ^ (uint64_t{info.who} << 48) ^
                                              (uint64_t{static_cast<uint8_t>(good)} << 40) ^
                                              (uint64_t{static_cast<uint8_t>(store)} << 32) ^ 0x57A11E5ULL));
    for (;;) {
        if (id == 0) id = 1;
        if (!item(id)) break;
        ++id;
    }
    Item& it = s_->items[s_->item_n++];
    it = Item{};
    it.id = id;
    it.good = static_cast<uint8_t>(good);
    it.store = static_cast<uint8_t>(store);
    it.owner = owner;
    it.condition = static_cast<uint16_t>(std::min<int>(condition, 1000));
    it.maker = info.who;
    it.day = static_cast<int32_t>(s_->today == INT32_MIN ? 0 : s_->today);
    ++e->qty;
    e->in_day = static_cast<uint16_t>(std::min(0xFFFF, e->in_day + 1));
    ++s_->sum[good];
    ++s_->totals[good].moved[static_cast<int>(kind)];
    touch(store);
    notify(kind, good, -1, store, 1, id, info);
    return id;
}

bool Ledger::move_item(uint32_t id, int to, const BookingInfo& info) {
    if (!s_ || to < 0 || to >= s_->store_n) return false;
    int k = -1;
    for (int i = 0; i < s_->item_n; ++i)
        if (s_->items[i].id == id) k = i;
    if (k < 0) return false;
    const int from = s_->items[k].store, good = s_->items[k].good;
    if (from == to || room(to, good) < 1) return false;
    Stock* d = entry(to, good, true);
    Stock* e = entry(from, good, false);
    if (!d || !e || e->qty <= e->reserved) return false;  // reservierte Stücke nur über die Reservierung (move)
    --e->qty;
    e->out_day = static_cast<uint16_t>(std::min(0xFFFF, e->out_day + 1));
    bucket(*e, info.at_us, 1);
    ++d->qty;
    d->in_day = static_cast<uint16_t>(std::min(0xFFFF, d->in_day + 1));
    s_->items[k].store = static_cast<uint8_t>(to);
    ++s_->totals[good].moved[static_cast<int>(Booking::Move)];
    touch(from);
    touch(to);
    notify(Booking::Move, good, from, to, 1, id, info);
    return true;
}

bool Ledger::take_item(uint32_t id, Booking kind, const BookingInfo& info) {
    if (!s_ || !booking_out(kind)) return false;
    int k = -1;
    for (int i = 0; i < s_->item_n; ++i)
        if (s_->items[i].id == id) k = i;
    if (k < 0) return false;
    const int store = s_->items[k].store, good = s_->items[k].good;
    Stock* e = entry(store, good, false);
    if (!e || e->qty <= e->reserved) return false;  // reservierte Stücke nur über die Reservierung (take)
    --e->qty;
    e->out_day = static_cast<uint16_t>(std::min(0xFFFF, e->out_day + 1));
    bucket(*e, info.at_us, 1);
    s_->items[k] = s_->items[--s_->item_n];
    --s_->sum[good];
    ++s_->totals[good].moved[static_cast<int>(kind)];
    used(good, info.at_us, 1);
    touch(store);
    notify(kind, good, store, -1, 1, id, info);
    return true;
}

const Item* Ledger::item(uint32_t id) const {
    if (!s_ || !id) return nullptr;
    for (int i = 0; i < s_->item_n; ++i)
        if (s_->items[i].id == id) return &s_->items[i];
    return nullptr;
}

int Ledger::items() const { return s_ ? s_->item_n : 0; }
const Item& Ledger::item_at(int i) const { return s_->items[i]; }

bool Ledger::set_condition(uint32_t id, int condition) {
    Item* it = const_cast<Item*>(item(id));
    if (!it) return false;
    it->condition = static_cast<uint16_t>(std::clamp(condition, 0, 1000));
    touch(it->store);
    return true;
}

bool Ledger::set_owner(uint32_t id, uint16_t owner) {
    Item* it = const_cast<Item*>(item(id));
    if (!it) return false;
    it->owner = owner;
    touch(it->store);
    return true;
}

// --- Reservierung ---

uint16_t Ledger::reserve(int store, int good, int n, uint16_t holder) {
    if (!s_ || n <= 0 || s_->res_n >= kMaxReservations) return 0;
    Stock* e = entry(store, good, false);
    if (!e) return 0;
    n = std::min(n, e->qty - e->reserved);
    if (n <= 0) return 0;
    Storage::Res& r = s_->res[s_->res_n++];
    r.id = s_->next_res;
    s_->next_res = static_cast<uint16_t>(s_->next_res == 0xFFFF ? 1 : s_->next_res + 1);
    r.holder = holder;
    r.store = static_cast<uint8_t>(store);
    r.good = static_cast<uint8_t>(good);
    r.qty = static_cast<uint16_t>(n);
    e->reserved = static_cast<uint16_t>(e->reserved + n);
    touch(store);
    return r.id;
}

int Ledger::reservation_left(uint16_t res) const {
    if (!s_ || !res) return 0;
    for (int i = 0; i < s_->res_n; ++i)
        if (s_->res[i].id == res) return s_->res[i].qty;
    return 0;
}

void Ledger::release(uint16_t res) {
    if (!s_ || !res) return;
    for (int i = 0; i < s_->res_n; ++i) {
        if (s_->res[i].id != res) continue;
        const Storage::Res r = s_->res[i];
        if (Stock* e = entry(r.store, r.good, false))
            e->reserved = static_cast<uint16_t>(e->reserved - std::min(e->reserved, r.qty));
        for (int k = i + 1; k < s_->res_n; ++k) s_->res[k - 1] = s_->res[k];  // Reihenfolge bleibt (Kennungen steigen)
        --s_->res_n;
        touch(r.store);
        return;
    }
}

void Ledger::release_holder(uint16_t holder) {
    if (!s_) return;
    for (int i = s_->res_n - 1; i >= 0; --i)
        if (s_->res[i].holder == holder) release(s_->res[i].id);
}

int Ledger::reservations() const { return s_ ? s_->res_n : 0; }

int Ledger::holders(uint16_t* out, int max) const {
    if (!s_) return 0;
    int n = 0;
    for (int i = 0; i < s_->res_n; ++i) {
        const uint16_t h = s_->res[i].holder;
        int at = 0;
        while (at < n && out[at] < h) ++at;
        if ((at < n && out[at] == h) || n >= max) continue;
        for (int k = n; k > at; --k) out[k] = out[k - 1];
        out[at] = h;
        ++n;
    }
    return n;
}

// --- Reichweite ---

void Ledger::roll(int64_t at_us) {
    if (!s_) return;
    Storage& s = *s_;
    const int64_t h = hour_of(at_us);
    if (s.hour != INT64_MIN && h <= s.hour) return;
    // Spalten der übersprungenen Stunden leeren (höchstens alle 24)
    const int64_t from = s.hour == INT64_MIN ? h - kHours + 1 : s.hour + 1;
    for (int64_t k = std::max(from, h - kHours + 1); k <= h; ++k) {
        const int c = static_cast<int>((k % kHours + kHours) % kHours);
        for (int i = 0; i < s.stock_n; ++i) s.stock[i].out[c] = 0;
        for (int g = 0; g < s.good_n; ++g) s.use[g][c] = 0;
    }
    s.hour = h;
}

int Ledger::out_24h(int store, int good) const {
    const Stock* e = entry(store, good);
    if (!e) return 0;
    int n = 0;
    for (uint16_t v : e->out) n += v;
    return n;
}

int Ledger::use_24h(int good) const {
    if (!s_ || good < 0 || good >= s_->good_n) return 0;
    int n = 0;
    for (uint16_t v : s_->use[good]) n += v;
    return n;
}

int Ledger::in_today(int store, int good) const {
    const Stock* e = entry(store, good);
    return e ? e->in_day : 0;
}

int Ledger::out_today(int store, int good) const {
    const Stock* e = entry(store, good);
    return e ? e->out_day : 0;
}

int Ledger::reach_hours(int store, int good) const {
    const int out = out_24h(store, good);
    return out > 0 ? static_cast<int>(int64_t{qty(store, good)} * 24 / out) : -1;
}

int Ledger::reach_hours(int good) const {
    const int use = use_24h(good);
    return use > 0 ? static_cast<int>(int64_t{total(good)} * 24 / use) : -1;
}

StockLevel Ledger::level(int good) const {
    if (!s_ || good < 0 || good >= s_->good_n) return StockLevel::Unknown;
    const int h = reach_hours(good);
    if (h < 0) return StockLevel::Unknown;
    const GoodDef& d = *s_->goods[good];
    if (h < 24) return StockLevel::Emergency;
    if (d.soll_days && h < d.soll_days * 24) return StockLevel::Low;
    if (d.max_days && h > d.max_days * 24) return StockLevel::High;
    return StockLevel::Ok;
}

void Ledger::step(int64_t at_us, int32_t day, EventBus* bus) {
    if (!s_) return;
    Storage& s = *s_;
    roll(at_us);
    if (day != s.today) {
        if (s.today != INT32_MIN)
            for (int i = 0; i < s.stock_n; ++i) s.stock[i].in_day = s.stock[i].out_day = 0;
        s.today = day;
    }
    // Besonderes für die Chronik: Lagerplatz leer (wenn er etwas vorrätig halten will) bzw. voll (Erzeugerlager),
    // Notbestand einer Ware im Dorf; je einmal, bis es vorbei ist
    for (int i = 0; i < s.stock_n; ++i) {
        Stock& e = s.stock[i];
        const StoreDef* d = s.stores[e.store].def;
        if (!d || d->kind == StoreKind::Site) continue;
        bool wants = false;
        for (int r = 0; r < d->rule_count; ++r) wants = wants || find_good(d->rules[r].good) == e.good;
        WorldEvent ev;
        ev.kind = EventKind::Stock;
        ev.day = day;
        ev.place = d->place;
        if (wants && e.qty == 0 && !(e.flags & kSeenEmpty)) {
            e.flags |= kSeenEmpty;
            ev.a = 0;
            std::snprintf(ev.text, sizeof(ev.text), "%.18s leer: kein %.24s", d->title, s.goods[e.good]->title);
            if (bus) bus->publish(ev);
        } else if (e.qty > 0) {
            e.flags &= static_cast<uint8_t>(~kSeenEmpty);
        }
    }
    for (int st = 0; st < s.store_n; ++st) {
        const StoreDef* d = s.stores[st].def;
        if (!d || d->kind != StoreKind::Producer || d->capacity == 0) continue;
        const int now = load(st);
        if (now >= d->capacity && !(s.store_seen[st] & kSeenFull)) {
            s.store_seen[st] |= kSeenFull;
            WorldEvent ev;
            ev.kind = EventKind::Stock;
            ev.a = 1;
            ev.day = day;
            ev.place = d->place;
            std::snprintf(ev.text, sizeof(ev.text), "%.30s ist voll", d->title);
            if (bus) bus->publish(ev);
        } else if (now * 10 < d->capacity * 9) {
            s.store_seen[st] &= static_cast<uint8_t>(~kSeenFull);
        }
    }
    for (int g = 0; g < s.good_n; ++g) {
        const StockLevel l = level(g);
        if (l == StockLevel::Emergency && !s.level_seen[g]) {
            s.level_seen[g] = 1;
            WorldEvent ev;
            ev.kind = EventKind::Stock;
            ev.a = 2;
            ev.day = day;
            std::snprintf(ev.text, sizeof(ev.text), "Notbestand: %.24s reicht keinen Tag", s.goods[g]->title);
            if (bus) bus->publish(ev);
        } else if (l == StockLevel::Ok || l == StockLevel::High) {
            s.level_seen[g] = 0;
        }
    }
}

// --- Bilanz ---

const Ledger::GoodTotals& Ledger::totals(int good) const { return s_->totals[good]; }

const char* Ledger::check(char* buf, int n) const {
    if (!s_) return nullptr;
    const Storage& s = *s_;
    for (int g = 0; g < s.good_n; ++g) {
        const GoodTotals& t = s.totals[g];
        int64_t expect = t.start;
        for (int k = 0; k < kBookingKinds; ++k) {
            if (booking_in(static_cast<Booking>(k))) expect += t.moved[k];
            if (booking_out(static_cast<Booking>(k))) expect -= t.moved[k];
        }
        int64_t sum = 0;
        for (int i = 0; i < s.stock_n; ++i)
            if (s.stock[i].good == g) sum += s.stock[i].qty;
        if (sum != expect || sum != s.sum[g]) {
            std::snprintf(buf, n, "Bilanz %s: Bestände %lld, Buchungen ergeben %lld (Summe %d)", s.goods[g]->id,
                          static_cast<long long>(sum), static_cast<long long>(expect), static_cast<int>(s.sum[g]));
            return buf;
        }
    }
    char key[12];
    for (int i = 0; i < s.stock_n; ++i) {
        const Stock& e = s.stock[i];
        if (i > 0 && s.stock[i - 1].key() >= e.key()) {
            std::snprintf(buf, n, "Bestände nicht sortiert bei %d", i);
            return buf;
        }
        if (e.reserved > e.qty) {
            std::snprintf(buf, n, "%s/%s: %d reserviert, aber nur %d da", store_key(e.store, key, sizeof(key)),
                          s.goods[e.good]->id, e.reserved, e.qty);
            return buf;
        }
        int res = 0;
        for (int r = 0; r < s.res_n; ++r)
            if (s.res[r].store == e.store && s.res[r].good == e.good) res += s.res[r].qty;
        if (res != e.reserved) {
            std::snprintf(buf, n, "%s/%s: Reservierungen %d, im Bestand %d", store_key(e.store, key, sizeof(key)),
                          s.goods[e.good]->id, res, e.reserved);
            return buf;
        }
        if (s.goods[e.good]->unique) {
            int items = 0;
            for (int k = 0; k < s.item_n; ++k) items += s.items[k].store == e.store && s.items[k].good == e.good;
            if (items != e.qty) {
                std::snprintf(buf, n, "%s/%s: %d Einzelstücke, Bestand %d", store_key(e.store, key, sizeof(key)),
                              s.goods[e.good]->id, items, e.qty);
                return buf;
            }
        }
    }
    for (int st = 0; st < s.store_n; ++st)
        if (load(st) > capacity(st)) {
            std::snprintf(buf, n, "%s über der Obergrenze: %d von %d", store_key(st, key, sizeof(key)), load(st),
                          capacity(st));
            return buf;
        }
    return nullptr;
}

// --- Speicherstand ---

namespace {

// Spalten mit Werten: Alter in Stunden (0 = jüngste) und Menge
void write_hours(SaveWriter& w, const uint16_t* v, int64_t hour) {
    int n = 0;
    for (int c = 0; c < Ledger::kHours; ++c) n += v[c] != 0;
    w.u8(static_cast<uint8_t>(n));
    if (!n) return;
    for (int age = 0; age < Ledger::kHours; ++age) {
        const int c = static_cast<int>(((hour - age) % Ledger::kHours + Ledger::kHours) % Ledger::kHours);
        if (!v[c]) continue;
        w.u8(static_cast<uint8_t>(age));
        w.u16(v[c]);
    }
}

void read_hours(SaveReader& r, uint16_t* v, int64_t hour) {
    const int n = r.u8();
    for (int k = 0; k < n; ++k) {
        const int age = r.u8();
        const uint16_t x = r.u16();
        if (age >= Ledger::kHours || hour == INT64_MIN) continue;
        v[static_cast<int>(((hour - age) % Ledger::kHours + Ledger::kHours) % Ledger::kHours)] = x;
    }
}

}  // namespace

void Ledger::save_goods(SaveWriter& w) const {
    if (!s_) return;
    const Storage& s = *s_;
    w.i64(s.hour);
    w.i32(s.today);
    // nach Kennung sortiert: unabhängig von der Reihenfolge der Daten
    int n = 0;
    for (int g = 0; g < s.good_n; ++g) s_->order[n++] = static_cast<uint16_t>(g);
    std::sort(s_->order, s_->order + n,
              [&](uint16_t a, uint16_t b) { return std::strcmp(s.goods[a]->id, s.goods[b]->id) < 0; });
    w.u16(static_cast<uint16_t>(n));
    for (int k = 0; k < n; ++k) {
        const int g = s.order[k];
        const GoodTotals& t = s.totals[g];
        const size_t r = w.record_begin();
        w.str(s.goods[g]->id);
        w.i64(t.start);
        w.u8(kBookingKinds);
        for (int64_t v : t.moved) w.i64(v);
        w.i64(t.want);
        w.i64(t.got);
        w.u8(s.level_seen[g]);
        write_hours(w, s.use[g], s.hour);
        w.record_end(r);
    }
    // gemeldetes Besonderes je Lagerplatz (voll), nur die Lagerplätze der Daten
    int m = 0;
    for (int st = 0; st < s.store_n; ++st)
        if (s.stores[st].def && s.store_seen[st]) s_->order[m++] = static_cast<uint16_t>(st);
    std::sort(s_->order, s_->order + m, [&](uint16_t a, uint16_t b) {
        return std::strcmp(s.stores[a].def->id, s.stores[b].def->id) < 0;
    });
    w.u16(static_cast<uint16_t>(m));
    for (int k = 0; k < m; ++k) {
        w.str(s.stores[s.order[k]].def->id);
        w.u8(s.store_seen[s.order[k]]);
    }
}

void Ledger::save_stock(SaveWriter& w) const {
    if (!s_) return;
    const Storage& s = *s_;
    // Reihenfolge: Kennung des Lagerplatzes, dann der Ware (Tragen „@00012“ nach Figur)
    char ka[12], kb[12];
    int n = 0;
    for (int i = 0; i < s.stock_n; ++i) {
        const Stock& e = s.stock[i];
        bool any = e.qty || e.in_day || e.out_day || e.flags;
        for (uint16_t v : e.out) any = any || v;
        if (any) s_->order[n++] = static_cast<uint16_t>(i);
    }
    std::sort(s_->order, s_->order + n, [&](uint16_t a, uint16_t b) {
        const Stock &x = s.stock[a], &y = s.stock[b];
        const int c = std::strcmp(store_key(x.store, ka, sizeof(ka)), store_key(y.store, kb, sizeof(kb)));
        return c != 0 ? c < 0 : std::strcmp(s.goods[x.good]->id, s.goods[y.good]->id) < 0;
    });
    w.u16(static_cast<uint16_t>(n));
    for (int k = 0; k < n; ++k) {
        const Stock& e = s.stock[s.order[k]];
        const size_t r = w.record_begin();
        w.str(store_key(e.store, ka, sizeof(ka)));
        w.str(s.goods[e.good]->id);
        w.u16(e.qty);
        w.u16(e.in_day);
        w.u16(e.out_day);
        w.u8(e.flags);
        write_hours(w, e.out, s.hour);
        w.record_end(r);
    }
}

void Ledger::save_items(SaveWriter& w) const {
    if (!s_) return;
    const Storage& s = *s_;
    for (int i = 0; i < s.item_n; ++i) s_->order[i] = static_cast<uint16_t>(i);
    std::sort(s_->order, s_->order + s.item_n,
              [&](uint16_t a, uint16_t b) { return s.items[a].id < s.items[b].id; });
    char key[12];
    w.u16(static_cast<uint16_t>(s.item_n));
    for (int k = 0; k < s.item_n; ++k) {
        const Item& it = s.items[s.order[k]];
        const size_t r = w.record_begin();
        w.u32(it.id);
        w.str(s.goods[it.good]->id);
        w.str(store_key(it.store, key, sizeof(key)));
        w.u16(it.owner);
        w.u16(it.condition);
        w.u16(it.maker);
        w.i32(it.day);
        w.record_end(r);
    }
}

void Ledger::save_reservations(SaveWriter& w) const {
    if (!s_) return;
    const Storage& s = *s_;
    char key[12];
    w.u16(s.next_res);
    w.u16(static_cast<uint16_t>(s.res_n));
    for (int i = 0; i < s.res_n; ++i) {  // Kennungen steigen (angelegt nur an Takt-Grenzen)
        const Storage::Res& x = s.res[i];
        const size_t r = w.record_begin();
        w.u16(x.id);
        w.u16(x.holder);
        w.str(store_key(x.store, key, sizeof(key)));
        w.str(s.goods[x.good]->id);
        w.u16(x.qty);
        w.record_end(r);
    }
}

namespace {

// erstes Gemeinschaftslager (Fundsachen beim Laden), −1 = keins
int fallback_store(const Ledger& l) {
    for (int i = 0; i < l.stores(); ++i)
        if (l.store_kind(i) == StoreKind::Community) return i;
    return -1;
}

// Lagerplatz zur Kennung im Speicherstand; eine Trage („@<Figur>“) entsteht dabei. moved: gibt es nicht mehr, liegt
// jetzt im ersten Gemeinschaftslager
int store_for(Ledger& l, std::string_view key, bool* moved) {
    *moved = false;
    if (const int s = l.find_store(key); s >= 0) return s;
    if (key.size() > 1 && key[0] == '@') {
        unsigned v = 0;
        for (size_t i = 1; i < key.size(); ++i) {
            if (key[i] < '0' || key[i] > '9') return -1;
            v = v * 10 + static_cast<unsigned>(key[i] - '0');
        }
        if (v > 0 && v <= 0xFFFF) return l.make_hand(static_cast<uint16_t>(v));
    }
    *moved = true;
    return fallback_store(l);
}

}  // namespace

void Ledger::load_goods(SaveReader& r, uint16_t /*version*/, LoadStats* /*st*/) {
    if (!s_) return;
    Storage& s = *s_;
    s.hour = r.i64();
    s.today = r.i32();
    const int n = r.u16();
    for (int k = 0; k < n && r.ok(); ++k) {
        SaveReader x = r.record();
        const int g = find_good(x.view());
        GoodTotals t;
        t.start = x.i64();
        const int kinds = x.u8();
        for (int j = 0; j < kinds; ++j) {
            const int64_t v = x.i64();
            if (j < kBookingKinds) t.moved[j] = v;
        }
        t.want = x.i64();
        t.got = x.i64();
        const uint8_t seen = x.u8();
        uint16_t use[kHours] = {};
        read_hours(x, use, s.hour);
        if (!x.ok() || g < 0) continue;
        s.totals[g] = t;
        s.level_seen[g] = seen;
        std::memcpy(s.use[g], use, sizeof(use));
    }
    const int m = r.u16();
    for (int k = 0; k < m && r.ok(); ++k) {
        const int st = find_store(r.view());
        const uint8_t f = r.u8();
        if (r.ok() && st >= 0) s.store_seen[st] = f;
    }
}

void Ledger::load_stock(SaveReader& r, uint16_t /*version*/, LoadStats* st) {
    if (!s_) return;
    Storage& s = *s_;
    const int n = r.u16();
    for (int k = 0; k < n && r.ok(); ++k) {
        SaveReader x = r.record();
        const std::string_view key = x.view();
        const std::string_view gid = x.view();
        const uint16_t q = x.u16(), in = x.u16(), out = x.u16();
        const uint8_t flags = x.u8();
        uint16_t hours[kHours] = {};
        read_hours(x, hours, s.hour);
        if (!x.ok()) continue;
        const int g = find_good(gid);
        if (g < 0) {
            if (st) st->lost += q;
            continue;
        }
        bool moved = false;
        const int store = store_for(*this, key, &moved);
        if (store < 0) {
            if (st) st->lost += q;
            s.adjust[g] -= q;  // Bilanz: diese Stück gibt es nicht mehr (finish_load)
            continue;
        }
        if (moved && st) st->moved += q;
        Stock* e = entry(store, g, true);
        if (!e) continue;
        e->qty = static_cast<uint16_t>(std::min(0xFFFF, e->qty + q));
        e->in_day = in;
        e->out_day = out;
        e->flags = flags;
        std::memcpy(e->out, hours, sizeof(hours));
        s.sum[g] += q;
        ++s.stores[store].version;
        if (st) ++st->stock;
    }
    ++version_;
}

void Ledger::load_items(SaveReader& r, uint16_t /*version*/, LoadStats* st) {
    if (!s_) return;
    Storage& s = *s_;
    const int n = r.u16();
    for (int k = 0; k < n && r.ok(); ++k) {
        SaveReader x = r.record();
        Item it;
        it.id = x.u32();
        const std::string_view gid = x.view();
        const std::string_view key = x.view();
        it.owner = x.u16();
        it.condition = static_cast<uint16_t>(std::min<int>(x.u16(), 1000));
        it.maker = x.u16();
        it.day = x.i32();
        const int g = find_good(gid);
        bool moved = false;
        const int store = x.ok() ? store_for(*this, key, &moved) : -1;
        if (!x.ok() || g < 0 || store < 0 || !it.id || item(it.id) || s.item_n >= kMaxItems) continue;
        it.good = static_cast<uint8_t>(g);
        it.store = static_cast<uint8_t>(store);
        s.items[s.item_n++] = it;
        if (st) ++st->items;
    }
}

void Ledger::load_reservations(SaveReader& r, uint16_t /*version*/, LoadStats* st) {
    if (!s_) return;
    Storage& s = *s_;
    s.next_res = r.u16();
    if (!s.next_res) s.next_res = 1;
    const int n = r.u16();
    for (int k = 0; k < n && r.ok(); ++k) {
        SaveReader x = r.record();
        Storage::Res v;
        v.id = x.u16();
        v.holder = x.u16();
        const int store = find_store(x.view());
        const int g = find_good(x.view());
        v.qty = x.u16();
        if (!x.ok() || store < 0 || g < 0 || !v.id || s.res_n >= kMaxReservations) continue;
        v.store = static_cast<uint8_t>(store);
        v.good = static_cast<uint8_t>(g);
        s.res[s.res_n++] = v;
        if (st) ++st->reservations;
    }
}

void Ledger::finish_load(LoadStats* st) {
    if (!s_) return;
    Storage& s = *s_;
    // Einzelstücke bestimmen den Bestand ihrer Ware: Eintrag für jedes Stück, Menge = Zahl der Stücke (eine fremde oder
    // beschädigte Datei darf nichts verdoppeln; was nicht passt, steht als Korrektur im Bilanz-Anfang)
    for (int k = 0; k < s.item_n; ++k) entry(s.items[k].store, s.items[k].good, true);
    for (int i = 0; i < s.stock_n; ++i) {
        Stock& e = s.stock[i];
        if (!s.goods[e.good]->unique) continue;
        int items = 0;
        for (int k = 0; k < s.item_n; ++k) items += s.items[k].store == e.store && s.items[k].good == e.good;
        if (items == e.qty) continue;
        if (st) st->lost += std::abs(e.qty - items);
        s.sum[e.good] += items - e.qty;
        s.adjust[e.good] += items - e.qty;
        e.qty = static_cast<uint16_t>(items);
    }
    for (int g = 0; g < s.good_n; ++g) {
        s.totals[g].start += s.adjust[g];
        s.adjust[g] = 0;
    }
    // Reservierungen nachzählen (nie mehr als da ist)
    for (int i = 0; i < s.stock_n; ++i) s.stock[i].reserved = 0;
    for (int k = 0; k < s.res_n; ++k) {
        Storage::Res& v = s.res[k];
        Stock* e = entry(v.store, v.good, false);
        const int free = e ? e->qty - e->reserved : 0;
        v.qty = static_cast<uint16_t>(std::clamp<int>(v.qty, 0, free));
        if (e) e->reserved = static_cast<uint16_t>(e->reserved + v.qty);
    }
    ++version_;
}

}  // namespace pixelia
