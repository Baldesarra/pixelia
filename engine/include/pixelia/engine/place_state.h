#pragma once
// Ortszustände (seit 0.5.8, SETTING.md Kapitel 10): was sich an einem Ort mit der Zeit ändert und weiterläuft, wenn
// niemand hinschaut. Licht mit Brennstoff, Feuer und Glut, Nässe, Füllstand, angefangene Arbeit.
//
// - Die Zustände gehören zur Welt (Umgebung, environment.h), nicht zur Szene: Die Bühne schreibt sie Minute für Minute
//   der Weltzeit fort (step), für alle Orte gleich, ob sie zu sehen sind oder nicht. Springt die Uhr vor, holt die
//   Bühne Minute für Minute nach (höchstens einen Tag); das Ergebnis ist dasselbe wie in Echtzeit. Springt sie zurück,
//   bleibt alles stehen.
// - Orte melden ihre Zustände mit Daten an (StateDef in PlaceDef::states) und zeigen sie: Zustandsbild eines Teils,
//   Lichtkreis, Farben. Ändern dürfen Orte sie nur über Handlungen (anzünden, löschen, abdecken, schöpfen,
//   weiterarbeiten), wie später die Figuren.
// - Bis zum Tagesgerüst (0.5.11) entscheiden Hausregeln (StateRule) über Licht und Feuer, nach Weltzeit und
//   Sonnenzeiten. Brennstoff war bis 0.5.9 unbegrenzt vorrätig (wer anzündet, füllt auf); seit 0.5.10 holt ein Zustand
//   mit StateDef::fuel jede Füllung aus dem Warenbuch (ledger.h, verbrauchen) und meldet dort seinen Bedarf. Ist nichts
//   da, bleibt das Feuer aus bzw. brennt herunter. Ohne fuel bleibt es unbegrenzt.
// - Lichtquellen-Arten (LightKind) mit Stärke nach SETTING.md Kapitel 9: Die Stärke ergibt Radius und Helligkeit des
//   Lichtkreises (light_circle); die Quellen eines Ortes addieren sich für den Schein nach außen bis zur Obergrenze.
// - Wechselt ein Zustand seine Stufe (Licht an/aus, Feuer → Glut → kalt, Becken voll …), meldet er das über den
//   Ereignis-Bus (EventKind::StateChanged) und zählt version() hoch.

#include <array>
#include <cstdint>
#include <string_view>

#include "pixelia/engine/calendar.h"
#include "pixelia/engine/events.h"
#include "pixelia/engine/light.h"

namespace pixelia {

class Ledger;

enum class StateKind : uint8_t {
    Light,    // Kerze, Lampe, Laterne: brennt, solange die Füllung reicht (Stufe 0 aus, 1 an)
    Fire,     // Herd, Kamin: brennt mit Brennstoff, dann Glut, kühlt langsam ab (0 kalt, 1 Glut, 2 Feuer)
    Wetness,  // Boden draußen: steigt bei Regen, trocknet langsam (0 trocken, 1 feucht, 2 nass, 3 sehr nass)
    Fill,     // Becken, Fass: steigt bei Regen, sinkt beim Schöpfen (0 leer, 1 wenig, 2 halb, 3 voll)
    Work,     // angefangene Arbeit: bleibt liegen, bis jemand weitermacht (0 offen, 1 angefangen, 2 fertig)
};

// Lichtquellen-Arten (SETTING.md Kapitel 9)
enum class LightKind : uint8_t { Ember, Splint, Tallow, Wax, Hearth, Lantern, OilLamp, Torch, kCount };
struct LightKindInfo {
    const char* id;     // glut, kienspan, talgkerze, wachskerze, kamin, laterne, oellampe, fackel
    const char* title;  // Glut, Kienspan …
    uint8_t strength;   // 1 … 4
    uint16_t burn;      // Brennminuten einer Füllung (Kerze, Lampenfüllung ≈ ein Lichtabend); 0 = mit dem Feuer
    bool outside;       // Schein nach außen (Glut: nur im Raum)
};
const LightKindInfo& light_kind(LightKind k);
inline constexpr int kMaxStrength = 4;
inline constexpr int kMaxOutsideStrength = 6;  // Obergrenze der Summe eines Ortes (Fenster nach außen)
// Lichtkreis einer Quelle der Stärke 1 … 4 um (x, y): Stärke 4 = 60 × 40 grobe Bildpunkte bis zur obersten Stufe
LightCircle light_circle(int x, int y, int strength);

// Hausregeln bis zum Tagesgerüst (0.5.11)
enum class StateRule : uint8_t {
    None,
    Evening,    // Abendlicht: brennt, wenn es dunkel ist und man wach ist (5:30–23 Uhr); Laterne genauso
    Homecomer,  // Kerze für Heimkehrer: nachts, solange die Hauptfigur nicht im Haus ist; wer heimkommt, löscht
    Hearth,     // Herdfeuer: morgens, mittags und abends geschürt, um 22 Uhr zum Schlafen abgedeckt
};

struct StateDef {
    StateKind kind = StateKind::Light;
    const char* id = "";     // Kennung im Ort: lampe, kerze, ofen, boden, becken …
    int8_t part = -1;        // Teil der Raumliste, das den Zustand zeigt (Zustandsbild = Stufe), −1 = keins
    LightKind light = LightKind::OilLamp;  // Licht: Art; Feuer: Art beim Brennen (die Glut ist immer Glut)
    int16_t x = 0, y = 0;    // Licht und Feuer: Mitte des Lichtkreises (grobe Bildpunkte)
    uint16_t capacity = 0;   // Füllstand: Liter; Arbeit: Arbeitsminuten bis fertig
    uint16_t rain = 0;       // Füllstand: Liter je Stunde bei vollem Regen (Dach, Rinne, offene Fläche)
    StateRule rule = StateRule::None;
    // Brennstoff aus dem Warenbuch (seit 0.5.10): Ware und Lagerplatz (Kennungen), Stück je Füllung (Feuer: 60
    // Brennminuten, Licht: eine Brenndauer). nullptr = unbegrenzt wie bis 0.5.9
    const char* fuel = nullptr;
    const char* fuel_store = nullptr;
    uint8_t fuel_units = 1;
};

// Was die Bühne je Weltminute mitgibt
struct StateInput {
    SunTimes sun;               // Auf- und Untergang (Minuten des Tages)
    int rain = 0;               // Regen jetzt 0–255 (Wetter)
    int dry_day = 96;           // Trocknen je Minute bei Tag in 1/16 (Nässe 0–1000), aus Wolken, Wind, Jahreszeit
    int dry_night = 16;         // … bei Nacht
    bool lead_known = false;    // gibt es eine Hauptfigur?
    const char* lead_house = nullptr;  // Haushalt, in dem sie gerade ist (nullptr = draußen oder unterwegs)
};

class PlaceStates {
public:
    static constexpr int kMax = 48;        // Zustände aller Orte
    static constexpr int kMaxPlaces = 16;
    static constexpr int kWetMax = 1000;   // Nässe

    // Zustände eines Ortes anmelden (place, house: feste Zeichenketten der Ortsdaten; open_sky: Regen kommt an,
    // unter Tage und drinnen nicht). Schon bekannt: Anfangswerte neu. Index des ersten, −1 = voll
    int declare(const char* place, const StateDef* defs, int n, const char* house = nullptr, bool open_sky = true);
    int count() const { return n_; }
    int find(std::string_view place, std::string_view id) const;  // −1 = gibt es nicht
    int first(std::string_view place, int* n) const;              // Zustände eines Ortes: erster Index, Zahl
    const StateDef& def(int i) const { return *e_[i].def; }
    const char* place(int i) const { return places_[e_[i].place].id; }

    // Abfragen
    int phase(int i) const { return e_[i].phase; }
    int value(int i) const { return e_[i].value; }  // Brennstoff (Minuten), Nässe 0–1000, Liter, Arbeit (Sekunden)
    int heat(int i) const { return e_[i].heat; }    // Feuer: 0–1000
    bool on(int i) const { return e_[i].on; }       // Licht brennt bzw. Feuer mit Flamme
    bool covered(int i) const { return e_[i].covered; }
    int strength(int i) const;      // Licht dieses Zustands jetzt (0 = dunkel), Glut 1
    bool outside(int i) const;      // scheint nach außen
    LightCircle circle(int i) const { return light_circle(e_[i].def->x, e_[i].def->y, strength(i)); }
    bool any_light(std::string_view place) const;     // brennt dort etwas (auch Glut)?
    // Zustand in Worten für Karte und Protokoll („Glut, abgedeckt“, „Wasser: 220 von 400 Litern“); Länge wie snprintf
    int describe(int i, char* buf, int n) const;
    int outside_strength(std::string_view place) const;  // Summe mit Schein nach außen, bis kMaxOutsideStrength

    // Handlungen (Figuren, Hausregeln, Prüfbefehle)
    void set_on(int i, bool on);       // anzünden (leere Füllung wird aufgefüllt) bzw. löschen
    void cover(int i, bool on);        // Feuer abdecken: keine Flamme, die Glut hält über Nacht
    int draw(int i, int liters);       // schöpfen: so viel kam
    void work(int i, int seconds);     // weiterarbeiten
    void set_value(int i, int v);      // Prüfbefehle: Brennstoff, Nässe, Liter, Arbeit
    void set_heat(int i, int v);

    // Eine Weltminute fortschreiben (minute: Minute des Tages)
    void step(int minute, const StateInput& in);
    // Ereignisse gehen auf diesen Bus (nullptr = keiner)
    void set_bus(EventBus* bus) { bus_ = bus; }
    // Brennstoff aus diesem Warenbuch (seit 0.5.10; nullptr = unbegrenzt); Weltzeit der Buchungen (die Bühne setzt sie
    // je Weltminute)
    void set_ledger(Ledger* ledger) { ledger_ = ledger; }
    void set_time(int64_t us) { now_us_ = us; }
    void set_day(int day) { day_ = day; }
    uint32_t version() const { return version_; }  // zählt jeden Stufenwechsel
    // Stufenwechsel der Zustände eines Ortes (seit 0.5.9 Teil 2: die Darstellung zeichnet nur nach eigenen Änderungen neu)
    uint32_t place_version(std::string_view place) const;

    // Speicherstand (seit 0.5.9, save.h): alles, was sich ändert, je Zustand; restore meldet nichts auf den Bus
    struct Saved {
        uint16_t value = 0, heat = 0, frac = 0;
        uint8_t bits = 0;  // 1 an, 2 abgedeckt, 4 Hausregel gewollt, 8 Hausregel angewendet
        uint8_t phase = 0;
    };
    Saved saved(int i) const;
    void restore(int i, const Saved& s);
    uint32_t restored() const { return restored_; }  // zählt eingespielte Zustände (Orte zeichnen dann neu)

private:
    struct Place {
        const char* id = nullptr;
        const char* house = nullptr;
        bool sky = true;
    };
    struct Entry {
        const StateDef* def = nullptr;
        uint8_t place = 0;
        uint16_t value = 0;
        uint16_t heat = 0;
        uint16_t frac = 0;
        bool on = false;
        bool covered = false;
        bool want = false;   // Hausregel: zuletzt gewollt (Wechsel lösen die Handlung aus)
        bool ruled = false;  // Hausregel schon einmal angewendet
        uint8_t phase = 0;
    };
    void reset(Entry& e) const;
    int compute_phase(const Entry& e) const;
    void update_phase(int i);
    // out: das Feuer ist in dieser Minute heruntergebrannt (Bedarf wird dann gemeldet, sonst nur am Beginn der Zeit)
    void apply_rule(int i, int minute, const StateInput& in, bool out);
    // Eine Füllung Brennstoff aus dem Warenbuch; tell = auch melden, wenn nichts da ist. false = nichts bekommen
    bool feed(int i, bool tell);
    void light(int i, bool on, bool tell);  // set_on mit Wahl, ob ein Mangel gemeldet wird

    std::array<Place, kMaxPlaces> places_{};
    int places_n_ = 0;
    std::array<Entry, kMax> e_{};
    std::array<uint16_t, kMax> rev_{};  // Stufenwechsel je Zustand
    int n_ = 0;
    EventBus* bus_ = nullptr;
    Ledger* ledger_ = nullptr;
    int64_t now_us_ = 0;
    int day_ = 0;
    uint32_t version_ = 0;
    uint32_t restored_ = 0;
};

}  // namespace pixelia
