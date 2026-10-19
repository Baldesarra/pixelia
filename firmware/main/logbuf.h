#pragma once
// Log-Ringpuffer (0.2.2 „Fernwartung“): Jede Log-Zeile (esp_log) geht weiter unverändert auf die serielle Konsole
// und zusätzlich in einen Ringpuffer, der sich per WLAN abrufen lässt (/api/log, WebSocket /ws/log, Diagnosepaket).
//
// - Hauptpuffer: 256 KB im PSRAM, No-Init-Bereich (EXT_RAM_NOINIT_ATTR, vom Speichertest beim Start ausgespart).
//   Übersteht Software-Neustarts, Abstürze und Watchdog-Resets, solange die Firmware den Bereich an derselben
//   Adresse hat (die Lage verschiebt sich in 64-KB-Schritten mit der Größe von Code und Konstanten).
// - Sicherung: die letzten 6 KB im RTC-Speicher (feste Adresse 0x50000000, unabhängig von der Version), für den
//   Fall, dass der nächste Start eine andere Version ist: im Probelauf nach einem Update zeilenweise (Absturz →
//   Rückfall auf die alte Version), sonst einmal beim Software-Neustart (Shutdown-Handler, z. B. nach einem Update).
// - Jede Zeile trägt eine fortlaufende Nummer, Laufzeit (ms), Uhrzeit (Unix-Sekunden, 0 = unbekannt) und den
//   Startzähler (NVS). Erst ein Stromausfall löscht beide Puffer.
// - Der Hook wartet nie auf eine Sperre einer anderen Task: kurzer Spinlock (wenige µs), kein malloc, kein Mutex.
//   Jeder Datensatz wird sofort aus dem Write-Back-Cache in den PSRAM geschrieben, sonst wären die letzten Zeilen
//   vor einem Neustart verloren.

#include <cstddef>
#include <cstdint>

namespace app::log {

inline constexpr size_t kRingBytes = 256 * 1024;
inline constexpr size_t kRtcBytes = 6 * 1024;
inline constexpr size_t kMaxText = 320;     // längere Zeilen werden im Puffer gekürzt (seriell vollständig)
inline constexpr uint16_t kBootUnknown = 0xFFFF;

struct Line {
    uint32_t seq;    // fortlaufende Nummer
    uint32_t up_ms;  // Laufzeit seit dem Start
    uint32_t epoch;  // Uhrzeit in Unix-Sekunden, 0 = noch nicht gestellt
    uint16_t boot;   // Startzähler
    char level;      // E, W, I, D, V oder '-' (Zeile ohne ESP-Log-Kopf)
    bool truncated;
    uint16_t len;
    char text[kMaxText + 1];
};

struct Stats {
    uint32_t calls;      // Hook-Aufrufe seit dem Start
    uint32_t stored;     // davon im Puffer gelandet
    uint32_t serial_only;  // bewusst nur seriell (Leistungsmonitor zwischen den Minutenwerten)
    uint32_t truncated;
    uint32_t errors, warnings;  // Zeilen der Stufe E/W in diesem Start
    // Kosten in CPU-Takten (240 MHz): Formatieren, Puffer (Spinlock + Kopieren + Cache zurückschreiben), seriell
    uint64_t fmt_cycles, store_cycles, uart_cycles;
    uint64_t main_cycles, rtc_cycles;  // Anteil PSRAM-Puffer (mit Cache zurückschreiben) und RTC-Sicherung
    uint32_t store_max_cycles, total_max_cycles;
};

struct Info {
    uint32_t rid;          // Kennung des Puffers, neu bei jedem Neuanlegen
    uint32_t first, next;  // älteste vorhandene Nummer, nächste Nummer (first == next: leer)
    size_t used, capacity;
    size_t rtc_used, rtc_capacity;
    uint32_t rtc_first;
    bool mirror;           // RTC-Sicherung wird gerade zeilenweise geführt (Probelauf)
    uint16_t boot;
    const char* origin;    // Zustand beim Start: "übernommen", "aus RTC-Sicherung", "neu"
    uint32_t prev_lines;   // beim Start übernommene Zeilen früherer Starts
    void* address;         // Lage im PSRAM (für die Doku der Adresslage)
    Stats stats;
};

// Ganz am Anfang von app_main: Puffer prüfen bzw. übernehmen, Hook setzen
void init();
// Sobald der Startzähler aus dem NVS bekannt ist (trägt ihn auch in die bisherigen Zeilen dieses Starts ein)
void set_boot(uint16_t boot);
uint16_t boot();
// Uhrzeit für die Zeitstempel übernehmen (beim Start und nach jedem NTP-Abgleich, nie aus dem Hook)
void sync_clock();
// RTC-Sicherung zeilenweise führen (im Probelauf einer neuen Version an, nach bestandenem Selbsttest aus)
void set_mirror(bool on);
Info info();
uint32_t next_seq();

// Zeilen ab Nummer from lesen (frühere werden übersprungen), bis fn false liefert oder max_bytes Text erreicht ist.
// Gibt die Nummer nach der letzten gelesenen Zeile zurück.
using LineFn = bool (*)(const Line& line, void* ctx);
uint32_t read(uint32_t from, size_t max_bytes, LineFn fn, void* ctx);

// Puffer leeren (Befehl „log clear“); die Nummerierung läuft weiter
void clear();

// Zeile kompakt formatieren: „2026-10-19 12:10:49 #12 I (133456) web: …“; gibt die Länge zurück
size_t format(const Line& line, char* out, size_t out_len);

// Alle Log-Zeilen der aufrufenden Task zusätzlich in buf sammeln (Ausgabe eines Befehls für /api/cmd).
// Immer nur eine Aufnahme gleichzeitig.
class Capture {
public:
    Capture(char* buf, size_t cap);
    ~Capture();
    size_t size() const;
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;
};

// Solange das Objekt lebt, gehen Log-Zeilen der aufrufenden Task nur auf die serielle Konsole
// (Leistungsmonitor: alle 2 s seriell, nur jede Minute in den Puffer)
class SerialOnly {
public:
    explicit SerialOnly(bool active = true);
    ~SerialOnly();
    SerialOnly(const SerialOnly&) = delete;
    SerialOnly& operator=(const SerialOnly&) = delete;

private:
    bool active_;
};

// Messung für „log bench“: n Datensätze direkt in den Puffer (ohne seriell), mit oder ohne RTC-Sicherung,
// Takte pro Zeile (Mittel, Maximum)
void bench_store(int n, bool with_rtc, uint32_t* avg_cycles, uint32_t* max_cycles);

}  // namespace app::log
