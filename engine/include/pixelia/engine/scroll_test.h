#pragma once
// Vollbild-Scrolling-Test für den Engine-Prototyp (Phase 0.1):
// Jedes Bild wird komplett neu gezeichnet: Himmel mit Dithering, Wolken, zwei Parallax-Ebenen,
// Kachel-Boden, Wasser mit Farbrotation und beliebig viele Vögel als Sprites (Lasttest).

#include <cstdint>
#include <vector>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/rng.h"

namespace pixelia {

class ScrollTest {
public:
    static constexpr int kMaxSprites = 2048;

    void init(uint64_t seed);
    // Ein Logik-Schritt (feste Rate, ca. 30 Hz)
    void tick();
    void render(Canvas& canvas) const;

    const Palette& palette() const { return palette_; }
    void set_sprite_count(int n);
    int sprite_count() const { return sprite_count_; }
    uint32_t ticks() const { return ticks_; }

private:
    struct Bird {
        int32_t x;  // 8.8 Festkomma
        int16_t y;
        int16_t speed;
        uint8_t phase;
        uint8_t frame;
    };

    void build_palette();
    void build_terrain();
    void build_tiles();
    void build_sprites();

    Rng rng_;
    Palette base_palette_{};
    Palette palette_{};
    uint32_t sky_rows_[kScreenHeight]{};
    uint8_t far_[512]{};
    uint8_t near_[512]{};
    uint8_t tiles_[8][16 * 16]{};
    uint8_t map_[2][64]{};
    uint8_t water_[32][128]{};
    uint8_t bird_px_[2][12 * 8]{};
    uint8_t cloud_px_[48 * 16]{};
    std::vector<Bird> birds_;
    int sprite_count_ = 48;
    uint32_t ticks_ = 0;
    int32_t cloud_x_ = 0, far_x_ = 0, near_x_ = 0, ground_x_ = 0, water_x_ = 0;  // 8.8 Festkomma
};

}  // namespace pixelia
