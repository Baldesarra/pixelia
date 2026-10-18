#pragma once
// Gestenerkennung aus rohen Touch-Daten: Tippen und langer Druck, plattformunabhängig (Gerät und Simulator).
//
// Grundsätze, damit spätere Gesten (Wischen, Doppeltippen, zwei Finger) ohne Umbau dazukommen:
// - Eine Berührung ist ein Druck vom ersten Aufsetzen bis zum Loslassen aller Finger.
// - Bewegt sich der Finger weiter als move_tolerance, ist der Druck kein Tippen und kein langer Druck mehr
//   (später: Wischen/Ziehen). Mit zwei oder mehr Fingern ebenso.
// - Der lange Druck feuert, sobald die Zeit erreicht ist (noch während der Finger liegt), danach kommt beim
//   Loslassen kein Tippen mehr.
// - Ereignisse landen in einer kleinen Warteschlange und werden mit poll() abgeholt; wer sie verteilt (System-UI,
//   Welten), entscheidet der Aufrufer.

#include <cstdint>

namespace pixelia::gesture {

enum class Type : uint8_t {
    Tap,        // kurz berührt und losgelassen, ohne Bewegung
    LongPress,  // still gehalten bis long_press_ms
};

struct Event {
    Type type;
    int x, y;              // Ort des ersten Aufsetzens (Koordinaten wie übergeben)
    uint32_t duration_ms;  // Dauer des Drucks bis zum Ereignis
};

struct Config {
    uint32_t long_press_ms = 3000;
    uint32_t tap_max_ms = 600;  // längere Drücke ohne Bewegung sind kein Tippen mehr
    int move_tolerance = 24;    // erlaubte Bewegung in Koordinateneinheiten (auf dem Gerät Panel-Pixel 800×480)
};

class Recognizer {
public:
    void set_config(const Config& config) { config_ = config; }
    const Config& config() const { return config_; }

    // Neue Touch-Daten: fingers = Zahl der Finger (0 = losgelassen), x/y des ersten Fingers, t in ms (monoton)
    void touch(int fingers, int x, int y, uint32_t t_ms);
    // Zeit fortschreiben: prüft den langen Druck auch dann, wenn keine neuen Touch-Daten kommen
    void tick(uint32_t t_ms);
    // Nächstes Ereignis abholen, false = keins
    bool poll(Event* out);

    // Fortschritt des möglichen langen Drucks 0..1, negativ = gerade keiner möglich (für eine Halte-Anzeige)
    float hold_progress(uint32_t t_ms) const;
    bool pressed() const { return down_; }

private:
    void push(Type type, uint32_t t_ms);

    Config config_;
    bool down_ = false;
    bool disqualified_ = false;  // bewegt, mehrere Finger oder schon als langer Druck gemeldet
    bool long_fired_ = false;
    int x0_ = 0, y0_ = 0;
    uint32_t t0_ = 0;

    static constexpr int kQueue = 4;
    Event queue_[kQueue] = {};
    int head_ = 0, count_ = 0;
};

}  // namespace pixelia::gesture
