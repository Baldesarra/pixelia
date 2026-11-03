#pragma once
// Wirtschafts-Protokoll (seit 0.5.10 Teil 4, WIRTSCHAFT.md Kapitel 15): Buchungen, Bedarf, Projektabschnitte,
// Lagerbestände am Tagesende und das Wetter je Welttag als CSV, je Welttag und Tabelle eine Datei
// (<dir>/t00003-bewegungen.csv). Gerät und Simulator schreiben dieselben Dateien.
//
// - Quelle: Beobachter am Warenbuch (Ledger::set_listener: Buchungen, Bedarf) und an den Projekten
//   (ProjectList::set_listener); Tageswerte und Wetter aus step() nach jedem Schritt der Bühne.
// - Protokoll und Speicherstand passen zusammen (Ergänzung A des Planungs-Chats): Die Zeilen liegen im Puffer (Gerät:
//   PSRAM) und werden erst angehängt, wenn der Speicherstand geschrieben ist. cut() beim Packen des Stands trennt, was
//   zu ihm gehört; flush() nach dem erfolgreichen Schreiben hängt genau das an. Ein Neustart ohne Speichern verwirft den
//   Rest im Puffer, das Nachholen bucht ihn neu: jede Buchung steht genau einmal im Protokoll.
// - Journal (seit 0.5.11, B04, Wahl des Users 2026-11-03): prepare() schreibt die ausstehenden Zeilen vor dem
//   Speicherstand als Stapel mit Generation, Zeitlinie, Prüfsumme und der Größe jeder Zieldatei davor nach
//   <dir>/journal.pxj. Fällt der Strom zwischen Speicherstand und Anhängen, trägt open() den Stapel beim Start genau
//   einmal nach (Generation ≤ geladener Stand; sonst gehört er zu einem nicht geschriebenen Stand und wird verworfen).
//   Angehängt wird je Datei mit Größenprüfung: steht der Teil schon da, wird er übersprungen, steht er halb da (Fehler
//   mitten in einer Zeile), wird die Datei auf die alte Größe gekürzt und neu angehängt. Schlägt eine Datei fehl,
//   bleiben genau ihre Zeilen im Puffer.
// - Grenzen (seit 0.5.11, B08): höchstens kMaxBytes für alles (laufende und alte Zeitlinien, Journal) schon vor dem
//   Anhängen; dafür werden die ältesten Tage gelöscht (alte Zeitlinien zuerst), nie ein Tag, an den gerade angehängt
//   wird. Ein Welttag darf höchstens kMaxDayBytes belegen (Wahl des Users 2026-11-03): weitere Zeilen werden gezählt
//   statt geschrieben (Stats::dropped, Vermerk in tNNNNN-verluste.csv); Tagesbestände und Wetter gelten nicht mit. Mit
//   set_free_space bleibt auf dem Dateisystem immer kReserveBytes frei für die Weltstände.
// - Zeitlinien (seit 0.5.11, B09, Wahl des Users 2026-11-03): <dir>/zeitlinie.txt nennt Welt und Zeitlinie der
//   Dateien. Passt sie beim Start nicht zum geladenen Stand (Sicherung eingespielt, Rückfall auf den älteren Platz,
//   fremde Welt), wandert der Ordner nach <dir>-alt/<n>; alte Zeitlinien zählen zur Grenze und gehen zuerst.
// - Tagesaufnahme (seit 0.5.11): bestand_lager_tag zeigt den Bestand genau um Mitternacht. Buchungen nach Mitternacht
//   im selben Schritt der Bühne werden herausgerechnet, so gilt Bestand am Vortag + Buchungen = Bestand am Tagesende.
// - Fäden: Zeilen entstehen und cut() läuft auf Kern 1 (Grafik-Task), prepare() und flush() auf Kern 0 (Lade-Task).
//   Der Speicher-Ablauf in gfx.cpp sorgt dafür, dass cut() und prepare()/flush() nie gleichzeitig laufen.
// - Format: UTF-8, Komma, Kopfzeile „#pixelia-wirtschaft,format=1,tabelle=…,quelle=…“, dann die Spaltennamen.
//   Personen als Kennung der Figur (Rollen kommen mit 0.6.x), Lager und Waren als Kennung der Daten.

#include <cstddef>
#include <cstdint>

namespace pixelia {

class Stage;
struct Environment;
struct BookingRecord;
struct DemandRecord;
struct ProjectRecord;

class EconomyLog {
public:
    static constexpr int kFormat = 1;
    static constexpr size_t kMaxBytes = 1024 * 1024;     // alles zusammen (Wahl des Users: 1 MB in LittleFS)
    static constexpr size_t kMaxDayBytes = 256 * 1024;   // ein Welttag ohne Tagesbestände und Wetter (B08)
    static constexpr size_t kReserveBytes = 256 * 1024;  // frei für die Weltstände (nur mit set_free_space)
    enum Table : uint8_t { Moves, Projects, Demand, StockDay, WeatherDay, Lost, kTables };
    // bewegungen, projekte, bedarf, bestand_lager_tag, wetter_tag, verluste
    static const char* table_name(int t);

    struct Stats {
        uint32_t rows = 0;       // Zeilen seit dem Start
        uint32_t lost = 0;       // Zeilen verworfen (Puffer voll)
        uint32_t written = 0;    // Zeilen angehängt (bestätigt)
        uint32_t buffered = 0;   // Bytes im Puffer (laufend und ausstehend)
        uint32_t files = 0, bytes = 0;  // Dateien und Bytes der laufenden Zeitlinie nach dem letzten Anhängen
        uint32_t flush_ms = 0;   // Dauer des letzten Anhängens
        uint32_t deleted = 0;    // gelöschte Dateien (älteste Tage) seit dem Start
        int32_t first_day = 0, last_day = 0;  // Welttage im Ordner
        bool error = false;      // letztes Anhängen fehlgeschlagen
        // seit 0.5.11
        uint32_t dropped = 0;     // Zeilen nicht geschrieben: Tag zu voll (B08)
        uint32_t late = 0;        // Buchungen für einen Tag, dessen Bestandsaufnahme schon steht (sollte 0 sein)
        uint32_t recovered = 0;   // Zeilen beim Start aus dem Journal nachgetragen
        uint32_t discarded = 0;   // Zeilen im Journal verworfen (Stand nie geschrieben, andere Zeitlinie)
        uint32_t archives = 0, archive_bytes = 0;  // alte Zeitlinien (B09)
        uint32_t journal_bytes = 0;  // letztes Journal
        bool full = false;          // letztes Anhängen verschoben: kein Platz, auch nach dem Löschen alter Tage
        bool cleanup_error = false; // Löschen alter Tage schlug fehl
    };

    EconomyLog() = default;
    ~EconomyLog();
    EconomyLog(const EconomyLog&) = delete;
    EconomyLog& operator=(const EconomyLog&) = delete;

    // Puffer anlegen (zweimal buffer_bytes im Grafik-Speicher) und an Warenbuch und Projekte hängen. source:
    // „geraet“ oder „simulator“ (Kopfzeile). false = kein Speicher
    bool attach(Environment& env, size_t buffer_bytes, const char* source);
    void detach();
    bool attached() const { return env_ != nullptr; }

    // Beim Start nach dem Laden des Speicherstands (seit 0.5.11): Journal nachtragen bzw. verwerfen (loaded_gen =
    // Generation des geladenen Stands, 0 = keiner), Ordner einer anderen Zeitlinie archivieren, Marke schreiben, zählen
    void open(const char* dir, uint32_t world, uint32_t line, uint32_t loaded_gen);
    // Nach jedem Schritt der Bühne (Kern 1): Stufe der Simulation (voll am betretenen Ort, grob, nachgeholt),
    // Wetter morgens/mittags/abends/nachts, beim Tageswechsel Bestände und Wetter des vergangenen Tages
    void step(const Stage& stage);
    // Beim Packen des Speicherstands (Kern 1): alles bis jetzt gehört zu diesem Stand
    void cut();
    bool pending() const { return pend_len_ > 0; }
    // Vor dem Schreiben des Speicherstands generation (Kern 0, seit 0.5.11): Platz schaffen, Journal schreiben.
    // false = kein Journal (dann hängt flush trotzdem an, wie vor 0.5.11)
    bool prepare(const char* dir, uint32_t generation);
    // Nach dem erfolgreichen Schreiben des Speicherstands (Kern 0): ausstehende Zeilen an die Dateien in dir hängen.
    // false = Fehler (die Zeilen der betroffenen Dateien bleiben für das nächste Mal)
    bool flush(const char* dir);
    // Nach einem fehlgeschlagenen Speicherstand (seit 0.5.11): Plan von prepare verwerfen, Zeilen bleiben im Puffer
    void abandon();
    // Puffer zu drei Vierteln voll: bitte bald speichern (Nachholen, Zeitraffer)
    bool wants_save() const { return cap_ && cur_len_ > cap_ * 3 / 4; }
    // Freier Platz des Dateisystems in Byte (Gerät, seit 0.5.11); ohne Angabe gilt nur kMaxBytes
    void set_free_space(int64_t (*fn)(void*), void* ctx) {
        free_fn_ = fn;
        free_ctx_ = ctx;
    }
    // Ordner zählen (Dateien, Bytes, Tage, alte Zeitlinien), ohne zu schreiben
    void scan(const char* dir);
    Stats stats() const { return st_; }
    // Bestände je Lagerplatz und Ware sowie das Wetter des Tages als Zeilen (sonst beim Tageswechsel in step; seit
    // 0.5.11 öffentlich für die Prüfungen: jede Ware eines Lagerplatzes, B12)
    void day_rows(int32_t day);

    // Nur für Prüfungen (seit 0.5.11, B04): Fehler beim Öffnen der n-ten Datei, nach budget Byte, beim n-ten Schließen
    struct Fault {
        int open_fail = -1;
        long write_budget = -1;
        int close_fail = -1;
    };
    static void set_fault(const Fault& f);

private:
    struct Group;
    struct DayUse;
    struct Delta {
        int16_t store;
        uint8_t good;
        int32_t day;
        int32_t qty;
    };
    static constexpr int kMaxGroups = 512;  // Dateien je Anhängen (Simulator im Zeitraffer: bis 60 Tage auf einmal)
    static constexpr int kMaxDays = 128;
    static constexpr int kMaxDeltas = 64;

    void row(Table t, int32_t day, const char* text, int len);
    int32_t day_of(int64_t at_us) const;
    void minute_text(int64_t at_us, char* out) const;
    void on_booking(const BookingRecord& r);
    void on_demand(const DemandRecord& r);
    void on_project(const ProjectRecord& r);
    void note_delta(int store, int good, int32_t day, int qty);
    int plan(const char* dir);
    bool make_room(const char* dir, uint32_t need, int groups);
    bool write_journal(const char* dir, uint32_t generation, int groups);
    void recover(const char* dir, uint32_t loaded_gen);

    Environment* env_ = nullptr;
    const char* source_ = "simulator";
    uint8_t* cur_ = nullptr;   // laufend (Kern 1)
    uint8_t* pend_ = nullptr;  // ausstehend bis zum Anhängen
    size_t cap_ = 0, cur_len_ = 0, pend_len_ = 0;
    Group* groups_ = nullptr;  // Plan von prepare (Grafik-Speicher, mit use_ und deltas_ zusammen ~13 KB)
    DayUse* use_ = nullptr;
    int planned_ = -1;         // Gruppen im Plan, −1 = keiner
    bool journal_ = false;     // Plan steht im Journal
    Delta* deltas_ = nullptr;  // Buchungen nach Mitternacht im laufenden Schritt (Grafik-Speicher)
    int delta_n_ = 0;
    int visible_ = -1;
    bool catching_ = false;
    int32_t day_ = INT32_MIN;          // laufender Welttag (step)
    int last_minute_ = -1;
    uint8_t weather_[4] = {0xFF, 0xFF, 0xFF, 0xFF};  // morgens 6, mittags 12, abends 18, nachts 0 Uhr
    uint32_t world_ = 0, line_ = 0;    // Zeitlinie der Dateien (open)
    int64_t (*free_fn_)(void*) = nullptr;
    void* free_ctx_ = nullptr;
    Stats st_{};
};

}  // namespace pixelia
