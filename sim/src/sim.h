#pragma once
// Gemeinsame Einstellungen und Hilfen des Simulators

#include <cstdint>
#include <string>

#include "pixelia/engine/stage.h"
#include "pixelia/engine/system_ui.h"

namespace sim {

struct Options {
    bool headless = false;
    std::string scene = "scrolltest";  // Ort, mit dem die Bühne startet
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
    std::string screen;           // Systemanzeige statt bzw. über der Szene: setup, update, info, toast …
    std::string go;               // Ortswechsel nach dem Vorspulen: ORT[:ÜBERGANG]
    std::string cmd;              // Befehle an den Startort, durch Komma getrennt (z. B. ausbau,baustelle)
    bool inspect = false;         // Prüfansicht (Stellflächen, Benutzungspunkte, Ausgänge)
    bool follow = true;           // Blick folgt der Begleiterin durch die Ausgänge
    std::string grid;             // Vergleichsbild (seit 0.5.2)
    std::string cells;            // Zellen „Beschriftung|Ort|HH:MM|Befehl+Befehl“, durch ; getrennt
    int cols = 3;
    long after = 0;               // Logik-Schritte nach dem Wechsel (vor Screenshot)
    std::string sequence;         // PNG-Datei mit einer Bildfolge (kopflos)
    int every = 3;                // Abstand der Bilder in Logik-Schritten
    int count = 8;                // Zahl der Bilder
    std::string world_report;     // Weltjahr im Zeitraffer als Bericht (seit 0.5.7)
    int report_days = 0;          // Tage im Bericht (0 = ein Weltjahr und der nächste Frühling)
    int season_len = 7;           // Tage je Jahreszeit
    std::string econ_dir;         // Wirtschafts-Protokoll in diesen Ordner (seit 0.5.10 Teil 4, wie am Gerät)
};

// Bühne mit den Orten des Geräts, Startort opt.scene; false = Ort unbekannt
bool setup_stage(const Options& opt, pixelia::Stage& stage);
// ORT[:ÜBERGANG] → Index und Übergang (Standard Abblenden); false = unbekannt
bool parse_go(const std::string& spec, const pixelia::Stage& stage, int* place, pixelia::Transition* t);
// Palette der Bühne mit den Farben der System-Oberfläche (240–255)
pixelia::Palette display_palette(const pixelia::Stage& stage);

int run_headless(const Options& opt);
int run_tests(const std::string& which);  // 0 = alle bestanden
int world_report(const Options& opt);     // Weltjahr im Zeitraffer (world.cpp)
int run_world_tests();                    // Zahl der Fehler
int run_ledger_tests();                   // Warenbuch (seit 0.5.10, ledger_tests.cpp), Zahl der Fehler
int run_window(const Options& opt);

// Beispieldaten der Info-Karte: info (im WLAN), info-setup, info-offline
pixelia::ui::InfoCardData sample_info(const std::string& variant);

}  // namespace sim
