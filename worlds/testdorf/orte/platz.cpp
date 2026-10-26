// Testort Platz: gepflasterter Platz mit Bänken, Büschen, Laterne und Wasserbecken; Weg am linken Bildrand (→ Wiese),
// seit 0.5.8 Feldweg am rechten Bildrand (→ Wiese, 30 Weltsekunden, nur bei Tag).
#include "testdorf/testdorf.h"

namespace pixelia::testdorf {
namespace {

constexpr const char* kFloor[kTilesY] = {
    "hhhhhhhhhhhhhhhhhhhhhhhhh", "hhhhhhhhhhhhhhhhhhhhhhhhh", "HHHHHHHHHHHHHHHHHHHHHHHHH",
    "..ococococococococococo..", "..cocococococococococoo..", "..ococococococococococo..",
    "..cocococococococococoo..", "..ococococococococococo..", "..cocococ~~~~~cocococoo..",
    "==ocococo~~~~~ococococo==", "==cocococococococococoo==", "..ococococococococococo..",
    "..cocococococococococoo..", "..ococococococococococo..", "hhhhhhhhhhhhhhhhhhhhhhhhh",
};
constexpr const char* kDeco[kTilesY] = {
    "", "", "", " f                     s ", "", "", "", "", "", "", "", "", "", " s                     f ", "",
};
constexpr RoomPart kParts[] = {
    {kPBush, 4, 3, 0, 0, 0, 0},           {kPBush, 19, 3, 0, 0, 0, 0},
    {kPBush, 4, 11, 0, 0, 0, 0},          {kPBush, 19, 11, 0, 0, 0, 0},
    {kPBench, 8, 4, 0, kRowWood, 0, 0},   {kPBench, 15, 4, 0, kRowWood, 0, 0},
    {kPBench, 11, 11, 0, kRowWood, 0, 0}, {kPLamp, 12, 6, 0, 0, 0, 0},
    {kPPlant, 7, 7, 0, 1, 0, 0},          {kPPlant, 17, 7, 0, 5, 0, 0},
};
constexpr Exit kExits[] = {
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Edge, "testort-wiese", 1, Transition::Fade},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "testort-wiese", 2, Transition::Fade, 30, kOpenDayOnly},
};
constexpr RoomDef kRoom = {{kFloor, nullptr, kDeco}, kLegend, {0, 0, 0}, kParts, 10, kExits, 2};
constexpr PlaceLight kLights[] = {{{100, 56, 64, 40, 255}, 7}};  // Laterne

}  // namespace

const PlaceDef kPlatz = {"testort-platz", "Testort Platz", kPlaceOutdoor, &kRoom, nullptr, 4, false,
                         {258, 254, 246}, 256, {118, 114, 108}, GlassRole::None, kLights, 1, nullptr, 0};

}  // namespace pixelia::testdorf
