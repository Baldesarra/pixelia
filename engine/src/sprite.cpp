#include "pixelia/engine/sprite.h"

namespace pixelia {

int anim_frame_at(const Animation& a, uint32_t t) {
    if (a.count == 0) return 0;
    // Folge der Bilder eines Durchgangs: hin (Schleife, einmal) bzw. hin und zurück ohne die Enden doppelt
    const int n = a.count;
    const int len = a.mode == AnimMode::PingPong && n > 1 ? 2 * n - 2 : n;
    uint32_t total = 0;
    auto at = [&](int i) { return a.mode == AnimMode::PingPong && i >= n ? 2 * n - 2 - i : i; };
    for (int i = 0; i < len; ++i) total += a.frames[at(i)].ticks ? a.frames[at(i)].ticks : 1;
    if (a.mode == AnimMode::Once && t >= total) return a.frames[n - 1].frame;
    t %= total;
    for (int i = 0; i < len; ++i) {
        const uint32_t d = a.frames[at(i)].ticks ? a.frames[at(i)].ticks : 1;
        if (t < d) return a.frames[at(i)].frame;
        t -= d;
    }
    return a.frames[0].frame;
}

void Animator::play(const Animation* anim, bool restart) {
    if (anim == anim_ && !restart) return;
    anim_ = anim;
    index_ = 0;
    dir_ = 1;
    done_ = false;
    left_ = anim && anim->count ? (anim->frames[0].ticks ? anim->frames[0].ticks : 1) : 0;
}

void Animator::tick() {
    if (!anim_ || anim_->count == 0 || done_) return;
    if (--left_ > 0) return;
    const int n = anim_->count;
    int next = index_ + dir_;
    switch (anim_->mode) {
        case AnimMode::Loop:
            if (next >= n) next = 0;
            break;
        case AnimMode::Once:
            if (next >= n) {
                done_ = true;  // bleibt auf dem letzten Bild stehen
                return;
            }
            break;
        case AnimMode::PingPong:
            if (n == 1) {
                next = 0;
            } else if (next >= n) {
                dir_ = -1;
                next = n - 2;
            } else if (next < 0) {
                dir_ = 1;
                next = 1;
            }
            break;
    }
    index_ = static_cast<uint8_t>(next);
    left_ = anim_->frames[index_].ticks ? anim_->frames[index_].ticks : 1;
}

Recolor::Recolor() {
    for (int i = 0; i < 256; ++i) map_[i] = static_cast<uint8_t>(i);
}

Recolor& Recolor::ramp(uint8_t from, uint8_t to, uint8_t length) {
    for (int i = 0; i < length; ++i) set(static_cast<uint8_t>(from + i), static_cast<uint8_t>(to + i));
    return *this;
}

Recolor& Recolor::set(uint8_t from, uint8_t to) {
    if (from != 0) map_[from] = to;
    return *this;
}

Recolor& Recolor::load(const uint8_t* table) {
    if (!table) return *this;
    for (int i = 1; i < 256; ++i) map_[i] = table[i];
    return *this;
}

}  // namespace pixelia
