#pragma once
// Gemeinsame Schnittstellen der Firmware-Module (Hardware-Detektiv, Grafik, Leistung, Konsole).
// Netzwerk, Webserver, Updates und Einstellungen haben eigene Header (net.h, web.h, ota.h, settings.h, ui.h).

#include <cstddef>
#include <cstdint>
#include <vector>

#include "board/display.h"
#include "board/touch_gt911.h"
#include "pixelia/engine/camera.h"
#include "pixelia/engine/stage.h"

namespace app {

// Kernzuordnung (Projektplan 4.2): Kern 1 = Grafik/Welten, Kern 0 = alles andere
inline constexpr int kGfxCore = 1;
inline constexpr int kSystemCore = 0;

// Nacht (seit 0.5.7): eine Definition für die gedimmte Pixel-Uhr und den Nachtmodus, Standard 22–6 Uhr, einstellbar
// (Wunsch des Users); Stunden 0–23, Beginn nach dem Ende = über Mitternacht
inline constexpr int kNightFrom = 22, kNightTo = 6;
bool is_night(int hour);

// Nachtmodus der Hintergrundbeleuchtung (night.cpp): ab 22 Uhr sanft auf die Nacht-Helligkeit, ab 6 Uhr zurück
namespace night {
void init();
void set_hours(int from, int to);  // aus den Einstellungen
int from();
int to();
void refresh();   // Einstellung geändert: sofort anwenden
void touched();   // Berühren: eine Minute hell
bool active();    // gerade gedimmt (oder aufgehellt nach Berühren)
bool awake();     // nach Berühren aufgehellt
}  // namespace night

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
    uint32_t holds;    // Schritte, die ein Abblenden auf Grafik-Pakete gewartet hat (seit 0.5.5, Summe)
    uint64_t overlay_us_sum;  // Einblendungen der Welt (Karte, Uhr, Kurzmeldung) je Bild, seit 0.5.6
    uint32_t overlay_us_max;
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

// Weltzeit (seit 0.5.7, engine/calendar.h, sky.h, weather_sim.h): Kalender, Mond und Wetter-Simulation laufen, sobald
// die Uhrzeit bekannt ist (NTP) oder die Testuhr gestellt ist. Der Kalender liegt bis zum Speicherstand (0.5.9) als
// Blob „welt“ im NVS. Stand für die Weboberfläche jede Sekunde neu.
struct WorldInfo {
    bool valid;            // Weltzeit läuft
    bool started;          // Kalender gestartet (Tag 1 = start_day)
    bool test;             // Testuhr oder Zeitraffer: Kalender wird nicht gespeichert
    int32_t start_day;     // Tage seit 1970
    int day, year, season, day_in_season, season_days, season_len, weekday;
    bool season_evening;
    int32_t season_first, season_last;  // Tage seit 1970
    int sunrise, sunset;   // Minuten des Tages
    int day_part;          // pixelia::DayPart
    int moon_illum;        // Promille
    int moon_phase;        // Promille (0 Neumond, 500 Vollmond)
    bool moon_waxing, moon_up, moon_visible;
    char moon_title[24];
    int weather, weather_pct;
    // Vorschau der Simulation (seit 0.5.7, Wunsch des Users): die nächsten drei Wetterzeiten
    int forecast_slot[3], forecast_kind[3], forecast_pct[3];
    bool weather_override; // Prüfbefehl übersteuert die Simulation bis zur nächsten Wetterzeit
    int firsts;            // Bits der Erstlinge im Weltjahr
};
WorldInfo world_info();
void set_season_length(int days);  // 1–28, die laufende Jahreszeit bleibt
void save_world_now();             // Kalender ins NVS und Speicherstand schreiben (vor einem Neustart, wartet ≤ 3 s)

// Speicherstand (seit 0.5.9, engine/save.h): Weltzustand als Ganzes in LittleFS (/data/welt-a.pxw, -b.pxw)
struct SaveInfo {
    bool loaded;            // beim Start geladen
    uint32_t generation;    // zuletzt geschrieben bzw. geladen
    uint32_t bytes;
    int64_t at_us;          // Laufzeit (esp_timer) der letzten Speicherung, 0 = seit dem Start noch keine
    int64_t next_us;        // nächste regelmäßige Speicherung
    uint32_t pack_us, write_ms;
    int figures, transit, chronicle;
    int64_t caught_up, skipped, catching_up;  // Nachholen nach dem Laden (Weltminuten)
    bool test;              // Testuhr oder Zeitraffer: es wird nicht gespeichert
    bool waiting;           // Weltzeit wartet auf die Uhrzeit: es wird noch nicht gespeichert
    char error[48];
    // Wirtschafts-Protokoll (seit 0.5.10 Teil 4, /data/wirtschaft): angehängt mit dem Speicherstand
    uint32_t econ_rows, econ_written, econ_lost, econ_files, econ_bytes, econ_flush_ms, econ_deleted;
    int32_t econ_first, econ_last;  // Welttage im Ordner
    bool econ_error;
};
SaveInfo save_info();
bool request_save();  // im nächsten Bild packen, danach schreibt die Lade-Task; false = läuft schon
void write_save();    // Lade-Task (Kern 0): gepackten Stand in den älteren Platz schreiben
// Speicherstand einer Sicherung (seit 0.5.9, Weboberfläche): neuester gültiger Stand bzw. Datei einspielen. Einspielen
// prüft die Datei, legt sie als neuesten Stand ab und sperrt das Speichern bis zum Neustart (danach startet der
// Aufrufer neu, die Welt lädt sie wie nach dem Einschalten); why = Grund in einem Satz
bool read_save(std::vector<uint8_t>* out, uint32_t* generation);
bool import_save(uint8_t* data, size_t n, const char** why);

// Figuren und Chronik für Weboberfläche und Konsole: die Grafik-Task füllt die Ansicht auf Anfrage im nächsten Bild
struct FigureRow {
    uint16_t id;
    uint8_t age;
    bool lead, moving;
    char name[16];
    char place[32];  // Ort bzw. „unterwegs nach …“
    char doing[16];  // laufende Tätigkeit (seit 0.5.9 Teil 2): steht, geht umher, ist beschäftigt, geht hinaus
    char home[16];
    uint8_t skill_n;
    struct {
        uint8_t skill, level, progress;  // progress 0–100 zur nächsten Stufe
    } skills[8];
    uint8_t relations, memories;
};
struct ChronicleRow {
    int32_t day;
    uint16_t minute;
    uint8_t kind;
    char text[60];
};
struct WorldView {
    int figures;
    FigureRow fig[48];
    int chronicle;       // neueste zuerst
    ChronicleRow ch[32];
    int visits, visits_open;
};
bool world_view(WorldView* out);  // false = Grafik-Task antwortet nicht

// Lager für die Weboberfläche (seit 0.5.10 Teil 4b, /api/world?lager=1) und die Konsole „lager“: Lagerplätze ohne
// Baustellen und Tragen, Waren im ganzen Dorf, laufende Projekte. Füllt die Grafik-Task auf Anfrage (~7 KB PSRAM).
// lv = pixelia::StockLevel (0 ohne Verbrauch, 1 Notbestand/leer, 2 knapp, 3 gut, 4 reichlich); hours −1 = kein Abgang
struct LagerStore {
    char title[20], place[24], kind[20];
    uint16_t load, cap;
    uint8_t lv;
    int16_t hours;
    char reach[28];  // nur bei einer Ware
    uint8_t goods;
    struct {
        char title[14];
        uint16_t qty, res;
    } g[6];
};
struct LagerGood {
    char title[14];
    uint16_t qty, res;  // qty im ganzen Dorf: Lagerplätze + Baustellen + getragen
    uint8_t lv;
    int16_t hours;
    char reach[28];
    uint16_t site, hand;  // davon an Baustellen bzw. gerade getragen (seit 0.5.10 Teil 4c)
    uint8_t sites;        // Zahl der Baustellen mit dieser Ware
    char site_place[24];  // Ort der ersten Baustelle
};
struct LagerProject {
    char title[24], place[24], step_title[24], wait[64], who[16];
    uint8_t step, steps, pct;  // step ab 1, pct des ganzen Projekts
    uint8_t step_pct;          // Fortschritt des laufenden Schritts
    uint8_t step_share[6];     // Anteil jedes Schritts an der ganzen Arbeit in Prozent (Segmente)
};
struct LagerView {
    int stores, goods, projects;
    LagerStore s[24];
    LagerGood g[16];
    LagerProject p[4];
};
bool lager_view(LagerView* out);  // false = Grafik-Task antwortet nicht

// Touch der Welt und Kamera (seit 0.5.6, engine/overlay.h und camera.h). Eingaben in Leinwand-Koordinaten
// (400 × 240) übernimmt der Grafik-Task vor dem nächsten Logik-Schritt: Probe = Finger aufgesetzt (Figur darunter?),
// Tippen = Karte zu Figur oder Teil, langer Druck = Folgen (ohne Figur: Info-Karte des Systems), Wischen = Ort.
struct WorldInput {
    enum class Kind : uint8_t { Probe, Tap, LongPress, Swipe };
    Kind kind;
    int16_t x, y, dx, dy;
};
void world_input(const WorldInput& in);
bool take_info_request();  // langer Druck ohne Figur: die Info-Karte des Systems zeigen
bool probe_on_figure();    // unter dem aufgesetzten Finger steht eine Figur
bool card_visible();
void set_clock(bool on, int corner, bool seconds);  // Pixel-Uhr (Einstellungen)
void set_camera(const pixelia::CameraSettings& s);  // Standard und Rückkehr (Einstellungen)
void camera_home();
void open_card(uint16_t figure);  // Karte einer Figur (Befehl „karte“): 0 = schließen, 0xFFFF = gefolgte Figur
void camera_follow(uint16_t id);
struct CameraState {
    int mode;           // pixelia::Camera::Mode: 0 Standard, 1 Frei, 2 Folgen
    uint16_t target;    // gefolgte Figur, 0 = keine
    char target_name[24];
    int place;          // sichtbarer Ort
    int return_in_s;    // Rückkehr zum Standard in s, −1 = keine
    pixelia::CameraSettings settings;
};
CameraState camera_state();
// Figuren aller Orte (Kopie, jede Sekunde neu): Kennung, Name, Ort
int figure_list(pixelia::FigureInfo* out, int* place, int max);

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
// Speicherstand schreiben (seit 0.5.9): Auftrag an die Lade-Task, ohne zu warten
bool save_world();
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
    uint32_t holds;      // seit 0.5.5: Warteschritte beim Abblenden (Summe seit dem Start)
    float overlay_ms;    // seit 0.5.6: Einblendungen der Welt (Karte, Uhr, Kurzmeldung) je Bild, im Zeichnen enthalten
    float overlay_max_ms;
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
