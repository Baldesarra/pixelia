#include "pixelia/engine/environment.h"

#include <algorithm>
#include <cstring>

namespace pixelia {

int LightBoard::find(std::string_view place) const {
    for (int i = 0; i < n_; ++i)
        if (place == e_[i].place) return i;
    return -1;
}

bool LightBoard::declare(std::string_view place, int sources) {
    const int n = std::clamp(sources, 0, kMaxSources);
    int i = find(place);
    if (i < 0) {
        if (n_ == kMaxPlaces || place.size() >= sizeof(Entry::place)) return false;
        i = n_++;
        std::memcpy(e_[i].place, place.data(), place.size());
        e_[i].place[place.size()] = 0;
        e_[i].on = 0xFFFF;
    }
    e_[i].sources = static_cast<uint8_t>(n);
    ++version_;
    return true;
}

void LightBoard::set(std::string_view place, int source, bool on) {
    const int i = find(place);
    if (i < 0 || source < 0 || source >= e_[i].sources) return;
    const uint16_t bit = static_cast<uint16_t>(1u << source);
    const uint16_t v = on ? (e_[i].on | bit) : (e_[i].on & ~bit);
    if (v == e_[i].on) return;
    e_[i].on = v;
    ++version_;
}

void LightBoard::set_all(std::string_view place, bool on) {
    for (int s = 0; s < sources(place); ++s) set(place, s, on);
}

bool LightBoard::on(std::string_view place, int source) const {
    const int i = find(place);
    return i >= 0 && source >= 0 && source < e_[i].sources && (e_[i].on >> source & 1);
}

bool LightBoard::any_on(std::string_view place) const {
    const int i = find(place);
    return i >= 0 && (e_[i].on & ((1u << e_[i].sources) - 1)) != 0;
}

int LightBoard::sources(std::string_view place) const {
    const int i = find(place);
    return i < 0 ? 0 : e_[i].sources;
}

}  // namespace pixelia
