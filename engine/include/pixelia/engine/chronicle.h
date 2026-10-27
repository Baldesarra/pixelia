#pragma once
// Chronik und Besuchsverlauf (seit 0.5.9, LEBENDIGKEIT.md Regel 10): was in der Welt passiert ist, mit Zeitstempel,
// und wer wann wo zu Besuch war. Beides gehört zum Weltzustand (environment.h) und zum Speicherstand (save.h).
//
// - Die Chronik hält die letzten kMax Einträge (Ringpuffer, älteste weichen): Jahreszeitenabende, Erstlinge, neue
//   Stufen von Fähigkeiten und Beziehungen, Start der Welt und Pausen („Pixelia war 5 Std. aus“). Die Bühne trägt sie
//   aus dem Ereignis-Bus ein. Später (SD-Karte) wandern ältere Einträge ins Archiv.
// - Ein Besuch beginnt, wenn eine Figur einen Ort eines fremden Haushalts betritt, und endet, wenn sie geht.
// - Beide liegen im Grafik-Speicher (auf dem Gerät PSRAM), angelegt beim ersten Eintrag.

#include <cstdint>
#include <string_view>

#include "pixelia/engine/events.h"

namespace pixelia {

class FigureBook;

enum class ChronicleKind : uint8_t {
    Event,     // aus dem Ereignis-Bus (event = Art)
    Start,     // Welt beginnt (Tag 1)
    Wake,      // nach dem Einschalten: so lange war Pixelia aus (a, b: Stunden, Minuten; nachgeholt in text)
    Restored,  // Speicherstand eingespielt (Sicherung)
};

struct ChronicleEntry {
    int32_t day = 0;        // Welttag
    uint16_t minute = 0;    // Minute des Tages
    ChronicleKind kind = ChronicleKind::Event;
    EventKind event = EventKind::First;
    uint8_t a = 0, b = 0;
    uint16_t who = 0, other = 0;
    char text[60] = {};     // UTF-8
};

class Chronicle {
public:
    static constexpr int kMax = 64;
    Chronicle() = default;
    ~Chronicle();
    Chronicle(const Chronicle&) = delete;
    Chronicle& operator=(const Chronicle&) = delete;
    void clear();
    void add(const ChronicleEntry& e);
    // Ereignis des Busses eintragen, wenn es in die Chronik gehört (Kalender, Fähigkeit, Beziehung); Namen aus figures
    bool note(const WorldEvent& e, const FigureBook& figures, int32_t day, int minute);
    int count() const { return n_; }
    const ChronicleEntry& at(int i) const;  // 0 = ältester
    uint32_t total() const { return total_; }  // je eingetragen (auch die gewichenen)
    void set_total(uint32_t t) { total_ = t; }

private:
    bool ensure();
    ChronicleEntry* e_ = nullptr;
    int head_ = 0, n_ = 0;
    uint32_t total_ = 0;
};

struct Visit {
    static constexpr uint16_t kOpen = 0xFFFF;
    uint16_t who = 0;
    uint8_t place = 0xFF;   // Ort (Nummer in der Ortsliste der Welt)
    uint8_t pad = 0;
    int32_t day = 0;        // Welttag der Ankunft
    uint16_t from = 0;      // Minute der Ankunft
    uint16_t to = kOpen;    // Minute des Aufbruchs (Tage später: + 1440 je Tag, gekappt), kOpen = noch da
};

class VisitLog {
public:
    static constexpr int kMax = 64;
    VisitLog() = default;
    ~VisitLog();
    VisitLog(const VisitLog&) = delete;
    VisitLog& operator=(const VisitLog&) = delete;
    void clear();
    void arrive(uint16_t who, uint8_t place, int32_t day, int minute);
    void leave(uint16_t who, int32_t day, int minute);  // beendet den offenen Besuch der Figur
    void add(const Visit& v);                           // Laden
    int count() const { return n_; }
    const Visit& at(int i) const;  // 0 = ältester
    int open(uint16_t who) const;  // Index des offenen Besuchs, −1 = keiner

private:
    bool ensure();
    Visit* v_ = nullptr;
    int head_ = 0, n_ = 0;
};

}  // namespace pixelia
