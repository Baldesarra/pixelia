#pragma once
// Seite Experimentell (seit 0.4.2): Schalter, Panel-Timing und Stromsparmodus im Probelauf, Displaytests, Touch-Test.
//
// - Schalter „Experimentell“: Standard aus, dauerhaft im NVS (pixelia/exp). Aus = alles gesperrt, das Panel läuft mit
//   den geprüften Standardwerten (docs/HARDWARE.md), eigene Werte werden gelöscht.
// - Panel-Timing und Stromsparmodus können das Bild schwarz machen. Jede Änderung gilt nur probeweise (Probe): ohne
//   Bestätigung aus dem Browser fällt sie nach kTrialSeconds auf die vorigen Werte zurück, wie beim Ändern der
//   Bildschirmauflösung am PC. Gespeichert wird erst nach der Bestätigung; ein Neustart während der Probe startet
//   daher immer mit den zuletzt bestätigten Werten.
// - Fluchtweg ohne Browser: Zwei Starts mit eigenen Werten hintereinander, die keine kStableSeconds überlebt haben
//   (Stecker gezogen, weil das Bild schwarz ist, oder Absturz), → der nächste Start nimmt die Standardwerte.
// - Stromsparmodus: halber Pixeltakt (Panel 30 statt 60 Hz), jedes Engine-Bild nur einmal gezeigt. Die Welt bleibt
//   bei 30 fps, der Hochskalier-Interrupt und der PSRAM-Verkehr halbieren sich.
// - Displaytests: Testbilder des Hardware-Detektivs (Farbbalken, Rahmen, Bewegung, Touch); sie kehren nach
//   kTestSeconds von selbst zur Welt zurück.
// - Eigenes Testbild (seit 0.5.0): POST /api/picture (web_exp.cpp) → ui::show_picture; nur mit Schalter an.

#include <cstddef>
#include <string>

#include "board/display.h"

namespace app::exper {

inline constexpr int kTrialSeconds = 15;
inline constexpr int kTestSeconds = 60;
inline constexpr int kStableSeconds = 60;

struct Panel {
    board::display::Timing timing;  // Pixeltakt ohne Stromsparmodus
    bool eco = false;               // Stromsparmodus: Pixeltakt halbiert
};

void init();  // nach settings::init, vor gfx::start
bool enabled();
void set_enabled(bool on);

// Startwerte für das Panel (bestätigt gespeichert, außer der Fluchtweg greift) und Engine-Bilder pro Panel-Bild
board::display::Timing boot_timing();
int boot_swap();

bool try_panel(const Panel& p, char* error, size_t error_len);  // Probe starten
bool confirm(char* error, size_t error_len);                     // Probe übernehmen und speichern
void revert(const char* why);                                    // Probe sofort beenden
void reset_panel();                                              // Standardwerte (sicher), gespeichert
// Testbild zeigen; Mode::Engine beendet den Test
bool start_test(board::display::Mode mode, char* error, size_t error_len);

std::string state_json();

}  // namespace app::exper
