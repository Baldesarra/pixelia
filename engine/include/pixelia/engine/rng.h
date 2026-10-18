#pragma once
// Deterministischer Zufall (PCG32): gleicher Startwert = gleiche Folge, auf jedem Gerät und im Simulator.

#include <cstdint>

namespace pixelia {

class Rng {
public:
    explicit Rng(uint64_t seed = 0x853c49e6748fea9bULL) { reseed(seed); }

    void reseed(uint64_t seed) {
        state_ = 0;
        next();
        state_ += seed;
        next();
    }

    uint32_t next() {
        uint64_t old = state_;
        state_ = old * 6364136223846793005ULL + kInc;
        uint32_t xorshifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = static_cast<uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

    // Gleichverteilt in [0, n)
    uint32_t below(uint32_t n) { return static_cast<uint32_t>((static_cast<uint64_t>(next()) * n) >> 32); }

    // Gleichverteilt in [lo, hi]
    int range(int lo, int hi) { return lo + static_cast<int>(below(static_cast<uint32_t>(hi - lo + 1))); }

private:
    static constexpr uint64_t kInc = 1442695040888963407ULL;
    uint64_t state_ = 0;
};

}  // namespace pixelia
