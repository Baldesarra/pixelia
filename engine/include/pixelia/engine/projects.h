#pragma once
// Projekte (seit 0.5.10 Teil 3, SETTING.md Kapitel 8 „Bauen“ und „Großprojekte bestehen aus Teilaufgaben“, WIRTSCHAFT.md
// Kapitel 7 und 15): was gebaut wird, in Teilaufgaben mit eigenem Material, eigener Fähigkeit und sichtbarem
// Zwischenstand. Ein Modul des Weltzustands (Environment::projects) mit eigenem Abschnitt im Speicherstand (PROJ,
// world_save.cpp). Ersetzt den Ortszustand „angefangene Arbeit“ (StateKind::Work bis 0.5.10 Teil 2).
//
// - Daten der Gebiete (ProjectDef in EconomyDef, ledger.h): Ort, Baustelle (Lagerplatz der Art Site), Teil der
//   Raumliste, das entsteht, Teilaufgaben (ProjectStepDef: Material, Fähigkeit mit Mindeststufe, Arbeitsminuten).
// - Material (Wahl des Users 2026-10-29: ein Schritt voraus): Die Baustelle holt über Aufgaben (TaskList::add_fetch mit
//   kTaskProject) das Material des laufenden und des nächsten Schritts. Ein Schritt beginnt erst, wenn sein ganzes
//   Material an der Baustelle liegt; dann ist es dort für ihn reserviert (Halter = Kennung des Projekts). Wer daran
//   arbeitet, verbaut mit jedem Arbeitsgang so viel, wie die Arbeit fortgeschritten ist (der Stapel schrumpft mit der
//   Arbeit), aus der Reservierung; am Ende des Schritts ist alles genau einmal verbaut. Was nach dem Ende übrig ist,
//   bringt jemand zur Quelle zurück.
// - Fähigkeit: Arbeiten darf nur, wer die Fähigkeit des Schritts mindestens in der Mindeststufe kann (Wahl des Users:
//   nur wer Tischlern kann); er lernt dabei. Wer arbeitet, wählt bis 0.5.11 ein PLATZHALTER im Planer der Orte
//   (claim, worlds/testdorf/test_place.cpp), danach die Blockwahl des Tagesgerüsts.
// - Warten mit Grund (Karte, Protokoll): Material, Fähigkeit (niemand kann es), Arbeitskraft (niemand ist dran), Wetter
//   (draußen bei Regen und Schnee), Feierabend (Arbeitszeit 7–19 Uhr, PLATZHALTER bis 0.5.11, Wahl des Users). Die
//   Zeit zählt je Grund mit, ebenso Personensekunden und verbautes Material: projekte.csv (WIRTSCHAFT 15) schreibt
//   0.5.10 Teil 4 aus ProjectRecord (set_listener), je Teilaufgabe bei ihrem Abschluss.
// - Stufen der Simulation (world_sim.cpp, Regeln wie im Warenbuch): Schritte beginnen und enden, Material wird
//   reserviert, Aufgaben entstehen, Zuständigkeiten enden nur in step() an der Takt-Grenze (nach der Aufgabenliste).
//   Übernommen wird nur in Scene::decide genau an einer Takt-Grenze (claim), in der festen Reihenfolge der Orte. Den
//   Zwischenstand ändert nur die Figur, die daran arbeitet, an ihrem Ort (work in Scene::finish); aufhören meldet sie
//   nur an (leave), frei wird das Projekt an der nächsten Takt-Grenze. Auswahl nur nach festen Schlüsseln, die Liste
//   ist nach Kennung sortiert. So baut ein Ort sichtbar wie unsichtbar dasselbe (P10).
// - Kennungen ab kFirstId (0x8000): Halter im Warenbuch, getrennt von den Aufgaben (1 … TaskList::kMaxId).
// - Liegt im Grafik-Speicher (auf dem Gerät PSRAM, ~2,5 KB), angelegt beim ersten Mal.

#include <array>
#include <cstdint>

#include "pixelia/engine/figure.h"

namespace pixelia {

class Ledger;
class TaskList;
class EventBus;
class SaveWriter;
class SaveReader;
struct EconomyDef;

// Art eines Projekts (projekte.csv, SETTING 8 „Dauer in echter Zeit“)
enum class ProjectKind : uint8_t { Small, Furniture, Shelter, Room, Upgrade, Large, Railway, kCount };
const char* project_kind_id(ProjectKind k);  // klein, moebel, notunterkunft, raum, ausbaustufe, grossbau, lok

// Warum ein Projekt gerade nicht weitergeht
enum class ProjectWait : uint8_t { None, Material, Skill, Worker, Weather, Rest, kCount };
inline constexpr int kProjectWaits = static_cast<int>(ProjectWait::kCount);
const char* project_wait_id(ProjectWait w);  // keiner, material, faehigkeit, arbeitskraft, wetter, feierabend

// --- Daten der Gebiete ---
struct ProjectNeed {
    const char* good;  // Kennung der Ware
    uint16_t qty;
};

struct ProjectStepDef {
    const char* id;     // gestell (Protokoll, Speicherstand, Befehle), ASCII
    const char* title;  // Gestell zimmern (Karte, Chronik)
    const char* label;  // Gestell (Schritt-Leiste der Karte, kurz)
    const ProjectNeed* needs = nullptr;
    uint8_t need_count = 0;
    Skill skill = Skill::Joinery;
    SkillLevel level = SkillLevel::Beginner;  // Unknown = jeder
    uint16_t minutes = 60;                    // Arbeitsminuten einer Person bis fertig
};

struct ProjectDef {
    const char* id;          // kammer_bett
    const char* title;       // Bett bauen
    ProjectKind kind = ProjectKind::Furniture;
    const char* place;       // Ort (Kennung)
    const char* site;        // Baustelle: Lagerplatz der Art Site an diesem Ort
    const char* source = nullptr;  // Material von hier; nullptr = Gemeinschaftslager mit der Ware
    int8_t part = -1;        // Teil der Raumliste, das entsteht (im Bau: Bau-Bild in Stufen), −1 = keins
    const ProjectStepDef* steps = nullptr;
    uint8_t step_count = 0;
    bool outdoor = false;    // draußen: wartet bei Regen, Schnee und Gewitter
    bool start = false;      // beginnt in einer neuen Welt (Testinhalt) und mit einem alten Stand ohne Projekte
};

// --- Laufzeit ---
struct Project {
    static constexpr int kMaxNeeds = 4;
    uint16_t id = 0;
    uint8_t def = 0;       // Nummer in der Datenliste
    uint8_t step = 0;      // laufender Schritt; = Zahl der Schritte: fertig
    bool begun = false;    // Material des Schritts reserviert, die Arbeit kann laufen
    bool leaving = false;  // wer daran arbeitet, hört auf (frei an der nächsten Takt-Grenze)
    ProjectWait wait = ProjectWait::None;
    uint16_t who = 0;      // Figur, die daran arbeitet, 0 = niemand
    uint32_t work_s = 0;   // Zwischenstand: Arbeit am laufenden Schritt in Weltsekunden
    std::array<uint16_t, kMaxNeeds> used{};  // im laufenden Schritt verbaut (je Bedarf)
    std::array<uint16_t, kMaxNeeds> res{};   // Reservierung an der Baustelle (je Bedarf), 0 = keine
    // Protokoll (projekte.csv): laufender Schritt und ganzes Projekt
    int64_t started_us = INT64_MIN;  // Beginn des Projekts (INT64_MIN: an der ersten Takt-Grenze)
    int64_t step_us = INT64_MIN;     // Beginn des laufenden Schritts (seit dem Ende des vorigen)
    int64_t done_us = INT64_MIN;     // fertig
    int64_t seen_us = INT64_MIN;     // letzte Takt-Grenze (Wartezeit zählt von dort)
    uint32_t person_s = 0;           // Arbeit aller Figuren am laufenden Schritt (Personensekunden)
    std::array<uint32_t, kProjectWaits> wait_s{};  // Wartezeit je Grund im laufenden Schritt (Weltsekunden)
    uint32_t total_person_s = 0;
    std::array<uint32_t, kProjectWaits> total_wait_s{};
    std::array<uint16_t, 8> built{};  // verbaut insgesamt je Ware des Projekts (Reihenfolge der Daten)
    bool done(int step_count) const { return step >= step_count; }
};

// Ein abgeschlossener Projektabschnitt (an den Beobachter: projekte.csv ab 0.5.10 Teil 4)
struct ProjectRecord {
    const ProjectDef* def = nullptr;
    const ProjectStepDef* step = nullptr;  // nullptr = Abschluss des ganzen Projekts
    uint16_t id = 0;
    int64_t begin_us = 0, end_us = 0;      // tatsächliche Dauer
    uint32_t planned_s = 0;                // geplante Arbeit (Arbeitsminuten × 60, eine Person)
    uint32_t person_s = 0;                 // Personensekunden (÷ 28 800 = Personentage zu 8 Stunden)
    std::array<uint32_t, kProjectWaits> wait_s{};
    // verbautes Material: Nummern der Waren im Warenbuch und Mengen
    uint8_t goods[Project::kMaxNeeds * 4] = {};
    uint16_t qty[Project::kMaxNeeds * 4] = {};
    uint8_t good_count = 0;
};

class ProjectList {
public:
    static constexpr int kMax = 16;
    static constexpr int kMaxDefs = 32;
    static constexpr uint16_t kFirstId = 0x8000;  // Halter im Warenbuch: 0x8000 … 0xFFFF
    static constexpr int kAheadSteps = 1;         // Material so viele Schritte voraus holen (Wahl des Users)
    static constexpr int kMaxTrips = 4;           // neue Holen-Aufgaben je Bedarf und Takt-Grenze
    // PLATZHALTER bis 0.5.11 (Tagesgerüst): gearbeitet wird von 7 bis 19 Uhr (Wahl des Users 2026-10-29)
    static constexpr int kWorkFrom = 7 * 60, kWorkTo = 19 * 60;
    static bool work_time(int64_t at_us);  // Arbeitszeit zu dieser Weltzeit?

    ProjectList() = default;
    ~ProjectList();
    ProjectList(const ProjectList&) = delete;
    ProjectList& operator=(const ProjectList&) = delete;

    // Datenliste aus den Gebieten (world_areas) bzw. einer eigenen Liste (Tests); nach Ledger::build. Leert alles
    void build(const Ledger& ledger);
    void build(const EconomyDef* const* defs, int n, const Ledger& ledger);
    void clear();         // keine Projekte, Kennungen von vorn
    void start_new();     // Projekte der Daten mit start beginnen (neue Welt, alter Stand ohne Projekte)
    int defs() const { return def_n_; }
    const ProjectDef& def(int d) const;
    int find_def(const char* id) const;  // −1 = gibt es nicht

    // An der Takt-Grenze (Bühne, nach TaskList::step): Zuständigkeiten aufräumen, Schritte abschließen und beginnen,
    // Material reservieren, Holen-Aufgaben anlegen (in fester Reihenfolge der Projekte und Bedarfe), Wartezeit zählen.
    // Fertige Teilaufgabe und fertiges Projekt meldet es auf den Bus (Ereignis Project, für die Chronik). bad_weather:
    // es regnet, schneit oder gewittert (draußen wird nicht gebaut)
    void step(int64_t at_us, int32_t day, Ledger& ledger, TaskList& tasks, const FigureBook& figures, EventBus* bus,
              bool bad_weather);

    int count() const { return n_; }
    const Project& at(int i) const;  // nach Kennung sortiert
    const Project* find(uint16_t id) const;
    const Project* of(uint16_t who) const;  // Projekt, an dem die Figur arbeitet (nicht fertig)
    const Project* at_place(int place, int part) const;  // laufendes Projekt dieses Teils an diesem Ort (Karte, Bild)
    int place_of(const Project& p) const;   // Nummer des Ortes in der Welt
    int site_of(const Project& p) const;    // Lagerplatz der Baustelle
    const ProjectStepDef* current(const Project& p) const;  // nullptr = fertig
    uint32_t step_total_s(const Project& p) const;          // Arbeit des laufenden Schritts in Weltsekunden
    // Kann die Figur am laufenden Schritt arbeiten (Fähigkeit und Stufe)?
    bool can_work(const Project& p, const FigureRecord& f) const;

    // Übernehmen (nur in Scene::decide genau an einer Takt-Grenze): Schritt begonnen, niemand dran, die Figur kann es
    // und arbeitet an keinem anderen Projekt (die Aufgabenliste prüft der Planer)
    bool claim(uint16_t id, const FigureRecord& f, int64_t at_us);
    // Arbeitsgang (Scene::finish der Figur am Ort der Baustelle): seconds Arbeit, höchstens bis der Schritt fertig ist;
    // verbaut so viel Material, wie der Fortschritt verlangt (aufgerundet), aus der Reservierung; lernt die Fähigkeit.
    // Rückgabe: angerechnete Sekunden
    int work(uint16_t id, uint16_t who, int seconds, int64_t at_us, int32_t day, Ledger& ledger, FigureBook* figures);
    void leave(uint16_t id, uint16_t who);  // aufhören (frei an der nächsten Takt-Grenze)

    // Prüfbefehle (Konsole, Seite Experimentell; nicht für die Simulation): neu beginnen (laufendes Projekt derselben
    // Daten fällt weg, sein Material bleibt an der Baustelle), Teilaufgaben bis vor n als fertig buchen (verbaut, was
    // an der Baustelle liegt; die Chronik bekommt die Meldungen), alles fertig = step_count. false = gibt es nicht
    bool test_start(int def, int64_t at_us, Ledger& ledger, TaskList& tasks);
    bool test_jump(int def, int step, int64_t at_us, int32_t day, Ledger& ledger, TaskList& tasks, EventBus* bus);

    // Eine Zeile für Log und Konsole („projekte“)
    const char* describe(int i, const Ledger& ledger, char* buf, int n) const;
    // Warten in einem Satz für die Karte („wartet auf 2 Bretter“, „wartet auf jemanden mit Tischlern“); leer = arbeitet
    const char* wait_text(const Project& p, const Ledger& ledger, char* buf, int n) const;
    uint32_t version() const { return version_; }  // zählt Schritte, Zuständigkeiten, neue und fertige Projekte
    uint32_t progress() const { return progress_; }  // zählt dazu jeden Arbeitsgang (Darstellung)

    // Prüfung: Kennungen steigen, Figuren höchstens einmal, Reservierungen nicht größer als der Bedarf, verbaut nicht
    // mehr als der Bedarf, Zwischenstand nicht über dem Schritt; nullptr = alles gut
    const char* check(const Ledger& ledger, char* buf, int n) const;

    using Listener = void (*)(void* ctx, const ProjectRecord& r);
    void set_listener(Listener fn, void* ctx) {
        on_record_ = fn;
        listen_ctx_ = ctx;
    }

    // --- Speicherstand (world_save.cpp, Abschnitt PROJ): Projekte, Teilaufgaben und Waren mit Kennung ---
    void save(SaveWriter& w, const Ledger& ledger) const;
    int load(SaveReader& r, uint16_t version, const Ledger& ledger);  // nach clear(); Rückgabe: geladene Projekte

private:
    struct DefData;  // aufgelöste Kennungen einer Datenliste (Ort, Lagerplätze, Waren je Schritt)
    struct Storage;  // Projekte und Daten im Grafik-Speicher
    bool ensure();
    Project* add(int def, int64_t at_us);
    Project* get(uint16_t id);
    int source_for(const DefData& d, const Ledger& l, int good) const;  // Quelle einer Ware, −1 = keine
    void finish_step(Project& p, int64_t at_us, int32_t day, Ledger& l, EventBus* bus);
    void fetch(Project& p, int64_t at_us, Ledger& l, TaskList& tasks);
    void tidy(Project& p, int64_t at_us, Ledger& l, TaskList& tasks);  // fertig: Übriges zurückbringen
    bool begin(Project& p, Ledger& l);
    void reset_step(Project& p);
    ProjectWait wait_of(const Project& p, int64_t at_us, const FigureBook& book, bool bad_weather) const;

    Storage* s_ = nullptr;
    int n_ = 0;
    uint16_t next_ = kFirstId;
    int def_n_ = 0;
    uint32_t version_ = 0, progress_ = 0;
    bool orphans_ = true;  // Reservierungen ohne Projekt beim nächsten step freigeben (nach Laden, clear)
    Listener on_record_ = nullptr;
    void* listen_ctx_ = nullptr;
};

}  // namespace pixelia
