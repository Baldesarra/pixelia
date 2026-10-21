#include "pixelia/engine/sprite.h"

namespace pixelia {

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

}  // namespace pixelia
