// Gebiete der Welt (seit 0.5.8): neues Gebiet = neuer Ordner in worlds/ und eine Zeile hier (SETTING.md Kapitel 15)
#include "pixelia/engine/place.h"
#include "testdorf/testdorf.h"

namespace pixelia {

namespace {
constexpr const AreaDef* kAreas[] = {
    &testdorf::kArea,
};
}  // namespace

int world_areas(const AreaDef* const** out) {
    *out = kAreas;
    return static_cast<int>(sizeof(kAreas) / sizeof(kAreas[0]));
}

}  // namespace pixelia
