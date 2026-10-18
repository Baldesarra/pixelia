#pragma once
// RGB-Panel ohne Bildspeicher (no_fb): Ein Callback füllt die Bounce-Buffer im internen RAM.
// - Testbilder werden direkt in 800×480 erzeugt (Hardware-Detektiv).
// - Engine-Modus: 400×240 mit 8-Bit-Palette, Palette + 2× Hochskalieren im Callback,
//   Doppelpuffer, Tausch am Bildende.
// init()/reconfigure() auf dem Grafik-Kern aufrufen: der Interrupt läuft auf dem Kern, der das Panel anlegt.

#include <cstdint>

#include "esp_err.h"

namespace board::display {

enum class Mode : uint8_t { Black, Bars, Frame, Motion, Touch, Engine };

// Geprüfte Werte siehe docs/HARDWARE.md: 24,6 MHz ergeben mit 820×500 Takten genau 60 Hz
struct Timing {
    uint32_t pclk_hz = 24600000;
    uint16_t hsync_pulse = 4, hsync_back = 8, hsync_front = 8;
    uint16_t vsync_pulse = 4, vsync_back = 8, vsync_front = 8;
    bool hsync_idle_low = true;
    bool vsync_idle_low = true;
    bool pclk_active_neg = true;
    bool pll240 = false;     // Taktquelle PLL 240 MHz statt 160 MHz
    int bounce_lines = 8;    // Zeilen pro Bounce-Buffer: gerade und Teiler von 480
};

struct Stats {
    uint32_t vsyncs;           // VSync-Interrupts seit Start
    uint32_t frames;           // vollständig befüllte Bilder
    uint32_t swaps;            // getauschte Engine-Puffer
    uint32_t fills;            // Aufrufe des Bounce-Callbacks
    uint32_t fill_cycles;      // CPU-Takte im Callback (läuft über, nur Differenzen nutzen)
    uint32_t fill_max_cycles;  // längster Callback seit dem letzten Abruf mit reset_max
    uint32_t desyncs;          // VSync-Perioden mit falscher Zahl an Füllungen (Treiber kam nicht hinterher)
};

esp_err_t init(const Timing& timing);
esp_err_t reconfigure(const Timing& timing);
const Timing& timing();
esp_err_t set_pclk(uint32_t hz);
// Bilddauer laut Timing in µs (tatsächlicher Takt kann durch Teiler leicht abweichen)
uint32_t frame_period_us();
int fills_per_frame();

void set_mode(Mode mode);
Mode mode();
const char* mode_name(Mode mode);
bool mode_from_name(const char* name, Mode* out);

Stats stats(bool reset_max);

// Bild um 180° drehen (Gerät steht mit dem USB-Anschluss auf der anderen Seite). Kostet nichts extra.
void set_rotation180(bool on);
bool rotation180();

// Touch-Testbild (Koordinaten in Panel-Ausrichtung, also ungedreht)
void set_touch_marker(int x, int y);  // x < 0: kein Marker
void set_corner_hits(uint8_t mask);   // Bit 0..3: oben links, oben rechts, unten links, unten rechts

// Engine: zwei Puffer mit 400×240 Byte, 4-Byte-ausgerichtet
void attach_buffers(uint8_t* a, uint8_t* b);
void detach_buffers();
// Wartet, bis der Hinterpuffer frei ist (vorheriges Bild wurde getauscht). nullptr bei Zeitüberschreitung.
uint8_t* acquire_back_buffer(uint32_t timeout_ms);
// Hinterpuffer mit Palette (256 × RGB565) zum Tausch am nächsten passenden Bildende freigeben
void present(const uint16_t* palette565);
void set_swap_interval(int frames);
int swap_interval();

}  // namespace board::display
