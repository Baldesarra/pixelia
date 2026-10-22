#pragma once
// Gemeinsame Schnittstellen der Firmware-Module (Hardware-Detektiv, Grafik, Leistung, Konsole).
// Netzwerk, Webserver, Updates und Einstellungen haben eigene Header (net.h, web.h, ota.h, settings.h, ui.h).

#include <cstddef>
#include <cstdint>

#include "board/display.h"
#include "board/touch_gt911.h"
#include "pixelia/engine/stage.h"

namespace app {

// Kernzuordnung (Projektplan 4.2): Kern 1 = Grafik/Welten, Kern 0 = alles andere
inline constexpr int kGfxCore = 1;
inline constexpr int kSystemCore = 0;

namespace sysinfo {
void log_all();
void log_heap();
// Freien internen RAM nach einem Startschritt ausgeben, mit Tiefstwert seit dem vorigen Schritt (Speicherspitzen finden)
void log_heap_step(const char* step);
// Resetgrund (esp_reset_reason_t) als Text
const char* reset_reason_text(int reason);
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
// Touch-Controller für die Hardware-Seite: false ohne GT911; touches = Berührungen seit dem Start
bool touch_info(board::touch::Info* info, uint32_t* touches);
// Touch-Test (Seite Experimentell): letzter Punkt in Bildausrichtung (800×480), getroffene Ecken (Bit 0–3)
struct TouchLive {
    bool ok;    // GT911 gefunden
    bool down;  // gerade berührt
    int x, y;   // letzter Punkt, -1 = noch keiner
    uint8_t corners;
    uint32_t touches;
};
TouchLive touch_live();
}  // namespace hwtest

namespace gfx {
struct EngineStats {
    uint32_t frames;          // gerenderte Bilder
    uint64_t render_us_sum;   // Summe Zeichenzeit
    uint32_t render_us_max;
    uint64_t logic_us_sum;    // Summe Logikzeit
    uint32_t logic_us_max;    // längste Logikzeit eines Bildes (seit 0.5.5; z. B. Start einer Überblendung)
    uint32_t logic_ticks;     // Logik-Schritte
    uint32_t tick_hist[3];    // Bilder mit 0, 1, 2+ Logik-Schritten (gleichmäßig = nur 1)
    int world_day;            // Weltzeit beim letzten Bild
    int world_minute;
    bool buffers_in_psram;
    bool buffers_ready;
    int sprites;
    int weather;      // Wetter (seit 0.5.3): pixelia::WeatherKind, Stärke in %
    int weather_pct;
    bool inner_light;  // Licht im Testraum (Fenster der Wiese)
    int particles;     // Partikel am sichtbaren Ort
};
void start(const board::display::Timing& timing);
void request_reconfigure(const board::display::Timing& timing);
void request_buffers(bool psram);
void set_sprites(int n);
// n weiße Quadrate oben links ins Bild zeichnen (Stufenanzeige für Tests)
void set_marks(int n);

// Orte der Bühne (seit 0.5.0, engine/stage.h). Die Liste steht nach start() fest. Wechsel, Übergang, Rundgang und
// Figurenzahl übernimmt der Grafik-Task vor dem nächsten Logik-Schritt (Seite Experimentell, Befehl „ort“).
int place_count();
const char* place_id(int i);     // Kennung (ASCII), "" = gibt es nicht
const char* place_title(int i);  // sichtbarer Name
int find_place(const char* id);  // −1 = gibt es nicht
int place();                     // sichtbarer Ort (während eines Übergangs das Ziel)
void go_place(int index);        // mit dem eingestellten Übergang
void set_transition(pixelia::Transition t);
pixelia::Transition transition();
void set_tour(int seconds);  // automatischer Rundgang: alle n Sekunden der nächste Ort, 0 = aus
int tour();
void set_figures(int n);  // Figuren der Testorte 1–64, 0 = Standard des Ortes
int figures();
// Befehl an die Bühne (Stage::command), wirkt im nächsten Logik-Schritt: Wetter („wetter regen 80“, „blitz“) und
// Ortsbefehle („ausbau“, „baustelle“, „innenlicht um“ …). Seit 0.5.3 beliebiger Text bis 47 Zeichen, das Ergebnis
// steht im Log; false = leer, zu lang oder Warteschlange (4) voll
bool place_command(const char* cmd);
void set_follow(bool on);  // Blick folgt der Begleiterin durch Türen und Bildränder (Standard an)
bool follow();
void set_inspect(bool on);  // Prüfansicht: Stellflächen, Benutzungspunkte, Ausgänge
bool inspect();
void reset_places();  // Landschaft, Standard-Übergang, kein Rundgang, Standardfiguren (Experimentell aus)
void request_coarse(bool psram);  // groben Puffer umziehen (Messung; Standard PSRAM)
// Weltzeit: folgt der echten Uhrzeit (NTP), sobald sie bekannt ist; ohne Uhrzeit startet die Welt um 12:00.
// Uhrzeit stellen oder Zeitraffer ungleich 1 schaltet das Folgen ab, set_real_time(true) wieder an.
void set_time_of_day(int hour, int minute);
void set_speed(float factor);
float speed();
void set_real_time(bool on);
bool real_time();
EngineStats engine_stats(bool reset_max);

// Live-Vorschau (seit 0.4.2): Kopie des fertigen Bildes (400×240 Paletten-Indizes) und der Palette (256 × RGB888),
// gemacht im Grafik-Task direkt vor dem Puffertausch, also nie halb gezeichnet. Nur aus einer Task gleichzeitig
// aufrufen (Webserver). Liegt die letzte Kopie weniger als min_age_ms zurück, kommt sie noch einmal (Bremse).
// false = kein Bild (Testbild aktiv, Engine nicht bereit oder Zeitüberschreitung).
struct Frame {
    const uint8_t* pixels;
    const uint8_t* palette;
    uint32_t number;   // laufende Bildnummer der Engine
    uint32_t copy_us;  // Dauer der Kopie im Grafik-Task
    bool fresh;        // neu kopiert (false = wiederholt wegen min_age_ms)
};
bool grab(Frame* out, int timeout_ms, int min_age_ms);
}  // namespace gfx

// Ablage für Grafiken (seit 0.5.5, assets.cpp): LittleFS (/data), microSD (/sd), Lade-Task für den Grafik-Speicher
// der Engine (engine/assets.h) auf Kern 0. start() vor gfx::start().
namespace assets {
struct Summary {
    uint32_t used, budget, peak;  // Grafik-Speicher in Byte
    int packs;
    uint32_t loads, evictions, failures;
    const char* source;  // eingebettet | littlefs | sd
    bool littlefs, sd;
    char last[16];       // zuletzt geladenes Paket
    uint32_t last_us;    // von der Anfrage bis geladen
    const char* last_source;
};
void start();
bool littlefs_ready();
bool sd_ready();
const char* source();
Summary summary();
// Flash-Last in LittleFS (Test: Bild bleibt ruhig?); läuft auf der Lade-Task, wartet bis zum Ende. false = ging nicht
bool flash_load(int seconds);
// Befehle „assets …“ und „sd …“ (argv[0] = Befehl)
void console(int argc, char** argv);
}  // namespace assets

namespace perf {
// Letzte Messung (alle 2 s, auch wenn das Log aus ist), z. B. für die Weboberfläche und /api/perf
struct Snapshot {
    uint32_t at_s;  // Laufzeit bei der Messung
    const char* mode;
    float lcd_hz;
    float pclk_mhz;
    float isr_pct;
    float isr_max_us;
    uint32_t desync;
    bool engine;  // Engine-Modus aktiv (sonst Testbild): dann gelten die Engine-Werte
    float fps;
    float render_ms;
    float render_max_ms;
    float logic_ms;
    float logic_max_ms;  // seit 0.5.5
    uint32_t ticks[3];  // Bilder mit 0, 1, 2+ Logik-Schritten
    int sprites;
    bool buf_psram;
    int world_day, world_minute;
    float cpu[2];
    unsigned ram_kb, ram_min_kb, ram_blk_kb, dma_kb, psram_kb, psram_min_kb, psram_blk_kb;
};
void start(int interval_s);
void set_interval(int interval_s);  // 0 = aus
int interval();
Snapshot snapshot();
}  // namespace perf

namespace console {
void start();

// Befehl über das Netz (POST /api/cmd): läuft in der Konsolen-Task wie ein serieller Befehl, die Log-Zeilen
// dieser Task während der Ausführung landen in out. Gefährliche Befehle brauchen confirmed (sonst Grund in why).
enum class Result { Done, NeedsConfirm, Busy, Timeout, Unknown };
Result run_remote(const char* line, bool confirmed, char* out, size_t out_len, char* why, size_t why_len,
                  int timeout_ms);
}  // namespace console

}  // namespace app
