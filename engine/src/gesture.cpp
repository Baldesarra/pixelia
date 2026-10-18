#include "pixelia/engine/gesture.h"

#include <cstdlib>

namespace pixelia::gesture {

void Recognizer::touch(int fingers, int x, int y, uint32_t t_ms) {
    if (fingers <= 0) {
        if (down_ && !disqualified_ && !long_fired_ && t_ms - t0_ <= config_.tap_max_ms) push(Type::Tap, t_ms);
        down_ = false;
        return;
    }
    if (!down_) {
        down_ = true;
        disqualified_ = false;
        long_fired_ = false;
        x0_ = x;
        y0_ = y;
        t0_ = t_ms;
    }
    if (fingers > 1 || std::abs(x - x0_) > config_.move_tolerance || std::abs(y - y0_) > config_.move_tolerance)
        disqualified_ = true;
    tick(t_ms);
}

void Recognizer::tick(uint32_t t_ms) {
    if (!down_ || disqualified_ || long_fired_) return;
    if (t_ms - t0_ >= config_.long_press_ms) {
        long_fired_ = true;
        push(Type::LongPress, t_ms);
    }
}

bool Recognizer::poll(Event* out) {
    if (count_ == 0) return false;
    *out = queue_[head_];
    head_ = (head_ + 1) % kQueue;
    --count_;
    return true;
}

float Recognizer::hold_progress(uint32_t t_ms) const {
    if (!down_ || disqualified_ || long_fired_ || config_.long_press_ms == 0) return -1.0f;
    float p = static_cast<float>(t_ms - t0_) / static_cast<float>(config_.long_press_ms);
    return p > 1.0f ? 1.0f : p;
}

void Recognizer::push(Type type, uint32_t t_ms) {
    if (count_ == kQueue) {  // voll: ältestes verwerfen
        head_ = (head_ + 1) % kQueue;
        --count_;
    }
    queue_[(head_ + count_) % kQueue] = {type, x0_, y0_, t_ms - t0_};
    ++count_;
}

}  // namespace pixelia::gesture
