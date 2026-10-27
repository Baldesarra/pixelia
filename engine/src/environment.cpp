#include "pixelia/engine/environment.h"

#include <algorithm>
#include <cstring>

namespace pixelia {

void WorldFlags::set(std::string_view key, bool on) {
    for (int i = 0; i < n_; ++i)
        if (key == k_[i].data()) {
            if (!on) {
                k_[i] = k_[--n_];
                k_[n_] = {};
            }
            return;
        }
    if (!on || n_ >= kMax || key.size() >= k_[0].size()) return;
    key.copy(k_[n_].data(), key.size());
    k_[n_][key.size()] = 0;
    ++n_;
}

bool WorldFlags::has(std::string_view key) const {
    for (int i = 0; i < n_; ++i)
        if (key == k_[i].data()) return true;
    return false;
}

}  // namespace pixelia
