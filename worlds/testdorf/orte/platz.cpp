// Testort Platz: gepflasterter Platz mit Bänken, Büschen, Laterne und Wasserbecken; Weg am linken Bildrand (→ Wiese),
// seit 0.5.8 Feldweg am rechten Bildrand (→ Wiese, 30 Weltsekunden, nur bei Tag). Ortszustände: Laterne
// (Abendlicht), Nässe (Pflaster dunkler, Pfützen), Füllstand des Beckens (Regen füllt, wer schöpft, leert).
#include "testdorf/testdorf.h"

namespace pixelia::testdorf {
namespace {

constexpr const char* kFloor[kTilesY] = {
    "hhhhhhhhhhhhhhhhhhhhhhhhh", "hhhhhhhhhhhhhhhhhhhhhhhhh", "HHHHHHHHHHHHHHHHHHHHHHHHH",
    "..ococococococococococo..", "..cocococococococococoo..", "..ococococococococococo..",
    "..cocococococococococoo..", "..ococococococococococo..", "..cocococococococococoo..",
    "==ococococococococococo==", "==cocococococococococoo==", "..ococococococococococo..",
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
    {kPPlant, 7, 7, 0, 1, 0, 0},          {kPPlant, 17, 7, 0, 3, 0, 0},
    {kPBasin, 9, 8, 0, 0, 0, 0},          {kPPuddle, 5, 6, 0, 0, 0, 0},
    {kPPuddle, 14, 10, 0, 0, 0, 0},
};
constexpr Exit kExits[] = {
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Edge, "testort-wiese", 1, Transition::Fade},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Path, "testort-wiese", 2, Transition::Fade, 30, kOpenDayOnly},
};
constexpr RoomDef kRoom = {{kFloor, nullptr, kDeco}, kLegend, {0, 0, 0}, kParts, 13, kExits, 2};
constexpr StateDef kStates[] = {
    {.kind = StateKind::Light, .id = "laterne", .part = 7, .light = LightKind::Lantern, .x = 100, .y = 56,
     .rule = StateRule::Evening},
    {.kind = StateKind::Wetness, .id = "boden"},
    {.kind = StateKind::Fill, .id = "becken", .part = 10, .capacity = 400, .rain = 120},
};

}  // namespace

const PlaceDef kPlatz = {"testort-platz", "Testort Platz", kPlaceOutdoor, &kRoom, nullptr, 4, false,
                         {258, 254, 246}, 256, {118, 114, 108}, GlassRole::None, kStates, 3, nullptr, 0};

}  // namespace pixelia::testdorf
