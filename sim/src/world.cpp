// Weltzeit im Simulator (seit 0.5.7): Weltjahr im Zeitraffer als Bericht und Selbsttests für Kalender, Mond und
// Wetter-Simulation. Läuft ohne Grafik über eine Bühne ohne Orte (nur Kalender, Himmel, Wetter).

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "pixelia/engine/calendar.h"
#include "pixelia/engine/sky.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/weather_sim.h"
#include "sim.h"

namespace sim {

namespace {

using namespace pixelia;

constexpr const char* kWeekdays[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

std::string date_text(int64_t day) {
    int y, m, d;
    civil_from_days(day, &y, &m, &d);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d", kWeekdays[weekday_of(day)], y, m, d);
    return buf;
}

std::string hm(int minute) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", minute / 60 % 24, minute % 60);
    return buf;
}

int64_t unix_of(int y, int mo, int d, int h, int mi) { return days_from_civil(y, mo, d) * 86400 + h * 3600 + mi * 60; }

// Bekannte Mondphasen (UTC, gerundet auf die Minute): Finsternisse und Meeus' Bezugs-Neumond
struct KnownPhase {
    int y, mo, d, h, mi;
    bool full;
    const char* what;
};
constexpr KnownPhase kKnown[] = {
    {2000, 1, 6, 18, 14, false, "Neumond 6.1.2000 (Meeus)"},
    {2022, 11, 8, 11, 2, true, "Vollmond 8.11.2022 (Mondfinsternis)"},
    {2024, 1, 25, 17, 54, true, "Vollmond 25.1.2024"},
    {2024, 4, 8, 18, 21, false, "Neumond 8.4.2024 (Sonnenfinsternis)"},
    {2025, 3, 14, 6, 55, true, "Vollmond 14.3.2025 (Mondfinsternis)"},
    {2026, 3, 3, 11, 38, true, "Vollmond 3.3.2026 (Mondfinsternis)"},
    {2026, 8, 12, 17, 37, false, "Neumond 12.8.2026 (Sonnenfinsternis)"},
};

// Abstand zur nächsten Phase in Stunden: Phase (0 Neu, 0,5 Voll) aus moon_at mit der mittleren Länge des Monats
double phase_error_hours(const KnownPhase& k) {
    const MoonInfo m = moon_at(unix_of(k.y, k.mo, k.d, k.h, k.mi), 0);
    double diff = m.phase - (k.full ? 0.5 : 0.0);
    if (diff > 0.5) diff -= 1.0;
    if (diff < -0.5) diff += 1.0;
    return diff * 29.530588 * 24.0;
}

struct Day {
    int64_t day = 0;
    CalendarDate date;
    WeatherKind slot[4] = {};
    int strength[4] = {};
    bool burst[4] = {};
    std::string moon;
    int illum = 0;
    std::string events;
};

}  // namespace

int world_report(const Options& opt) {
    Stage stage;  // ohne Orte: nur Weltzeit
    stage.init(opt.seed);
    Environment& env = stage.environment();
    env.calendar.start(opt.day, opt.season_len);
    WorldClock clock;
    clock.set(opt.day, 0, 0);
    // Tage: bis ein Weltjahr um ist und der nächste Frühling begonnen hat
    int days = opt.report_days;
    if (days <= 0) {
        int no;
        int64_t f, l;
        env.calendar.season_at(opt.day, &no, &f, &l);
        int64_t end = l;
        for (int s = 1; s < 4; ++s) env.calendar.season_at(end + 1, &no, &f, &end);
        days = static_cast<int>(end - opt.day + 2);
    }
    std::vector<Day> log(static_cast<size_t>(days));
    int counts[4][kWeatherKinds] = {};
    int slots[4] = {};
    int bursts[4] = {};
    std::vector<std::string> firsts;
    constexpr int kStepsPerMinute = 4;  // Wetter-Übergänge laufen je Schritt (Erstlinge brauchen sichtbares Wetter)
    for (int64_t m = 0; m < static_cast<int64_t>(days) * 1440; ++m) {
        const int64_t day = opt.day + m / 1440;
        const int minute = static_cast<int>(m % 1440);
        for (int k = 0; k < kStepsPerMinute; ++k) {
            clock.set_world_us(day * WorldClock::kUsPerDay + minute * WorldClock::kUsPerMinute +
                               k * (WorldClock::kUsPerMinute / kStepsPerMinute));
            stage.tick(clock);
        }
        Day& d = log[static_cast<size_t>(m / 1440)];
        d.day = day;
        const CalendarDate& cd = env.calendar.date();
        const int season = static_cast<int>(cd.season);
        for (int s = 0; s < 4; ++s) {
            if (minute != kSlotStart[s]) continue;
            d.slot[s] = env.weather.kind();
            d.strength[s] = env.weather.strength() * 100 / 255;
            ++counts[season][static_cast<int>(d.slot[s])];
            ++slots[season];
        }
        // Gewitter mittendrin: Ziel wird Gewitter, obwohl die Wetterzeit anders begann
        const int64_t key = weather_slot_key(day, minute);
        const int sl = static_cast<int>(slot_of_key(key));
        Day& owner = log[static_cast<size_t>(std::max<int64_t>(0, day_of_key(key) - opt.day))];
        if (env.weather.kind() == WeatherKind::Storm && owner.slot[sl] != WeatherKind::Storm && !owner.burst[sl]) {
            owner.burst[sl] = true;
            ++bursts[season];
        }
        if (minute == 0) {
            d.date = cd;
            const MoonInfo mi = moon_at(clock.unix_s() + 22 * 3600, 22 * 60);  // Mond um 22 Uhr
            d.moon = moon_phase_title(mi);
            d.illum = mi.illum;
        }
        WorldEvent e;
        while (stage.poll_event(&e)) {
            d.events += (d.events.empty() ? "" : "; ") + hm(minute) + " " + e.text;
            if (e.kind == WorldEvent::Kind::First) firsts.push_back(date_text(day) + " " + hm(minute) + " " + e.text);
        }
    }

    std::string out;
    char line[512];
    std::snprintf(line, sizeof(line),
                  "# Weltjahr im Zeitraffer\n\nStart %s, Startwert %llu, Jahreszeit %d Tage, %d Tage gerechnet.\n\n",
                  date_text(opt.day).c_str(), static_cast<unsigned long long>(opt.seed), opt.season_len, days);
    out += line;
    out += "| Datum | Welttag | Jahreszeit | Sonne | Mond 22 Uhr | morgens | mittags | abends | nachts | Ereignisse |\n";
    out += "|---|---|---|---|---|---|---|---|---|---|\n";
    for (const Day& d : log) {
        std::string w[4];
        for (int s = 0; s < 4; ++s) {
            w[s] = weather_title(d.slot[s]);
            if (d.slot[s] != WeatherKind::Clear) w[s] += " " + std::to_string(d.strength[s]) + "%";
            if (d.burst[s]) w[s] += " + Gewitter";
        }
        std::snprintf(line, sizeof(line), "| %s | %d | %s %d/%d%s | %s–%s | %s %d%% | %s | %s | %s | %s | %s |\n",
                      date_text(d.day).c_str(), d.date.day, season_title(d.date.season), d.date.day_in_season,
                      d.date.season_days, d.date.season_evening ? " (Abend)" : "", hm(d.date.sun.rise).c_str(),
                      hm(d.date.sun.set).c_str(), d.moon.c_str(), (d.illum + 5) / 10, w[0].c_str(), w[1].c_str(),
                      w[2].c_str(), w[3].c_str(), d.events.c_str());
        out += line;
    }
    out += "\n## Wetter je Jahreszeit (Anteil der Wetterzeiten)\n\n| Jahreszeit | Wetterzeiten |";
    for (int k = 0; k < kWeatherKinds; ++k) out += std::string(" ") + weather_title(static_cast<WeatherKind>(k)) + " |";
    out += " Gewitter mittendrin |\n|---|---|---|---|---|---|---|---|---|---|\n";
    for (int s = 0; s < 4; ++s) {
        std::snprintf(line, sizeof(line), "| %s | %d |", season_title(static_cast<Season>(s)), slots[s]);
        out += line;
        for (int k = 0; k < kWeatherKinds; ++k) {
            std::snprintf(line, sizeof(line), " %d %% |", slots[s] ? (counts[s][k] * 100 + slots[s] / 2) / slots[s] : 0);
            out += line;
        }
        std::snprintf(line, sizeof(line), " %d |\n", bursts[s]);
        out += line;
    }
    out += "\n## Erstlinge\n\n";
    for (const std::string& f : firsts) out += "- " + f + "\n";
    out += "\n## Mondphasen gegen bekannte Daten (UTC)\n\n| Termin | Abweichung |\n|---|---|\n";
    for (const KnownPhase& k : kKnown) {
        std::snprintf(line, sizeof(line), "| %s | %+.1f h |\n", k.what, phase_error_hours(k));
        out += line;
    }
    // Prüfungen: Wetter passt zur Jahreszeit
    const int snow_summer = counts[1][static_cast<int>(WeatherKind::Snow)];
    const int storm_winter = counts[3][static_cast<int>(WeatherKind::Storm)] + bursts[3];
    const int snow_winter = counts[3][static_cast<int>(WeatherKind::Snow)];
    const int storm_summer = counts[1][static_cast<int>(WeatherKind::Storm)] + bursts[1];
    std::snprintf(line, sizeof(line),
                  "\n## Prüfung\n\n- %s kein Schnee im Sommer (%d)\n- %s kein Gewitter im Winter (%d)\n"
                  "- %s Schnee im Winter (%d Wetterzeiten)\n- %s Gewitter im Sommer (%d)\n",
                  snow_summer == 0 ? "✓" : "✗", snow_summer, storm_winter == 0 ? "✓" : "✗", storm_winter,
                  snow_winter > 0 ? "✓" : "✗", snow_winter, storm_summer > 0 ? "✓" : "✗", storm_summer);
    out += line;
    const bool ok = snow_summer == 0 && storm_winter == 0 && snow_winter > 0 && storm_summer > 0;
    FILE* f = std::fopen(opt.world_report.c_str(), "wb");
    if (!f) {
        std::printf("FEHLER: %s nicht schreibbar\n", opt.world_report.c_str());
        return 1;
    }
    std::fwrite(out.data(), 1, out.size(), f);
    std::fclose(f);
    std::printf("sim: Weltjahr %s (%d Tage, %zu Erstlinge, Prüfung %s)\n", opt.world_report.c_str(), days,
                firsts.size(), ok ? "bestanden" : "NICHT bestanden");
    return ok ? 0 : 1;
}

// --- Selbsttests ---
int run_world_tests() {
    struct Case {
        const char* name;
        std::function<std::string()> run;
    };
    const std::vector<Case> cases = {
        {"Datum: Tage seit 1970 hin und zurück, Wochentag",
         [] {
             int y, m, d;
             for (int64_t z : {-1000LL, 0LL, 10957LL, 20387LL, 30000LL}) {
                 civil_from_days(z, &y, &m, &d);
                 if (days_from_civil(y, m, d) != z) return std::string("Rundweg falsch");
             }
             if (days_from_civil(2026, 10, 26) != 20752) return std::string("26.10.2026 falsch");
             if (weekday_of(days_from_civil(2026, 10, 26)) != 1) return std::string("26.10.2026 ist ein Montag");
             if (weekday_of(0) != 4) return std::string("1.1.1970 ist ein Donnerstag");
             return std::string();
         }},
        {"Erster Frühling 7–13 Tage bis Sonntag, danach Montag bis Sonntag",
         [] {
             const int64_t monday = days_from_civil(2026, 10, 26);
             for (int k = 0; k < 7; ++k) {
                 WorldCalendar c;
                 c.start(monday + k);
                 c.update(monday + k, 600);
                 const CalendarDate& d = c.date();
                 if (d.season_days < 7 || d.season_days > 13) return "Länge " + std::to_string(d.season_days);
                 if (weekday_of(d.season_last) != 0) return std::string("Ende nicht am Sonntag");
                 int no;
                 int64_t f, l;
                 c.season_at(d.season_last + 1, &no, &f, &l);
                 if (no != 1 || weekday_of(f) != 1 || l - f != 6) return std::string("Sommer nicht Mo–So");
             }
             return std::string();
         }},
        {"Jahreszeitenabend und Wechsel als Ereignis, Weltjahr 2",
         [] {
             const int64_t start = days_from_civil(2026, 10, 26);  // Montag: Frühling 7 Tage
             WorldCalendar c;
             c.start(start);
             std::vector<std::string> texts;
             for (int64_t m = 0; m < 30LL * 1440; m += 10) {
                 c.update(start + m / 1440, static_cast<int>(m % 1440));
                 WorldEvent e;
                 while (c.poll(&e)) texts.push_back(e.text);
             }
             const char* want[] = {"Letzter Abend des Frühlings", "Der Sommer beginnt · Tag 8",
                                   "Letzter Abend des Sommers", "Der Herbst beginnt · Tag 15",
                                   "Letzter Abend des Herbstes", "Der Winter beginnt · Tag 22",
                                   "Letzter Abend des Winters", "Der Frühling beginnt · Weltjahr 2"};
             if (texts.size() != 8) return "Ereignisse: " + std::to_string(texts.size());
             for (int i = 0; i < 8; ++i)
                 if (texts[static_cast<size_t>(i)] != want[i]) return "Ereignis " + texts[static_cast<size_t>(i)];
             if (c.date().year != 2 || c.date().season != Season::Spring) return std::string("Jahr 2 fehlt");
             return std::string();
         }},
        {"Andere Länge: Abend am letzten Tag, Länge ändern hält die Jahreszeit",
         [] {
             const int64_t start = days_from_civil(2026, 10, 28);
             WorldCalendar c;
             c.start(start, 3);
             c.update(start + 2, 600);
             if (!c.date().season_evening || c.date().season != Season::Spring) return std::string("Abend Tag 3");
             c.update(start + 3, 600);
             if (c.date().season != Season::Summer) return std::string("Sommer an Tag 4");
             c.update(start + 4, 600);
             c.set_season_length(7);  // mitten im Sommer: endet am nächsten Sonntag
             if (c.date().season != Season::Summer || weekday_of(c.date().season_last) != 0)
                 return std::string("Sommer nach Längenänderung");
             if (c.date().season_last - c.date().season_first + 1 < 7) return std::string("Sommer zu kurz");
             return std::string();
         }},
        {"Testuhr: Erstlinge aus dem Test zählen nicht, Rücksprung schadet nicht",
         [] {
             const int64_t start = days_from_civil(2026, 10, 26);
             WorldCalendar c;
             c.start(start);
             c.update(start, 600);
             c.begin_test();
             c.update(start + 25, 600);  // Winter
             c.first(First::Snow);
             c.end_test();
             c.update(start, 610);
             if (c.has_first(First::Snow) || c.date().season != Season::Spring) return std::string("Test blieb hängen");
             return std::string();
         }},
        {"Sonne: Sommer lang, Winter kurz, Tag- und Nachtgleiche 12 h",
         [] {
             const SunTimes sp = sun_times(0), su = sun_times(0.25), wi = sun_times(0.75);
             if (std::abs(sp.set - sp.rise - 720) > 1) return std::string("Frühlingsbeginn nicht 12 h");
             if (su.rise > 5 * 60 || su.set < 20 * 60 + 30) return "Sommer " + hm(su.rise) + "–" + hm(su.set);
             if (wi.rise < 8 * 60 + 30 || wi.set > 17 * 60) return "Winter " + hm(wi.rise) + "–" + hm(wi.set);
             // Tageslicht mit Sonnenzeiten: mittags hell, nach Untergang + 3 h Nacht
             const Daylight dl = daylight_preset(0);
             const Ambient noon = dl.at(13LL * 3600 * 1000000, &wi), late = dl.at((wi.set + 180) * 60LL * 1000000, &wi);
             if (noon.lamp != 0 || late.lamp < 200) return std::string("Dämmerung im Winter falsch");
             const Ambient summer_eve = dl.at(21LL * 3600 * 1000000, &su);
             if (summer_eve.lamp > 180) return std::string("Sommerabend 21 Uhr schon Nacht");
             return std::string();
         }},
        {"Mondphasen: bekannte Voll- und Neumonde auf ±8 h",
         [] {
             for (const KnownPhase& k : kKnown) {
                 const double e = phase_error_hours(k);
                 if (std::abs(e) > 8.0) return std::string(k.what) + ": " + std::to_string(e) + " h";
                 const MoonInfo m = moon_at(unix_of(k.y, k.mo, k.d, k.h, k.mi), 0);
                 if (k.full ? m.illum < 990 : m.illum > 10) return std::string(k.what) + ": Anteil";
             }
             // Vollmond steht um Mitternacht hoch, Neumond mittags
             const MoonInfo full = moon_at(unix_of(2026, 3, 3, 11, 38), 45);
             const MoonInfo fnoon = moon_at(unix_of(2026, 3, 3, 11, 38), 12 * 60 + 45);
             if (full.up < 200 || fnoon.up != 0) return std::string("Vollmond nicht nachts oben");
             return std::string();
         }},
        {"Wetter-Simulation: deterministisch, Wetterlage hält, Jahreszeit passt",
         [] {
             const int64_t start = days_from_civil(2026, 10, 26);
             WorldCalendar c;
             c.start(start);
             WeatherSim a, b;
             a.init(7);
             b.init(7);
             int counts[4][kWeatherKinds] = {};
             int same = 0, total = 0;
             WeatherKind prev = WeatherKind::Clear;
             for (int64_t key = start * 4; key < (start + 28) * 4; ++key) {
                 const WeatherPick p = a.slot(c, key), q = b.slot(c, key);
                 if (p.kind != q.kind || p.strength != q.strength) return std::string("nicht deterministisch");
                 ++counts[static_cast<int>(c.season_of(day_of_key(key)))][static_cast<int>(p.kind)];
                 same += p.kind == prev;
                 prev = p.kind;
                 ++total;
             }
             if (counts[1][static_cast<int>(WeatherKind::Snow)]) return std::string("Schnee im Sommer");
             if (counts[3][static_cast<int>(WeatherKind::Storm)]) return std::string("Gewitter im Winter");
             if (!counts[3][static_cast<int>(WeatherKind::Snow)]) return std::string("kein Schnee im Winter");
             if (same * 100 / total < 25) return "Wetterlage hält kaum: " + std::to_string(same * 100 / total) + " %";
             return std::string();
         }},
        {"Bühne: Simulation speist das Wetter, Prüfbefehl übersteuert bis zur nächsten Wetterzeit",
         [] {
             Stage stage;
             stage.init(3);
             WorldClock clock;
             const int64_t day = days_from_civil(2026, 10, 26);
             clock.set(static_cast<int>(day), 12, 0);
             stage.tick(clock);
             const WeatherKind sim = stage.weather().kind();
             const WeatherKind other = sim == WeatherKind::Snow ? WeatherKind::Fog : WeatherKind::Snow;
             const std::string cmd = std::string("wetter ") + weather_name(other);
             stage.command(cmd.c_str());
             clock.set(static_cast<int>(day), 13, 0);
             stage.tick(clock);
             if (stage.weather().kind() != other || !stage.weather_overridden()) return std::string("Übersteuerung weg");
             clock.set(static_cast<int>(day), 17, 1);  // neue Wetterzeit
             stage.tick(clock);
             if (stage.weather_overridden()) return std::string("Übersteuerung bleibt");
             stage.command("wetter sonne");
             stage.command("wetter auto");
             clock.set(static_cast<int>(day), 17, 2);
             stage.tick(clock);
             if (stage.weather_overridden()) return std::string("wetter auto wirkt nicht");
             return std::string();
         }},
    };
    int failed = 0;
    for (const Case& c : cases) {
        const std::string err = c.run();
        if (err.empty()) {
            std::printf("  ok    %s\n", c.name);
        } else {
            ++failed;
            std::printf("  FEHLER %s: %s\n", c.name, err.c_str());
        }
    }
    std::printf("sim: Weltzeit %d/%d bestanden\n", static_cast<int>(cases.size()) - failed, static_cast<int>(cases.size()));
    return failed;
}

}  // namespace sim
