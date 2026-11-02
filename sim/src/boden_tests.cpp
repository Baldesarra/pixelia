// Prüfungen zu 0.5.11 „Fester Boden“ (--test boden): die Befunde der Code-Prüfungen vom 31.10. und 1.11.2026 (andere
// KI, Berichte außerhalb des Repos) als normale Tests mit der richtigen Erwartung. Die Gegenproben der Prüfung zeigten
// den Fehler; hier muss er weg sein. Teil a: Simulation und Warenbuch (V01–V05, B12–B15, B17, B18, P-F).

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/econ_log.h"
#include "pixelia/engine/events.h"
#include "pixelia/engine/figure.h"
#include "pixelia/engine/ledger.h"
#include "pixelia/engine/place.h"
#include "pixelia/engine/projects.h"
#include "pixelia/engine/save.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/tasks.h"
#include "sim.h"

namespace sim {
namespace {

using pixelia::Booking;
using pixelia::EconomyDef;
using pixelia::EventBus;
using pixelia::GoodDef;
using pixelia::Ledger;
using pixelia::SaveReader;
using pixelia::SaveWriter;
using pixelia::StoreDef;
using pixelia::StoreKind;
using pixelia::SupplyWait;
using pixelia::Task;
using pixelia::TaskKind;
using pixelia::TaskList;
using pixelia::WorldEvent;

struct Case {
    const char* name;
    std::function<std::string()> run;
};

std::string checked(const Ledger& l) {
    char buf[200];
    const char* e = l.check(buf, sizeof(buf));
    return e ? std::string(e) : std::string();
}

pixelia::BookingInfo at(int64_t us) {
    pixelia::BookingInfo i;
    i.at_us = us;
    i.reason = "test";
    return i;
}

// Bühne mit dem Testdorf (wie in ledger_tests.cpp)
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
    pixelia::Environment& env() { return stage.environment(); }
    std::vector<uint8_t> save(uint32_t gen = 1) const {
        std::vector<uint8_t> b(96 * 1024);
        b.resize(stage.save_state(b.data(), b.size(), gen));
        return b;
    }
};

// eigene Wirtschaft mit 96 Waren und fünf Lagern (wie die Gegenproben der Prüfung)
struct Wide {
    std::array<std::string, Ledger::kMaxGoods> names;
    std::array<GoodDef, Ledger::kMaxGoods> goods{};
    std::array<StoreDef, 5> stores{};
    EconomyDef def;
    Ledger ledger;
    explicit Wide(bool unique = false, uint16_t cap = 1000, int good_count = Ledger::kMaxGoods) {
        for (int i = 0; i < good_count; ++i) {
            names[i] = "w" + std::to_string(i);
            goods[i] = GoodDef{names[i].c_str(), names[i].c_str()};
        }
        goods[0].unique = unique;
        static const char* const kIds[] = {"s0", "s1", "s2", "s3", "s4"};
        for (int s = 0; s < 5; ++s) stores[s] = StoreDef{kIds[s], kIds[s], "testort-wiese", StoreKind::Community, cap};
        def.goods = goods.data();
        def.good_count = static_cast<uint8_t>(good_count);
        def.stores = stores.data();
        def.store_count = 5;
        const EconomyDef* defs[] = {&def};
        ledger.build(defs, 1);
    }
};

// Warenbuch als Speicherstand-Abschnitte; resv ersetzt den Abschnitt RESV (eigene, auch fehlerhafte Daten)
std::vector<uint8_t> ledger_bytes(const Ledger& l, const std::function<void(SaveWriter&)>& resv = nullptr) {
    std::vector<uint8_t> b(96 * 1024);
    SaveWriter w(b.data(), b.size());
    w.begin(1, 0);
    w.chunk("WARE", 1);
    l.save_goods(w);
    w.chunk("LAGR", 1);
    l.save_stock(w);
    w.chunk("STCK", 1);
    l.save_items(w);
    w.chunk("RESV", 1);
    if (resv)
        resv(w);
    else
        l.save_reservations(w);
    b.resize(w.finish());
    return b;
}

Ledger::LoadStats load_into(Ledger& l, const std::vector<uint8_t>& b) {
    l.clear();
    Ledger::LoadStats st;
    pixelia::SaveChunks c(b.data(), b.size());
    char tag[5];
    uint16_t v = 0;
    SaveReader d(nullptr, 0);
    while (c.next(tag, &v, &d)) {
        if (std::strcmp(tag, "WARE") == 0) l.load_goods(d, v, &st);
        if (std::strcmp(tag, "LAGR") == 0) l.load_stock(d, v, &st);
        if (std::strcmp(tag, "STCK") == 0) l.load_items(d, v, &st);
        if (std::strcmp(tag, "RESV") == 0) l.load_reservations(d, v, &st);
    }
    l.finish_load(&st);
    return st;
}

// Dorf mit den Orten des Testdorfs (Wege: Raum–Wiese, Raum–Kammer, Wiese–Platz) und eigenen Lagern
struct Village {
    GoodDef goods[2] = {{"holz", "Holz"}, {"brot", "Brot"}};
    pixelia::StockRule rule[1] = {{"holz", 4, 10}};
    StoreDef stores[7] = {
        {"haus", "Haus", "testort-raum", StoreKind::House, 20, -1, rule, 1},
        {"k_platz", "Kiste Platz", "testort-platz", StoreKind::Community, 100},
        {"k_wiese", "Kiste Wiese", "testort-wiese", StoreKind::Community, 100},
        {"k_kammer", "Kiste Kammer", "testort-kammer", StoreKind::Community, 100},
        {"e_wiese", "Holzplatz", "testort-wiese", StoreKind::Producer, 100},
        {"haus2", "Haus 2", "testort-raum", StoreKind::House, 20, -1, rule, 1, "k_platz"},  // Quelle eingetragen
        {"site", "Baustelle", "testort-kammer", StoreKind::Site, 100},
    };
    pixelia::ProjectNeed needs[1] = {{"holz", 6}};
    pixelia::ProjectStepDef steps[1] = {{"stapeln", "Holz stapeln", "Stapeln", needs, 1, pixelia::Skill::Joinery,
                                         pixelia::SkillLevel::Unknown, 10}};
    pixelia::ProjectDef projects[1] = {{"stapel", "Stapel bauen", pixelia::ProjectKind::Furniture, "testort-kammer",
                                        "site", nullptr, -1, steps, 1, false, true}};
    EconomyDef def{goods, 2, stores, 7, nullptr, 0, projects, 1};
    Ledger l;
    pixelia::FigureBook book;
    TaskList tasks;
    int64_t now = 0;
    Village() {
        const EconomyDef* defs[] = {&def};
        l.build(defs, 1);
    }
    int store(const char* id) const { return l.find_store(id); }
    void put(const char* id, int n) { l.add(store(id), l.find_good("holz"), n, Booking::Produce, at(now)); }
    void step() {
        now += pixelia::Stage::kGridUs;
        tasks.step(now, 0, l, book, nullptr);
    }
    // Quelle der Holen-Aufgabe zu diesem Lager, −1 = keine
    int fetch_from(const char* to) const {
        for (int i = 0; i < tasks.count(); ++i)
            if (tasks.at(i).kind == TaskKind::Fetch && tasks.at(i).to == store(to)) return tasks.at(i).from;
        return -1;
    }
    std::string name(int s) const { return s >= 0 ? l.store_def(s)->id : "keine"; }
};

// vier Orte für die Wege (B15): A–B direkt über eine Brücke (erst nach dem Bau), Umweg A–C–D–B mit einem Steg in C–D
using pixelia::ExitKind;
using pixelia::Facing;
using pixelia::Transition;
constexpr pixelia::Exit kRA[] = {
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-b", 0, Transition::Fade, 0, pixelia::kOpenAfterBuild, "bruecke"},
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-c", 0, Transition::Fade}};
constexpr pixelia::Exit kRB[] = {
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-a", 0, Transition::Fade, 0, pixelia::kOpenAfterBuild, "bruecke"},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-d", 1, Transition::Fade}};
constexpr pixelia::Exit kRC[] = {
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-a", 1, Transition::Fade},
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-d", 0, Transition::Fade, 0, pixelia::kOpenAfterBuild, "steg"}};
constexpr pixelia::Exit kRD[] = {
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "b15-c", 1, Transition::Fade, 0, pixelia::kOpenAfterBuild, "steg"},
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Path, "b15-b", 1, Transition::Fade}};
constexpr pixelia::RoomDef kRoomA = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRA, 2};
constexpr pixelia::RoomDef kRoomB = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRB, 2};
constexpr pixelia::RoomDef kRoomC = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRC, 2};
constexpr pixelia::RoomDef kRoomD = {{nullptr, nullptr, nullptr}, "", {0, 0, 0}, nullptr, 0, kRD, 2};
const pixelia::PlaceDef kPA = {"b15-a", "A", pixelia::kPlaceOutdoor, &kRoomA, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kPB = {"b15-b", "B", pixelia::kPlaceOutdoor, &kRoomB, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kPC = {"b15-c", "C", pixelia::kPlaceOutdoor, &kRoomC, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};
const pixelia::PlaceDef kPD = {"b15-d", "D", pixelia::kPlaceOutdoor, &kRoomD, nullptr, 0, false,
                               {256, 256, 256}, 256, {0, 0, 0}, pixelia::GlassRole::None, nullptr, 0, nullptr, 0};

// Bestände nach Kennung von Lagerplatz und Ware (Tragen entstehen nach dem Laden in anderer Reihenfolge)
std::string stock_text(const Ledger& l) {
    std::vector<std::string> rows;
    char key[12], line[96];
    for (int s = 0; s < l.stores(); ++s)
        for (int g = 0; g < l.goods(); ++g)
            if (l.qty(s, g) || l.reserved(s, g)) {
                std::snprintf(line, sizeof(line), "%s/%s=%d(%d)", l.store_key(s, key, sizeof(key)), l.good(g).id,
                              l.qty(s, g), l.reserved(s, g));
                rows.push_back(line);
            }
    std::sort(rows.begin(), rows.end());
    std::string out;
    for (const std::string& r : rows) out += r + " ";
    return out;
}

// 48 Figuren (FigureBook::kMax) an einem Ort, über den Speicherstand auf freie Kacheln gesetzt
std::string fill_figures(Run& a, const char* place) {
    pixelia::FigureBook& book = a.env().figures;
    const int p = pixelia::place_index(place);
    pixelia::FigureDef def{};
    def.name = "Messfigur";
    while (book.count() < pixelia::FigureBook::kMax && book.add(def, static_cast<uint8_t>(p))) {
    }
    for (int k = 0; k < book.count(); ++k) {
        book.at(k).where = pixelia::Whereabouts{};
        book.at(k).where.kind = pixelia::Whereabouts::Kind::Place;
        book.at(k).where.place = static_cast<uint8_t>(p);
        const uint32_t seq = book.at(k).doing.seq;
        book.at(k).doing = pixelia::Doing{};
        book.at(k).doing.seq = seq;
    }
    const std::vector<uint8_t> s = a.save();
    if (!a.stage.load_state(s.data(), s.size())) return "Laden";
    return book.count() == pixelia::FigureBook::kMax ? std::string() : std::string("keine 48 Figuren");
}

std::vector<Case> cases() {
    return {
        // --- V01 Ereignis-Bus ---
        {"V01 Ereignis-Bus: läuft der Ring über, liest niemand eine Meldung vor ihrer Zustellung (Gegenprobe der Prüfung)",
         [] {
             EventBus bus;
             const int rd = bus.subscribe(pixelia::kAllEvents);
             const int q = EventBus::kQueue;
             for (int i = 0; i < q; ++i) {
                 WorldEvent e;
                 e.who = static_cast<uint16_t>(i + 1);
                 bus.publish(e);
             }
             bus.deliver();
             WorldEvent future;
             future.who = 999;
             bus.publish(future);
             std::vector<int> got;
             for (WorldEvent e; bus.poll(rd, &e);) got.push_back(e.who);
             if (std::find(got.begin(), got.end(), 999) != got.end()) return std::string("vor der Zustellung gelesen");
             if (static_cast<int>(got.size()) != q - 1 || got.front() != 2 || got.back() != q || bus.lost() != 1)
                 return "gelesen " + std::to_string(got.size()) + ", verloren " + std::to_string(bus.lost());
             bus.deliver();
             WorldEvent e;
             if (!bus.poll(rd, &e) || e.who != 999) return std::string("nach der Zustellung nicht gelesen");
             if (bus.poll(rd, &e)) return std::string("doppelt gelesen");
             return std::string();
         }},
        {"V01 Ereignis-Bus: Ring knapp unter, genau und über voll, Überlauf vor der Zustellung; Verluste genau gezählt",
         [] {
             const int q = EventBus::kQueue;
             for (const int c : {q - 1, q, q + 1, 2 * q + 5}) {
                 EventBus bus;
                 const int rd = bus.subscribe(pixelia::kAllEvents);
                 for (int i = 0; i < c; ++i) {
                     WorldEvent e;
                     e.who = static_cast<uint16_t>(i + 1);
                     bus.publish(e);
                 }
                 bus.deliver();
                 int n = 0, first = 0, last = 0;
                 for (WorldEvent e; bus.poll(rd, &e);) {
                     if (!n) first = e.who;
                     if (n && e.who != last + 1) return "Lücke bei " + std::to_string(c) + " Meldungen";
                     last = e.who;
                     ++n;
                 }
                 if (n != std::min(c, q) || first != std::max(1, c - q + 1) ||
                     static_cast<int>(bus.lost()) != std::max(0, c - q))
                     return std::to_string(c) + " Meldungen: " + std::to_string(n) + " gelesen ab " + std::to_string(first) +
                            ", verloren " + std::to_string(bus.lost());
             }
             EventBus bus;
             const int rd = bus.subscribe(pixelia::kAllEvents);
             for (int i = 0; i < q + 5; ++i) {
                 WorldEvent e;
                 e.who = static_cast<uint16_t>(i + 1);
                 bus.publish(e);
             }
             WorldEvent e;
             if (bus.poll(rd, &e)) return std::string("Überlauf vor der Zustellung: trotzdem gelesen");
             bus.deliver();
             int n = 0, first = 0;
             while (bus.poll(rd, &e)) first = n++ ? first : e.who;
             if (n != q || first != 6 || bus.lost() != 5)
                 return "nach dem Überlauf: " + std::to_string(n) + " ab " + std::to_string(first) + ", verloren " +
                        std::to_string(bus.lost());
             return std::string();
         }},
        {"V01 Ereignis-Bus: drei verschieden schnelle Leser, Meldungen auch zwischen ihren Abfragen; jede genau einmal gelesen oder verloren",
         [] {
             EventBus bus;
             int rd[3];
             for (int& r : rd) r = bus.subscribe(pixelia::kAllEvents);
             const int every[3] = {1, 5, 17};
             int published = 0, delivered = 0, last[3] = {0, 0, 0};
             long reads = 0;
             std::string err;
             const auto drain = [&](int i) {
                 for (WorldEvent e; bus.poll(rd[i], &e);) {
                     if (e.who <= last[i] || e.who > delivered) err = "Leser " + std::to_string(i) + ": Meldung " +
                                                                      std::to_string(e.who) + " zur falschen Zeit";
                     last[i] = e.who;
                     ++reads;
                 }
             };
             for (int s = 0; s < 400 && err.empty(); ++s) {
                 const int n = (s * 7) % 23;
                 for (int k = 0; k < n; ++k) {
                     WorldEvent e;
                     e.who = static_cast<uint16_t>(++published);
                     bus.publish(e);
                     if (k == n / 2) drain(0);  // mitten im Schritt: sieht nur Zugestelltes
                 }
                 bus.deliver();
                 delivered = published;
                 for (int i = 0; i < 3; ++i)
                     if (s % every[i] == 0) drain(i);
             }
             bus.deliver();
             delivered = published;
             for (int i = 0; i < 3; ++i) drain(i);
             if (!err.empty()) return err;
             if (reads + static_cast<long>(bus.lost()) != 3L * published)
                 return std::to_string(reads) + " gelesen + " + std::to_string(bus.lost()) + " verloren ≠ 3 × " +
                        std::to_string(published);
             return bus.lost() > 0 ? std::string() : std::string("langsamer Leser hat nichts verloren (Prüfung zu schwach)");
         }},
        {"V01 48 Figuren im Zeitraffer 3600: kein Ereignis verloren, die Chronik liest an jeder Takt-Grenze",
         [] {
             Run a(51, "scrolltest", 7, 0, 1.0);
             if (std::string e = fill_figures(a, "testort-wiese"); !e.empty()) return e;
             a.clock.set_speed(3600.0);
             const uint32_t before = a.env().events.published();
             for (int t = 0; t < 30 * 20; ++t) {  // 20 Weltstunden, je Schritt 120 Takt-Grenzen
                 a.run(1);
                 for (WorldEvent e; a.stage.poll_event(&e);) {
                 }
             }
             const pixelia::EventBus& bus = a.env().events;
             std::printf("  info  48 Figuren, 20 Weltstunden: %u Meldungen, höchstens %u zwischen zwei Zustellungen (Platz %d)\n",
                         bus.published() - before, bus.peak(), EventBus::kQueue);
             if (bus.lost()) return "verloren: " + std::to_string(bus.lost());
             if (bus.peak() * 2 > static_cast<uint32_t>(EventBus::kQueue)) return std::string("weniger als die Hälfte Reserve");
             return checked(a.env().ledger);
         }},
        // --- V02 Reservierungsnummern ---
        {"V02 Reservierungen: nach dem Umlauf der Nummern nie eine belegte; Speichern und Laden danach behält die Zuordnung",
         [] {
             Wide f;
             Ledger& l = f.ledger;
             l.add(0, 0, 10, Booking::Produce, {});
             l.add(1, 0, 10, Booking::Produce, {});
             const uint16_t old = l.reserve(0, 0, 1, 10);
             for (int i = 0; i < 65534; ++i) {
                 const uint16_t r = l.reserve(1, 0, 1, 20);
                 if (!r || r == old) return "belegte oder keine Kennung bei " + std::to_string(i);
                 l.release(r);
             }
             const uint16_t next = l.reserve(1, 0, 2, 30);
             if (!next || next == old) return std::string("nach dem Umlauf die belegte Kennung");
             if (l.reservation_left(next) != 2 || l.reservation_left(old) != 1) return std::string("falsche Mengen");
             Wide g;
             const Ledger::LoadStats st = load_into(g.ledger, ledger_bytes(l));
             if (st.rejected || g.ledger.reservation_left(old) != 1 || g.ledger.reservation_left(next) != 2)
                 return std::string("nach dem Laden andere Zuordnung");
             if (std::string e = checked(g.ledger); !e.empty()) return "geladen: " + e;
             l.release(next);
             if (l.reserved(0, 0) != 1 || l.reserved(1, 0) != 0) return std::string("falsche Reservierung freigegeben");
             return checked(l);
         }},
        {"V02 Reservierungen: eine doppelte Kennung im Speicherstand wird abgelehnt; die Bilanz-Prüfung bleibt sauber",
         [] {
             Wide f;
             f.ledger.add(0, 0, 5, Booking::Produce, {});
             const std::vector<uint8_t> b = ledger_bytes(f.ledger, [](SaveWriter& w) {
                 w.u16(8);
                 w.u16(2);
                 for (int k = 0; k < 2; ++k) {
                     const size_t r = w.record_begin();
                     w.u16(7);
                     w.u16(static_cast<uint16_t>(40 + k));
                     w.str("s0");
                     w.str("w0");
                     w.u16(1);
                     w.record_end(r);
                 }
             });
             Wide g;
             const Ledger::LoadStats st = load_into(g.ledger, b);
             if (st.rejected != 1 || st.reservations != 1) return "übernommen " + std::to_string(st.reservations) +
                                                                   ", abgelehnt " + std::to_string(st.rejected);
             if (g.ledger.reservation_left(7) != 1 || g.ledger.reserved(0, 0) != 1) return std::string("Menge falsch");
             return checked(g.ledger);
         }},
        // --- V03 Bestandstabelle ---
        {"V03 Bestandstabelle: 384 einmal benutzte, leere Einträge werden an der Takt-Grenze frei; ein neues Lager nimmt Ware an (Gegenprobe)",
         [] {
             Wide f;
             Ledger& l = f.ledger;
             l.step(0, 0, nullptr);
             for (int s = 0; s < 4; ++s)
                 for (int g = 0; g < Ledger::kMaxGoods; ++g)
                     if (l.add(s, g, 1, Booking::Produce, {}) != 1 || l.take(s, g, 1, Booking::Consume, {}) != 1)
                         return std::string("Buchung");
             if (l.entries() != Ledger::kMaxStock) return "belegt " + std::to_string(l.entries());
             l.step(2 * pixelia::WorldClock::kUsPerDay, 2, nullptr);
             if (l.entries() != 0 || l.entries_freed() != Ledger::kMaxStock)
                 return "nach zwei Tagen belegt " + std::to_string(l.entries());
             if (l.add(4, 0, 1, Booking::Produce, {}) != 1) return std::string("neues Lager nimmt nichts an");
             return checked(l);
         }},
        {"V03 Bestandstabelle: Verbrauch der letzten 24 Stunden, Reservierung und Gemeldetes halten einen leeren Eintrag",
         [] {
             Wide f;
             Ledger& l = f.ledger;
             const int64_t h = 60 * pixelia::WorldClock::kUsPerMinute;
             l.step(0, 0, nullptr);
             l.add(0, 0, 5, Booking::Produce, at(h));
             l.take(0, 0, 5, Booking::Consume, at(h));
             l.add(1, 1, 3, Booking::Produce, at(h));
             const uint16_t r = l.reserve(1, 1, 3, 9);
             l.step(2 * h, 0, nullptr);
             if (l.out_24h(0, 0) != 5 || l.use_24h(0) != 5 || l.entries() != 2) return std::string("zu früh freigegeben");
             l.step(26 * h, 1, nullptr);
             if (l.out_24h(0, 0) != 0 || l.entries() != 1) return "nach 25 Stunden belegt " + std::to_string(l.entries());
             if (l.reservation_left(r) != 3 || l.total(1) != 3) return std::string("Reservierung oder Summe verändert");
             return checked(l);
         }},
        {"V03 Testdorf 30 Weltstunden: an der Takt-Grenze dieselbe Tabelle wie nach einem Neustart, danach dasselbe Warenbuch",
         [] {
             Run a(31, "testort-raum", 6, 0, 600.0);
             a.run(30 * 180);  // 30 Weltstunden
             const int64_t g = pixelia::Stage::kGridUs;
             a.clock.set_world_us((a.clock.world_us() / g + 1) * g);
             a.stage.tick(a.clock);
             const std::vector<uint8_t> s = a.save();
             Run b(31, "testort-raum");
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             b.clock.set_world_us(a.clock.world_us());
             b.clock.set_speed(600.0);
             if (a.env().ledger.entries() != b.env().ledger.entries())
                 return "Einträge " + std::to_string(a.env().ledger.entries()) + " ohne, " +
                        std::to_string(b.env().ledger.entries()) + " mit Neustart";
             std::printf("  info  Testdorf nach 30 Weltstunden: %d Einträge belegt, %u wieder frei geworden\n",
                         a.env().ledger.entries(), a.env().ledger.entries_freed());
             a.run(30 * 60);
             b.run(30 * 60);
             for (Run* r : {&a, &b}) {
                 r->clock.set_world_us((r->clock.world_us() / g + 1) * g);
                 r->stage.tick(r->clock);
             }
             const Ledger &x = a.env().ledger, &y = b.env().ledger;
             if (x.entries() != y.entries()) return std::string("Einträge weichen nach weiteren 10 Stunden ab");
             const std::string dx = stock_text(x), dy = stock_text(y);
             if (dx != dy) return "ohne Neustart: " + dx + " | mit Neustart: " + dy;
             return checked(y);
         }},
        // --- V04 Einzelstück-Transport ---
        {"V04 Einzelstück: Transport scheitert an voller Tabelle, vollem oder ungültigem Ziel und ändert nichts (Gegenprobe)",
         [] {
             Wide f(true);
             Ledger& l = f.ledger;
             for (int s = 0; s < 4; ++s)
                 for (int g = 0; g < Ledger::kMaxGoods; ++g)
                     if (l.add(s, g, 1, Booking::Produce, at(s * 100 + g)) != 1) return std::string("Buchung");
             const uint16_t res = l.reserve(0, 0, 1, 10);
             const uint32_t v = l.version();
             const auto same = [&]() -> std::string {
                 if (l.qty(0, 0) != 1 || l.qty(4, 0) != 0 || l.reservation_left(res) != 1 || l.reserved(0, 0) != 1)
                     return "Menge oder Reservierung verändert";
                 int at0 = 0, at4 = 0;
                 for (int i = 0; i < l.items(); ++i) {
                     at0 += l.item_at(i).good == 0 && l.item_at(i).store == 0;
                     at4 += l.item_at(i).good == 0 && l.item_at(i).store == 4;
                 }
                 if (at0 != 1 || at4 != 0) return "Einzelstück gewandert";
                 return l.version() == v ? checked(l) : std::string("Buchungszähler verändert");
             };
             if (l.move(0, 4, 0, 1, {}, res) != 0) return std::string("Tabelle voll: trotzdem bewegt");
             if (std::string e = same(); !e.empty()) return "Tabelle voll: " + e;
             if (l.move(0, 99, 0, 1, {}, res) != 0) return std::string("ungültiges Ziel: bewegt");
             if (std::string e = same(); !e.empty()) return "ungültiges Ziel: " + e;
             Wide s(true, 2);
             Ledger& m = s.ledger;
             m.add(0, 0, 1, Booking::Produce, at(1));
             m.add(1, 1, 2, Booking::Produce, at(2));  // Ziel voll
             const uint16_t r2 = m.reserve(0, 0, 1, 10);
             if (m.move(0, 1, 0, 1, {}, r2) != 0 || m.reservation_left(r2) != 1 || m.reserved(0, 0) != 1)
                 return std::string("Ziel voll: Reservierung verändert");
             return checked(m);
         }},
        {"V04 Einzelstück: ein Teil gelingt, verbraucht genau so viel Reservierung; Abbuchen ebenso",
         [] {
             Wide f(true, 3);
             Ledger& l = f.ledger;
             l.add(0, 0, 3, Booking::Produce, at(1));
             l.add(1, 1, 2, Booking::Produce, at(2));  // Ziel hat Platz für eins
             const uint16_t res = l.reserve(0, 0, 2, 10);
             if (l.move(0, 1, 0, 3, {}, res) != 1) return std::string("nicht genau eins bewegt");
             if (l.reservation_left(res) != 1 || l.reserved(0, 0) != 1 || l.qty(0, 0) != 2 || l.qty(1, 0) != 1)
                 return std::string("Reservierung passt nicht zur Teilmenge");
             if (l.take(0, 0, 1, Booking::Consume, {}, res) != 1 || l.reservation_left(res) != 0 || l.reserved(0, 0) != 0)
                 return std::string("Abbuchen mit Reservierung");
             if (l.take(0, 0, 5, Booking::Consume, {}) != 1 || l.qty(0, 0) != 0) return std::string("freies Stück");
             return checked(l);
         }},
        // --- V05 Nachschub ohne Quelle, P-F Gründe ---
        {"V05 Ohne Quelle wählt Pixelia: nächstes Lager mit genug Vorrat, Gleichstand nach Kennung, Erzeuger zählen mit",
         [] {
             struct Want {
                 std::vector<std::pair<const char*, int>> stock;
                 const char* expect;
             };
             const Want cases[] = {
                 {{{"k_platz", 20}}, "k_platz"},                       // die einzige (zwei Wege weit)
                 {{{"k_platz", 20}, {"k_wiese", 20}}, "k_wiese"},       // die nähere
                 {{{"k_wiese", 20}, {"k_kammer", 20}}, "k_kammer"},     // gleich weit: kleinere Kennung
                 {{{"k_wiese", 20}, {"e_wiese", 20}}, "e_wiese"},       // Erzeuger zählt mit
                 {{{"k_wiese", 2}, {"k_platz", 20}}, "k_platz"},        // weniger als die halbe Ladung: lieber weiter
                 {{}, nullptr},                                         // nirgends Vorrat
             };
             for (const Want& w : cases) {
                 Village v;
                 for (const auto& [id, n] : w.stock) v.put(id, n);
                 v.step();
                 const int got = v.fetch_from("haus");
                 if (got != (w.expect ? v.store(w.expect) : -1))
                     return std::string("erwartet ") + (w.expect ? w.expect : "keine") + ", gewählt " + v.name(got);
                 if (!w.expect && v.tasks.supply_wait(v.store("haus")) != SupplyWait::NoSource)
                     return std::string("Grund fehlt: keine Quelle");
                 if (std::string e = checked(v.l); !e.empty()) return e;
             }
             return std::string();
         }},
        {"V05 Eingetragene Quelle bleibt verbindlich: ist sie leer, wird gewartet (auch wenn anderswo Vorrat liegt); Projekte wie Aufgaben",
         [] {
             Village v;
             v.put("k_wiese", 20);
             v.step();
             if (v.fetch_from("haus2") >= 0) return std::string("leere eingetragene Quelle umgangen");
             if (v.tasks.supply_wait(v.store("haus2")) != SupplyWait::SourceEmpty) return std::string("Grund: Quelle leer");
             if (v.fetch_from("haus") != v.store("k_wiese")) return std::string("ohne Quelle nicht gewählt");
             // Projekt ohne Quelle: dieselbe Wahl (Kammer: Wiese zwei Wege, Platz drei)
             Village p;
             p.put("k_platz", 20);
             p.put("k_wiese", 20);
             pixelia::ProjectList pl;
             const EconomyDef* defs[] = {&p.def};
             pl.build(defs, 1, p.l);
             pl.start_new();
             p.now += pixelia::Stage::kGridUs;
             pl.step(p.now, 0, p.l, p.tasks, p.book, nullptr, false);
             if (pl.count() != 1) return std::string("Projekt nicht begonnen");
             if (p.fetch_from("site") != p.store("k_wiese")) return "Projekt holt von " + p.name(p.fetch_from("site"));
             return checked(p.l);
         }},
        {"P-F Gründe ohne Nachschub: Quelle leer, lohnt noch nicht, keine Quelle; Zähler je Takt-Grenze; kommt Vorrat, geht es weiter",
         [] {
             Village v;
             v.put("haus", 3);  // unter low (4), aber nicht leer
             v.put("k_wiese", 2);
             v.step();
             v.step();
             if (v.tasks.supply_wait(v.store("haus2")) != SupplyWait::SourceEmpty ||
                 v.tasks.supply_wait(v.store("haus")) != SupplyWait::NotWorth)
                 return std::string("Gründe: ") + pixelia::supply_wait_id(v.tasks.supply_wait(v.store("haus2"))) + ", " +
                        pixelia::supply_wait_id(v.tasks.supply_wait(v.store("haus")));
             if (v.tasks.supply_waits(SupplyWait::SourceEmpty) != 2 || v.tasks.supply_waits(SupplyWait::NotWorth) != 2)
                 return std::string("Zähler je Takt-Grenze");
             int good = -1;
             v.tasks.supply_wait(v.store("haus2"), &good);
             if (good != v.l.find_good("holz")) return std::string("Ware zum Grund fehlt");
             v.put("k_platz", 30);
             v.step();
             if (v.fetch_from("haus2") != v.store("k_platz") || v.fetch_from("haus") != v.store("k_platz"))
                 return std::string("nach dem Vorrat kein Nachschub");
             if (v.tasks.supply_wait(v.store("haus2")) != SupplyWait::None) return std::string("Grund bleibt stehen");
             return checked(v.l);
         }},
        {"Daten prüfen: eine unbekannte Quelle oder Ware in den Lagerplätzen wird gemeldet; das Testdorf ist sauber",
         [] {
             Village v;
             char buf[160];
             if (const char* e = v.l.data_problem(buf, sizeof(buf))) return std::string("ohne Fehler gemeldet: ") + e;
             Village bad;
             bad.stores[5].source = "k_plat";
             const EconomyDef* defs[] = {&bad.def};
             bad.l.build(defs, 1);
             const char* e = bad.l.data_problem(buf, sizeof(buf));
             if (!e || !std::strstr(e, "haus2") || !std::strstr(e, "k_plat")) return std::string("Tippfehler nicht gemeldet");
             Run a(5);
             if (const char* x = a.env().ledger.data_problem(buf, sizeof(buf))) return std::string("Testdorf: ") + x;
             if (const char* x = a.env().projects.data_problem(buf, sizeof(buf))) return std::string("Testdorf: ") + x;
             return std::string();
         }},
        // --- B12, B13 ---
        {"B12 Tagesprotokoll: ein Lager mit 25, 30 und 96 Waren steht vollständig in bestand_lager_tag",
         [] {
             for (const int n : {25, 30, Ledger::kMaxGoods}) {
                 Wide f(false, 1000, n);
                 pixelia::Environment env;
                 const EconomyDef* defs[] = {&f.def};
                 env.ledger.build(defs, 1);
                 for (int g = 0; g < n; ++g) env.ledger.add(0, g, 1 + g % 5, Booking::Produce, {});
                 pixelia::EconomyLog log;
                 if (!log.attach(env, 128 * 1024, "simulator")) return std::string("Puffer");
                 const std::filesystem::path dir = std::filesystem::path(".logs") / "test-boden-b12";
                 std::error_code ec;
                 std::filesystem::remove_all(dir, ec);
                 std::filesystem::create_directories(dir, ec);
                 log.day_rows(3);
                 log.cut();
                 if (!log.flush(dir.string().c_str())) return std::string("Anhängen");
                 std::ifstream in(dir / "t00003-bestand_lager_tag.csv");
                 int rows = 0, sum = 0;
                 for (std::string line; std::getline(in, line);) {
                     if (line.rfind("3,s0,", 0) != 0) continue;
                     ++rows;
                     sum += std::atoi(line.c_str() + line.find(',', 5) + 1);
                 }
                 int want = 0;
                 for (int g = 0; g < n; ++g) want += 1 + g % 5;
                 if (rows != n || sum != want)
                     return std::to_string(n) + " Waren: " + std::to_string(rows) + " Zeilen, Summe " + std::to_string(sum);
             }
             return std::string();
         }},
        {"B13 Laden: Lager, die es nicht mehr gibt, landen im Gemeinschaftslager nur, soweit Platz ist; der eigene Bestand zuerst; Bilanz exakt",
         [] {
             const GoodDef goods[] = {{"holz", "Holz"}};
             const StoreDef before[] = {{"neu", "Neu", "testort-platz", StoreKind::Community, 100},
                                        {"alt1", "Alt 1", "testort-platz", StoreKind::Community, 60000},
                                        {"alt2", "Alt 2", "testort-platz", StoreKind::Community, 60000}};
             const EconomyDef da{goods, 1, before, 3};
             const EconomyDef db{goods, 1, before, 1};  // nur noch „neu“
             Ledger a, b;
             const EconomyDef* pa[] = {&da};
             const EconomyDef* pb[] = {&db};
             a.build(pa, 1);
             b.build(pb, 1);
             a.add(1, 0, 40000, Booking::Produce, {});
             a.add(2, 0, 40000, Booking::Produce, {});
             a.add(0, 0, 30, Booking::Produce, {});
             const Ledger::LoadStats st = load_into(b, ledger_bytes(a));
             if (b.qty(0, 0) != 100 || b.total(0) != 100) return "im Lager " + std::to_string(b.qty(0, 0));
             if (st.lost != 80030 - 100 || st.moved != 70)
                 return "verloren " + std::to_string(st.lost) + ", umgelegt " + std::to_string(st.moved);
             return checked(b);
         }},
        // --- B14 Reisen ---
        {"B14 48 Figuren brechen an derselben Takt-Grenze über den Feldweg auf: keine kommt vor der Zeit an, jede genau einmal, auch mit Neustart",
         [] {
             Run a(61, "scrolltest", 10, 0, 1.0);
             if (std::string e = fill_figures(a, "testort-wiese"); !e.empty()) return e;
             pixelia::FigureBook& book = a.env().figures;
             const int platz = pixelia::place_index("testort-platz");
             const int64_t g = pixelia::Stage::kGridUs, go = (a.clock.world_us() / g + 2) * g;
             for (int k = 0; k < book.count(); ++k) {
                 pixelia::Doing& d = book.at(k).doing;
                 d.kind = pixelia::DoingKind::Leave;
                 d.exit = 2;  // Feldweg nach dem Platz, 30 Weltsekunden
                 d.walk_us = go;
                 d.end_us = go;
             }
             const int rd = a.env().events.subscribe(pixelia::event_bit(pixelia::EventKind::Arrived));
             while (a.clock.world_us() < go + 2 * g) a.run(1);
             if (a.stage.in_transit() != pixelia::FigureBook::kMax)
                 return "unterwegs " + std::to_string(a.stage.in_transit()) + " von 48";
             const std::vector<uint8_t> s = a.save();
             Run b(61, "scrolltest");
             pixelia::Stage::LoadReport rep;
             if (!b.stage.load_state(s.data(), s.size(), &rep) || rep.transit != pixelia::FigureBook::kMax)
                 return "nach dem Laden unterwegs " + std::to_string(rep.transit);
             b.clock.set_world_us(a.clock.world_us());
             const int rb = b.env().events.subscribe(pixelia::event_bit(pixelia::EventKind::Arrived));
             int arrived[2] = {0, 0};
             const auto early = [&](Run& r) {
                 for (int k = 0; k < r.env().figures.count(); ++k) {
                     const pixelia::FigureRecord& f = r.env().figures.at(k);
                     if (f.where.kind == pixelia::Whereabouts::Kind::Place && f.where.place == platz &&
                         r.clock.world_us() < go + 30 * g)
                         return true;
                 }
                 return false;
             };
             while (a.clock.world_us() < go + 30 * g) {  // bis zur Takt-Grenze der Ankunft, danach gehen sie weiter
                 a.run(1);
                 b.run(1);
                 if (early(a) || early(b)) return std::string("vor der Reisezeit angekommen");
                 for (WorldEvent e; a.env().events.poll(rd, &e);) ++arrived[0];
                 for (WorldEvent e; b.env().events.poll(rb, &e);) ++arrived[1];
             }
             for (Run* r : {&a, &b}) {
                 int here = 0;
                 for (int k = 0; k < r->env().figures.count(); ++k)
                     here += r->env().figures.at(k).where.place == platz;
                 if (here != pixelia::FigureBook::kMax || r->stage.in_transit() != 0)
                     return "am Platz " + std::to_string(here) + ", unterwegs " + std::to_string(r->stage.in_transit());
                 char buf[160];
                 if (const char* e = r->stage.check_figures(buf, sizeof(buf))) return std::string(e);
             }
             if (arrived[0] != pixelia::FigureBook::kMax || arrived[1] != pixelia::FigureBook::kMax)
                 return "Ankünfte " + std::to_string(arrived[0]) + " / " + std::to_string(arrived[1]);
             // V01: 48 Aufbrüche an einer Takt-Grenze sind der Grund für 128 Plätze im Ring (vorher 32)
             std::printf("  info  48 Aufbrüche an einer Takt-Grenze: höchstens %u Meldungen zwischen zwei Zustellungen, verloren %u\n",
                         a.env().events.peak(), a.env().events.lost());
             if (a.env().events.peak() < 48 || a.env().events.lost()) return std::string("Ereignisse der Aufbrüche fehlen");
             return checked(b.env().ledger);
         }},
        // --- B15 Wege ---
        {"B15 Wege: ist die Brücke zu, zählt der offene Umweg; ist auch der Steg zu, nennt die Karte den Grund",
         [] {
             const pixelia::PlaceDef* list[] = {&kPA, &kPB, &kPC, &kPD};
             pixelia::PlaceGraph gr;
             gr.build(list, 4);
             char buf[160];
             const char* err = gr.check(buf, sizeof(buf));
             if (err && *err) return std::string("Graph: ") + err;
             pixelia::Environment env;
             env.flags.set("steg", true);
             int8_t d[pixelia::PlaceGraph::kMaxPlaces];
             gr.open_hops_to(1, env, d);
             if (gr.hops(0, 1) != 1 || d[0] != 3 || d[2] != 2 || d[3] != 1)
                 return "Brücke zu: A " + std::to_string(d[0]) + ", C " + std::to_string(d[2]) + ", D " + std::to_string(d[3]);
             if (gr.closed_why(0, 1, env)) return std::string("Umweg offen, trotzdem ein Grund");
             env.flags.set("bruecke", true);
             gr.open_hops_to(1, env, d);
             if (d[0] != 1) return std::string("Brücke offen: nicht der direkte Weg");
             env.flags.set("bruecke", false);
             env.flags.set("steg", false);
             gr.open_hops_to(1, env, d);
             int8_t from[pixelia::PlaceGraph::kMaxPlaces];
             gr.open_hops_from(0, env, from);
             if (d[0] != -1 || from[2] != 1 || from[1] != -1) return std::string("alles zu: trotzdem ein Weg");
             const char* why = gr.closed_why(0, 1, env);
             if (!why || std::strcmp(why, "Erst nach dem Bau begehbar.") != 0)
                 return std::string("Grund: ") + (why ? why : "keiner");
             return std::string();
         }},
        {"B15 Testdorf nachts: der Feldweg ist zu, Figuren gehen über den Rand zum Platz und warten nie auf einen Weg",
         [] {
             Run a(71, "testort-wiese", 23, 0, 60.0);
             const pixelia::PlaceGraph& gr = pixelia::world_graph();
             const int wiese = pixelia::place_index("testort-wiese"), platz = pixelia::place_index("testort-platz");
             if (gr.closed_why(wiese, platz, a.env())) return std::string("nachts kein Weg zum Platz");
             int8_t d[pixelia::PlaceGraph::kMaxPlaces];
             gr.open_hops_to(platz, a.env(), d);
             for (int p = 0; p < gr.places(); ++p)
                 if (d[p] != gr.hops(p, platz)) return std::string("offene Wege kürzer oder länger als die Daten");
             a.run(30 * 120);  // 2 Weltstunden nachts
             char buf[160];
             if (const char* e = a.stage.check_figures(buf, sizeof(buf))) return std::string(e);
             return checked(a.env().ledger);
         }},
        // --- B17 Projekte ---
        {"B17 Projektdaten: unbekannte Ware, zu viele Bedarfe, falsche Baustelle sperren das Projekt mit Meldung; gültige laufen weiter",
         [] {
             struct Bad {
                 const char* what;
                 std::function<void(Village&)> spoil;
                 const char* expect;
             };
             static pixelia::ProjectNeed five[5] = {{"holz", 1}, {"holz", 1}, {"holz", 1}, {"holz", 1}, {"holz", 1}};
             const Bad bad[] = {
                 {"Ware", [](Village& v) { v.needs[0].good = "brettt"; }, "Schritt stapeln: Ware „brettt“ gibt es nicht"},
                 {"Bedarfe", [](Village& v) { v.steps[0].needs = five; v.steps[0].need_count = 5; }, "5 Bedarfe"},
                 {"Baustelle", [](Village& v) { v.projects[0].site = "nix"; }, "Baustelle „nix“"},
                 {"Baustelle ist kein Bauplatz", [](Village& v) { v.projects[0].site = "k_kammer"; }, "Baustelle „k_kammer“"},
                 {"Quelle", [](Village& v) { v.projects[0].source = "k_plat"; }, "Quelle „k_plat“"},
             };
             for (const Bad& b : bad) {
                 Village v;
                 b.spoil(v);
                 pixelia::ProjectList pl;
                 const EconomyDef* defs[] = {&v.def};
                 v.l.build(defs, 1);
                 pl.build(defs, 1, v.l);
                 char buf[160];
                 const char* e = pl.data_problem(buf, sizeof(buf));
                 if (!e || !std::strstr(e, "Projekt stapel") || !std::strstr(e, b.expect))
                     return std::string(b.what) + ": Meldung „" + (e ? e : "keine") + "“";
                 pl.start_new();
                 if (pl.count() != 0 || pl.test_start(0, 0, v.l, v.tasks)) return std::string(b.what) + ": trotzdem begonnen";
             }
             Village ok;
             pixelia::ProjectList pl;
             const EconomyDef* defs[] = {&ok.def};
             pl.build(defs, 1, ok.l);
             char buf[160];
             if (const char* e = pl.data_problem(buf, sizeof(buf))) return std::string("gültig gemeldet: ") + e;
             pl.start_new();
             return pl.count() == 1 ? std::string() : std::string("gültiges Projekt beginnt nicht");
         }},
        // --- B18 Erstmalig-Merker ---
        {"B18 Erstes Mal: Gezeigtes verdeckt das eigene erste Gelingen nicht, Vergessen macht kein zweites erstes Mal",
         [] {
             using pixelia::MemoryKind;
             using pixelia::Skill;
             pixelia::FigureBook book;
             pixelia::FigureDef def{};
             def.name = "Greta";
             const uint16_t t = book.add(def, 0)->id;
             def.name = "Lisl";
             const uint16_t s = book.add(def, 0)->id;
             book.learn(t, Skill::Baking, 3000, 1, 600);  // Meisterin
             if (!book.teach(t, s, Skill::Baking, 30, 1, 610)) return std::string("zeigen geht nicht");
             const auto count = [&](MemoryKind k, uint16_t who) {
                 int n = 0;
                 for (const pixelia::Memory& m : book.find(s)->memories)
                     n += m.kind == k && m.arg == static_cast<uint8_t>(Skill::Baking) && m.who == who;
                 return n;
             };
             if (count(MemoryKind::Taught, t) != 1 || count(MemoryKind::First, t) != 0)
                 return std::string("Gezeigtes ist kein eigenes Gelingen");
             const uint32_t bit = 1u << static_cast<int>(Skill::Baking);
             pixelia::Rng rng(7);
             int successes = 0;
             for (int i = 0; i < 200 && successes < 3; ++i)
                 successes += book.try_skill(s, Skill::Baking, 0, rng, 2, 600).outcome == pixelia::Outcome::Success;
             if (successes < 3) return std::string("kein Gelingen in 200 Versuchen");
             if (count(MemoryKind::First, 0) != 1 || !(book.find(s)->first_ok & bit))
                 return "erstes Gelingen " + std::to_string(count(MemoryKind::First, 0)) + "-mal erinnert";
             // vergessen (der Platz wird frei): danach darf kein zweites erstes Mal entstehen
             for (pixelia::Memory& m : book.find(s)->memories)
                 if (m.kind == MemoryKind::First) m = pixelia::Memory{};
             for (int i = 0; i < 50; ++i) book.try_skill(s, Skill::Baking, 0, rng, 4, 600);
             if (count(MemoryKind::First, 0) != 0) return std::string("nach dem Vergessen ein zweites erstes Mal");
             if (!(book.find(s)->first_ok & bit)) return std::string("Merker weg");
             // alle Plätze mit Wichtigerem belegt: das erste Gelingen zählt trotzdem (der Merker, nicht die Erinnerung)
             def.name = "Jakob";
             const uint16_t j = book.add(def, 0)->id;
             for (int i = 0; i < pixelia::FigureRecord::kMemories; ++i) {
                 pixelia::Memory m;
                 m.kind = MemoryKind::Event;
                 m.weight = 250;
                 m.day = 3;
                 m.minute = static_cast<uint16_t>(i);
                 book.remember(j, m);
             }
             book.learn(j, Skill::Baking, 400, 3, 600);
             for (int i = 0; i < 200 && !(book.find(j)->first_ok & bit); ++i) book.try_skill(j, Skill::Baking, 0, rng, 5, 600);
             return (book.find(j)->first_ok & bit) ? std::string() : std::string("volle Erinnerung: erstes Gelingen fehlt");
         }},
        {"B18 Erstmalig-Merker überstehen Speichern und Laden; ein Stand aus 0.5.9 leitet sie aus den Erinnerungen ab",
         [] {
             Run a(81);
             pixelia::FigureRecord& f = a.env().figures.at(2);
             f.first_ok |= 1u << static_cast<int>(pixelia::Skill::Baking);
             f.first_fail |= 1u << static_cast<int>(pixelia::Skill::Smithing);
             const uint16_t id = f.id;
             const std::vector<uint8_t> s = a.save();
             Run b(82);
             if (!b.stage.load_state(s.data(), s.size())) return std::string("Laden");
             const pixelia::FigureRecord* g = b.env().figures.find(id);
             if (!g || g->first_ok != f.first_ok || g->first_fail != f.first_fail) return std::string("Merker verloren");
             std::vector<uint8_t> old;
             if (FILE* fp = std::fopen("sim/testdata/welt-0.5.9.pxw", "rb")) {
                 uint8_t buf[4096];
                 for (size_t n; (n = std::fread(buf, 1, sizeof(buf), fp)) > 0;) old.insert(old.end(), buf, buf + n);
                 std::fclose(fp);
             }
             if (old.empty()) return std::string("Vorlage sim/testdata/welt-0.5.9.pxw fehlt");
             Run c(83);
             if (!c.stage.load_state(old.data(), old.size())) return std::string("alter Stand lädt nicht");
             for (int k = 0; k < c.env().figures.count(); ++k) {
                 const pixelia::FigureRecord& x = c.env().figures.at(k);
                 uint32_t ok = 0, fail = 0;
                 for (const pixelia::Memory& m : x.memories) {
                     if (m.kind == pixelia::MemoryKind::First && m.who) return std::string("Gezeigtes noch als erstes Mal");
                     if (m.kind == pixelia::MemoryKind::First) ok |= 1u << m.arg;
                     if (m.kind == pixelia::MemoryKind::Failure) fail |= 1u << m.arg;
                 }
                 if (x.first_ok != ok || x.first_fail != fail) return std::string("aus dem alten Stand falsch abgeleitet");
             }
             return std::string();
         }},
    };
}

}  // namespace

int run_boden_tests() {
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
    std::printf("sim: Fester Boden (0.5.11) %d/%d bestanden\n", static_cast<int>(list.size()) - failed,
                static_cast<int>(list.size()));
    return failed;
}

}  // namespace sim
