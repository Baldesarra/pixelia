// Testort Wiese: Wiese mit Hecke, Hauswand mit Tür (→ Raum), Weg am rechten Bildrand (→ Platz) und seit 0.5.8 ein
// Feldweg nach unten (→ Platz, 30 Weltsekunden unterwegs, nur bei Tag). Die Fenster gehören zum Raum dahinter.
#include "testdorf/testdorf.h"

namespace pixelia::testdorf {
namespace {

constexpr const char* kFloor[kTilesY] = {
    "hhhhhhhh.........hhhhhhhh", "hhhhhhhh.........hhhhhhhh", "HHHHHHHH.........HHHHHHHH",
    ".........................", "...........==............", "...........==............",
    "...........==............", "...........==............", "...........==............",
    "...........==============", "...........==============", ".......==................",
    ".......==................", ".......==................", ".......==................",
};
constexpr const char* kWalls[kTilesY] = {
    "        #########        ", "        TTTTTTTTT        ", "        WWWWWWWWW        ",
    "        SSSSSSSSS        ",
};
constexpr const char* kDeco[kTilesY] = {
    "", "", "", "", "   f                 s   ", "", "      s          f       ", "", "",
    "", "", "  f            s        ", "", "                     f   ", "",
};
constexpr RoomPart kParts[] = {
    {kPDoor, 11, 1, 0, kRowWood, 0, 0},  {kPWindow, 9, 1, 0, kRowWood, 0, 0},
    {kPWindow, 14, 1, 0, kRowWood, 0, 0}, {kPBush, 2, 4, 0, 0, 0, 0},
    {kPBush, 20, 4, 0, 0, 0, 0},          {kPBush, 4, 12, 0, 0, 0, 0},
    {kPCrate, 15, 4, 0, kRowWood, 0, 0},  {kPBench, 18, 12, 0, kRowWood, 0, 0},
};
constexpr Exit kExits[] = {
    {11, 3, 2, 1, 11, 4, Facing::Down, ExitKind::Door, "testort-raum", 0, Transition::Fade},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Edge, "testort-platz", 0, Transition::Fade},
    {7, 14, 2, 1, 7, 13, Facing::Up, ExitKind::Path, "testort-platz", 1, Transition::Fade, 30, kOpenDayOnly},
};
constexpr RoomDef kRoom = {{kFloor, kWalls, kDeco}, kLegend, {0, 0, 0}, kParts, 8, kExits, 3};
// Fenster des Hauses: Lichtkreis davor, leuchten nur, wenn im Raum dahinter Licht brennt
constexpr WindowLight kWindows[] = {{{80, 37, 22, 9, 190}, "testort-raum"}, {{120, 37, 22, 9, 190}, "testort-raum"}};

}  // namespace

const PlaceDef kWiese = {"testort-wiese", "Testort Wiese", kPlaceOutdoor | kPlaceMeadow, &kRoom, "wiese", 6, true,
                         {246, 256, 268}, 256, {0, 0, 0}, GlassRole::Glow, nullptr, 0, kWindows, 2};

}  // namespace pixelia::testdorf
