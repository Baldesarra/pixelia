// Testort Raum: Tapete, zwei große Fenster mit Vorhang und Fensterbank (Kerze, Blumentöpfe auf Stellplätzen),
// Schrank, Bett, Tisch, Stühle, Teppich, Lampe, seit 0.5.8 Teil 2 ein Ofen; Tür unten (→ Wiese), Innentür rechts
// (→ Kammer). Bett und Tisch haben zwei Ausbaustufen (Befehl „ausbau“), alle Möbel einen Zustand „im Bau“
// („baustelle“). Ortszustände: Lampe (Abendlicht), Kerze für Heimkehrer, Ofen mit Glut (Herdfeuer), Bett im Bau.
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
// Rückwand seit 0.5.8 eine Reihe höher: Platz für Fenster mit Fensterbank (STIL.md Kapitel 8)
constexpr const char* kWalls[kTilesY] = {
    "#########################", "#TTTTTTTTTTTTTTTTTTTTTTT#", "#WWWWWWWWWWWWWWWWWWWWWWW#",
    "#WWWWWWWWWWWWWWWWWWWWWWW#", "#WWWWWWWWWWWWWWWWWWWWWWW#", "#SSSSSSSSSSSSSSSSSSSSSSS#",
    "#                       #", "#                       #", "#                       #",
    "#                        ", "#                        ", "#                       #",
    "#                       #", "#                       #", "##########  #############",
};
// Fenster 0 und 1 tragen auf ihren Fensterbänken Kerze und Blumentöpfe (Stellplätze, seit 0.5.8)
constexpr RoomPart kParts[] = {
    {kPWindowBig, 4, 2, 0, kRowWood, 0, 0}, {kPWindowBig, 16, 2, 0, kRowWood, 0, 0},
    {kPPicture, 22, 3, 0, kRowWood, 0, 0},  {kPPicture, 15, 3, 0, kRowWood, 0, 0},
    {kPRug, 8, 8, 0, 4, 0, 0},              {kPWardrobe, 2, 6, 0, kRowWood, 0, 0},
    {kPBed, 21, 6, 0, 3, 0, 0},             {kPTable, 10, 8, 0, kRowWood, 0, 0},
    {kPChair, 9, 9, 0, kRowWood, 0, 0},     {kPChair, 12, 9, 0, kRowWood, 0, kPartFlipped},
    {kPLamp, 23, 12, 0, 3, 0, 0},           {kPPlant, 1, 12, 0, 1, 0, 0},
    {kPBreadShelf, 9, 3, 0, kRowWood, 0, 0}, {kSignaturePart, 17, 12, 0, 2, 1, 0},
    {kPCandle, 0, 0, 0, 0, 0, kPartOnSlot}, {kPPot, 0, 2, 0, 0, 0, kPartOnSlot},
    {kPPot, 1, 1, 0, 0, 0, kPartOnSlot},    {kPOven, 12, 6, 0, 1, 0, 0},
    {kPWoodshed, 14, 6, 0, kRowWood, 0, 0},  // seit 0.5.10 Teil 2b: Holzlege neben dem Herd
};
constexpr Exit kExits[] = {
    {10, 14, 2, 1, 10, 13, Facing::Up, ExitKind::Door, "testort-wiese", 0, Transition::Fade},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Door, "testort-kammer", 0, Transition::Fade},
};
constexpr RoomDef kRoom = {{kFloor, kWalls, nullptr}, kLegend, {0, kRowPaper, 0}, kParts, 19, kExits, 2};
// Ortszustände: Rüböllampe (Teil 10), Talgkerze auf der Fensterbank (Teil 14), Ofen (Teil 17), Bett (Teil 6)
constexpr StateDef kStates[] = {
    {.kind = StateKind::Light, .id = "lampe", .part = 10, .light = LightKind::OilLamp, .x = 188, .y = 100,
     .rule = StateRule::Evening},
    {.kind = StateKind::Light, .id = "kerze", .part = 14, .light = LightKind::Tallow, .x = 41, .y = 46,
     .rule = StateRule::Homecomer},
    {.kind = StateKind::Fire, .id = "ofen", .part = 17, .light = LightKind::Hearth, .x = 104, .y = 60,
     .rule = StateRule::Hearth, .fuel = "brennholz", .fuel_store = "raum_holzlege", .fuel_units = 1},
    {.kind = StateKind::Work, .id = "bett", .part = 6, .capacity = 1},
};

}  // namespace

const PlaceDef kRaum = {"testort-raum", "Testort Raum", 0, &kRoom, "raum", 3, false,
                        {268, 248, 222}, 220, {100, 74, 56}, GlassRole::Sky, kStates, 4, nullptr, 0, "testhaus"};

}  // namespace pixelia::testdorf
