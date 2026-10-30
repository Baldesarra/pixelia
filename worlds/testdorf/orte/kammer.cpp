// Testort Kammer (seit 0.5.8): zweiter Raum im Haus hinter einer Innentür (→ Raum), ein großes Fenster mit
// Blumentopf, Bett, Stuhl, Kiste. Ohne eigenes Licht: nachts nur Mond und Dunkelheit. Seit 0.5.10 Teil 3b liegt links
// neben dem Bett der Materialstapel der Baustelle (Teil 6, unsichtbar, solange nichts darauf liegt; Bett-Projekt),
// darunter die Bauplane (Teil 7, Boden, nur solange gebaut wird).
#include "testdorf/testdorf.h"

namespace pixelia::testdorf {
namespace {

constexpr const char* kFloor[kTilesY] = {
    "_________________________", "-------------------------", "_________________________",
    "-------------------------", "_________________________", "-------------------------",
    "_________________________", "-------------------------", "_________________________",
    "-------------------------", "_________________________", "-------------------------",
    "_________________________", "-------------------------", "_________________________",
};
constexpr const char* kWalls[kTilesY] = {
    "#########################", "#TTTTTTTTTTTTTTTTTTTTTTT#", "#WWWWWWWWWWWWWWWWWWWWWWW#",
    "#WWWWWWWWWWWWWWWWWWWWWWW#", "#WWWWWWWWWWWWWWWWWWWWWWW#", "#SSSSSSSSSSSSSSSSSSSSSSS#",
    "#                       #", "#                       #", "#                       #",
    "                        #", "                        #", "#                       #",
    "#                       #", "#                       #", "#########################",
};
constexpr RoomPart kParts[] = {
    {kPWindowBig, 9, 2, 0, kRowWood, 0, 0}, {kPBed, 20, 6, 0, 1, 0, 0},
    {kPCrate, 2, 12, 0, kRowWood, 0, 0},    {kPChair, 12, 10, 0, kRowWood, 0, 0},
    {kPPlant, 22, 12, 0, 5, 0, 0},          {kPPot, 0, 1, 0, 0, 0, kPartOnSlot},
    {kPBuildPile, 14, 6, 0, 0, 0, 0},       {kPBuildCloth, 13, 6, 0, 0, 0, 0},
};
constexpr Exit kExits[] = {
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Door, "testort-raum", 1, Transition::Fade},
};
constexpr RoomDef kRoom = {{kFloor, kWalls, nullptr}, kLegend, {0, kRowPaper, 0}, kParts, 8, kExits, 1};

}  // namespace

const PlaceDef kKammer = {"testort-kammer", "Testort Kammer", 0, &kRoom, "raum", 1, false,
                          {262, 248, 230}, 210, {100, 74, 56}, GlassRole::Sky, nullptr, 0, nullptr, 0, "testhaus"};

}  // namespace pixelia::testdorf
