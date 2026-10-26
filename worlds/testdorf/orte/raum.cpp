// Testort Raum: Tapete, zwei große Fenster mit Vorhang und Fensterbank (Kerze, Blumentöpfe auf Stellplätzen),
// Schrank, Bett, Tisch, Stühle, Teppich, Lampe; Tür unten (→ Wiese), seit 0.5.8 Innentür rechts (→ Kammer).
// Bett und Tisch haben zwei Ausbaustufen (Befehl „ausbau“), alle Möbel einen Zustand „im Bau“ („baustelle“).
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
    {kPPicture, 11, 3, 0, kRowWood, 0, 0},  {kPPicture, 13, 3, 0, kRowWood, 0, 0},
    {kPRug, 8, 8, 0, 4, 0, 0},              {kPWardrobe, 2, 6, 0, kRowWood, 0, 0},
    {kPBed, 21, 6, 0, 3, 0, 0},             {kPTable, 10, 8, 0, kRowWood, 0, 0},
    {kPChair, 9, 9, 0, kRowWood, 0, 0},     {kPChair, 12, 9, 0, kRowWood, 0, kPartFlipped},
    {kPLamp, 23, 12, 0, 3, 0, 0},           {kPPlant, 1, 12, 0, 1, 0, 0},
    {kPCrate, 3, 12, 0, kRowWood, 0, 0},    {kSignaturePart, 17, 12, 0, 2, 1, 0},
    {kPCandle, 0, 0, 0, 0, 0, kPartOnSlot}, {kPPot, 0, 2, 0, 0, 0, kPartOnSlot},
    {kPPot, 1, 1, 0, 0, 0, kPartOnSlot},
};
constexpr Exit kExits[] = {
    {10, 14, 2, 1, 10, 13, Facing::Up, ExitKind::Door, "testort-wiese", 0, Transition::Fade},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Door, "testort-kammer", 0, Transition::Fade},
};
constexpr RoomDef kRoom = {{kFloor, kWalls, nullptr}, kLegend, {0, kRowPaper, 0}, kParts, 17, kExits, 2};
// Lampe (Quelle 0, Teil 10) und Kerze auf der Fensterbank (Quelle 1, Teil 14)
constexpr PlaceLight kLights[] = {{{188, 100, 58, 38, 255}, 10}, {{41, 46, 30, 22, 200}, 14}};

}  // namespace

const PlaceDef kRaum = {"testort-raum", "Testort Raum", 0, &kRoom, "raum", 3, false,
                        {268, 248, 222}, 220, {100, 74, 56}, GlassRole::Sky, kLights, 2, nullptr, 0};

}  // namespace pixelia::testdorf
