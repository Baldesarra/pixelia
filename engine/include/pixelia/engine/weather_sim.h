#pragma once
// Wetter-Simulation (seit 0.5.7, SETTING.md Kapitel 9): ein Wetter für die ganze Welt, passend zur Weltjahreszeit.
//
// - Viermal am Tag darf es wechseln (morgens 5 Uhr, mittags 11, abends 17, nachts 23; calendar.h WeatherSlot).
//   Die weichen Übergänge macht Weather (weather.h): erst ziehen Wolken auf, dann setzt Regen langsam ein.
// - Je Jahreszeit und Tageszeit eine Tabelle mit Gewichten; das Wetter davor bleibt mit einer gewissen
//   Wahrscheinlichkeit (Wetterlagen statt Würfeln). Nebel vor allem morgens und im Herbst, Gewitter am Nachmittag und
//   Abend im Sommer, Schnee im Winter und an den Rändern (Ende Herbst, Anfang Frühling).
// - Ein Gewitter darf auch mitten in einer Wetterzeit kommen (Sommernachmittag, 45–100 min).
// - Eine reine Rechnung aus Startwert, Kalender und Uhrzeit: Die Kette beginnt acht Wetterzeiten vorher, so ist das
//   Wetter nach einem Neustart, im Zeitraffer und mit der Testuhr dasselbe, ohne etwas zu speichern.

#include <cstdint>

#include "pixelia/engine/calendar.h"
#include "pixelia/engine/weather.h"

namespace pixelia {

struct WeatherPick {
    WeatherKind kind = WeatherKind::Clear;
    uint8_t strength = 255;
    int8_t wind_dir = 1;
    bool burst = false;  // Gewitter mitten in der Wetterzeit
};

class WeatherSim {
public:
    void init(uint64_t seed) { seed_ = seed; }
    // Wetter einer Wetterzeit (ohne Gewitter mittendrin)
    WeatherPick slot(const WorldCalendar& cal, int64_t key) const;
    // Wetter zu Tag und Minute, mit Gewitter mittendrin
    WeatherPick at(const WorldCalendar& cal, int64_t epoch_day, int minute) const;
    // Gewitter mittendrin in dieser Wetterzeit: Beginn und Ende in Minuten seit Beginn der Wetterzeit, false = keins
    bool burst(const WorldCalendar& cal, int64_t key, int* from, int* to) const;

private:
    WeatherPick roll(Season s, WeatherSlot slot, int edge, WeatherKind prev, uint64_t key) const;
    uint64_t seed_ = 0;
};

}  // namespace pixelia
