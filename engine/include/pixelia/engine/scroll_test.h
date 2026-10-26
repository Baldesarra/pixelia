#pragma once
// Vollbild-Scrolling-Test für den Engine-Prototyp (Phase 0.1):
// Jedes Bild wird komplett neu gezeichnet: Himmel mit Dithering, Wolken, zwei Parallax-Ebenen,
// Kachel-Boden, Wasser mit Farbrotation und beliebig viele Vögel als Sprites (Lasttest).
// Tag und Nacht folgen der Weltzeit: Himmelsfarben, Licht auf der Landschaft, Sterne, nachts schlafen die Vögel.

#include <cstdint>
#include <vector>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/rng.h"
#include "pixelia/engine/scene.h"

namespace pixelia {

class ScrollTest : public Scene {
public:
    static constexpr int kMaxSprites = 2048;

    const char* name() const override { return "scrolltest"; }
    const char* title() const override { return "Landschaft"; }
    int colors() const override { return 80; }  // Indizes 0–79 (scroll_test.cpp, kLitEnd)
    void init(uint64_t seed) override;
    void tick(const WorldClock& clock) override;
    void render(Canvas& canvas) const override;
    bool sky_view(SkyView* out) const override;  // Himmel für den Mond (seit 0.5.7)
    const Palette& palette() const override { return palette_; }
    void set_load(int sprites) override { set_sprite_count(sprites); }

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
    struct Star {
        int16_t x, y;
        uint8_t phase;
    };
    static constexpr int kClouds = 6;
    static constexpr int kStars = 70;

    void build_palette();
    void build_terrain();
    void build_tiles();
    void build_sprites();
    void apply_daylight(double hours);

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
    int8_t bob_[32]{};
    int16_t clouds_[kClouds][2]{};
    Star stars_[kStars]{};
    std::vector<Bird> birds_;
    int sprite_count_ = 48;
    int visible_birds_ = 48;
    uint8_t night_ = 0;  // 0 = Tag, 255 = tiefe Nacht
    uint32_t ticks_ = 0;
    int32_t cloud_x_ = 0, far_x_ = 0, near_x_ = 0, ground_x_ = 0, water_x_ = 0;  // 8.8 Festkomma
};

}  // namespace pixelia
