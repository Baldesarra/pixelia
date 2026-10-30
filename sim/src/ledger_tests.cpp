// Prüfungen des Warenbuchs (seit 0.5.10, --test warenbuch): Buchungen, Obergrenzen, Tragen, Reservierung,
// Einzelstücke, Reichweite, Speicherstand, Herd mit Brennholz, Nachholen und Neustart. Der Nachweis für 0.5.10 laut
// Systemplan: Bilanz exakt (auch über Neustart und Nachholen bis 3 Tage), Material genau einmal, angefangene Arbeit
// geht am selben Stand weiter.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/econ_log.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/png.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/stage.h"
#include "sim.h"
#include "testdorf/test_place.h"

namespace sim {
namespace {

using pixelia::Booking;
using pixelia::BookingInfo;
using pixelia::Ledger;

struct Case {
    const char* name;
    std::function<std::string()> run;
};

// --- eigene kleine Wirtschaft für die Grundprüfungen ---

constexpr pixelia::GoodDef kGoods[] = {
    {.id = "holz", .title = "Holz", .weight = 10, .soll_days = 2, .max_days = 6},
    {.id = "brot", .title = "Brot", .weight = 5},
    {.id = "stein", .title = "Stein", .weight = 40},
    {.id = "axt", .title = "Axt", .weight = 20, .unique = true},
};
constexpr pixelia::StockRule kShedRule[] = {{"holz", 4, 12}};
constexpr pixelia::StoreDef kStores[] = {
    {.id = "stapel", .title = "Stapel", .place = "x", .kind = pixelia::StoreKind::Producer, .capacity = 30},
    {.id = "lege", .title = "Lege", .place = "x", .kind = pixelia::StoreKind::House, .capacity = 12,
     .rules = kShedRule, .rule_count = 1},
    {.id = "kiste", .title = "Kiste", .place = "y", .kind = pixelia::StoreKind::Community, .capacity = 100},
};
constexpr pixelia::StartStock kStart[] = {{"stapel", "holz", 20}, {"kiste", "brot", 10}, {"kiste", "axt", 2, 700}};
constexpr pixelia::EconomyDef kEco = {kGoods, 4, kStores, 3, kStart, 3};

void build(Ledger& l) {
    const pixelia::EconomyDef* defs[] = {&kEco};
    l.build(defs, 1);
    l.start_stock(1);
}

std::string checked(const Ledger& l) {
    char buf[200];
    const char* e = l.check(buf, sizeof(buf));
    return e ? std::string(e) : std::string();
}

BookingInfo at(int64_t us, uint16_t who = 0, const char* why = "test") {
    BookingInfo i;
    i.at_us = us;
    i.who = who;
    i.reason = why;
    return i;
}

// --- Bühne mit dem Testdorf ---

struct Run {
    pixelia::Stage stage;
    pixelia::WorldClock clock;
    explicit Run(uint64_t seed, const char* start = "testort-raum", int hour = 16, int minute = 0, double speed = 1.0) {
        Options opt;
        opt.scene = start;
        opt.seed = seed;
        setup_stage(opt, stage);
        stage.set_world_time(true);
        stage.set_follow(false);
        clock.set_time_of_day(hour, minute);
        clock.set_speed(speed);
        clock.advance(0);
        stage.tick(clock);
    }
    void run(int ticks) {
        for (int i = 0; i < ticks; ++i) {
            clock.advance(pixelia::kTickUs);
            stage.tick(clock);
        }
    }
    Ledger& ledger() { return stage.environment().ledger; }
    std::vector<uint8_t> save(uint32_t gen = 1) const {
        std::vector<uint8_t> b(96 * 1024);
        b.resize(stage.save_state(b.data(), b.size(), gen));
        return b;
    }
};

std::string first_byte_diff(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    size_t k = 0;
    while (k < a.size() && k < b.size() && a[k] == b[k]) ++k;
    return "Bytes weichen ab bei " + std::to_string(k) + " von " + std::to_string(a.size()) + "/" +
           std::to_string(b.size());
}

// Warenbuch als Text: Bestände je Lagerplatz und Zähler je Ware (Vergleich zweier Abläufe). Die Lagerplätze nach
// Kennung sortiert: Tragen entstehen in beliebiger Reihenfolge, der Speicherstand legt sie sortiert an
std::string ledger_text(const Ledger& l) {
    std::vector<std::string> rows;
    char key[12], line[160];
    for (int s = 0; s < l.stores(); ++s) {
        uint8_t g[16];
        uint16_t q[16];
        const int n = l.contents(s, g, q, 16);
        for (int k = 0; k < n; ++k) {
            std::snprintf(line, sizeof(line), "%s/%s=%u(%d) ", l.store_key(s, key, sizeof(key)), l.good(g[k]).id, q[k],
                          l.reserved(s, g[k]));
            rows.push_back(line);
        }
    }
    std::sort(rows.begin(), rows.end());
    std::string t;
    for (const std::string& r : rows) t += r;
    for (int g = 0; g < l.goods(); ++g) {
        const Ledger::GoodTotals& x = l.totals(g);
        std::snprintf(line, sizeof(line), "\n%s: Anfang %lld, verbraucht %lld, Bedarf %lld/%lld", l.good(g).id,
                      static_cast<long long>(x.start),
                      static_cast<long long>(x.moved[static_cast<int>(Booking::Consume)]),
                      static_cast<long long>(x.got), static_cast<long long>(x.want));
        t += line;
    }
    return t;
}

int64_t consumed(const Ledger& l, const char* good) {
    const int g = l.find_good(good);
    return g < 0 ? -1 : l.totals(g).moved[static_cast<int>(Booking::Consume)];
}

int64_t booked(const Ledger& l, const char* good, Booking b) {
    const int g = l.find_good(good);
    return g < 0 ? -1 : l.totals(g).moved[static_cast<int>(b)];
}

// --- Aufgabenliste (seit 0.5.10 Teil 2) ---

using pixelia::Task;
using pixelia::TaskKind;
using pixelia::TaskList;
using pixelia::TaskPhase;

std::string tasks_text(const TaskList& t, const Ledger& l) {
    std::string s;
    char line[160], a[12], b[12];
    for (int i = 0; i < t.count(); ++i) {
        const Task& x = t.at(i);
        std::snprintf(line, sizeof(line), "#%u %s %s %s→%s %u/%u @%u; ", static_cast<unsigned>(x.id),
                      l.good(x.good).id, pixelia::task_phase_id(x.phase), x.from >= 0 ? l.store_key(x.from, a, 12) : "-",
                      l.store_key(x.to, b, 12), static_cast<unsigned>(x.carried), static_cast<unsigned>(x.qty),
                      static_cast<unsigned>(x.who));
        s += line;
    }
    return s;
}

// Warenbuch und Aufgabenliste prüfen
std::string checked(const Ledger& l, const TaskList& t) {
    std::string e = checked(l);
    char buf[200];
    if (const char* x = t.check(l, buf, sizeof(buf)); e.empty() && x) e = x;
    return e;
}

// seit 0.5.10 Teil 3 dazu die Projekte
std::string checked(Run& r) {
    std::string e = checked(r.ledger(), r.stage.environment().tasks);
    char buf[200];
    if (const char* x = r.stage.environment().projects.check(r.ledger(), buf, sizeof(buf)); e.empty() && x) e = x;
    return e;
}

// laufendes Bett-Projekt der Kammer (seit 0.5.10 Teil 3), nullptr = keins
const pixelia::Project* bed(Run& r) {
    const pixelia::ProjectList& pl = r.stage.environment().projects;
    const int d = pl.find_def("kammer_bett");
    for (int i = 0; i < pl.count(); ++i)
        if (pl.at(i).def == d) return &pl.at(i);
    return nullptr;
}

// Uhr auf die nächste Takt-Grenze, dann ein Schritt (Vergleich zweier Abläufe an derselben Stelle)
void to_grid(Run& r) {
    const int64_t g = pixelia::Stage::kGridUs;
    r.clock.set_world_us((r.clock.world_us() / g + 1) * g);
    r.stage.tick(r.clock);
}

// verbautes Material als Text
std::string built_text(const Ledger& l) {
    return "Bretter " + std::to_string(booked(l, "bretter", Booking::Build)) + ", Nägel " +
           std::to_string(booked(l, "naegel", Booking::Build)) + ", Stroh " +
           std::to_string(booked(l, "stroh", Booking::Build));
}

// Projekte als Text (Vergleich zweier Abläufe)
std::string projects_text(const pixelia::ProjectList& p, const Ledger& l) {
    std::string s;
    char line[240];
    for (int i = 0; i < p.count(); ++i) {
        p.describe(i, l, line, sizeof(line));
        s += std::string(line) + "; ";
    }
    return s;
}

// Vorrat auf keep verringern (verderben), damit Aufgaben entstehen
void drain(Run& r, const char* store, const char* good, int keep) {
    Ledger& l = r.ledger();
    const int s = l.find_store(store), g = l.find_good(good);
    if (s >= 0 && g >= 0 && l.qty(s, g) > keep)
        l.take(s, g, l.qty(s, g) - keep, Booking::Spoil, at(r.clock.world_us()));
}

// Testort einer Bühne nach Kennung (seit 0.5.10 Teil 2b: Stellplätze)
pixelia::TestPlace* test_place(Run& r, const char* name) {
    for (int i = 0; i < r.stage.count(); ++i)
        if (std::strcmp(r.stage.scene(i).name(), name) == 0) return static_cast<pixelia::TestPlace*>(&r.stage.scene(i));
    return nullptr;
}

// Bestand eines Lagerplatzes auf n setzen (Zugang bzw. Verderben)
void set_stock(Run& r, const char* store, const char* good, int n) {
    Ledger& l = r.ledger();
    const int s = l.find_store(store), g = l.find_good(good);
    if (s < 0 || g < 0) return;
    if (l.qty(s, g) > n) l.take(s, g, l.qty(s, g) - n, Booking::Spoil, at(r.clock.world_us()));
    if (l.qty(s, g) < n) l.add(s, g, n - l.qty(s, g), Booking::Produce, at(r.clock.world_us()));
}

// kleine Wirtschaft für die Aufgabenliste: Holzstapel (Erzeuger) → Holzlege, Kiste → Brotregal
constexpr pixelia::StockRule kPile2[] = {{"holz", 20, 40}};
constexpr pixelia::StockRule kShed2[] = {{"holz", 4, 12}};
constexpr pixelia::StockRule kShelf2[] = {{"brot", 2, 8}};
constexpr pixelia::StoreDef kStores2[] = {
    {.id = "stapel", .title = "Stapel", .place = "x", .kind = pixelia::StoreKind::Producer, .capacity = 40,
     .rules = kPile2, .rule_count = 1},
    {.id = "lege", .title = "Lege", .place = "y", .kind = pixelia::StoreKind::House, .capacity = 12, .rules = kShed2,
     .rule_count = 1, .source = "stapel"},
    {.id = "regal", .title = "Regal", .place = "y", .kind = pixelia::StoreKind::House, .capacity = 8, .rules = kShelf2,
     .rule_count = 1, .source = "kiste"},
    {.id = "kiste", .title = "Kiste", .place = "z", .kind = pixelia::StoreKind::Community, .capacity = 100},
};
constexpr pixelia::StartStock kStart2[] = {{"stapel", "holz", 24}, {"lege", "holz", 3}, {"regal", "brot", 1},
                                           {"kiste", "brot", 3}};
constexpr pixelia::EconomyDef kEco2 = {kGoods, 4, kStores2, 4, kStart2, 4};

std::vector<Case> cases() {
    return {
        {"Buchungen: Zugang bis zur Obergrenze, Abgang nie unter null, Umlagern ändert die Welt nicht; Bilanz stimmt",
         [] {
             Ledger l;
             build(l);
             const int stapel = l.find_store("stapel"), lege = l.find_store("lege"), kiste = l.find_store("kiste");
             const int holz = l.find_good("holz"), brot = l.find_good("brot");
             if (stapel < 0 || lege < 0 || kiste < 0 || holz < 0) return std::string("Daten nicht gefunden");
             if (l.qty(stapel, holz) != 20 || l.total(holz) != 20) return std::string("Startvorrat fehlt");
             if (l.add(stapel, holz, 25, Booking::Produce, at(1)) != 10) return std::string("Obergrenze 30 nicht gehalten");
             if (l.take(lege, holz, 3, Booking::Consume, at(2)) != 0) return std::string("aus leerem Lager gebucht");
             if (l.move(stapel, lege, holz, 20, at(3)) != 12) return std::string("Umlagern über die Obergrenze der Lege");
             if (l.take(lege, holz, 5, Booking::Consume, at(4)) != 5) return std::string("Verbrauch");
             if (l.add(kiste, brot, 1, Booking::Consume, at(5)) != 0) return std::string("Abgangsart als Zugang gebucht");
             if (l.total(holz) != 25 || l.qty(stapel, holz) != 18 || l.qty(lege, holz) != 7)
                 return "Bestände falsch: " + ledger_text(l);
             const Ledger::GoodTotals& t = l.totals(holz);
             if (t.start != 20 || t.moved[static_cast<int>(Booking::Produce)] != 10 ||
                 t.moved[static_cast<int>(Booking::Consume)] != 5 || t.moved[static_cast<int>(Booking::Move)] != 12)
                 return std::string("Zähler falsch");
             return checked(l);
         }},
        {"Tragen: die Trage einer Figur zählt nach Gewicht (Korb 10 Trageeinheiten), was unterwegs ist, gehört ihr",
         [] {
             Ledger l;
             build(l);
             const int stapel = l.find_store("stapel"), kiste = l.find_store("kiste");
             const int holz = l.find_good("holz"), brot = l.find_good("brot");
             const int hand = l.make_hand(7);
             if (hand < 0 || l.hand(7) != hand || l.make_hand(7) != hand) return std::string("Trage nicht eindeutig");
             if (l.move(stapel, hand, holz, 15, at(1, 7)) != 10) return std::string("mehr als 10 Scheite im Korb");
             if (l.move(kiste, hand, brot, 5, at(2, 7)) != 0) return std::string("voller Korb nimmt noch Brot");
             l.move(hand, stapel, holz, 4, at(3, 7));
             if (l.move(kiste, hand, brot, 20, at(4, 7)) != 8) return std::string("Gewicht: 6 Scheite + 8 Brote");
             char key[12];
             if (std::string(l.store_key(hand, key, sizeof(key))) != "@00007" || l.find_store("@00007") != hand)
                 return std::string("Kennung der Trage");
             if (l.load(hand) != 100 || l.room(hand, holz) != 0) return std::string("Last der Trage");
             return checked(l);
         }},
        {"Reservierung: andere sehen nur Freies, wer reserviert hat, holt; freigeben gibt den Rest zurück",
         [] {
             Ledger l;
             build(l);
             const int stapel = l.find_store("stapel"), lege = l.find_store("lege");
             const int holz = l.find_good("holz");
             const uint16_t r = l.reserve(stapel, holz, 15, 900);
             if (!r || l.available(stapel, holz) != 5 || l.reserved(stapel, holz) != 15)
                 return std::string("Reservierung nicht gehalten");
             if (l.take(stapel, holz, 10, Booking::Consume, at(1)) != 5) return std::string("Reserviertes verbraucht");
             if (l.move(stapel, lege, holz, 8, at(2), r) != 8 || l.reservation_left(r) != 7)
                 return std::string("mit Reservierung holen");
             if (l.reserve(stapel, holz, 99, 901) != 0) return std::string("reserviert, was nicht frei ist");
             l.release_holder(900);
             if (l.reserved(stapel, holz) != 0 || l.available(stapel, holz) != 7 || l.reservations() != 0)
                 return std::string("freigeben");
             return checked(l);
         }},
        {"Einzelstücke: Besitzer und Zustand bleiben, wandern einzeln, Kennung hängt nicht von der Reihenfolge ab",
         [] {
             Ledger l;
             build(l);
             const int kiste = l.find_store("kiste"), stapel = l.find_store("stapel");
             const int axt = l.find_good("axt");
             if (l.items() != 2 || l.qty(kiste, axt) != 2) return std::string("zwei Äxte im Startvorrat");
             if (l.item_at(0).condition != 700) return std::string("Zustand aus dem Startvorrat");
             const uint32_t a = l.add_item(stapel, axt, 12, 1000, Booking::Produce, at(5000, 3));
             Ledger l2;
             build(l2);
             const uint32_t b = l2.add_item(l2.find_store("stapel"), axt, 12, 1000, Booking::Produce, at(5000, 3));
             if (!a || a != b) return std::string("Kennung hängt von der Reihenfolge ab");
             if (!l.set_condition(a, 640) || !l.move_item(a, l.make_hand(3), at(5001, 3)))
                 return std::string("Axt in die Trage");
             const pixelia::Item* it = l.item(a);
             if (!it || it->owner != 12 || it->maker != 3 || it->condition != 640 || l.hand_owner(it->store) != 3)
                 return std::string("Besitzer, Hersteller oder Zustand verloren");
             if (l.take(kiste, axt, 1, Booking::Spoil, at(5002)) != 1 || l.items() != 2)
                 return std::string("eine Axt aus der Kiste ausmustern");
             return checked(l);
         }},
        {"Reichweite: Abgänge der letzten 24 Weltstunden, Notbestand unter einem Tag, Soll in Tagen",
         [] {
             Ledger l;
             build(l);
             const int lege = l.find_store("lege"), stapel = l.find_store("stapel");
             const int holz = l.find_good("holz");
             const int64_t h = 3600LL * 1000000;
             l.move(stapel, lege, holz, 12, at(0));
             for (int k = 0; k < 6; ++k) l.take(lege, holz, 1, Booking::Consume, at(k * 4 * h));  // 6 am Tag
             l.roll(23 * h);
             if (l.out_24h(lege, holz) != 6 || l.reach_hours(lege, holz) != 24) return std::string("Reichweite der Lege");
             if (l.use_24h(holz) != 6 || l.reach_hours(holz) != 14 * 24 / 6)
                 return "Reichweite im Dorf: " + std::to_string(l.reach_hours(holz));
             if (l.level(holz) != pixelia::StockLevel::Ok) return std::string("2⅓ Tage sind über dem Soll von 2");
             l.roll(30 * h);  // die ersten 6 Stunden fallen heraus
             if (l.out_24h(lege, holz) != 4) return std::string("alte Stunden nicht herausgefallen");
             pixelia::EventBus bus;
             const int rd = bus.subscribe(pixelia::event_bit(pixelia::EventKind::Stock));
             l.take(stapel, holz, 8, Booking::Consume, at(30 * h));
             l.take(lege, holz, 6, Booking::Consume, at(30 * h));
             l.step(31 * h, 2, &bus);
             l.step(32 * h, 2, &bus);  // nur einmal melden
             bus.deliver();
             std::string texts;
             for (pixelia::WorldEvent e; bus.poll(rd, &e);) texts += std::string(e.text) + "; ";
             if (texts.find("Lege leer") == std::string::npos || texts.find("Notbestand: Holz") == std::string::npos ||
                 texts.size() > 80)
                 return "Ereignisse: " + texts;
             return checked(l);
         }},
        {"Speicherstand: Warenbuch speichern, laden, wieder speichern ergibt dieselben Bytes; Trage und Reservierung bleiben",
         [] {
             Run a(11);
             a.run(30 * 60);
             Ledger& l = a.ledger();
             const int vorrat = l.find_store("platz_vorrat"), bretter = l.find_good("bretter");
             const int hand = l.make_hand(a.stage.environment().figures.at(2).id);
             if (vorrat < 0 || bretter < 0 || hand < 0) return std::string("Testdorf ohne Vorratskiste");
             l.move(vorrat, hand, bretter, 3, at(a.clock.world_us()));
             const uint16_t r = l.reserve(vorrat, bretter, 4, 77);
             // seit 0.5.10 Teil 3 holt das Bett-Projekt auch Bretter: verglichen wird mit dem Stand vor dem Speichern
             const int left = l.reservation_left(r), free = l.available(vorrat, bretter);
             if (!r || left == 0) return std::string("nichts reserviert");
             const std::vector<uint8_t> s1 = a.save(3);
             Run b(99, "testort-wiese");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s1.data(), s1.size(), &rep)) return std::string("Laden: ") + rep.why;
             if (!rep.ledger || rep.stock == 0 || rep.items != 1 || rep.reservations < 1 || rep.lost || rep.moved)
                 return "Bericht: " + std::to_string(rep.stock) + " Einträge, " + std::to_string(rep.items) + " Stücke";
             const std::vector<uint8_t> s2 = b.save(3);
             if (s1 != s2) return first_byte_diff(s1, s2);
             const Ledger& m = b.ledger();
             if (m.qty(m.hand(a.stage.environment().figures.at(2).id), bretter) != 3) return std::string("Trage leer");
             if (m.reservation_left(r) != left || m.available(vorrat, bretter) != free) return std::string("Reservierung weg");
             if (ledger_text(m) != ledger_text(l))
                 return "Warenbuch nach dem Laden anders:\n" + ledger_text(l) + "\n" + ledger_text(m);
             return checked(m);
         }},
        {"Alter Stand 0.5.9 lädt weiter und bekommt den Test-Startvorrat; unbekannter Lagerplatz → Gemeinschaftslager",
         [] {
             std::vector<uint8_t> old;
             if (FILE* f = std::fopen("sim/testdata/welt-0.5.9.pxw", "rb")) {
                 uint8_t buf[4096];
                 for (size_t k; (k = std::fread(buf, 1, sizeof(buf), f)) > 0;) old.insert(old.end(), buf, buf + k);
                 std::fclose(f);
             }
             if (old.empty()) return std::string("Vorlage sim/testdata/welt-0.5.9.pxw fehlt");
             Run b(1);
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(old.data(), old.size(), &rep)) return std::string("Laden: ") + rep.why;
             const Ledger& l = b.ledger();
             if (rep.ledger || l.total(l.find_good("brennholz")) != 32 || l.items() != 1)
                 return "kein Startvorrat: " + ledger_text(l);
             // Stand mit Warenbuch, in dem ein Lagerplatz umbenannt ist
             Run a(2);
             std::vector<uint8_t> s = a.save();
             const std::string from = "raum_brotregal", to = "raum_brotkiste";
             auto it = std::search(s.begin(), s.end(), from.begin(), from.end());
             if (it == s.end()) return std::string("Brotregal nicht im Stand");
             std::copy(to.begin(), to.end(), it);
             const uint32_t c = pixelia::crc32(s.data() + 8, s.size() - 8);
             for (int k = 0; k < 4; ++k) s[4 + k] = static_cast<uint8_t>(c >> (8 * k));
             Run m(3);
             if (!m.stage.load_state(s.data(), s.size(), &rep)) return std::string("umbenannt: ") + rep.why;
             const Ledger& lm = m.ledger();
             const int brot = lm.find_good("brot");
             if (rep.moved != 6 || lm.qty(lm.find_store("platz_vorrat"), brot) != 30 || lm.total(brot) != 30 + 12)
                 return "umgelegt " + std::to_string(rep.moved) + ": " + ledger_text(lm);
             return checked(lm);
         }},
        // --- Aufgaben, Holen und Bringen (seit 0.5.10 Teil 2) ---
        {"Aufgaben: Vorrat unter low → Holen mit Reservierung (volle Ladung, leer: was da ist), je Ziel eine; Erzeuger → Herstellen; Notbestand → Gemeinschaft",
         [] {
             Ledger l;
             const pixelia::EconomyDef* defs[] = {&kEco2};
             l.build(defs, 1);
             l.start_stock(1);
             pixelia::FigureBook book;
             pixelia::FigureDef def{};
             book.add(def, 0, 7);
             book.add(def, 0, 8);
             TaskList t;
             t.clear();
             const int stapel = l.find_store("stapel"), lege = l.find_store("lege"), regal = l.find_store("regal");
             const int kiste = l.find_store("kiste"), holz = l.find_good("holz"), brot = l.find_good("brot");
             const int64_t s = 1000000;
             t.step(1 * s, 1, l, book);
             // Lege 3 < 4: 9 Scheite holen (bis 12), reserviert; Regal 1 < 2, aber Kiste hat nur 3 von 7: warten
             if (t.count() != 1 || t.at(0).kind != TaskKind::Fetch || t.at(0).from != stapel || t.at(0).to != lege ||
                 t.at(0).qty != 9 || l.reserved(stapel, holz) != 9)
                 return "Holen: " + tasks_text(t, l);
             t.step(2 * s, 1, l, book);
             if (t.count() != 1) return "doppelt: " + tasks_text(t, l);
             l.take(regal, brot, 1, Booking::Consume, at(3 * s));
             t.step(3 * s, 1, l, book);  // Regal leer: holt die 3, die da sind
             const Task* b = t.count() == 2 ? &t.at(1) : nullptr;
             if (!b || b->from != kiste || b->qty != 3 || l.available(kiste, brot) != 0) return "leer: " + tasks_text(t, l);
             // übernehmen: eine je Figur, nur offene
             const uint16_t first = t.at(0).id;
             if (!t.claim(first, 7, 4 * s) || t.claim(b->id, 7, 4 * s) || t.claim(first, 8, 4 * s))
                 return "übernehmen: " + tasks_text(t, l);
             if (t.of(7) == nullptr || t.of(7)->phase != TaskPhase::Fetch) return std::string("Phase nach dem Übernehmen");
             // Herstellen: Stapel unter 20
             l.take(stapel, holz, 10, Booking::Consume, at(5 * s));
             t.step(5 * s, 1, l, book);
             int produce = 0;
             for (int i = 0; i < t.count(); ++i) produce += t.at(i).kind == TaskKind::Produce;
             if (produce != 1) return "Herstellen: " + tasks_text(t, l);
             // fertig (ohne zu liefern): weg, Reservierung frei; die Lege braucht noch Holz, also gleich eine neue
             const uint16_t res = t.of(7)->res;
             t.of(7)->phase = TaskPhase::Done;
             t.step(6 * s, 1, l, book);
             if (t.find(first) || l.reservation_left(res) != 0 || t.of(7) || l.reserved(stapel, holz) != 9)
                 return "aufräumen: " + tasks_text(t, l);
             // Notbestand: viel Verbrauch in der letzten Stunde, wenig Holz → drei helfen am Stapel
             l.add(stapel, holz, 20, Booking::Produce, at(7 * s));
             l.take(stapel, holz, 30, Booking::Consume, at(7 * s));
             l.roll(8 * s);
             if (l.level(holz) != pixelia::StockLevel::Emergency) return "kein Notbestand: " + ledger_text(l);
             pixelia::EventBus bus;
             const int rd = bus.subscribe(pixelia::event_bit(pixelia::EventKind::Stock));
             t.step(8 * s, 1, l, book, &bus);
             t.step(9 * s, 1, l, book, &bus);  // nur einmal in die Chronik
             bus.deliver();
             int helpers = 0, told = 0;
             for (int i = 0; i < t.count(); ++i)
                 helpers += t.at(i).kind == TaskKind::Produce && t.at(i).active() && t.at(i).to == stapel;
             for (pixelia::WorldEvent e; bus.poll(rd, &e);) told += e.a == 3 && std::strstr(e.text, "Gemeinschaftsaufgabe");
             if (helpers != TaskList::kHelpers || told != 1)
                 return "Gemeinschaftsaufgabe (" + std::to_string(told) + " Meldungen): " + tasks_text(t, l);
             // Speicherstand der Liste
             std::vector<uint8_t> b1(4096), b2(4096);
             pixelia::SaveWriter w1(b1.data(), b1.size());
             w1.begin(1, 0);
             w1.chunk("AUFG", 1);
             t.save(w1, l);
             b1.resize(w1.finish());
             TaskList u;
             u.clear();
             pixelia::SaveChunks c(b1.data(), b1.size());
             char tag[5];
             uint16_t v = 0;
             pixelia::SaveReader sr;
             if (!c.next(tag, &v, &sr) || u.load(sr, v, l) != t.count()) return std::string("Laden der Liste");
             pixelia::SaveWriter w2(b2.data(), b2.size());
             w2.begin(1, 0);
             w2.chunk("AUFG", 1);
             u.save(w2, l);
             b2.resize(w2.finish());
             if (b1 != b2) return first_byte_diff(b1, b2);
             return checked(l, t);
         }},
        {"Holzkette: ohne Holz bleibt der Herd aus, bis Nachschub kommt; Hackklotz spaltet, Holzlege holt vom Holzstapel; Bilanz über einen Tag",
         [] {
             Run a(5, "testort-raum", 5, 59, 60.0);
             drain(a, "raum_holzlege", "brennholz", 0);
             drain(a, "wiese_holzstapel", "brennholz", 0);
             a.run(32);  // bis 6:00:04: die Herdzeit beginnt ohne Holz
             const Ledger& l = a.ledger();
             const int holz = l.find_good("brennholz");
             const pixelia::PlaceStates& st = a.stage.environment().states;
             const int ofen = st.find("testort-raum", "ofen");
             if (ofen < 0 || st.on(ofen)) return std::string("Herd brennt ohne Holz");
             if (l.totals(holz).want <= l.totals(holz).got) return std::string("Bedarf ohne Holz nicht im Buch");
             a.run(30 * 24 * 60);  // ein Weltag in 24 Minuten Zeitraffer
             const int64_t made = booked(l, "brennholz", Booking::Produce), burnt = consumed(l, "brennholz");
             const int64_t moved = booked(l, "brennholz", Booking::Move);
             if (made < 10 || burnt < 6 || moved < 2 * burnt)
                 return "gespalten " + std::to_string(made) + ", getragen " + std::to_string(moved) + ", verbrannt " +
                        std::to_string(burnt) + ": " + ledger_text(l);
             bool empty = false;
             const pixelia::Chronicle& ch = a.stage.environment().chronicle;
             for (int i = 0; i < ch.count(); ++i) empty = empty || std::strstr(ch.at(i).text, "Holzlege leer");
             if (!empty) return std::string("Chronik ohne „Holzlege leer“");
             return checked(a);
         }},
        {"Brot: Mahlzeiten aus dem Brotregal (Platzhalter), Nachschub aus der Vorratskiste über zwei Orte, die vom Backofen holt; Bedarf gedeckt",
         [] {
             Run a(8, "testort-raum", 7, 0, 60.0);
             drain(a, "raum_brotregal", "brot", 1);
             a.run(30 * 24 * 60 * 2);  // zwei Welttage
             const Ledger& l = a.ledger();
             const int brot = l.find_good("brot");
             const int64_t eaten = consumed(l, "brot"), moved = booked(l, "brot", Booking::Move);
             const int64_t baked = booked(l, "brot", Booking::Produce);
             if (eaten < 6 || moved < 2 * 6 || baked < 2)
                 return "gegessen " + std::to_string(eaten) + ", getragen " + std::to_string(moved) + ", gebacken " +
                        std::to_string(baked) + ": " + ledger_text(l);
             const Ledger::GoodTotals& t = l.totals(brot);
             if (t.got != eaten || t.want - t.got > 2)
                 return "Bedarf " + std::to_string(t.got) + "/" + std::to_string(t.want) + ", gegessen " + std::to_string(eaten);
             return checked(a);
         }},
        {"Nachholen bis 3 Tage: Bilanz exakt, Herd verbrennt nur, was in der Holzlege lag; Tätigkeiten warten, danach Nachschub",
         [] {
             // a läuft Minute für Minute weiter, b lädt den Stand 30 Stunden später und holt die Ortszustände nach; die
             // Figuren holen dabei nichts (Tätigkeiten setzen erst danach neu an, Entscheidung 0.5.10 Teil 1)
             Run a(17, "testort-raum", 6, 30, 600.0);
             a.run(3);
             const std::vector<uint8_t> s = a.save();
             a.run(30 * 180);  // 30 Stunden
             Run b(17);
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             const int64_t burnt0 = consumed(b.ledger(), "brennholz");
             const int lege = b.ledger().find_store("raum_holzlege"), holz = b.ledger().find_good("brennholz");
             const int in_lege = b.ledger().qty(lege, holz);
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(1.0);
             const uint32_t before = b.stage.decisions();
             b.run(1);
             int held = 0;
             for (int k = 0; k < 30 && b.stage.catching_up() > 0; ++k) {
                 if (b.stage.decisions() != before) return std::string("Tätigkeiten liefen während des Nachholens");
                 b.run(1);
                 ++held;
             }
             if (b.stage.catching_up() > 0 || held < 3) return "Nachholen: " + std::to_string(held) + " Schritte";
             const int64_t burnt = consumed(b.ledger(), "brennholz") - burnt0;
             if (burnt != in_lege || b.ledger().qty(lege, holz) != 0)
                 return "nachgeholt verbrannt " + std::to_string(burnt) + " von " + std::to_string(in_lege);
             if (consumed(a.ledger(), "brennholz") <= consumed(b.ledger(), "brennholz"))
                 return std::string("Minute für Minute (mit Nachschub) nicht mehr verbrannt");
             const int64_t moved = booked(b.ledger(), "brennholz", Booking::Move);
             b.clock.set_speed(60.0);
             b.run(30 * 20);  // 40 Weltminuten
             if (b.stage.decisions() == before) return std::string("Tätigkeiten danach nicht wieder angelaufen");
             if (booked(b.ledger(), "brennholz", Booking::Move) <= moved) return std::string("danach kein Nachschub");
             return checked(b);
         }},
        {"Neustart mitten am Tag: nach dem Laden läuft alles genau so weiter (Speicherstand nach 2 Weltstunden gleich)",
         [] {
             Run a(29, "testort-raum", 16, 40, 30.0);
             a.run(30 * 40);  // bis 17:00, der Herd brennt
             Ledger& l = a.ledger();
             const int hand = l.make_hand(a.stage.environment().figures.at(1).id);
             l.move(l.find_store("wiese_holzstapel"), hand, l.find_good("brennholz"), 5, at(a.clock.world_us()));
             const std::vector<uint8_t> s = a.save();
             Run b(29, "testort-raum");
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(30.0);
             a.run(30 * 240);
             b.run(30 * 240);
             const int64_t g = pixelia::Stage::kGridUs;
             for (Run* r : {&a, &b}) {
                 r->clock.set_world_us((r->clock.world_us() / g + 1) * g);
                 r->stage.tick(r->clock);
             }
             // verglichen wird das Warenbuch (das Wetter läuft nach dem Laden weich neu an und gehört nicht hierher)
             const std::string x = ledger_text(a.ledger()), y = ledger_text(b.ledger());
             if (x != y) return "ohne Neustart: " + x + "\n          mit Neustart: " + y;
             if (consumed(a.ledger(), "brennholz") < 2) return std::string("Herd hat nichts verbrannt");
             return checked(b.ledger());
         }},
        {"Neustart mitten im Transport: Brot in der Trage, Figur auf dem Feldweg; kommt genau einmal an, alles wie ohne Neustart",
         [] {
             Run a(23, "testort-raum", 8, 0, 60.0);
             uint16_t who = 0;
             int tries = 0;
             const pixelia::FigureBook& book = a.stage.environment().figures;
             for (int t = 0; t < 30 * 60 * 72 && !who; ++t) {
                 // Regal immer wieder leeren, bis jemand mit Brot über den Feldweg (30 Weltsekunden) geht
                 const TaskList& tl = a.stage.environment().tasks;
                 bool busy = false;
                 for (int i = 0; i < tl.count(); ++i) busy = busy || tl.at(i).good == a.ledger().find_good("brot");
                 if (!busy && a.ledger().qty(a.ledger().find_store("raum_brotregal"), a.ledger().find_good("brot")) > 1) {
                     drain(a, "raum_brotregal", "brot", 1);
                     ++tries;
                 }
                 a.run(1);
                 for (int k = 0; k < book.count() && !who; ++k) {
                     const pixelia::FigureRecord& f = book.at(k);
                     const Task* x = tl.of(f.id);
                     if (f.where.kind == pixelia::Whereabouts::Kind::Link && x && x->phase == TaskPhase::Bring &&
                         x->carried > 0 && a.clock.world_us() + 4000000 < f.where.arrive_us)
                         who = f.id;
                 }
             }
             if (!who) return "niemand mit Brot auf dem Feldweg (" + std::to_string(tries) + " Versuche)";
             const int brot = a.ledger().find_good("brot"), regal = a.ledger().find_store("raum_brotregal");
             const int carried = a.ledger().qty(a.ledger().hand(who), brot);
             const int before = a.ledger().qty(regal, brot);
             const std::vector<uint8_t> s = a.save();
             Run b(23, "testort-raum");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s.data(), s.size(), &rep)) return std::string("Laden: ") + rep.why;
             if (rep.tasks == 0 || rep.transit == 0) return std::string("Aufgabe oder Weg nicht im Stand");
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(60.0);
             a.run(30 * 60);  // eine Weltstunde
             b.run(30 * 60);
             const int64_t g = pixelia::Stage::kGridUs;
             for (Run* r : {&a, &b}) {
                 r->clock.set_world_us((r->clock.world_us() / g + 1) * g);
                 r->stage.tick(r->clock);
             }
             const Ledger& lb = b.ledger();
             if (lb.qty(lb.hand(who), brot) != 0 || lb.in_today(regal, brot) < carried)
                 return "nicht angekommen: Trage " + std::to_string(lb.qty(lb.hand(who), brot)) + ", Regal vorher " +
                        std::to_string(before) + ", jetzt " + std::to_string(lb.qty(regal, brot));
             const std::string x = ledger_text(a.ledger()) + tasks_text(a.stage.environment().tasks, a.ledger());
             const std::string y = ledger_text(lb) + tasks_text(b.stage.environment().tasks, lb);
             if (x != y) return "ohne Neustart: " + x + "\n          mit Neustart: " + y;
             return checked(b);
         }},
        {"P10 mit Transporten: kein Ort sichtbar, Wiese sichtbar, Orte wechselnd ergibt denselben Speicherstand (Zeitraffer 60)",
         [] {
             std::vector<uint8_t> save[3];
             int64_t moved = 0, made = 0;
             uint32_t tasks = 0;
             for (int run = 0; run < 3; ++run) {
                 Run r(31, run == 0 ? "scrolltest" : "testort-wiese", 6, 0, 60.0);
                 drain(r, "raum_holzlege", "brennholz", 1);
                 drain(r, "raum_brotregal", "brot", 1);
                 drain(r, "wiese_holzstapel", "brennholz", 12);
                 const pixelia::Transition kinds[] = {pixelia::Transition::Cut, pixelia::Transition::Dissolve,
                                                      pixelia::Transition::Fade};
                 const int ticks = 30 * 240;  // 8 Weltstunden
                 for (int t = 0, k = 0; t < ticks; t += 7 * 30, ++k) {
                     r.run(std::min(7 * 30, ticks - t));
                     if (run == 2) r.stage.go(1 + (k * 3) % (r.stage.count() - 1), kinds[k % 3]);
                 }
                 const int64_t g = pixelia::Stage::kGridUs;
                 r.clock.set_world_us((r.clock.world_us() / g + 1) * g);
                 r.stage.tick(r.clock);
                 save[run] = r.save();
                 if (std::string e = checked(r); !e.empty()) return "Lauf " + std::to_string(run) + ": " + e;
                 if (run == 0) {
                     moved = booked(r.ledger(), "brennholz", Booking::Move) + booked(r.ledger(), "brot", Booking::Move);
                     made = booked(r.ledger(), "brennholz", Booking::Produce);
                     tasks = r.stage.environment().tasks.created();
                 }
             }
             if (moved < 20 || made < 4 || tasks < 4)
                 return "zu wenig getragen (" + std::to_string(moved) + " Stück, " + std::to_string(made) + " gespalten, " +
                        std::to_string(tasks) + " Aufgaben)";
             for (int run = 1; run < 3; ++run)
                 if (save[run] != save[0])
                     return std::string(run == 1 ? "Wiese sichtbar: " : "Orte wechselnd: ") + first_byte_diff(save[0], save[run]);
             return std::string();
         }},
        // --- Projekte (seit 0.5.10 Teil 3) ---
        {"Projekt Bett: Material genau einmal über Reservierung, drei Teilaufgaben nur mit Tischlern und tagsüber; Chronik nur Besonderes",
         [] {
             Run r(41, "testort-raum", 6, 30, 60.0);
             std::vector<pixelia::ProjectRecord> rec;
             r.stage.environment().projects.set_listener(
                 [](void* c, const pixelia::ProjectRecord& x) {
                     static_cast<std::vector<pixelia::ProjectRecord>*>(c)->push_back(x);
                 },
                 &rec);
             const Ledger& l = r.ledger();
             const int vorrat = l.find_store("platz_vorrat"), site = l.find_store("kammer_baustelle");
             const int bretter = l.find_good("bretter"), naegel = l.find_good("naegel"), stroh = l.find_good("stroh");
             const int b0 = l.qty(vorrat, bretter), n0 = l.qty(vorrat, naegel), s0 = l.qty(vorrat, stroh);
             pixelia::TestPlace* kammer = test_place(r, "testort-kammer");
             if (!kammer || !bed(r)) return std::string("kein Bett-Projekt in der Kammer");
             const pixelia::FigureBook& book = r.stage.environment().figures;
             bool worked = false, building_seen = false;
             int max_stack = 0;
             for (int t = 0; t < 30 * 60 * 48 && bed(r); ++t) {  // höchstens zwei Welttage
                 r.run(1);
                 const pixelia::Project* p = bed(r);
                 if (!p) break;
                 max_stack = std::max(max_stack, l.load(site));
                 if (kammer->room().building(1)) building_seen = true;
                 if (p->who) {
                     const pixelia::FigureRecord* f = book.find(p->who);
                     if (!f || f->skills[static_cast<int>(pixelia::Skill::Joinery)].level < pixelia::SkillLevel::Beginner)
                         return std::string("arbeitet ohne Tischlern");
                     if (!p->leaving && p->work_s > 0) worked = true;
                 }
                 // Arbeitsgänge enden spätestens um 19 Uhr (die letzte Takt-Grenze liegt höchstens 2 Weltsekunden zurück)
                 if (p->who && !p->leaving && p->wait == pixelia::ProjectWait::None &&
                     !pixelia::ProjectList::work_time(r.clock.world_us()) &&
                     !pixelia::ProjectList::work_time(r.clock.world_us() - 3 * pixelia::Stage::kGridUs))
                     return "arbeitet nach Feierabend um " + std::to_string(r.clock.hour()) + " Uhr";
                 if (t % 600 == 0)
                     if (std::string e = checked(r); !e.empty()) return e;
             }
             if (bed(r)) return "nicht fertig: " + projects_text(r.stage.environment().projects, l);
             r.run(30);  // die Chronik liest den Bus im nächsten Schritt
             if (!worked || !building_seen) return std::string("niemand hat am Bett gearbeitet");
             if (kammer->room().building(1)) return std::string("Bett noch im Bau");
             if (built_text(l) != "Bretter 8, Nägel 6, Stroh 4") return "verbaut: " + built_text(l);
             if (l.qty(vorrat, bretter) != b0 - 8 || l.qty(vorrat, naegel) != n0 - 6 || l.qty(vorrat, stroh) != s0 - 4 ||
                 l.load(site) != 0)
                 return "Vorratskiste: " + ledger_text(l);
             if (max_stack < 6) return "Stapel an der Baustelle höchstens " + std::to_string(max_stack);
             if (rec.size() != 4 || !rec[0].step || !rec[2].step || rec[3].step)
                 return "Protokoll: " + std::to_string(rec.size()) + " Einträge";
             uint32_t person = 0;
             for (int k = 0; k < 3; ++k) person += rec[k].person_s;
             if (person != 480u * 60u || rec[3].person_s != person || rec[3].planned_s != 480u * 60u)
                 return "Personensekunden " + std::to_string(person);
             if (rec[0].wait_s[static_cast<int>(pixelia::ProjectWait::Material)] == 0)
                 return std::string("keine Wartezeit auf Material gezählt");
             int q = 0;
             for (int k = 0; k < rec[3].good_count; ++k) q += rec[3].qty[k];
             if (q != 18) return "verbaut laut Protokoll " + std::to_string(q);
             const pixelia::Chronicle& ch = r.stage.environment().chronicle;
             int steps = 0, done = 0;
             for (int i = 0; i < ch.count(); ++i) {
                 const std::string x = ch.at(i).text;
                 steps += x.find("Bett bauen: ") == 0 && x.find(" fertig") != std::string::npos;
                 done += x == "Projekt fertig: Bett bauen";
             }
             if (steps != 3 || done != 1)
                 return "Chronik: " + std::to_string(steps) + " Teilaufgaben, " + std::to_string(done) + " Projekt";
             return checked(r);
         }},
        {"Neustart mitten im Bau: Zwischenstand, Reservierung und verbautes Material bleiben; alles wie ohne Neustart",
         [] {
             Run a(43, "testort-raum", 6, 30, 60.0);
             const pixelia::Project* p = nullptr;
             for (int t = 0; t < 30 * 60 * 24; ++t) {
                 a.run(1);
                 p = bed(a);
                 if (p && p->begun && p->step == 0 && p->work_s >= 90 * 60 && p->who && !p->leaving) break;
                 p = nullptr;
             }
             if (!p) return "Gestell nicht halb fertig: " + projects_text(a.stage.environment().projects, a.ledger());
             const uint32_t work = p->work_s;
             const std::vector<uint8_t> s = a.save();
             Run b(43, "testort-raum");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s.data(), s.size(), &rep)) return std::string("Laden: ") + rep.why;
             const pixelia::Project* q = bed(b);
             if (rep.projects != 1 || !q || q->work_s != work || !q->begun) return std::string("Zwischenstand nicht geladen");
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(60.0);
             a.run(30 * 60 * 3);  // drei Weltstunden
             b.run(30 * 60 * 3);
             to_grid(a);
             to_grid(b);
             const pixelia::Environment& ea = a.stage.environment();
             const pixelia::Environment& eb = b.stage.environment();
             const std::string x = ledger_text(a.ledger()) + built_text(a.ledger()) + tasks_text(ea.tasks, a.ledger()) +
                                   projects_text(ea.projects, a.ledger());
             const std::string y = ledger_text(b.ledger()) + built_text(b.ledger()) + tasks_text(eb.tasks, b.ledger()) +
                                   projects_text(eb.projects, b.ledger());
             if (x != y) return "ohne Neustart: " + x + "\n          mit Neustart: " + y;
             if (booked(b.ledger(), "bretter", Booking::Build) <= 0) return std::string("nichts verbaut");
             return checked(b);
         }},
        {"Neustart mitten im Materialtransport zur Baustelle: kommt genau einmal an und wird genau einmal verbaut",
         [] {
             Run a(47, "testort-raum", 6, 30, 60.0);
             uint16_t who = 0;
             const pixelia::FigureBook& book = a.stage.environment().figures;
             const TaskList& tl = a.stage.environment().tasks;
             const int kammer = pixelia::place_index("testort-kammer");
             for (int t = 0; t < 30 * 60 * 12 && !who; ++t) {
                 a.run(1);
                 for (int k = 0; k < book.count() && !who; ++k) {
                     const Task* x = tl.of(book.at(k).id);
                     if (x && (x->flags & pixelia::kTaskProject) && x->phase == TaskPhase::Bring && x->carried > 0 &&
                         book.at(k).where.place != kammer)
                         who = book.at(k).id;
                 }
             }
             if (!who) return std::string("niemand trägt Material zur Kammer");
             const std::vector<uint8_t> s = a.save();
             Run b(47, "testort-raum");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s.data(), s.size(), &rep)) return std::string("Laden: ") + rep.why;
             if (rep.tasks == 0 || rep.projects != 1) return std::string("Aufgabe oder Projekt nicht im Stand");
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(60.0);
             a.run(30 * 60 * 14);  // bis zum Abend: das Bett ist fertig
             b.run(30 * 60 * 14);
             to_grid(a);
             to_grid(b);
             const pixelia::Environment& ea = a.stage.environment();
             const pixelia::Environment& eb = b.stage.environment();
             const std::string x = ledger_text(a.ledger()) + built_text(a.ledger()) + projects_text(ea.projects, a.ledger());
             const std::string y = ledger_text(b.ledger()) + built_text(b.ledger()) + projects_text(eb.projects, b.ledger());
             if (x != y) return "ohne Neustart: " + x + "\n          mit Neustart: " + y;
             if (bed(b) || built_text(b.ledger()) != "Bretter 8, Nägel 6, Stroh 4")
                 return "verbaut: " + built_text(b.ledger()) + "; " + projects_text(eb.projects, b.ledger());
             return checked(b);
         }},
        {"P10 mit Bau: kein Ort sichtbar, Kammer sichtbar, Orte wechselnd ergibt denselben Speicherstand (Zeitraffer 60)",
         [] {
             std::vector<uint8_t> save[3];
             std::string built;
             for (int run = 0; run < 3; ++run) {
                 Run r(53, run == 0 ? "scrolltest" : "testort-kammer", 6, 30, 60.0);
                 const pixelia::Transition kinds[] = {pixelia::Transition::Cut, pixelia::Transition::Dissolve,
                                                      pixelia::Transition::Fade};
                 const int ticks = 30 * 60 * 6;  // 6 Weltstunden
                 for (int t = 0, k = 0; t < ticks; t += 9 * 30, ++k) {
                     r.run(std::min(9 * 30, ticks - t));
                     if (run == 2) r.stage.go(1 + (k * 3) % (r.stage.count() - 1), kinds[k % 3]);
                 }
                 to_grid(r);
                 save[run] = r.save();
                 if (std::string e = checked(r); !e.empty()) return "Lauf " + std::to_string(run) + ": " + e;
                 if (run == 0) built = built_text(r.ledger());
             }
             if (built == "Bretter 0, Nägel 0, Stroh 0") return std::string("nichts gebaut");
             for (int run = 1; run < 3; ++run)
                 if (save[run] != save[0])
                     return std::string(run == 1 ? "Kammer sichtbar: " : "Orte wechselnd: ") +
                            first_byte_diff(save[0], save[run]);
             return std::string();
         }},
        {"Alter Stand mit „Bett im Bau“ (0.5.9, 0.5.10 Teil 1–2) lädt: das Bett-Projekt beginnt frisch, das Bett im Raum ist fertig",
         [] {
             std::vector<uint8_t> old;
             if (FILE* f = std::fopen("sim/testdata/welt-0.5.9.pxw", "rb")) {
                 uint8_t buf[4096];
                 for (size_t k; (k = std::fread(buf, 1, sizeof(buf), f)) > 0;) old.insert(old.end(), buf, buf + k);
                 std::fclose(f);
             }
             if (old.empty()) return std::string("Vorlage sim/testdata/welt-0.5.9.pxw fehlt");
             Run b(1);
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(old.data(), old.size(), &rep)) return std::string("Laden: ") + rep.why;
             const pixelia::Project* p = bed(b);
             if (rep.projects != 0 || !p || p->step != 0 || p->begun || p->work_s != 0)
                 return std::string("kein frisches Projekt");
             b.run(30 * 60);
             pixelia::TestPlace* raum = test_place(b, "testort-raum");
             if (!raum || raum->room().building(6)) return std::string("Bett im Raum noch im Bau");
             if (std::string e = checked(b); !e.empty()) return "0.5.9: " + e;
             // Stand aus Teil 1–2: ohne Abschnitt PROJ (hier umbenannt), aber mit Reservierungen eines begonnenen Bettes
             Run a(5, "testort-raum", 6, 30, 60.0);
             for (int t = 0; t < 30 * 60 * 12 && !(bed(a) && bed(a)->begun); ++t) a.run(1);
             if (!bed(a) || !bed(a)->begun) return std::string("Bett nicht begonnen");
             std::vector<uint8_t> s = a.save();
             const std::string tag = "PROJ";
             auto it = std::search(s.begin(), s.end(), tag.begin(), tag.end());
             if (it == s.end()) return std::string("Abschnitt PROJ fehlt");
             it[3] = 'X';
             const uint32_t c = pixelia::crc32(s.data() + 8, s.size() - 8);
             for (int k = 0; k < 4; ++k) s[4 + k] = static_cast<uint8_t>(c >> (8 * k));
             Run m(6, "testort-raum");
             if (!m.stage.load_state(s.data(), s.size(), &rep)) return std::string("ohne PROJ: ") + rep.why;
             m.clock.set_world_us(a.clock.world_us());
             m.clock.set_speed(60.0);
             if (!bed(m) || bed(m)->begun) return std::string("ohne PROJ: kein frisches Projekt");
             m.run(30 * 60 * 14);
             if (bed(m) || built_text(m.ledger()) != "Bretter 8, Nägel 6, Stroh 4")
                 return "ohne PROJ verbaut: " + built_text(m.ledger());
             return checked(m);
         }},
        {"Prüfbefehl „projekt bett start|stufe <n>|fertig“: springt im Projekt, die Bilanz bleibt exakt",
         [] {
             Run r(7, "testort-kammer", 9, 0, 60.0);
             r.run(30 * 60);  // eine Weltstunde: Material unterwegs, vielleicht schon begonnen
             if (!r.stage.command("projekt bett stufe 2") || !bed(r) || bed(r)->step != 2) return std::string("stufe 2");
             if (r.stage.command("projekt tisch fertig")) return std::string("unbekanntes Projekt angenommen");
             if (!r.stage.command("projekt bett fertig")) return std::string("fertig");
             r.run(30 * 60 * 2);  // Rest zurück in die Vorratskiste
             pixelia::TestPlace* kammer = test_place(r, "testort-kammer");
             const Ledger& l = r.ledger();
             if (bed(r) || !kammer || kammer->room().building(1) || l.load(l.find_store("kammer_baustelle")) != 0)
                 return "nicht aufgeräumt: " + projects_text(r.stage.environment().projects, l) + ledger_text(l);
             if (!r.stage.command("projekt bett start") || !bed(r) || bed(r)->step != 0) return std::string("start");
             r.run(30);
             if (!kammer->room().building(1)) return std::string("neues Bett nicht im Bau");
             return checked(r);
         }},
        // --- Werkzeug am Projekt (seit 0.5.10 Teil 4): die Säge ---
        {"Säge: genau eine unterwegs, nie verbaut, zählt in der Trage; liegt bei Gestell und Liegefläche da und kommt nach dem Projekt zurück",
         [] {
             Run r(61, "testort-raum", 6, 30, 60.0);
             std::vector<pixelia::ProjectRecord> rec;
             r.stage.environment().projects.set_listener(
                 [](void* c, const pixelia::ProjectRecord& x) {
                     static_cast<std::vector<pixelia::ProjectRecord>*>(c)->push_back(x);
                 },
                 &rec);
             const Ledger& l = r.ledger();
             const TaskList& tl = r.stage.environment().tasks;
             const int saege = l.find_good("saege"), vorrat = l.find_store("platz_vorrat");
             const int site = l.find_store("kammer_baustelle");
             if (saege < 0 || l.qty(vorrat, saege) != 1) return std::string("keine Säge in der Vorratskiste");
             bool at_site = false, carried = false;
             int carry_load = 0;
             for (int t = 0; t < 30 * 60 * 48 && bed(r); ++t) {
                 r.run(1);
                 if (l.total(saege) != 1 || l.items() < 1) return "Sägen im Dorf: " + std::to_string(l.total(saege));
                 int open = 0;
                 for (int i = 0; i < tl.count(); ++i) open += tl.at(i).good == saege && tl.at(i).active();
                 if (open > 1) return "zwei Aufgaben mit der Säge: " + tasks_text(tl, l);
                 for (int i = 0; i < l.items(); ++i) {
                     const pixelia::Item& it = l.item_at(i);
                     if (it.good != saege) continue;
                     at_site |= it.store == site;
                     if (l.store_kind(it.store) == pixelia::StoreKind::Hand) {
                         carried = true;
                         carry_load = std::max(carry_load, l.load(it.store));
                     }
                 }
                 const pixelia::Project* p = bed(r);
                 if (p && p->begun && p->step < 2 && (l.qty(site, saege) != 1 || l.reserved(site, saege) != 1))
                     return "Schritt " + std::to_string(p->step) + " läuft ohne reservierte Säge";
                 if (t % 600 == 0)
                     if (std::string e = checked(r); !e.empty()) return e;
             }
             if (bed(r)) return "nicht fertig: " + projects_text(r.stage.environment().projects, l);
             r.run(30 * 60 * 2);  // das Projekt endet, sobald die Baustelle leer ist; die Säge ist dann noch unterwegs
             if (l.total(saege) != 1) return std::string("Säge verloren");
             if (!at_site || !carried) return std::string("Säge nie an der Baustelle bzw. nie getragen");
             if (carry_load < l.good(saege).weight) return "Trage mit Säge nur " + std::to_string(carry_load);
             if (l.qty(vorrat, saege) != 1 || booked(l, "saege", Booking::Build) != 0)
                 return "Säge nicht zurück: " + ledger_text(l);
             if (built_text(l) != "Bretter 8, Nägel 6, Stroh 4") return "verbaut: " + built_text(l);
             if (rec.size() != 4 || rec[0].tool_count != 1 || rec[0].tools[0] != saege || rec[2].tool_count != 0 ||
                 rec[3].tool_count != 1)
                 return std::string("Protokoll ohne Werkzeug");
             int q = 0;
             for (int k = 0; k < rec[3].good_count; ++k) q += rec[3].qty[k];
             if (q != 18) return "verbaut laut Protokoll " + std::to_string(q);
             return checked(r);
         }},
        {"Neustart, während die Säge getragen wird: kommt genau einmal an, alles wie ohne Neustart, danach zurück",
         [] {
             Run a(67, "testort-raum", 6, 30, 60.0);
             const Ledger& la = a.ledger();
             const int saege = la.find_good("saege");
             const int kammer = pixelia::place_index("testort-kammer");
             bool found = false;
             for (int t = 0; t < 30 * 60 * 12 && !found; ++t) {
                 a.run(1);
                 for (int i = 0; i < la.items() && !found; ++i) {
                     const pixelia::Item& it = la.item_at(i);
                     const uint16_t who = it.good == saege ? la.hand_owner(it.store) : 0;
                     const pixelia::FigureRecord* f = who ? a.stage.environment().figures.find(who) : nullptr;
                     found = f && f->where.place != kammer;
                 }
             }
             if (!found) return std::string("niemand trägt die Säge");
             const std::vector<uint8_t> s = a.save();
             Run b(67, "testort-raum");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s.data(), s.size(), &rep)) return std::string("Laden: ") + rep.why;
             if (b.ledger().total(saege) != 1) return std::string("nach dem Laden nicht genau eine Säge");
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(60.0);
             a.run(30 * 60 * 14);
             b.run(30 * 60 * 14);
             to_grid(a);
             to_grid(b);
             const std::string x = ledger_text(a.ledger()) + projects_text(a.stage.environment().projects, a.ledger());
             const std::string y = ledger_text(b.ledger()) + projects_text(b.stage.environment().projects, b.ledger());
             if (x != y) return "ohne Neustart: " + x + "\n          mit Neustart: " + y;
             const Ledger& lb = b.ledger();
             if (bed(b) || lb.qty(lb.find_store("platz_vorrat"), saege) != 1 || lb.total(saege) != 1)
                 return "Säge nicht zurück: " + ledger_text(lb);
             return checked(b);
         }},
        {"Säge vergeben: ein zweites Projekt wartet („an der Baustelle“), bekommt sie nach dem ersten; nie zwei Aufgaben",
         [] {
             // eigene kleine Wirtschaft: zwei Baustellen, eine Säge in der Kiste
             static constexpr pixelia::GoodDef goods[] = {
                 {.id = "bretter", .title = "Bretter", .weight = 20},
                 {.id = "saege", .title = "Säge", .weight = 20, .unique = true, .the = "die Säge"},
             };
             static constexpr pixelia::StoreDef stores[] = {
                 {.id = "kiste", .title = "Kiste", .place = "testort-platz", .kind = pixelia::StoreKind::Community,
                  .capacity = 100},
                 {.id = "bau_a", .title = "Baustelle", .place = "testort-kammer", .kind = pixelia::StoreKind::Site,
                  .capacity = 40},
                 {.id = "bau_b", .title = "Baustelle", .place = "testort-raum", .kind = pixelia::StoreKind::Site,
                  .capacity = 40},
             };
             static constexpr pixelia::ProjectNeed needs[] = {{"bretter", 2}, {"saege", 1, true}};
             static constexpr pixelia::ProjectStepDef steps[] = {
                 {.id = "a", .title = "Sägen", .label = "Sägen", .needs = needs, .need_count = 2,
                  .level = pixelia::SkillLevel::Unknown, .minutes = 60},
                 {.id = "b", .title = "Sägen", .label = "Sägen", .needs = needs, .need_count = 2,
                  .level = pixelia::SkillLevel::Unknown, .minutes = 60},
             };
             static constexpr pixelia::ProjectDef projects[] = {
                 {.id = "pa", .title = "A", .place = "testort-kammer", .site = "bau_a", .source = "kiste", .steps = steps,
                  .step_count = 2, .start = true},
                 {.id = "pb", .title = "B", .place = "testort-raum", .site = "bau_b", .source = "kiste", .steps = steps,
                  .step_count = 1, .start = true},
             };
             static constexpr pixelia::StartStock start[] = {{"kiste", "bretter", 10}, {"kiste", "saege", 1}};
             static constexpr pixelia::EconomyDef eco = {goods, 2, stores, 3, start, 2, projects, 2};
             const pixelia::EconomyDef* defs[] = {&eco};
             Ledger l;
             l.build(defs, 1);
             l.start_stock(1);
             TaskList tl;
             pixelia::ProjectList pl;
             pl.build(defs, 1, l);
             pl.start_new();
             pixelia::FigureBook book;
             const int saege = l.find_good("saege"), kiste = l.find_store("kiste"), a = l.find_store("bau_a"),
                       b = l.find_store("bau_b");
             int64_t now = 8 * 3600LL * 1000000;
             // offene Aufgaben ausführen (wie ein Träger: mit der Reservierung umlagern)
             auto carry = [&] {
                 for (int i = 0; i < tl.count(); ++i) {
                     const Task& t = tl.at(i);
                     if (t.phase != TaskPhase::Open) continue;
                     l.move(t.from, t.to, t.good, t.qty, at(now), t.res);
                     l.release(t.res);
                     tl.cancel(t.id);
                 }
             };
             auto saw_tasks = [&] {
                 int n = 0;
                 for (int i = 0; i < tl.count(); ++i) n += tl.at(i).good == saege && tl.at(i).phase == TaskPhase::Open;
                 return n;
             };
             auto proj = [&](int k) { return &pl.at(k); };
             char buf[96];
             for (int round = 0; round < 3; ++round) {
                 pl.step(now, 1, l, tl, book, nullptr, false);
                 if (saw_tasks() > 1) return "zwei Sägen-Aufgaben: " + tasks_text(tl, l);
                 carry();
                 now += 2'000'000;
             }
             pl.step(now, 1, l, tl, book, nullptr, false);
             if (pl.count() != 2 || !proj(0)->begun || proj(1)->begun || l.qty(a, saege) != 1)
                 return "A nicht mit Säge begonnen: " + ledger_text(l);
             if (proj(1)->wait != pixelia::ProjectWait::Tool) return std::string("B wartet nicht auf das Werkzeug");
             const std::string w = pl.wait_text(*proj(1), l, buf, sizeof(buf));
             if (w != "Säge ist an der Baustelle (Testort Kammer)") return "Text: " + w;
             // A: zwei Schritte, die Säge bleibt dazwischen an der Baustelle (nur eine Aufgabe, nie zurück)
             pl.test_jump(0, 1, now, 1, l, tl, nullptr);
             pl.step(now, 1, l, tl, book, nullptr, false);
             if (l.qty(a, saege) != 1 || saw_tasks() != 0) return "Säge zwischen den Schritten: " + ledger_text(l);
             carry();
             pl.step(now, 1, l, tl, book, nullptr, false);
             pl.test_jump(0, 2, now, 1, l, tl, nullptr);
             // A fertig: das Übrige und die Säge zurück in die Kiste, dann holt B sie
             for (int round = 0; round < 6 && !(pl.count() == 1 && pl.at(0).begun); ++round) {
                 pl.step(now, 1, l, tl, book, nullptr, false);
                 if (saw_tasks() > 1) return "zwei Sägen-Aufgaben: " + tasks_text(tl, l);
                 carry();
                 now += 2'000'000;
             }
             if (pl.count() != 1 || !pl.at(0).begun || l.qty(b, saege) != 1 || l.total(saege) != 1)
                 return "B hat die Säge nicht: " + ledger_text(l) + projects_text(pl, l);
             if (booked(l, "saege", Booking::Build) != 0 || booked(l, "bretter", Booking::Build) != 4)
                 return "verbaut: " + ledger_text(l);
             if (l.qty(kiste, saege) != 0) return std::string("Säge doppelt");
             if (const char* e = pl.check(l, buf, sizeof(buf))) return std::string(e);
             return checked(l, tl);
         }},
        // --- Wirtschafts-Protokoll (seit 0.5.10 Teil 4) ---
        {"Protokoll: jede Buchung genau einmal, Neustart nach dem Speichern und ohne Speichern (Puffer bis zum Speicherstand)",
         [] {
             for (int lost_hour = 0; lost_hour < 2; ++lost_hour) {
                 const std::string dir = ".logs/test-wirtschaft-" + std::to_string(lost_hour);
                 std::error_code ec0;
                 std::filesystem::remove_all(dir, ec0);
                 Run a(71, "testort-raum", 6, 0, 60.0);
                 pixelia::EconomyLog la;
                 if (!la.attach(a.stage.environment(), 128 * 1024, "simulator")) return std::string("kein Puffer");
                 // Zähler beim Anhängen: das Protokoll muss genau die Buchungen danach enthalten
                 std::vector<pixelia::Ledger::GoodTotals> base;
                 for (int g = 0; g < a.ledger().goods(); ++g) base.push_back(a.ledger().totals(g));
                 auto run = [](Run& r, pixelia::EconomyLog& log, int ticks) {
                     for (int i = 0; i < ticks; ++i) {
                         r.run(1);
                         log.step(r.stage);
                     }
                 };
                 run(a, la, 30 * 60 * 2);  // zwei Weltstunden, dann Speicherstand wie am Gerät: packen, Schnitt, schreiben, anhängen
                 const std::vector<uint8_t> s = a.save();
                 la.cut();
                 if (!la.flush(dir.c_str())) return std::string("Anhängen fehlgeschlagen");
                 if (lost_hour) run(a, la, 30 * 60);  // eine Stunde ohne Speichern: Strom weg, der Puffer ist verloren
                 const int64_t restart_us = a.clock.world_us();
                 la.detach();
                 Run b(71, "testort-raum");
                 pixelia::Stage::LoadReport rep;
                 if (!b.stage.load_state(s.data(), s.size(), &rep)) return std::string("Laden: ") + rep.why;
                 pixelia::EconomyLog lb;
                 lb.attach(b.stage.environment(), 128 * 1024, "simulator");
                 b.clock.set_world_us(restart_us);  // nachholen ab dem Stand
                 b.clock.set_speed(60.0);
                 run(b, lb, 30 * 60 * 2);
                 lb.cut();
                 if (!lb.flush(dir.c_str())) return std::string("Anhängen nach dem Neustart fehlgeschlagen");
                 // je Ware und Art: Summe im Protokoll = Zähler im Warenbuch seit dem Anhängen
                 const Ledger& l = b.ledger();
                 std::vector<std::array<int64_t, pixelia::kBookingKinds>> sum(l.goods());
                 for (auto& x : sum) x.fill(0);
                 int rows = 0;
                 for (const auto& e : std::filesystem::directory_iterator(dir)) {
                     if (e.path().filename().string().find("-bewegungen.csv") == std::string::npos) continue;
                     std::ifstream f(e.path());
                     for (std::string line; std::getline(f, line);) {
                         if (line.empty() || line[0] == '#' || line.rfind("welttag", 0) == 0) continue;
                         std::vector<std::string> c;
                         for (size_t p = 0;;) {
                             const size_t q = line.find(',', p);
                             c.push_back(line.substr(p, q == std::string::npos ? std::string::npos : q - p));
                             if (q == std::string::npos) break;
                             p = q + 1;
                         }
                         if (c.size() != 12) return "Zeile mit " + std::to_string(c.size()) + " Spalten: " + line;
                         const int g = l.find_good(c[4]);
                         int k = 0;
                         while (k < pixelia::kBookingKinds && c[3] != pixelia::booking_id(static_cast<Booking>(k))) ++k;
                         if (g < 0 || k == pixelia::kBookingKinds) return "unbekannt: " + line;
                         sum[g][k] += std::stoi(c[5]);
                         ++rows;
                     }
                 }
                 if (rows < 10) return "nur " + std::to_string(rows) + " Bewegungen";
                 for (int g = 0; g < l.goods(); ++g)
                     for (int k = 0; k < pixelia::kBookingKinds; ++k)
                         if (sum[g][k] != l.totals(g).moved[k] - base[g].moved[k])
                             return std::string(lost_hour ? "ohne Speichern: " : "nach dem Speichern: ") + l.good(g).id +
                                    " " + pixelia::booking_id(static_cast<Booking>(k)) + " im Protokoll " +
                                    std::to_string(sum[g][k]) + ", im Warenbuch " +
                                    std::to_string(l.totals(g).moved[k] - base[g].moved[k]);
                 if (la.stats().lost || lb.stats().lost) return std::string("Zeilen verloren");
                 std::printf("  info  %s: %d Bewegungen, %u Dateien, %.1f KB\n",
                             lost_hour ? "ohne Speichern" : "nach dem Speichern", rows, lb.stats().files,
                             lb.stats().bytes / 1024.0);
                 std::filesystem::remove_all(dir, ec0);
             }
             return std::string();
         }},
        {"Protokoll: über 1 MB werden die ältesten Tage gelöscht; Projekte und Kopfzeile mit Formatversion",
         [] {
             const std::string dir = ".logs/test-wirtschaft-max";
             std::error_code ec0;
             std::filesystem::remove_all(dir, ec0);
             std::filesystem::create_directories(dir);
             const std::string big(400 * 1024, 'x');
             for (int d = 1; d <= 3; ++d) {
                 char name[64];
                 std::snprintf(name, sizeof(name), "%s/t%05d-bewegungen.csv", dir.c_str(), d);
                 std::ofstream(name) << big;
             }
             Run r(73, "testort-kammer", 6, 0, 60.0);
             r.clock.set_world_us(r.clock.world_us() + 9 * pixelia::WorldClock::kUsPerDay);  // Welttag 10
             r.run(1);
             pixelia::EconomyLog log;
             log.attach(r.stage.environment(), 128 * 1024, "simulator");
             r.stage.command("projekt bett fertig");  // ein Projekt fertig: projekte.csv
             for (int i = 0; i < 60; ++i) {
                 r.run(1);
                 log.step(r.stage);
             }
             log.cut();
             if (!log.flush(dir.c_str())) return std::string("Anhängen fehlgeschlagen");
             const pixelia::EconomyLog::Stats st = log.stats();
             if (std::filesystem::exists(dir + "/t00001-bewegungen.csv") || !std::filesystem::exists(dir + "/t00002-bewegungen.csv"))
                 return std::string("nicht der älteste Tag gelöscht");
             if (st.bytes > pixelia::EconomyLog::kMaxBytes || st.deleted != 1)
                 return "danach " + std::to_string(st.bytes) + " Bytes, gelöscht " + std::to_string(st.deleted);
             std::string projects;
             for (const auto& e : std::filesystem::directory_iterator(dir))
                 if (e.path().filename().string().find("-projekte.csv") != std::string::npos) projects = e.path().string();
             std::ifstream f(projects);
             std::string head, cols, row;
             std::getline(f, head);
             std::getline(f, cols);
             std::getline(f, row);
             if (head != "#pixelia-wirtschaft,format=1,tabelle=projekte,quelle=simulator" || cols.rfind("welttag,", 0) != 0 ||
                 row.find("kammer_bett") == std::string::npos)
                 return "projekte.csv: " + head + " / " + row;
             f.close();
             std::error_code ec;
             std::filesystem::remove_all(dir, ec);
             return std::string();
         }},
        {"Stellplätze: jedes Stück einzeln wie im Warenbuch; neu gezeichnet wird nur das Rechteck, pixelgleich mit dem ganzen Bild",
         [] {
             Run r(51, "testort-raum", 11, 0, 1.0);
             pixelia::TestPlace* raum = test_place(r, "testort-raum");
             if (!raum || !raum->room().has_background()) return std::string("Raum nicht betreten");
             const pixelia::Room& room = raum->room();
             Ledger& l = r.ledger();
             const int lege = l.find_store("raum_holzlege"), regal = l.find_store("raum_brotregal");
             const int holz = l.find_good("brennholz"), brot = l.find_good("brot");
             const int lp = l.store_def(lege)->part, rp = l.store_def(regal)->part;
             if (room.slot_count(lp) != 12 || room.slot_count(rp) != 8) return std::string("Stellplätze fehlen");
             std::vector<uint8_t> part_bg(pixelia::kCoarsePixels);
             for (int round = 0; round < 6; ++round) {
                 const int nh = (round * 5) % 13, nb = (round * 3 + 1) % 9;
                 set_stock(r, "raum_holzlege", "brennholz", nh);
                 set_stock(r, "raum_brotregal", "brot", nb);
                 const uint32_t before = room.bakes();
                 r.run(1);
                 if (room.fill_shown(lp) != l.qty(lege, holz) || room.fill_shown(rp) != l.qty(regal, brot))
                     return "Runde " + std::to_string(round) + ": gezeigt " + std::to_string(room.fill_shown(lp)) + "/" +
                            std::to_string(room.fill_shown(rp)) + " statt " + std::to_string(l.qty(lege, holz)) + "/" +
                            std::to_string(l.qty(regal, brot));
                 if (room.bakes() - before > 2) return std::string("zu oft neu gezeichnet");
                 std::copy_n(room.background(), pixelia::kCoarsePixels, part_bg.begin());
                 const_cast<pixelia::Room&>(room).invalidate();  // ganzes Bild zum Vergleich
                 if (!std::equal(part_bg.begin(), part_bg.end(), room.background()))
                     return "Runde " + std::to_string(round) + ": Teil-Neuzeichnen weicht vom ganzen Bild ab";
             }
             // Antippen: ein Stück gehört zu seinem Lagerplatz
             set_stock(r, "raum_holzlege", "brennholz", 12);
             r.run(1);
             int sx, sy;
             if (!room.slot_point(lp, 11, &sx, &sy) || room.hit_part(sx, sy - 1, nullptr) != lp)
                 return std::string("Stück antippen trifft den Lagerplatz nicht");
             // gleicher Bestand: kein Neuzeichnen
             const uint32_t b = room.bakes();
             r.run(30);
             if (room.bakes() != b) return std::string("Neuzeichnen ohne Änderung");
             return checked(l) .empty() ? std::string() : checked(l);
         }},
        {"Materialstapel und Bau-Bild: Stapel je Ware wie im Warenbuch, Bett in Stufen, Bauplane, pixelgleich; Wege frei; nach dem Laden sofort richtig; Kurzmeldung",
         [] {
             Run r(57, "testort-kammer", 6, 30, 60.0);
             pixelia::TestPlace* kammer = test_place(r, "testort-kammer");
             if (!kammer || !kammer->room().has_background()) return std::string("Kammer nicht betreten");
             const pixelia::Room& room = kammer->room();
             Ledger& l = r.ledger();
             const int site = l.find_store("kammer_baustelle");
             const int pile = l.store_def(site)->part, bett = 1;
             // erwartete Stücke je Bereich (test_place.cpp kSitePieces): Bretter 0–7, Stroh 8–11, Nägel 12–14 je zwei
             auto expect = [&](pixelia::Ledger& lg, int st, int first) {
                 const char* good = first == 0 ? "bretter" : first == 8 ? "stroh" : "naegel";
                 const int q = lg.qty(st, lg.find_good(good));
                 return first == 12 ? std::min((q + 1) / 2, 3) : std::min(q, first == 0 ? 8 : 4);
             };
             // Wege frei: Grundfläche begehbar, kein Benutzungspunkt darauf, Bett und Stapel von der Tür erreichbar
             const pixelia::Rect pr = room.part_rect(pile);
             for (int i = 0; i < room.part_count(); ++i)
                 for (int k = 0; k < 4; ++k) {
                     int tx, ty;
                     if (i == pile || !room.use_point(i, k, &tx, &ty, nullptr)) continue;
                     if (tx * 8 >= pr.x && tx * 8 < pr.x + pr.w && ty * 8 >= pr.y && ty * 8 < pr.y + pr.h)
                         return "Benutzungspunkt von Teil " + std::to_string(i) + " liegt unter dem Stapel";
                 }
             for (int ty = pr.y / 8; ty < (pr.y + pr.h) / 8; ++ty)
                 for (int tx = pr.x / 8; tx < (pr.x + pr.w) / 8; ++tx)
                     if (!room.walkable(tx, ty)) return std::string("Stapel versperrt den Weg");
             std::vector<uint8_t> part_bg(pixelia::kCoarsePixels);
             int seen_states = 0, max_boards = 0, checks = 0;
             bool shrank = false;
             int last_boards = -1;
             std::vector<uint8_t> mid;
             int mid_state = -1, mid_shown[3] = {};
             const int plane = 7;  // Bauplane (orte/kammer.cpp)
             int toasts = 0;
             for (int t = 0; t < 30 * 60 * 16 && (bed(r) || !seen_states); ++t) {
                 r.run(1);
                 const pixelia::Project* p = bed(r);
                 for (pixelia::WorldEvent e; r.stage.poll_event(&e);)
                     if (e.kind == pixelia::EventKind::Project) {
                         if (e.a != 1) return "Kurzmeldung für eine Teilaufgabe: " + std::string(e.text);
                         ++toasts;
                     }
                 if (p && !p->done(3) && room.part(plane).state != 1) return std::string("Bauplane fehlt während des Baus");
                 for (int first : {0, 8, 12})
                     if (room.fill_shown(pile, first) != expect(l, site, first))
                         return "Bereich " + std::to_string(first) + ": gezeigt " +
                                std::to_string(room.fill_shown(pile, first)) + " statt " +
                                std::to_string(expect(l, site, first));
                 const int boards = room.fill_shown(pile, 0);
                 if (last_boards >= 0 && boards < last_boards) shrank = true;
                 last_boards = boards;
                 max_boards = std::max(max_boards, boards);
                 if (p) {
                     seen_states |= 1 << room.part(bett).state;
                     for (int k : {0, 1})  // Bett und Stapel erreichbar (Arbeiter und Träger)
                         for (int part : {bett, pile}) {
                             int tx, ty;
                             if (room.use_point(part, k, &tx, &ty, nullptr) && room.path(1, 9, tx, ty) < 0)
                                 return "Teil " + std::to_string(part) + " nicht erreichbar";
                         }
                     // mitten im Bau mit Material am Stapel: Stand zum Laden merken
                     if (mid.empty() && p->begun && p->step == 1 && p->work_s > 0 && room.fill_shown(pile) > 0) {
                         mid = r.save();
                         mid_state = room.part(bett).state;
                         for (int k = 0; k < 3; ++k) mid_shown[k] = room.fill_shown(pile, k == 0 ? 0 : k == 1 ? 8 : 12);
                     }
                 }
                 if (t % 600 == 0) {  // Teil-Neuzeichnen pixelgleich mit dem ganzen Bild
                     std::copy_n(room.background(), pixelia::kCoarsePixels, part_bg.begin());
                     const_cast<pixelia::Room&>(room).invalidate();
                     if (!std::equal(part_bg.begin(), part_bg.end(), room.background()))
                         return "Schritt " + std::to_string(t) + ": Teil-Neuzeichnen weicht vom ganzen Bild ab";
                     ++checks;
                 }
             }
             if (bed(r)) return "Bett nicht fertig: " + projects_text(r.stage.environment().projects, l);
             if (seen_states != 0xF) return "Bau-Bild zeigte nicht alle vier Stufen (" + std::to_string(seen_states) + ")";
             if (max_boards < 6 || !shrank) return std::string("Stapel wuchs nicht bzw. schrumpfte nicht");
             if (room.fill_shown(pile) != 0 || room.building(bett)) return std::string("nach dem Ende nicht aufgeräumt");
             for (int t = 0; t < 30 * 60 && l.load(site) > 0; ++t) r.run(1);  // Übriges zurück, dann ist die Plane weg
             r.run(1);
             if (l.load(site) > 0 || room.part(plane).state != 0) return std::string("Bauplane bleibt liegen");
             if (toasts != 1) return "Kurzmeldungen „Projekt fertig“: " + std::to_string(toasts);
             if (mid.empty()) return std::string("kein Stand mitten im Bau");
             // nach dem Laden mitten im Bau: Stapel und Stufe schon im ersten Bild, ohne Takt-Grenze
             Run b(58, "testort-kammer", 6, 30, 1.0);
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(mid.data(), mid.size(), &rep)) return std::string("Laden: ") + rep.why;
             b.run(1);
             const pixelia::Room& rb = test_place(b, "testort-kammer")->room();
             if (rb.part(bett).state != mid_state || !rb.building(bett))
                 return "nach dem Laden Stufe " + std::to_string(rb.part(bett).state) + " statt " + std::to_string(mid_state);
             for (int k = 0; k < 3; ++k)
                 if (rb.fill_shown(pile, k == 0 ? 0 : k == 1 ? 8 : 12) != mid_shown[k])
                     return "nach dem Laden Stapel " + std::to_string(k) + " falsch";
             // alter Stand 0.5.9 ohne Baustellen-Waren: leerer Stapel, Bauplatz
             std::vector<uint8_t> old;
             if (FILE* f = std::fopen("sim/testdata/welt-0.5.9.pxw", "rb")) {
                 uint8_t buf[4096];
                 for (size_t k; (k = std::fread(buf, 1, sizeof(buf), f)) > 0;) old.insert(old.end(), buf, buf + k);
                 std::fclose(f);
             }
             Run c(59, "testort-kammer", 6, 30, 1.0);
             if (old.empty() || !c.stage.load_state(old.data(), old.size(), &rep)) return std::string("alter Stand");
             c.run(1);
             const pixelia::Room& rc = test_place(c, "testort-kammer")->room();
             if (rc.fill_shown(pile) != 0 || rc.part(bett).state != 0 || !rc.building(bett))
                 return std::string("alter Stand: Stapel nicht leer oder nicht Bauplatz");
             std::printf("  info  Bretter höchstens %d gezeigt, %d Vergleiche mit dem ganzen Bild\n", max_boards, checks);
             return checked(r);
         }},
        {"Lehmbackofen glüht, solange jemand an ihm backt; frisches Brot liegt auf dem Brett daneben",
         [] {
             Run r(53, "testort-platz", 9, 0, 60.0);
             pixelia::TestPlace* platz = test_place(r, "testort-platz");
             if (!platz) return std::string("Platz fehlt");
             const pixelia::Room& room = platz->room();
             Ledger& l = r.ledger();
             const int ofen = l.find_store("platz_backofen");
             const int part = l.store_def(ofen)->part;
             set_stock(r, "platz_backofen", "brot", 2);  // unter low: es wird gebacken
             int glow = 0, ticks = 0, made0 = static_cast<int>(booked(l, "brot", Booking::Produce));
             for (int t = 0; t < 30 * 60 * 4; ++t) {  // 4 Weltstunden im Zeitraffer 60
                 r.run(1);
                 ++ticks;
                 if (room.part(part).state == 1) {
                     ++glow;
                     // im Hintergrundbild steht die Glut (helle Glut, Grundfarbe 42) in der Öffnung
                     const pixelia::Rect ir = room.image_rect(part);
                     bool lit = false;
                     for (int y = ir.y; y < ir.y + ir.h && !lit; ++y)
                         for (int x = ir.x; x < ir.x + 24 && !lit; ++x) lit = room.background()[y * pixelia::kCoarseWidth + x] == 42;
                     if (!lit) return std::string("Glut nicht im Bild");
                 }
                 if (room.fill_shown(part) != std::min(l.qty(ofen, l.find_good("brot")), 6))
                     return std::string("Brett zeigt nicht den Bestand");
             }
             const int made = static_cast<int>(booked(l, "brot", Booking::Produce)) - made0;
             if (made < 2) return "nichts gebacken (" + std::to_string(made) + ")";
             if (glow == 0 || glow == ticks) return "Glut " + std::to_string(glow) + " von " + std::to_string(ticks) + " Schritten";
             return checked(l);
         }},
        {"Messung: Zeichenkosten der 60 Stücke (Holzstapel 40, Holzlege 12, Brotregal 8), Neuzeichnen eines Lagers (PC)",
         [] {
             char line[200];
             for (const char* place : {"testort-wiese", "testort-raum"}) {
                 Run r(52, place, 11, 0, 1.0);
                 pixelia::TestPlace* tp = test_place(r, place);
                 if (!tp) return std::string("Ort fehlt");
                 const pixelia::Room& room = tp->room();
                 const bool wiese = place[8] == 'w';
                 auto fill = [&](bool full) {
                     if (wiese) {
                         set_stock(r, "wiese_holzstapel", "brennholz", full ? 40 : 0);
                     } else {
                         set_stock(r, "raum_holzlege", "brennholz", full ? 12 : 0);
                         set_stock(r, "raum_brotregal", "brot", full ? 8 : 0);
                     }
                     r.run(1);
                 };
                 std::vector<uint8_t> frame(pixelia::kScreenPixels);
                 auto frames = [&] {
                     pixelia::Canvas c(frame.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
                     const auto t0 = std::chrono::steady_clock::now();
                     for (int k = 0; k < 400; ++k) r.stage.render(c);
                     return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 400;
                 };
                 fill(false);
                 const double empty = frames();
                 fill(true);
                 const double full = frames();
                 // Neuzeichnen: ein Stück weg und wieder dazu (Teil) gegen das ganze Bild
                 const int store = r.ledger().find_store(wiese ? "wiese_holzstapel" : "raum_holzlege");
                 const int part = r.ledger().store_def(store)->part;
                 auto ticks = [&](bool change) {
                     const auto t0 = std::chrono::steady_clock::now();
                     for (int k = 0; k < 200; ++k) {
                         if (change)
                             set_stock(r, wiese ? "wiese_holzstapel" : "raum_holzlege", "brennholz",
                                       wiese ? 39 + k % 2 : 11 + k % 2);
                         tp->tick(r.clock);
                     }
                     return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 200;
                 };
                 const double base = ticks(false);
                 const double part_us = ticks(true) - base;  // ohne den Rest des Schritts
                 const auto t1 = std::chrono::steady_clock::now();
                 for (int k = 0; k < 200; ++k) const_cast<pixelia::Room&>(room).invalidate();
                 const double all_us =
                     std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t1).count() / 200;
                 std::snprintf(line, sizeof(line),
                               "  info  %s: Bild leer %.1f µs, voll (%d Stücke) %.1f µs; Neuzeichnen Teil %.1f µs, ganz %.1f µs",
                               place + 8, empty, wiese ? 40 : 20, full, part_us, all_us);
                 std::printf("%s\n", line);
                 if (room.fill_shown(part) == 0) return std::string("keine Stücke zu sehen");
             }
             return std::string();
         }},
        {"Messung: Materialstapel (15 Stücke) und Bau-Bild des Betts, Neuzeichnen nur des Rechtecks (PC)",
         [] {
             Run r(60, "testort-kammer", 11, 0, 1.0);
             pixelia::TestPlace* tp = test_place(r, "testort-kammer");
             if (!tp) return std::string("Ort fehlt");
             pixelia::Room& room = const_cast<pixelia::Room&>(tp->room());
             auto fill = [&](bool full) {
                 set_stock(r, "kammer_baustelle", "bretter", full ? 8 : 0);
                 set_stock(r, "kammer_baustelle", "stroh", full ? 4 : 0);
                 set_stock(r, "kammer_baustelle", "naegel", full ? 6 : 0);
                 r.run(1);
             };
             std::vector<uint8_t> frame(pixelia::kScreenPixels);
             auto frames = [&] {
                 pixelia::Canvas c(frame.data(), pixelia::kScreenWidth, pixelia::kScreenHeight);
                 const auto t0 = std::chrono::steady_clock::now();
                 for (int k = 0; k < 400; ++k) r.stage.render(c);
                 return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 400;
             };
             fill(false);
             const double empty = frames();
             fill(true);
             const double full = frames();
             if (room.fill_shown(r.ledger().store_def(r.ledger().find_store("kammer_baustelle"))->part) != 15)
                 return std::string("Stapel nicht voll");
             auto per = [](auto&& fn) {
                 const auto t0 = std::chrono::steady_clock::now();
                 for (int k = 0; k < 200; ++k) fn(k);
                 return std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / 200;
             };
             const double brett = per([&](int k) { set_stock(r, "kammer_baustelle", "bretter", 7 + k % 2); tp->tick(r.clock); }) -
                                  per([&](int) { tp->tick(r.clock); });
             const double stufe = per([&](int k) { room.set_state(1, 1 + k % 3); });
             const double ganz = per([&](int) { room.invalidate(); });
             std::printf("  info  Kammer: Bild leer %.1f µs, voller Stapel %.1f µs; Neuzeichnen ein Brett %.1f µs, "
                         "Stufe des Betts %.1f µs, ganzes Bild %.1f µs\n", empty, full, brett, stufe, ganz);
             return std::string();
         }},
        {"Messung: Planer an der Takt-Grenze mit 48 Figuren an einem Ort (PC; Gerät etwa zehnmal langsamer)",
         [] {
             Run a(41, "scrolltest", 6, 0, 1.0);
             pixelia::FigureBook& book = a.stage.environment().figures;
             const int raum = pixelia::place_index("testort-raum");
             pixelia::FigureDef def{};
             def.name = "Messfigur";
             while (book.count() < pixelia::FigureBook::kMax && book.add(def, static_cast<uint8_t>(raum))) {
             }
             for (int k = 0; k < book.count(); ++k) {
                 book.at(k).where = pixelia::Whereabouts{};
                 book.at(k).where.kind = pixelia::Whereabouts::Kind::Place;
                 book.at(k).where.place = static_cast<uint8_t>(raum);
                 const uint32_t seq = book.at(k).doing.seq;
                 book.at(k).doing = pixelia::Doing{};  // ohne Tätigkeit: beim Laden irgendwo im Raum, neu entscheiden
                 book.at(k).doing.seq = seq;
             }
             // über den Speicherstand: alle stehen danach auf freien Kacheln im Raum und entscheiden neu
             const std::vector<uint8_t> s = a.save();
             if (!a.stage.load_state(s.data(), s.size())) return std::string("Laden");
             drain(a, "raum_holzlege", "brennholz", 1);
             drain(a, "raum_brotregal", "brot", 1);
             double sum = 0, worst = 0;
             int grids = 0;
             for (int t = 0; t < 30 * 600; ++t) {  // 10 Weltminuten
                 const auto t0 = std::chrono::steady_clock::now();
                 a.run(1);
                 const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
                 if (a.stage.last_grid_steps() == 0) continue;
                 sum += us;
                 worst = std::max(worst, us);
                 ++grids;
             }
             char line[160];
             std::snprintf(line, sizeof(line), "  info  %d Figuren im Raum: Schritt mit Takt-Grenze im Mittel %.1f µs, höchstens %.1f µs, %u Aufgaben",
                           book.count(), grids ? sum / grids : 0.0, worst, a.stage.environment().tasks.created());
             std::printf("%s\n", line);
             if (book.count() != pixelia::FigureBook::kMax || grids < 500) return std::string("Messung unvollständig");
             return checked(a);
         }},
    };
}

}  // namespace

int run_ledger_tests() {
    int failed = 0;
    const auto list = cases();
    for (const auto& c : list) {
        const std::string err = c.run();
        if (err.empty()) {
            std::printf("  ok    %s\n", c.name);
        } else {
            ++failed;
            std::printf("  FEHLER %s: %s\n", c.name, err.c_str());
        }
    }
    std::printf("sim: Warenbuch %d/%d bestanden\n", static_cast<int>(list.size()) - failed, static_cast<int>(list.size()));
    return failed;
}

}  // namespace sim
