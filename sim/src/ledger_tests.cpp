// Prüfungen des Warenbuchs (seit 0.5.10, --test warenbuch): Buchungen, Obergrenzen, Tragen, Reservierung,
// Einzelstücke, Reichweite, Speicherstand, Herd mit Brennholz, Nachholen und Neustart. Der Nachweis für 0.5.10 laut
// Systemplan: Bilanz exakt (auch über Neustart und Nachholen bis 3 Tage), Material genau einmal, angefangene Arbeit
// geht am selben Stand weiter.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/ledger.h"
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
             if (rep.moved != 6 || lm.qty(lm.find_store("platz_vorrat"), brot) != 30 || lm.total(brot) != 30)
                 return "umgelegt " + std::to_string(rep.moved) + ": " + ledger_text(lm);
             return checked(lm);
         }},
        {"Herd: Jede Füllung kommt aus der Holzlege; ohne Holz bleibt er aus, der Bedarf steht im Buch; Bilanz über einen Tag",
         [] {
             Run a(5, "testort-raum", 5, 0, 60.0);
             a.run(30 * 24 * 60);  // ein Weltag in 24 Minuten Zeitraffer
             const Ledger& l = a.ledger();
             const int lege = l.find_store("raum_holzlege"), holz = l.find_good("brennholz");
             const Ledger::GoodTotals& t = l.totals(holz);
             const int64_t burnt = t.moved[static_cast<int>(Booking::Consume)];
             if (burnt != 8 || l.qty(lege, holz) != 0) return "verbrannt " + std::to_string(burnt) + ": " + ledger_text(l);
             if (t.want <= t.got || t.got != 8) return "Bedarf " + std::to_string(t.got) + "/" + std::to_string(t.want);
             const pixelia::PlaceStates& st = a.stage.environment().states;
             const int ofen = st.find("testort-raum", "ofen");
             if (ofen < 0 || st.on(ofen)) return std::string("Herd brennt ohne Holz");
             bool empty = false;
             const pixelia::Chronicle& ch = a.stage.environment().chronicle;
             for (int i = 0; i < ch.count(); ++i) empty = empty || std::strstr(ch.at(i).text, "Holzlege leer");
             if (!empty) return std::string("Chronik ohne „Holzlege leer“");
             return checked(l);
         }},
        {"Nachholen bis 3 Tage: dasselbe Holz verbrannt wie Minute für Minute, Tätigkeiten warten bis dahin",
         [] {
             // a läuft Minute für Minute weiter, b lädt den Stand 30 Stunden später und holt nach
             Run a(17, "testort-raum", 6, 30, 600.0);
             a.run(3);
             const std::vector<uint8_t> s = a.save();
             a.run(30 * 180);  // 30 Stunden
             Run b(17);
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
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
             b.run(31);
             a.clock.set_speed(1.0);
             a.run(31);
             if (b.stage.decisions() == before) return std::string("Tätigkeiten danach nicht wieder angelaufen");
             if (consumed(a.ledger(), "brennholz") != consumed(b.ledger(), "brennholz"))
                 return "verbrannt: Minute für Minute " + std::to_string(consumed(a.ledger(), "brennholz")) +
                        ", nachgeholt " + std::to_string(consumed(b.ledger(), "brennholz"));
             const std::string x = ledger_text(a.ledger()), y = ledger_text(b.ledger());
             if (x != y) return "Minute für Minute: " + x + "\n          nachgeholt: " + y;
             return checked(b.ledger());
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
