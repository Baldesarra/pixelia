#include <cstring>

#include "pixelia/engine/scene.h"
#include "pixelia/engine/scroll_test.h"
#include "pixelia/engine/stage.h"
#include "pixelia/engine/place.h"

namespace pixelia {

std::unique_ptr<Scene> make_scene(const char* name) {
    if (name == nullptr || std::strcmp(name, "scrolltest") == 0) return std::make_unique<ScrollTest>();
    return make_place(name);  // Orte der Welt (seit 0.5.8, worlds/gebiete.cpp)
}

const char* scene_names() { return "scrolltest, testort-wiese, testort-raum, testort-platz, testort-kammer"; }

void add_standard_scenes(Stage& stage) {
    stage.add(make_scene("scrolltest"));
    const AreaDef* const* areas = nullptr;
    const int n = world_areas(&areas);
    for (int a = 0; a < n; ++a)
        for (int i = 0; i < areas[a]->count; ++i) stage.add(make_place(areas[a]->places[i]->id));
}

}  // namespace pixelia
