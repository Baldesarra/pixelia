#pragma once
// Warenbuch (seit 0.5.10, SETTING.md Kapitel 8, WIRTSCHAFT.md Kapitel 6 und 13, LEBENDIGKEIT.md Regel 10): ein Buch für
// die ganze Welt, alle Orte buchen nur darüber.
//
// - Waren (GoodDef) und Lagerplätze (StoreDef) sind Daten der Gebiete (EconomyDef in AreaDef, place.h); die Engine kennt
//   keine Ware beim Namen. Mengen sind ganze Stück. Einzelstücke (GoodDef::unique: Werkzeug, Fundstück) haben dazu je
//   einen Datensatz mit Besitzer, Zustand, Hersteller und Tag; sie zählen im Bestand ihres Lagerplatzes mit.
// - Bestand = Menge je Lagerplatz und Ware. Obergrenze je Lagerplatz in Stück (alle Waren zusammen); die Trage einer
//   Figur (Lagerplatz der Art Hand, entsteht beim ersten Tragen) zählt nach Gewicht in Zehntel-Trageeinheiten. Was
//   unterwegs ist, liegt so immer in genau einem Lagerplatz, auch auf einer Verbindung und über einen Neustart.
// - Buchungen: Zugang (herstellen, sammeln, finden, eintauschen), Abgang (verbrauchen, verbauen, verderben, austauschen)
//   und Umlagern. Eine Buchung ist ein Aufruf und ändert alles auf einmal; nie negativ, nie über der Obergrenze (die
//   Rückgabe sagt, wie viel wirklich gebucht ist). Zähler je Ware und Art führen die Bilanz: Anfang + Zugänge − Abgänge
//   = Summe aller Bestände (check). Bedarf und Fehlmenge (demand) zählen getrennt: Ein leeres Lager „verbraucht“ sonst
//   einfach nichts.
// - Reservierung: Ein Vorhaben (Aufgabe, Projekt; holder) hält eine Menge an einem Lagerplatz fest, andere sehen nur, was
//   frei ist. Wer reserviert hat, holt mit seiner Reservierung (take/move mit res).
// - Reichweite: Abgänge je Lagerplatz und Ware sowie der Verbrauch je Ware (verbrauchen, verbauen, verderben) über die
//   letzten 24 Weltstunden in Stunden-Fächern; Reichweite = Bestand ÷ Abgang je Tag. Soll und Obergrenze je Ware in Tagen,
//   Notbestand unter einem Tag (WIRTSCHAFT 13).
// - Stufen der Simulation (world_sim.cpp): Bestände eines Ortes ändern nur Figuren an diesem Ort (in Zeitreihenfolge) und
//   seine Ortszustände; was mehrere Orte betrifft (Reservierungen, Aufgaben, Projekte), ändert sich nur an Takt-Grenzen.
//   Die Fächer rollt die Bühne an jeder Takt-Grenze weiter (roll), Besonderes (leer, voll, Notbestand) meldet step dort
//   auf den Ereignis-Bus. Die Reihenfolge im Speicher ist gleichgültig: Auswahl nur nach festen Schlüsseln, der
//   Speicherstand schreibt sortiert. So bucht ein Ort sichtbar wie unsichtbar dasselbe (P10).
// - Speicherstand mit Kennungen statt Nummern (save/load, Abschnitte WARE, LAGR, STCK, RESV in world_save.cpp). Liegt der
//   Bestand eines Lagerplatzes, den es nicht mehr gibt, im Speicherstand, kommt er ins erste Gemeinschaftslager.
// - Liegt im Grafik-Speicher (auf dem Gerät PSRAM, ~40 KB), angelegt beim ersten build().

#include <cstdint>
#include <string_view>

#include "pixelia/engine/events.h"

namespace pixelia {

class SaveWriter;
class SaveReader;

// --- Buchungsarten ---
enum class Booking : uint8_t {
    Produce, Gather, Find, TradeIn,      // Zugang
    Consume, Build, Spoil, TradeOut,     // Abgang
    Move,                                // umlagern (Bestand der Welt gleich)
    kCount
};
inline constexpr int kBookingKinds = static_cast<int>(Booking::kCount);
// herstellen, sammeln, finden, eintauschen, verbrauchen, verbauen, verderben, austauschen, umlagern
const char* booking_id(Booking b);
inline constexpr bool booking_in(Booking b) { return b <= Booking::TradeIn; }
inline constexpr bool booking_out(Booking b) { return b >= Booking::Consume && b <= Booking::TradeOut; }

// --- Daten der Gebiete ---
struct GoodDef {
    const char* id;          // brennholz (Kennung: Speicherstand, Protokoll, Befehle), ASCII
    const char* title;       // Brennholz
    uint8_t weight = 10;     // Gewicht je Stück in Zehntel-Trageeinheiten (Korb: 10 Trageeinheiten = 100)
    uint8_t soll_days = 0;   // Sollbestand im ganzen Dorf in Tagen Reichweite (0 = keiner)
    uint8_t max_days = 0;    // Obergrenze in Tagen (0 = keine)
    bool unique = false;     // Einzelstücke
    const char* one = nullptr;   // Zähleinheit (seit 0.5.10 Teil 2, Karten): „Scheit“, „Laib“; nullptr = title
    const char* many = nullptr;  // Mehrzahl: „Scheite“, „Laibe“
    const char* the = nullptr;   // mit Artikel (seit Teil 4, Werkzeug auf der Karte): „die Säge“; nullptr = title
};
// „1 Scheit“, „8 Scheite“ (ohne Zähleinheit: „8 Brennholz“); out mindestens 32 Zeichen
const char* good_count_text(const GoodDef& g, int n, char* out, int len);

enum class StoreKind : uint8_t {
    Producer,   // Erzeugerlager am Arbeitsplatz (Holzstapel, Auslage)
    House,      // Hausvorrat (Holzlege, Brotkorb)
    Community,  // Gemeinschaftslager (Güterschuppen, Vorratsscheune)
    Site,       // Baustelle eines Projekts (seit 0.5.10 Teil 3)
    Hand,       // Trage einer Figur (keine Daten, entsteht beim ersten Tragen)
};
const char* store_kind_title(StoreKind k);  // Erzeugerlager, Hausvorrat, Gemeinschaftslager, Baustelle, Trage

// Was ein Lagerplatz vorrätig halten will (Stück, Testwerte bis 0.6.x): unter low füllt ihn jemand auf want auf
// (Hausvorrat: Nachschub holen; Erzeugerlager: herstellen)
struct StockRule {
    const char* good;
    uint16_t low, want;
};

struct StoreDef {
    const char* id;           // wiese_holzstapel
    const char* title;        // Holzstapel
    const char* place;        // Ort (Kennung)
    StoreKind kind;
    uint16_t capacity;        // Obergrenze in Stück, alle Waren zusammen
    int8_t part = -1;         // Teil der Raumliste: Stellplätze zeigen den Bestand, Benutzungspunkt zum Holen und Bringen
    const StockRule* rules = nullptr;
    uint8_t rule_count = 0;
    const char* source = nullptr;  // Nachschub von hier (nullptr: Erzeuger- oder Gemeinschaftslager mit der Ware)
    int8_t work_part = -1;         // Erzeugerlager: Arbeitsplatz in der Raumliste (Hackklotz), −1 = am Lager selbst
};

struct StartStock {
    const char* store;
    const char* good;
    uint16_t qty;
    uint16_t condition = 1000;  // Einzelstücke: Zustand 0–1000
};

struct ProjectDef;  // projects.h

struct EconomyDef {
    const GoodDef* goods = nullptr;
    uint8_t good_count = 0;
    const StoreDef* stores = nullptr;
    uint8_t store_count = 0;
    const StartStock* start = nullptr;  // Startvorrat (neue Welt; alter Speicherstand ohne Warenbuch)
    uint8_t start_count = 0;
    const ProjectDef* projects = nullptr;  // Projekte (seit 0.5.10 Teil 3, projects.h)
    uint8_t project_count = 0;
};

// --- Laufzeit ---

// Einzelstück: Kennung bleibt für immer (aus Zeit, Figur, Ware und Lagerplatz gewürfelt, nie 0)
struct Item {
    uint32_t id = 0;
    uint8_t good = 0;
    uint8_t store = 0;
    uint16_t owner = 0;        // Figur, 0 = Gemeinschaft
    uint16_t condition = 1000; // 0–1000
    uint16_t maker = 0;        // Figur, 0 = unbekannt (Startvorrat, Fund)
    int32_t day = 0;           // Welttag der Herkunft
};

// Wer gebucht hat und warum (Protokoll: bewegungen.csv)
struct BookingInfo {
    int64_t at_us = 0;          // Weltzeit
    uint16_t who = 0;           // Figur, 0 = niemand (Ortszustand, Startvorrat)
    const char* reason = "";    // Kennung des Grundes (feste Zeichenkette): herd, nachschub, essen …
    uint16_t holder = 0;        // Aufgabe bzw. Projekt, 0 = keins
};

// Eine ausgeführte Buchung (an den Beobachter, Protokoll ab 0.5.10 Teil 4)
struct BookingRecord {
    BookingInfo info;
    Booking kind = Booking::Move;
    uint8_t good = 0;
    int16_t from = -1, to = -1;  // Lagerplätze, −1 = Welt (Zugang bzw. Abgang)
    uint16_t qty = 0;
    uint32_t item = 0;           // Einzelstück, 0 = Menge
};

// Bedarf: gewollt und bekommen (Fehlmenge = gewollt − bekommen)
struct DemandRecord {
    int64_t at_us = 0;
    uint8_t good = 0;
    int16_t store = -1;
    uint16_t want = 0, got = 0;
    uint16_t who = 0;
    const char* reason = "";
};

// Stand einer Ware im ganzen Dorf nach Reichweite (WIRTSCHAFT 13)
enum class StockLevel : uint8_t { Unknown, Emergency, Low, Ok, High };  // ohne Verbrauch, Notbestand, unter Soll, gut, über Obergrenze
const char* stock_level_title(StockLevel l);

class Ledger {
public:
    static constexpr int kMaxGoods = 96;
    static constexpr int kMaxStores = 128;         // aus den Daten plus je Figur eine Trage
    static constexpr int kMaxStock = 384;          // Einträge Lagerplatz × Ware
    static constexpr int kMaxItems = 192;
    static constexpr int kMaxReservations = 96;
    static constexpr int kHours = 24;
    static constexpr int kHandCapacity = 100;      // Korb: 10 Trageeinheiten (Testwert, WIRTSCHAFT 6)

    Ledger() = default;
    ~Ledger();
    Ledger(const Ledger&) = delete;
    Ledger& operator=(const Ledger&) = delete;

    // Waren und Lagerplätze aus den Gebieten (world_areas) bzw. einer eigenen Liste (Tests); leert alles
    void build();
    void build(const EconomyDef* const* defs, int n);
    // Startvorrat der Daten einbuchen (zählt als Anfang der Bilanz, nicht als Zugang)
    void start_stock(int32_t day);
    void clear();  // alle Bestände, Einzelstücke, Reservierungen, Zähler und Fächer leer; Waren und Lagerplätze bleiben
    bool ready() const { return s_ != nullptr; }

    // --- Waren und Lagerplätze ---
    int goods() const;
    const GoodDef& good(int g) const;
    int find_good(std::string_view id) const;  // −1 = gibt es nicht
    int stores() const;
    const StoreDef* store_def(int s) const;     // nullptr = Trage
    StoreKind store_kind(int s) const;
    // Kennung (Daten) bzw. „@<Figur>“ bei einer Trage; out mindestens 8 Zeichen
    const char* store_key(int s, char* out, int n) const;
    const char* store_title(int s) const;       // Trage: „Trage“
    int find_store(std::string_view key) const; // Kennung oder „@<Figur>“, −1 = gibt es nicht
    int store_place(int s) const;               // Nummer des Ortes in der Welt; −1 = Trage (der Ort der Figur zählt)
    uint16_t hand_owner(int s) const;           // Figur einer Trage, 0 = keine Trage
    int hand(uint16_t figure) const;            // Trage der Figur, −1 = hat noch keine
    int make_hand(uint16_t figure);             // anlegen, wenn es sie nicht gibt; −1 = kein Platz

    // --- Bestand ---
    int qty(int store, int good) const;
    int reserved(int store, int good) const;
    int available(int store, int good) const { return qty(store, good) - reserved(store, good); }
    int load(int store) const;      // Stück bzw. bei der Trage Zehntel-Trageeinheiten
    int capacity(int store) const;  // Obergrenze in derselben Einheit
    int room(int store, int good) const;  // so viel Stück passen noch hinein
    int total(int good) const;            // im ganzen Dorf (mit Tragen)
    // Waren eines Lagerplatzes (nur Bestand > 0, nach Ware): Nummern der Waren und Mengen; Zahl
    int contents(int store, uint8_t* goods, uint16_t* qtys, int max) const;

    // --- Buchungen ---
    // Zugang: höchstens so viel, wie Platz ist; Rückgabe = gebucht
    int add(int store, int good, int n, Booking kind, const BookingInfo& info);
    // Abgang: höchstens so viel, wie frei ist (mit res: aus dieser Reservierung, dann auch Reserviertes)
    int take(int store, int good, int n, Booking kind, const BookingInfo& info, uint16_t res = 0);
    // Umlagern: höchstens so viel, wie frei ist und am Ziel Platz hat
    int move(int from, int to, int good, int n, const BookingInfo& info, uint16_t res = 0);
    // Bedarf melden (gewollt, bekommen): Zähler je Ware, Protokoll bedarf.csv
    void demand(int good, int store, int want, int got, const BookingInfo& info);

    // --- Einzelstücke ---
    uint32_t add_item(int store, int good, uint16_t owner, uint16_t condition, Booking kind, const BookingInfo& info);
    bool move_item(uint32_t id, int to, const BookingInfo& info);
    bool take_item(uint32_t id, Booking kind, const BookingInfo& info);
    const Item* item(uint32_t id) const;
    int items() const;
    const Item& item_at(int i) const;  // Reihenfolge beliebig
    bool set_condition(uint32_t id, int condition);
    bool set_owner(uint32_t id, uint16_t owner);

    // --- Reservierung (nur an Takt-Grenzen) ---
    uint16_t reserve(int store, int good, int n, uint16_t holder);  // so viel wie frei, Kennung; 0 = nichts frei
    int reservation_left(uint16_t res) const;  // 0 = verbraucht oder unbekannt
    void release(uint16_t res);                // Rest freigeben
    void release_holder(uint16_t holder);      // alle Reservierungen eines Vorhabens
    int reservations() const;
    int holders(uint16_t* out, int max) const;  // Halter aller Reservierungen, aufsteigend, je einmal; Zahl

    // --- Reichweite und Soll ---
    void roll(int64_t at_us);  // Stunden-Fächer bis zur Stunde dieser Weltzeit (an jeder Takt-Grenze)
    int out_24h(int store, int good) const;  // Abgang der letzten 24 Weltstunden
    int use_24h(int good) const;             // Verbrauch der Ware im Dorf (verbrauchen, verbauen, verderben)
    int in_today(int store, int good) const; // Zu- und Abgänge des laufenden Welttags (Lager-Karte)
    int out_today(int store, int good) const;
    // Reichweite in Stunden (Bestand ÷ Abgang der letzten 24 Stunden); −1 = kein Abgang
    int reach_hours(int store, int good) const;
    int reach_hours(int good) const;
    StockLevel level(int good) const;
    // Stand eines Lagerplatzes nach seinen Regeln (seit 0.5.10 Teil 4b, Lager-Karte): Notbestand = leer oder reicht
    // keinen Tag, knapp = unter low bzw. unter dem Soll der Ware, reichlich = über der Obergrenze; schlechteste Regel
    // zählt; ohne Regeln (Baustelle, Trage) Unknown
    StockLevel store_level(int store) const;

    // An der Takt-Grenze (Bühne): Fächer rollen, Besonderes melden (Lager leer bzw. wieder da, voll, Notbestand) — in
    // fester Reihenfolge der Lagerplätze und Waren
    void step(int64_t at_us, int32_t day, EventBus* bus);

    // --- Bilanz und Beobachter ---
    struct GoodTotals {
        int64_t start = 0;                 // Anfang (Startvorrat)
        int64_t moved[kBookingKinds] = {};  // je Art gebuchte Stück
        int64_t want = 0, got = 0;         // Bedarf
    };
    const GoodTotals& totals(int good) const;
    // Prüfung: je Ware Anfang + Zugänge − Abgänge = Summe der Bestände, nichts negativ, nichts über der Obergrenze,
    // Reservierungen passen, Einzelstücke stimmen mit dem Bestand; nullptr = alles gut, sonst der Fehler in buf
    const char* check(char* buf, int n) const;
    uint32_t version() const { return version_; }            // zählt jede Änderung am Bestand
    uint32_t store_version(int store) const;                  // Änderungen eines Lagerplatzes (Darstellung)
    using BookingListener = void (*)(void* ctx, const BookingRecord& r);
    using DemandListener = void (*)(void* ctx, const DemandRecord& r);
    void set_listener(BookingListener b, DemandListener d, void* ctx) {
        on_booking_ = b;
        on_demand_ = d;
        listen_ctx_ = ctx;
    }

    // --- Speicherstand (world_save.cpp) ---
    void save_goods(SaveWriter& w) const;   // WARE: Waren mit Zählern und Verbrauchs-Fächern
    void save_stock(SaveWriter& w) const;   // LAGR: Bestände je Lagerplatz (Kennung) mit Fächern
    void save_items(SaveWriter& w) const;   // STCK: Einzelstücke
    void save_reservations(SaveWriter& w) const;  // RESV
    struct LoadStats {
        int stock = 0, items = 0, reservations = 0;
        int lost = 0;   // Stück unbekannter Waren (gibt es nicht mehr)
        int moved = 0;  // Stück unbekannter Lagerplätze, ins erste Gemeinschaftslager gelegt
    };
    // Abschnitte in beliebiger Reihenfolge nach clear(); fehlende bleiben leer
    void load_goods(SaveReader& r, uint16_t version, LoadStats* st);
    void load_stock(SaveReader& r, uint16_t version, LoadStats* st);
    void load_items(SaveReader& r, uint16_t version, LoadStats* st);
    void load_reservations(SaveReader& r, uint16_t version, LoadStats* st);
    // nach dem Laden aller Abschnitte: Reservierungen in den Beständen nachzählen, Bilanz-Anfang aus den Zählern
    void finish_load(LoadStats* st);

private:
    struct Storage;
    struct Stock;
    bool ensure();
    Stock* entry(int store, int good, bool create);
    const Stock* entry(int store, int good) const;
    void bucket(Stock& e, int64_t at_us, int n);
    void used(int good, int64_t at_us, int n);
    void touch(int store);
    void notify(Booking kind, int good, int from, int to, int n, uint32_t item, const BookingInfo& info);
    int weight_of(int store) const;
    int64_t hour_of(int64_t at_us) const;

    Storage* s_ = nullptr;
    uint32_t version_ = 0;
    BookingListener on_booking_ = nullptr;
    DemandListener on_demand_ = nullptr;
    void* listen_ctx_ = nullptr;
};

}  // namespace pixelia
