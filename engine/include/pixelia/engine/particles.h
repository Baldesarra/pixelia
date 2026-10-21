#pragma once
// Partikel (seit 0.5.3): Regen, Schnee, Blätter, später Rauch, Funken, Staub. Feste Obergrenze, Speicher im
// Grafik-Speicher (auf dem Gerät PSRAM). Gerechnet wird im Logik-Schritt (deterministisch, Reihenfolge fest),
// gezeichnet fein (400 × 240) nach dem Verdoppeln, über allem, was sich bewegt (weather.h, PlaceWeather).

#include <cstdint>

namespace pixelia {

enum class ParticleKind : uint8_t { Drop, Splash, Flake, BigFlake, Leaf };

struct Particle {
    int32_t x, y;    // feine Bildpunkte, 8.8 Festkomma
    int16_t vx, vy;  // 1/256 feine Bildpunkte je Logik-Schritt
    int16_t ground;  // feine Zeile des Aufschlags (Regen spritzt, Schnee schmilzt)
    ParticleKind kind;
    uint8_t age;
};

class ParticlePool {
public:
    ParticlePool() = default;
    ~ParticlePool() { release(); }
    ParticlePool(const ParticlePool&) = delete;
    ParticlePool& operator=(const ParticlePool&) = delete;

    bool init(int capacity);  // false = kein Platz im Grafik-Speicher
    void release();
    bool ready() const { return p_ != nullptr; }
    int capacity() const { return cap_; }
    int size() const { return n_; }
    void clear() { n_ = 0; }
    // Neuer Partikel am Ende; nullptr = Obergrenze erreicht (dann entsteht keiner)
    Particle* add() { return n_ < cap_ ? &p_[n_++] : nullptr; }
    // Entfernen: der letzte rückt nach (Reihenfolge bleibt deterministisch)
    void remove(int i) { p_[i] = p_[--n_]; }
    Particle& operator[](int i) { return p_[i]; }
    const Particle& operator[](int i) const { return p_[i]; }

private:
    Particle* p_ = nullptr;
    int cap_ = 0, n_ = 0;
};

}  // namespace pixelia
