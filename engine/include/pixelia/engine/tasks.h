#pragma once
// Aufgabenliste der Gemeinschaft (seit 0.5.10 Teil 2, WIRTSCHAFT.md Kapitel 6 und 13, SETTING.md Kapitel 8): was im
// Dorf zu tun ist, damit kein Lager leer wird. Ein Modul des Weltzustands (Environment::tasks) mit eigenem Abschnitt im
// Speicherstand (AUFG, world_save.cpp).
//
// - Holen und Bringen (TaskKind::Fetch): Ware, Menge, von, nach, Reservierung, wer, Phase. Entsteht aus den Regeln der
//   Lagerplätze (StockRule, ledger.h): Fällt ein Vorrat unter low, holt jemand Nachschub bis want von seiner Quelle
//   (StoreDef::source). Die Menge passt in einen leeren Korb (Ledger::kHandCapacity nach Gewicht) und liegt am
//   Quell-Lager reserviert, bis sie geholt ist: Niemand sonst nimmt sie weg. Ist dort nicht genug, wartet die Aufgabe,
//   außer der Vorrat ist schon leer (dann holt jemand, was da ist).
// - Herstellen (TaskKind::Produce): Ein Erzeugerlager unter low füllt jemand am Arbeitsplatz auf want auf.
// - Notbestand (eine Ware reicht im Dorf keinen Tag, WIRTSCHAFT 13): Herstellen wird Gemeinschaftsaufgabe, bis zu
//   kHelpers Figuren helfen zugleich („Holzaktion“), bis die Ware wieder einen Tag reicht.
// - Stufen der Simulation (world_sim.cpp, Regeln wie im Warenbuch): Aufgaben entstehen, reservieren und enden nur in
//   step() an der Takt-Grenze, nach allen Planern. Übernommen werden sie nur in Scene::decide mit at genau auf einer
//   Takt-Grenze (claim), in der festen Reihenfolge der Orte. Danach gehört eine Aufgabe ihrer Figur: Nur sie ändert
//   Phase und Getragenes (in Scene::finish an ihrem Ort); andere sehen nur offene Aufgaben, step() nur an der
//   Takt-Grenze. Die Liste ist nach Kennung sortiert, Auswahl nur nach festen Schlüsseln.
// - Kennungen 1 … kMaxId sind zugleich Halter ihrer Reservierung im Warenbuch (Projekte ab 0.5.10 Teil 3 darüber).
// - Liegt im Grafik-Speicher (auf dem Gerät PSRAM, ~1,3 KB), angelegt beim ersten Mal.

#include <cstdint>

namespace pixelia {

class Ledger;
class FigureBook;
class EventBus;
class SaveWriter;
class SaveReader;

enum class TaskKind : uint8_t { Fetch, Produce, kCount };  // Holen und Bringen, Herstellen

enum class TaskPhase : uint8_t {
    Open,    // wartet auf jemanden
    Fetch,   // unterwegs zum Quell-Lager
    Bring,   // Ware in der Trage, unterwegs zum Ziel
    Return,  // Rest passte nicht ins Ziel: zurück zur Quelle
    Work,    // stellt am Arbeitsplatz her
    Done,    // fertig (step räumt an der nächsten Takt-Grenze auf)
    kCount
};
const char* task_phase_id(TaskPhase p);  // offen, holen, bringen, zurueck, arbeiten, fertig (Speicherstand, Log)

enum TaskFlag : uint8_t { kTaskCommunity = 1 };  // Gemeinschaftsaufgabe (Notbestand)

struct Task {
    uint16_t id = 0;
    TaskKind kind = TaskKind::Fetch;
    TaskPhase phase = TaskPhase::Open;
    uint8_t flags = 0;
    uint8_t good = 0;            // Ware (Nummer im Warenbuch)
    int16_t from = -1, to = -1;  // Lagerplätze; Herstellen: to = Erzeugerlager, from = −1
    uint16_t qty = 0;            // Holen: Menge (reserviert); Herstellen: bis so viel im Lager liegt (want)
    uint16_t carried = 0;        // Holen: in der Trage der Figur
    uint16_t res = 0;            // Reservierung am Quell-Lager, 0 = keine (mehr)
    uint16_t who = 0;            // Figur, 0 = offen
    int64_t created_us = 0, claimed_us = 0;
    bool active() const { return phase != TaskPhase::Done; }
};

class TaskList {
public:
    static constexpr int kMax = 32;
    static constexpr int kHelpers = 3;          // Gemeinschaftsaufgabe: so viele helfen zugleich
    static constexpr uint16_t kMaxId = 0x7FFF;  // Kennungen 1 … kMaxId (Halter im Warenbuch)

    TaskList() = default;
    ~TaskList();
    TaskList(const TaskList&) = delete;
    TaskList& operator=(const TaskList&) = delete;

    void clear();  // keine Aufgaben, Kennungen von vorn (Reservierungen im Warenbuch bleiben: step gibt sie frei)
    // An der Takt-Grenze (Bühne, nach allen Planern und Ledger::step): Fertiges aufräumen, Herstellen beenden, neue
    // Aufgaben aus den Regeln der Lagerplätze (in fester Reihenfolge der Lagerplätze und Regeln), Notbestand. Beginnt
    // eine Gemeinschaftsaufgabe, meldet sie das auf den Bus (Ereignis Stock, a = 3, für die Chronik)
    void step(int64_t at_us, int32_t day, Ledger& ledger, const FigureBook& figures, EventBus* bus = nullptr);

    int count() const { return n_; }
    const Task& at(int i) const { return t_[i]; }  // nach Kennung sortiert
    const Task* find(uint16_t id) const;
    // laufende Aufgabe einer Figur (nicht fertig); ändern darf sie nur die Figur selbst
    Task* of(uint16_t who);
    const Task* of(uint16_t who) const;
    // Übernehmen (nur in Scene::decide genau an einer Takt-Grenze): offen und die Figur hat keine andere
    bool claim(uint16_t id, uint16_t who, int64_t at_us);
    // eine Zeile für Log und Konsole („aufgaben“): „#3 holen 0/9 Brennholz Holzstapel → Holzlege, Figur 1004“
    const char* describe(int i, const Ledger& ledger, char* buf, int n) const;
    uint32_t version() const { return version_; }  // zählt neue, übernommene und aufgeräumte Aufgaben
    uint32_t created() const { return created_; }  // Aufgaben seit clear() (Messung, Prüfungen)

    // Prüfung: Kennungen steigen, Figuren höchstens einmal, Reservierung nicht größer als die Menge, Getragenes liegt
    // in der Trage; nullptr = alles gut
    const char* check(const Ledger& ledger, char* buf, int n) const;

    // --- Speicherstand (world_save.cpp, Abschnitt AUFG): Waren und Lagerplätze mit Kennung ---
    void save(SaveWriter& w, const Ledger& ledger) const;
    int load(SaveReader& r, uint16_t version, const Ledger& ledger);  // nach clear(); Rückgabe: geladene Aufgaben

private:
    bool ensure();
    Task* add(TaskKind kind, int good, int from, int to, int qty, int64_t at_us, uint8_t flags);
    void remove(int i);
    int active(TaskKind kind, int to, int good, int flags_mask, int flags) const;

    Task* t_ = nullptr;  // kMax Plätze
    int n_ = 0;
    uint16_t next_ = 1;
    uint32_t version_ = 0, created_ = 0;
    bool orphans_ = true;  // Reservierungen ohne Aufgabe beim nächsten step freigeben (nach Laden, clear)
};

}  // namespace pixelia
