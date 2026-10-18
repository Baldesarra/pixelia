#include <cstring>

#include "pixelia/engine/scene.h"
#include "pixelia/engine/scroll_test.h"

namespace pixelia {

std::unique_ptr<Scene> make_scene(const char* name) {
    if (name == nullptr || std::strcmp(name, "scrolltest") == 0) return std::make_unique<ScrollTest>();
    return nullptr;
}

const char* scene_names() { return "scrolltest"; }

}  // namespace pixelia
