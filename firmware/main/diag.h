#pragma once
// Diagnose (0.2.2 „Fernwartung“): Startzähler, Reset-Historie, Absturzbericht, Task-Liste und die JSON-Bausteine
// für /api/perf, /api/resets, /api/coredump/summary und das Diagnosepaket.
//
// - Startzähler und Reset-Historie (letzte 16 Starts) im NVS, Namensraum „diag“: Zeit, Grund, Version, Partition,
//   Probelauf, Rückfall, Absturzstelle. Die Laufzeit vor einem Reset kommt aus einem Herzschlag im RTC-Speicher
//   (alle 2 s geschrieben, übersteht jeden Reset außer Stromausfall).
// - Absturzbericht: Core-Dump (ELF) in der Partition „coredump“, Zusammenfassung über esp_core_dump_get_summary().

#include <cstddef>
#include <cstdint>
#include <string>

#include "esp_err.h"

namespace app::diag {

inline constexpr int kResetHistory = 16;

struct ResetEntry {
    uint32_t boot;
    uint8_t reason;     // esp_reset_reason_t
    bool trial;         // Start im Probelauf einer neuen Version
    bool rollback;      // in diesem Start einen Rückfall erkannt
    bool crash;         // Reset durch Absturz oder Watchdog, Absturzbericht dazu
    bool time_late;     // Startzeit erst nach dem NTP-Abgleich nachgetragen
    uint32_t t_boot;    // Uhrzeit beim Start (Unix-Sekunden), 0 = unbekannt
    uint32_t prev_up_s;  // Laufzeit des vorigen Starts bis kurz vor dem Reset, UINT32_MAX = unbekannt
    uint32_t prev_t;    // Uhrzeit des letzten Lebenszeichens davor (≈ Zeitpunkt des Resets), 0 = unbekannt
    uint32_t crash_pc;
    char version[32];
    char part[8];
    char crash_task[16];
    char note[128];     // Rückfall-Grund bzw. Absturzgrund
};

struct Crash {
    bool present;       // Absturzbericht in der Partition
    bool valid;         // Prüfsumme in Ordnung
    uint32_t addr, size;
    char task[16];
    uint32_t pc;
    uint32_t bt[16];
    int depth;
    bool bt_corrupted;
    uint32_t exc_cause, exc_vaddr;
    char reason[160];   // Panic-Grund aus dem Bericht
    char sha[17];       // ELF-SHA256 der abgestürzten Firmware (Anfang)
    char version[32];   // passende Version, wenn sie in einer der App-Partitionen liegt
    char part[8];
};

// Nach NVS und ota::init(): Startzähler erhöhen, Reset-Eintrag anlegen, Startzähler an den Log-Puffer geben
void init();
// Alle 2 s (Leistungsmonitor): Lebenszeichen im RTC-Speicher, Uhrzeit nachtragen
void heartbeat();
uint32_t boot();

int resets(ResetEntry* out, int max);  // neueste zuerst
bool crash(Crash* out);
esp_err_t crash_erase();

// JSON für die Schnittstellen und das Diagnosepaket
// with_tasks: Task-Liste mit Stack und CPU-Last (kostet etwa einen Display-Aussetzer, siehe diag.cpp)
std::string perf_json(bool with_tasks = false);
std::string resets_json();
std::string crash_json();
std::string system_json();
std::string log_info_json();

// Ausgabe auf der Konsole (Befehle tasks, resets, coredump)
void log_tasks();
void log_resets();
void log_crash();

}  // namespace app::diag
