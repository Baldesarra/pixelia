#pragma once
// Gemeinsame Einstellungen und Hilfen des Simulators

#include <cstdint>
#include <string>

namespace sim {

struct Options {
    bool headless = false;
    std::string scene = "scrolltest";
    uint64_t seed = 1;
    int day = 0;
    int hour = 12;
    int minute = 0;
    double speed = 1.0;           // Zeitraffer (1 = Echtzeit)
    long frames = 0;              // Logik-Schritte vorspulen
    std::string screenshot;       // PNG-Datei (kopflos)
    int scale = 2;                // Screenshot-Vergrößerung, 2 = wie auf dem Gerät
    std::string contact_sheet;    // PNG-Datei mit zufälligen Momenten
    int moments = 24;
    double span_days = 1.0;       // Zeitraum, aus dem die Momente stammen
    int load = -1;                // Lastregler der Szene (z. B. Sprites), -1 = Standard
    bool device_colors = true;    // Farben wie auf dem Display (über RGB565)
    double run_seconds = 0;       // Fenster nach X Sekunden schließen (0 = offen lassen), für Tests
};

int run_headless(const Options& opt);
int run_window(const Options& opt);

}  // namespace sim
