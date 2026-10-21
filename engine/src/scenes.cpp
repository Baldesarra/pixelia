#include <cstring>

#include "pixelia/engine/scene.h"
#include "pixelia/engine/scroll_test.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/test_places.h"

namespace pixelia {

std::unique_ptr<Scene> make_scene(const char* name) {
    if (name == nullptr || std::strcmp(name, "scrolltest") == 0) return std::make_unique<ScrollTest>();
    if (std::strcmp(name, "testort-wiese") == 0) return std::make_unique<TestPlace>(TestPlace::Kind::Field);
    if (std::strcmp(name, "testort-raum") == 0) return std::make_unique<TestPlace>(TestPlace::Kind::Room);
    if (std::strcmp(name, "testort-platz") == 0) return std::make_unique<TestPlace>(TestPlace::Kind::Square);
    return nullptr;
}

const char* scene_names() { return "scrolltest, testort-wiese, testort-raum, testort-platz"; }

void add_standard_scenes(Stage& stage) {
    stage.add(make_scene("scrolltest"));
    stage.add(make_scene("testort-wiese"));
    stage.add(make_scene("testort-raum"));
    stage.add(make_scene("testort-platz"));
}

}  // namespace pixelia
