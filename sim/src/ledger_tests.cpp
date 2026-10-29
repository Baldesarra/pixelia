// Prüfungen des Warenbuchs (seit 0.5.10, --test warenbuch): Buchungen, Obergrenzen, Tragen, Reservierung,
// Einzelstücke, Reichweite, Speicherstand, Herd mit Brennholz, Nachholen und Neustart. Der Nachweis für 0.5.10 laut
// Systemplan: Bilanz exakt (auch über Neustart und Nachholen bis 3 Tage), Material genau einmal, angefangene Arbeit
// geht am selben Stand weiter.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/ledger.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/png.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/stage.h"
#include "sim.h"

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

// Warenbuch als Text: Bestände je Lagerplatz und Zähler je Ware (Vergleich zweier Abläufe)
std::string ledger_text(const Ledger& l) {
    std::string t;
    char key[12], line[160];
    for (int s = 0; s < l.stores(); ++s) {
        uint8_t g[16];
        uint16_t q[16];
        const int n = l.contents(s, g, q, 16);
        for (int k = 0; k < n; ++k) {
            std::snprintf(line, sizeof(line), "%s/%s=%u(%d) ", l.store_key(s, key, sizeof(key)), l.good(g[k]).id, q[k],
                          l.reserved(s, g[k]));
            t += line;
        }
    }
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

std::string checked(Run& r) { return checked(r.ledger(), r.stage.environment().tasks); }

// Vorrat auf keep verringern (verderben), damit Aufgaben entstehen
void drain(Run& r, const char* store, const char* good, int keep) {
    Ledger& l = r.ledger();
    const int s = l.find_store(store), g = l.find_good(good);
    if (s >= 0 && g >= 0 && l.qty(s, g) > keep)
        l.take(s, g, l.qty(s, g) - keep, Booking::Spoil, at(r.clock.world_us()));
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
             const std::vector<uint8_t> s1 = a.save(3);
             Run b(99, "testort-wiese");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s1.data(), s1.size(), &rep)) return std::string("Laden: ") + rep.why;
             if (!rep.ledger || rep.stock == 0 || rep.items != 1 || rep.reservations != 1 || rep.lost || rep.moved)
                 return "Bericht: " + std::to_string(rep.stock) + " Einträge, " + std::to_string(rep.items) + " Stücke";
             const std::vector<uint8_t> s2 = b.save(3);
             if (s1 != s2) return first_byte_diff(s1, s2);
             const Ledger& m = b.ledger();
             if (m.qty(m.hand(a.stage.environment().figures.at(2).id), bretter) != 3) return std::string("Trage leer");
             if (m.reservation_left(r) != 4 || m.available(vorrat, bretter) != 5) return std::string("Reservierung weg");
             if (ledger_text(m) != ledger_text(l)) return std::string("Warenbuch nach dem Laden anders");
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
