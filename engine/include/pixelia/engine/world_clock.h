#pragma once
// Weltzeit: Tag und Uhrzeit der Welt, unabhängig von der echten Uhr.
// Läuft pro Logik-Schritt um (echte Schrittdauer × Zeitraffer) weiter. Auf dem Gerät später per NTP gestellt,
// im Simulator frei wählbar (feste Uhrzeit, Zeitraffer für Langzeit-Tests).

#include <cstdint>

namespace pixelia {

// Logik-Rate der Engine: ein Schritt pro Engine-Bild (≈ 30 Hz, auf dem Gerät aus dem Bildzähler)
inline constexpr int kTicksPerSecond = 30;
inline constexpr int64_t kTickUs = 1000000 / kTicksPerSecond;

class WorldClock {
public:
    static constexpr int64_t kUsPerSecond = 1000000;
    static constexpr int64_t kUsPerMinute = 60 * kUsPerSecond;
    static constexpr int64_t kUsPerDay = 24 * 60 * kUsPerMinute;

    // Tag (ab 0) und Uhrzeit setzen
    void set(int day, int hour, int minute) {
        world_us_ = day * kUsPerDay + (hour * 60LL + minute) * kUsPerMinute;
    }
    void set_time_of_day(int hour, int minute) { set(day(), hour, minute); }
    void set_world_us(int64_t us) { world_us_ = us; }

    // Zeitraffer: 1 = Echtzeit, 60 = eine Weltminute pro echter Sekunde
    void set_speed(double factor) { speed_ = factor > 0 ? factor : 1.0; }
    double speed() const { return speed_; }

    // Einen Logik-Schritt weiterzählen; real_us = echte Dauer des Schritts
    void advance(int64_t real_us) {
        last_step_us_ = static_cast<int64_t>(real_us * speed_ + 0.5);
        world_us_ += last_step_us_;
        ++ticks_;
    }

    int64_t world_us() const { return world_us_; }
    int64_t last_step_us() const { return last_step_us_; }  // Weltzeit des letzten Schritts
    uint64_t ticks() const { return ticks_; }
    int day() const { return static_cast<int>(world_us_ / kUsPerDay); }
    int64_t us_of_day() const { return world_us_ % kUsPerDay; }
    int hour() const { return static_cast<int>(us_of_day() / (60 * kUsPerMinute)); }
    int minute() const { return static_cast<int>(us_of_day() / kUsPerMinute % 60); }
    int second() const { return static_cast<int>(us_of_day() / kUsPerSecond % 60); }
    // Tageszeit als Stunde mit Nachkommastellen (0 … 24)
    double hours() const { return static_cast<double>(us_of_day()) / (60.0 * kUsPerMinute); }

private:
    int64_t world_us_ = 12 * 60 * kUsPerMinute;  // Tag 0, 12:00
    int64_t last_step_us_ = 0;
    uint64_t ticks_ = 0;
    double speed_ = 1.0;
};

}  // namespace pixelia
