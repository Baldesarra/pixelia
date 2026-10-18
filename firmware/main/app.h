#pragma once
// Gemeinsame Schnittstellen der Firmware-Module (Phase 0.1: Hardware-Detektiv + Engine-Prototyp)

#include <cstdint>

#include "board/display.h"

namespace app {

// Kernzuordnung (Projektplan 4.2): Kern 1 = Grafik/Welten, Kern 0 = alles andere
inline constexpr int kGfxCore = 1;
inline constexpr int kSystemCore = 0;

namespace sysinfo {
void log_all();
void log_heap();
}  // namespace sysinfo

namespace hwtest {
// I²C-Bus anlegen, Bus scannen, GT911 suchen
void init_i2c_and_scan();
void scan();
void touch_reset_test();
void start_touch_task();
void reset_corner_hits();
void backlight_cycle(bool on);
}  // namespace hwtest

namespace gfx {
struct EngineStats {
    uint32_t frames;          // gerenderte Bilder
    uint64_t render_us_sum;   // Summe Zeichenzeit
    uint32_t render_us_max;
    uint64_t logic_us_sum;    // Summe Logikzeit
    uint32_t logic_ticks;     // Logik-Schritte
    bool buffers_in_psram;
    bool buffers_ready;
    int sprites;
};
void start(const board::display::Timing& timing);
void request_reconfigure(const board::display::Timing& timing);
void request_buffers(bool psram);
void set_sprites(int n);
EngineStats engine_stats(bool reset_max);
}  // namespace gfx

namespace perf {
void start(int interval_s);
void set_interval(int interval_s);  // 0 = aus
int interval();
}  // namespace perf

namespace console {
void start();
}  // namespace console

}  // namespace app
