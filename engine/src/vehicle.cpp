#include "pixelia/engine/vehicle.h"

namespace pixelia {

int vehicle_cycle(const VehicleDef& v) {
    int sum = 0;
    for (int i = 0; i < v.stop_count; ++i) sum += v.stops[i].dwell + v.stops[i].travel;
    return sum;
}

VehiclePos vehicle_at(const VehicleDef& v, int64_t world_minute) {
    VehiclePos p;
    const int cycle = vehicle_cycle(v);
    if (cycle <= 0 || v.stop_count == 0) return p;
    int64_t t = (world_minute - v.start) % cycle;
    if (t < 0) t += cycle;
    for (int i = 0; i < v.stop_count; ++i) {
        const VehicleStop& s = v.stops[i];
        p.stop = i;
        p.next = (i + 1) % v.stop_count;
        if (t < s.dwell) {
            p.standing = true;
            p.minutes = static_cast<int>(t);
            p.left = static_cast<int>(s.dwell - t);
            return p;
        }
        t -= s.dwell;
        if (t < s.travel) {
            p.standing = false;
            p.minutes = static_cast<int>(t);
            p.left = static_cast<int>(s.travel - t);
            return p;
        }
        t -= s.travel;
    }
    return p;
}

bool Vehicles::add(const VehicleDef* v) {
    if (!v) return false;
    for (int i = 0; i < n_; ++i)
        if (v_[i] == v) return true;
    if (n_ >= kMax) return false;
    v_[n_++] = v;
    return true;
}

const VehicleDef* Vehicles::find(std::string_view id) const {
    for (int i = 0; i < n_; ++i)
        if (id == v_[i]->id) return v_[i];
    return nullptr;
}

bool Vehicles::stands_at(std::string_view id, std::string_view place, int64_t world_minute) const {
    const VehicleDef* v = find(id);
    if (!v || v->stop_count == 0) return false;
    const VehiclePos p = vehicle_at(*v, world_minute);
    return p.standing && place == v->stops[p.stop].place;
}

}  // namespace pixelia
