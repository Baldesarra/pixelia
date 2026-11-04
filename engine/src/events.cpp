#include "pixelia/engine/events.h"

#include <algorithm>
#include <new>

#include "pixelia/engine/memory.h"

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

EventBus::~EventBus() {
    if (q_) asset_free(q_);  // WorldEvent ist trivial zerstörbar
}

bool EventBus::ensure() {
    // erst beim ersten Melden: die Bühne entsteht vor dem Speicher-Verteiler der Firmware (globales Objekt)
    if (q_) return true;
    void* p = asset_alloc(sizeof(WorldEvent) * kQueue);
    if (!p) return false;
    q_ = static_cast<WorldEvent*>(p);
    for (int i = 0; i < kQueue; ++i) new (&q_[i]) WorldEvent();
    return true;
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
    if (!ensure()) {
        ++lost_;  // kein Speicher: die Meldung geht verloren (gezählt), nichts Halbes im Ring
        return;
    }
    q_[written_ % kQueue] = e;
    ++written_;
}

void EventBus::deliver() {
    peak_ = std::max(peak_, written_ - visible_);
    visible_ = written_;
}

bool EventBus::poll(int reader, WorldEvent* out) {
    if (reader < 0 || reader >= kReaders || !r_[reader].used || !q_) return false;
    Reader& r = r_[reader];
    // Seit 0.5.11 (V01): Im Ring stehen die Nummern [written_ − kQueue, written_), lesbar sind nur die vor visible_.
    // Wer überholt wurde, verliert genau die überschriebenen Nummern und springt auf die älteste vorhandene, auch wenn
    // sie noch nicht zugestellt ist (dann gibt es bis zum nächsten deliver() nichts). Abstände als vorzeichenlose
    // Differenz: gilt auch beim Umlauf der Zähler
    if (written_ - r.next > static_cast<uint32_t>(kQueue)) {
        lost_ += written_ - r.next - kQueue;
        r.next = written_ - kQueue;
    }
    while (static_cast<int32_t>(visible_ - r.next) > 0) {
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
    peak_ = 0;
    for (Reader& r : r_) r.next = 0;
}

}  // namespace pixelia
