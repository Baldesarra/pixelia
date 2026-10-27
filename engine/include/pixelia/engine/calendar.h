#pragma once
// Weltkalender (seit 0.5.7, SETTING.md Kapitel 9): Tageszeit = echte Zeit, Jahreszeiten beschleunigt.
//
// - Tag 1 ist der Spielstart. Eine Jahreszeit dauert 7 echte Tage (einstellbar), ein Weltjahr vier Jahreszeiten,
//   begonnen wird mit dem Frühling.
// - Bei 7 Tagen wechselt die Jahreszeit in der Nacht von Sonntag auf Montag; der Sonntagabend ist der
//   Jahreszeitenabend. Der erste Frühling dauert bis zum ersten Sonntag, der mindestens eine Woche nach dem Start
//   liegt (7–13 Tage). Bei anderer Länge ist der Jahreszeitenabend am letzten Tag der Jahreszeit.
// - Der Kalender ist eine reine Rechnung aus dem Zustand (CalendarState) und dem heutigen Datum: Zeitraffer und
//   Testuhr dürfen vor- und zurückspringen, ohne etwas kaputt zu machen. Gespeichert wird nur der Zustand (Start,
//   Länge, Anker nach einer Längenänderung, Erstlinge); bis zum Speicherstand (0.5.9) als kleiner Block im NVS.
// - Zeitfenster (morgens … nachts) und Schlafenszeiten als Schnittstelle fürs Tagesgerüst (0.5.11).
// - Sonnenauf- und -untergang nach der Weltjahreszeit (Dämmerung, light.h Daylight).
// - Ereignisse (Jahreszeitenabend, Jahreszeitenwechsel, Erstlinge) gehen seit 0.5.8 auf den Ereignis-Bus
//   (events.h, set_bus); die Anzeige macht daraus Kurzmeldungen. Ohne Bus zählen nur die Erstlinge.

#include <cstdint>
#include <string>

#include "pixelia/engine/events.h"

namespace pixelia {

// --- Datum ---
// Tage seit 1970-01-01 (Ortszeit) ↔ Jahr, Monat (1–12), Tag (1–31)
int64_t days_from_civil(int y, int m, int d);
void civil_from_days(int64_t days, int* y, int* m, int* d);
inline int weekday_of(int64_t days) { return static_cast<int>(((days % 7) + 11) % 7); }  // 0 = Sonntag

// --- Jahreszeiten ---
enum class Season : uint8_t { Spring, Summer, Autumn, Winter };
const char* season_name(Season s);   // Kennung: fruehling, sommer, herbst, winter
const char* season_title(Season s);  // Frühling …
const char* season_genitive(Season s);  // des Frühlings …

// --- Zeitfenster des Tages (SETTING.md Kapitel 6) ---
// Passend zu den Schlafenszeiten: Frühaufsteher gehen gegen 22 Uhr ins Bett, die meisten gegen 23 Uhr, Nachteulen
// gegen 1 Uhr; aufgestanden wird zwischen 5 und 7½ Uhr. Nachts (1–5 Uhr) schlafen alle.
enum class DayPart : uint8_t { Night, Early, Morning, Midday, Afternoon, Evening, Late };
inline constexpr int kDayParts = 7;
struct TimeWindow {
    uint16_t from, to;  // Minuten des Tages; to < from: über Mitternacht
    bool contains(int minute) const {
        return from <= to ? minute >= from && minute < to : minute >= from || minute < to;
    }
};
TimeWindow day_part_window(DayPart p);
DayPart day_part(int minute_of_day);
const char* day_part_name(DayPart p);   // nacht, frueh, vormittag, mittag, nachmittag, abend, spaet
const char* day_part_title(DayPart p);  // Nacht, früher Morgen, …

enum class Chronotype : uint8_t { Early, Normal, Late };  // Frühaufsteher, die meisten, Nachteule
struct SleepTimes {
    uint16_t bed, wake;  // Minuten des Tages
};
SleepTimes sleep_times(Chronotype c);

// Wetterzeiten: viermal am Tag darf sich das Wetter ändern (morgens 5–11, mittags 11–17, abends 17–23, nachts 23–5)
enum class WeatherSlot : uint8_t { Morning, Midday, Evening, Night };
inline constexpr int kSlotStart[4] = {5 * 60, 11 * 60, 17 * 60, 23 * 60};
// Fortlaufende Nummer der Wetterzeit (Tag × 4 + Zeit); die Nacht ab 23 Uhr gehört zum Tag davor
int64_t weather_slot_key(int64_t day, int minute_of_day);
inline WeatherSlot slot_of_key(int64_t key) { return static_cast<WeatherSlot>(((key % 4) + 4) % 4); }
inline int64_t day_of_key(int64_t key) { return key >= 0 ? key / 4 : (key - 3) / 4; }

// --- Sonne nach der Weltjahreszeit ---
// year_phase 0 … 1 über das Weltjahr, 0 = Frühlingsbeginn (Tag- und Nachtgleiche), 0,25 = Sommerbeginn (längster
// Tag), 0,75 = Winterbeginn (Lichterabend am Ende des Herbstes = Wintersonnenwende, SETTING.md Kapitel 11).
// Tageslänge 12 h ± 4 h 18 min um 12:45 Uhr: im Sommer etwa 4:30–21:00, im Winter etwa 8:50–16:40.
struct SunTimes {
    int rise = 390, set = 1185;  // Minuten des Tages (Vorgabe: Tageslicht wie bis 0.5.6)
};
SunTimes sun_times(double year_phase);

// --- Kalender ---
// Erstlinge (einmal je Weltjahr bzw. je Jahreszeit)
enum class First : uint8_t {
    Snow,       // erster Schnee in diesem Jahr
    Storm,      // erstes Gewitter in diesem Jahr
    AutumnFog,  // erster Herbstnebel
    FullMoon,   // erster Vollmond der Jahreszeit (Bit je Jahreszeit)
};

// Zustand zum Speichern (bis 0.5.9 im NVS, danach Teil des Speicherstands). Nur feste Felder, Version vorn.
struct CalendarState {
    static constexpr uint8_t kVersion = 1;
    uint8_t version = kVersion;
    uint8_t season_len = 7;     // Tage je Jahreszeit (1 … 28)
    uint16_t firsts = 0;        // Bits der Erstlinge im Weltjahr firsts_year
    int32_t start_day = -1;     // Tag 1 (Tage seit 1970), −1 = noch nicht gestartet
    int32_t anchor_first = 0;   // erste Jahreszeit der Rechnung: ihr erster Tag …
    int32_t anchor_season = 0;  // … ihre Nummer seit dem Start (0 = erster Frühling) …
    int32_t anchor_min_last = 0;  // … und ihr letzter Tag frühestens (nach einer Längenänderung: heute)
    int32_t firsts_year = 0;
};

struct CalendarDate {
    bool started = false;
    int64_t epoch_day = 0;    // Tage seit 1970 (Ortszeit)
    int day = 1;              // Welttag ab 1
    int year = 1;             // Weltjahr ab 1
    int season_no = 0;        // Jahreszeiten seit dem Start
    Season season = Season::Spring;
    int day_in_season = 1;    // ab 1
    int season_days = 7;      // Länge der laufenden Jahreszeit
    int64_t season_first = 0, season_last = 0;  // Tage seit 1970
    bool season_evening = false;  // heute ist der letzte Tag der Jahreszeit
    int weekday = 0;          // 0 = Sonntag
    int minute = 0;           // Minute des Tages
    double year_phase = 0;    // 0 … 1, stetig (Sonne)
    SunTimes sun;
};

class WorldCalendar {
public:
    static constexpr int kMinSeasonDays = 1, kMaxSeasonDays = 28;
    static constexpr int kEveningMinute = 18 * 60;  // Hinweis auf den Jahreszeitenabend
    static constexpr int kMorningMinute = 6 * 60;   // Hinweis auf die neue Jahreszeit

    // Spielstart am Tag epoch_day (Tage seit 1970, Ortszeit)
    void start(int64_t epoch_day, int season_len = 7);
    void set_state(const CalendarState& s);
    const CalendarState& state() const { return st_; }
    bool started() const { return st_.start_day >= 0; }
    // Länge ändern: die laufende Jahreszeit bleibt, sie endet frühestens heute (bei 7 Tagen am nächsten Sonntag)
    void set_season_length(int days);
    // Zähler für Änderungen am Zustand (zum Speichern)
    uint32_t revision() const { return revision_; }

    // Tag und Minute (Ortszeit) übernehmen; billig, solange sich die Minute nicht ändert. Ereignisse entstehen nur
    // beim Überschreiten eines Zeitpunkts im laufenden Betrieb (nicht beim ersten Aufruf, nicht nach Sprüngen > 2 h).
    void update(int64_t epoch_day, int minute);
    const CalendarDate& date() const { return date_; }
    // Jahreszeit eines beliebigen Tages (Wetter-Simulation); vor dem Start: Frühling
    Season season_of(int64_t epoch_day) const;
    // Jahreszeit-Nummer, erster und letzter Tag zu einem Tag
    void season_at(int64_t epoch_day, int* no, int64_t* first, int64_t* last) const;

    // Erstling melden: true = neu (Ereignis erzeugt, Bit gesetzt)
    bool first(First f);
    bool has_first(First f) const;

    // Ereignisse (seit 0.5.8): Jahreszeitenabend, neue Jahreszeit, Erstlinge gehen auf diesen Bus (a = Jahreszeit,
    // b = Erstling, text = Kurzmeldung „Der Sommer beginnt · Tag 8“)
    void set_bus(EventBus* bus) { bus_ = bus; }

    // Testuhr und Zeitraffer: Zustand vorher merken, danach zurück (Erstlinge aus dem Test zählen nicht)
    void begin_test();
    void end_test();
    bool testing() const { return testing_; }

private:
    int64_t season_end(int64_t first, int64_t min_last) const;
    void compute(int64_t epoch_day, int minute);
    void push(EventKind kind, First f, const std::string& text);
    uint16_t first_bit(First f) const;

    CalendarState st_{};
    CalendarState saved_{};
    bool testing_ = false;
    int pending_len_ = 0;  // nach dem Test mit dem echten Datum neu verankern
    CalendarDate date_{};
    int64_t last_day_ = INT64_MIN;
    int last_minute_ = -1;
    // Zwischenspeicher der zuletzt gefundenen Jahreszeit (die Rechnung läuft sonst ab dem Anker)
    mutable int cache_no_ = -1;
    mutable int64_t cache_first_ = 0, cache_last_ = 0;
    EventBus* bus_ = nullptr;
    uint32_t revision_ = 0;
};

}  // namespace pixelia
