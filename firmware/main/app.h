#pragma once
// Gemeinsame Schnittstellen der Firmware-Module (Hardware-Detektiv, Grafik, Leistung, Konsole).
// Netzwerk, Webserver, Updates und Einstellungen haben eigene Header (net.h, web.h, ota.h, settings.h, ui.h).

#include <cstdint>

#include "board/display.h"

namespace app {

// Kernzuordnung (Projektplan 4.2): Kern 1 = Grafik/Welten, Kern 0 = alles andere
inline constexpr int kGfxCore = 1;
inline constexpr int kSystemCore = 0;

namespace sysinfo {
void log_all();
void log_heap();
// Freien internen RAM nach einem Startschritt ausgeben, mit Tiefstwert seit dem vorigen Schritt (Speicherspitzen finden)
void log_heap_step(const char* step);
}  // namespace sysinfo

namespace hwtest {
// I²C-Bus anlegen, Bus scannen, GT911 suchen
void init_i2c_and_scan();
void scan();
void touch_reset_test();
void start_touch_task();
void reset_corner_hits();
void log_touch_status();
// Geführter Test; ohne Ecken startet er erst nach einem Tippen aufs Display
void start_tour(bool with_corners);
// Zweiter Test: PWM-Frequenzen der Hintergrundbeleuchtung, dann A/B normal gegen Flash-Schreiben
void start_tour2();
// Flash-Sektoren in der Partition „storage“ löschen/beschreiben (Test: Bild bleibt ruhig?)
void flash_stress(int seconds);
void backlight_cycle(bool on);
}  // namespace hwtest

namespace gfx {
struct EngineStats {
    uint32_t frames;          // gerenderte Bilder
    uint64_t render_us_sum;   // Summe Zeichenzeit
    uint32_t render_us_max;
    uint64_t logic_us_sum;    // Summe Logikzeit
    uint32_t logic_ticks;     // Logik-Schritte
    uint32_t tick_hist[3];    // Bilder mit 0, 1, 2+ Logik-Schritten (gleichmäßig = nur 1)
    int world_day;            // Weltzeit beim letzten Bild
    int world_minute;
    bool buffers_in_psram;
    bool buffers_ready;
    int sprites;
};
void start(const board::display::Timing& timing);
void request_reconfigure(const board::display::Timing& timing);
void request_buffers(bool psram);
void set_sprites(int n);
// n weiße Quadrate oben links ins Bild zeichnen (Stufenanzeige für Tests)
void set_marks(int n);
// Weltzeit: folgt der echten Uhrzeit (NTP), sobald sie bekannt ist; ohne Uhrzeit startet die Welt um 12:00.
// Uhrzeit stellen oder Zeitraffer ungleich 1 schaltet das Folgen ab, set_real_time(true) wieder an.
void set_time_of_day(int hour, int minute);
void set_speed(float factor);
float speed();
void set_real_time(bool on);
bool real_time();
EngineStats engine_stats(bool reset_max);
}  // namespace gfx

namespace perf {
// Letzte Messung (alle 2 s, auch wenn das Log aus ist), z. B. für die Weboberfläche
struct Snapshot {
    float lcd_hz;
    float fps;
    float render_ms;
    float isr_pct;
    float cpu[2];
    uint32_t desync;
    unsigned ram_kb, ram_min_kb, ram_blk_kb, psram_kb, psram_min_kb;
};
void start(int interval_s);
void set_interval(int interval_s);  // 0 = aus
int interval();
Snapshot snapshot();
}  // namespace perf

namespace console {
void start();
}  // namespace console

}  // namespace app
