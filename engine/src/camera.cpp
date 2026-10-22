#include "pixelia/engine/camera.h"

#include "pixelia/engine/stage.h"

namespace pixelia {

void Camera::set_settings(const CameraSettings& s) {
    settings_ = s;
    mode_ = Mode::Home;
    idle_ = 0;
    dirty_ = true;
}

uint16_t Camera::target(const Stage& stage) const {
    if (mode_ == Mode::Follow) return follow_;
    if (mode_ == Mode::Free) return 0;
    switch (settings_.home) {
        case CameraHome::Lead: return stage.lead();
        case CameraHome::Figure: return settings_.figure;
        default: return 0;
    }
}

int Camera::return_in_s() const {
    if (mode_ == Mode::Home || settings_.return_s == 0) return -1;
    const uint32_t limit = static_cast<uint32_t>(settings_.return_s) * kTicksPerSecond;
    return idle_ >= limit ? 0 : static_cast<int>((limit - idle_ + kTicksPerSecond - 1) / kTicksPerSecond);
}

void Camera::swipe(Stage& stage, int dir) {
    const int n = stage.count();
    if (n <= 0) return;
    go(stage, ((stage.active() + (dir < 0 ? -1 : 1)) % n + n) % n);
}

void Camera::go(Stage& stage, int place, Transition t) {
    idle_ = 0;
    mode_ = Mode::Free;
    apply(stage);
    if (place >= 0 && place < stage.count() && place != stage.active()) stage.go(place, t);
}

void Camera::follow(Stage& stage, uint16_t figure) {
    idle_ = 0;
    mode_ = Mode::Follow;
    follow_ = figure;
    apply(stage);
    seek(stage, figure);
}

void Camera::home(Stage& stage) {
    mode_ = Mode::Home;
    idle_ = 0;
    apply(stage);
    if (settings_.home == CameraHome::Place) {
        if (settings_.place >= 0 && settings_.place < stage.count() && settings_.place != stage.active())
            stage.go(settings_.place);
    } else {
        seek(stage, target(stage));
    }
}

void Camera::apply(Stage& stage) {
    // Wem der Blick durch die Ausgänge folgt
    uint16_t who = Stage::kNobody;
    if (mode_ == Mode::Follow) who = follow_;
    else if (mode_ == Mode::Home && settings_.home == CameraHome::Lead) who = 0;
    else if (mode_ == Mode::Home && settings_.home == CameraHome::Figure) who = settings_.figure;
    stage.set_follow_who(who);
}

void Camera::seek(Stage& stage, uint16_t figure) {
    if (!figure || stage.transitioning()) return;
    const int at = stage.find_figure(figure);
    if (at >= 0 && at != stage.active()) stage.go(at);
}

void Camera::tick(Stage& stage) {
    if (dirty_) {
        dirty_ = false;
        home(stage);
    }
    if (idle_ < 0xFFFFFFFFu) ++idle_;
    if (mode_ != Mode::Home && settings_.return_s > 0 &&
        idle_ >= static_cast<uint32_t>(settings_.return_s) * kTicksPerSecond) {
        home(stage);
        return;
    }
    // Einmal je Sekunde nachsehen, ob die verfolgte Figur woanders ist (z. B. während eines Übergangs gegangen)
    if (++check_ >= static_cast<uint32_t>(kTicksPerSecond)) {
        check_ = 0;
        const uint16_t t = target(stage);
        if (t) seek(stage, t);
        else if (mode_ == Mode::Home && settings_.home == CameraHome::Place && !stage.transitioning() &&
                 settings_.place >= 0 && settings_.place < stage.count() && settings_.place != stage.active())
            stage.go(settings_.place);
    }
}

}  // namespace pixelia
