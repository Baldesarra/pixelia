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
//   Rest im Puffer, das Nachholen bucht ihn neu: jede Buchung steht genau einmal im Protokoll. Schlägt das Schreiben
//   fehl, bleibt der Teil im Puffer und kommt mit dem nächsten Stand. Fällt der Strom genau zwischen Speicherstand und
//   Anhängen (wenige Millisekunden), fehlen diese Zeilen (nie doppelt).
// - Fäden: Zeilen entstehen und cut() läuft auf Kern 1 (Grafik-Task), flush() auf Kern 0 (Lade-Task). Der
//   Speicher-Ablauf in gfx.cpp sorgt dafür, dass cut() und flush() nie gleichzeitig laufen (Zustand 1 → 2 → 0).
// - Grenze: höchstens kMaxBytes für alle Dateien; darüber werden die ältesten Tage gelöscht.
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
    static constexpr size_t kMaxBytes = 1024 * 1024;  // alle Dateien zusammen (Wahl des Users: 1 MB in LittleFS)
    enum Table : uint8_t { Moves, Projects, Demand, StockDay, WeatherDay, kTables };
    // bewegungen, projekte, bedarf, bestand_lager_tag, wetter_tag
    static const char* table_name(int t);

    struct Stats {
        uint32_t rows = 0;       // Zeilen seit dem Start
        uint32_t lost = 0;       // Zeilen verworfen (Puffer voll)
        uint32_t written = 0;    // Zeilen angehängt
        uint32_t buffered = 0;   // Bytes im Puffer (laufend und ausstehend)
        uint32_t files = 0, bytes = 0;  // Dateien und Bytes nach dem letzten Anhängen
        uint32_t flush_ms = 0;   // Dauer des letzten Anhängens
        uint32_t deleted = 0;    // gelöschte Dateien (älteste Tage) seit dem Start
        int32_t first_day = 0, last_day = 0;  // Welttage im Ordner
        bool error = false;      // letztes Anhängen fehlgeschlagen
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

    // Nach jedem Schritt der Bühne (Kern 1): Stufe der Simulation (voll am betretenen Ort, grob, nachgeholt),
    // Wetter morgens/mittags/abends/nachts, beim Tageswechsel Bestände und Wetter des vergangenen Tages
    void step(const Stage& stage);
    // Beim Packen des Speicherstands (Kern 1): alles bis jetzt gehört zu diesem Stand
    void cut();
    bool pending() const { return pend_len_ > 0; }
    // Nach dem erfolgreichen Schreiben des Speicherstands (Kern 0): ausstehende Zeilen an die Dateien in dir hängen,
    // dann über kMaxBytes die ältesten Tage löschen. false = Fehler (die Zeilen bleiben für das nächste Mal)
    bool flush(const char* dir);
    // Puffer zu drei Vierteln voll: bitte bald speichern (Nachholen, Zeitraffer)
    bool wants_save() const { return cap_ && cur_len_ > cap_ * 3 / 4; }
    // Ordner zählen (Dateien, Bytes, Tage), ohne zu schreiben
    void scan(const char* dir);
    Stats stats() const { return st_; }

private:
    void row(Table t, int32_t day, const char* text, int len);
    int32_t day_of(int64_t at_us) const;
    void minute_text(int64_t at_us, char* out) const;
    void on_booking(const BookingRecord& r);
    void on_demand(const DemandRecord& r);
    void on_project(const ProjectRecord& r);
    void day_rows(int32_t day);
    bool enforce(const char* dir);

    Environment* env_ = nullptr;
    const char* source_ = "simulator";
    uint8_t* cur_ = nullptr;   // laufend (Kern 1)
    uint8_t* pend_ = nullptr;  // ausstehend bis zum Anhängen
    size_t cap_ = 0, cur_len_ = 0, pend_len_ = 0;
    int visible_ = -1;
    bool catching_ = false;
    int32_t day_ = INT32_MIN;          // laufender Welttag (step)
    int last_minute_ = -1;
    uint8_t weather_[4] = {0xFF, 0xFF, 0xFF, 0xFF};  // morgens 6, mittags 12, abends 18, nachts 0 Uhr
    Stats st_{};
};

}  // namespace pixelia
