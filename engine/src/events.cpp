#include "pixelia/engine/events.h"

namespace pixelia {

const char* event_kind_name(EventKind k) {
    switch (k) {
        case EventKind::SeasonEvening: return "jahreszeitenabend";
        case EventKind::SeasonBegins: return "jahreszeit";
        case EventKind::First: return "erstling";
        case EventKind::Arrived: return "ankunft";
        case EventKind::Departed: return "aufbruch";
        case EventKind::StateChanged: return "zustand";
        case EventKind::WeatherChanged: return "wetter";
        case EventKind::SkillUp: return "faehigkeit";
        case EventKind::BondChanged: return "beziehung";
        case EventKind::Stock: return "lager";
        case EventKind::Project: return "projekt";
        default: return "?";
    }
}

int EventBus::subscribe(uint32_t mask) {
    for (int i = 0; i < kReaders; ++i)
        if (!r_[i].used) {
            r_[i] = Reader{mask, visible_, true};
            return i;
        }
    return -1;
}

void EventBus::unsubscribe(int reader) {
    if (reader >= 0 && reader < kReaders) r_[reader] = Reader{};
}

void EventBus::publish(const WorldEvent& e) {
    q_[written_ % kQueue] = e;
    ++written_;
}

bool EventBus::poll(int reader, WorldEvent* out) {
    if (reader < 0 || reader >= kReaders || !r_[reader].used) return false;
    Reader& r = r_[reader];
    if (visible_ - r.next > static_cast<uint32_t>(kQueue)) {
        // überholt: die ältesten sind schon überschrieben
        lost_ += visible_ - r.next - kQueue;
        r.next = visible_ - kQueue;
    }
    while (r.next != visible_) {
        const WorldEvent& e = q_[r.next % kQueue];
        ++r.next;
        if (r.mask & event_bit(e.kind)) {
            if (out) *out = e;
            return true;
        }
    }
    return false;
}

void EventBus::clear() {
    written_ = visible_ = 0;
    lost_ = 0;
    for (Reader& r : r_) r.next = 0;
}

}  // namespace pixelia
