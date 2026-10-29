#pragma once
// Testdorf (seit 0.5.8): Gebiet mit den Testorten, gebaut aus dem Baukasten der Testorte (test_place.h). Jeder Ort
// ist eine eigene Datei in orte/ mit seinen Daten (Kacheln, Raumliste, Verbindungen, Lichter); die Ortsliste steht in
// testdorf.cpp. Kein Setting-Inhalt: neutrale Platzhalter zum Prüfen der Engine-Bausteine.

#include <cstdint>

#include "pixelia/engine/place.h"

namespace pixelia::testdorf {

// Teile des Katalogs (Bilder in assets/testorte, Paket „moebel“ bzw. „raum“)
enum Part : uint8_t {
    kPDoor, kPWindow, kPPicture, kPWardrobe, kPBed, kPTable, kPChair, kPCrate, kPRug, kPLamp, kPPlant, kPToken,
    kPBush, kPBench, kPWindowBig, kPCandle, kPPot, kPOven, kPBasin, kPPuddle,
    // seit 0.5.10 Teil 2b: Lagerplätze mit Stellplätzen für jedes Stück, Arbeitsplätze
    kPWoodpile, kPWoodshed, kPBreadShelf, kPChopBlock, kPBakeOven, kPartCount,
};

// Farbreihen: 0 wie gezeichnet, 1–4 rot, blau, gelb, lavendel, 5 Holz, 6 Tapete (seit 0.5.8 Teil 2 ohne Grün: dessen
// vier Farben sind die Glut); seit 0.5.9 Teil 3 für den Figuren-Baukasten 7–9 Haut hell, mittel, dunkel, 10–13 Haar
// braun, blond, rot, grau, 14 Schwarz, 15 Leinen, 16 Grün
inline constexpr uint8_t kRowRed = 1, kRowBlue = 2, kRowYellow = 3, kRowLavender = 4, kRowWood = 5, kRowPaper = 6;
inline constexpr uint8_t kRowSkinLight = 7, kRowSkinMid = 8, kRowSkinDark = 9, kRowHairBrown = 10,
                         kRowHairBlond = 11, kRowHairRed = 12, kRowHairGrey = 13, kRowBlack = 14, kRowLinen = 15,
                         kRowGreen = 16;
inline constexpr int kRowCount = 17;

// Kacheln: Zeichen in den Kachelzeilen (tiles.h); Leerzeichen = leer
//  . , gras   " büschel   * blüte   = weg   _ - dielen   o c pflaster   W S T # wand   h H hecke   s kiesel
//  f blumen   ~ wasser
inline constexpr const char* kLegend = " .,\"*=_-ocWST#hHsf~";

extern const PlaceDef kWiese, kRaum, kKammer, kPlatz;
extern const AreaDef kArea;
extern const EconomyDef kEconomy;  // Waren, Lagerplätze, Startvorrat (seit 0.5.10, waren.cpp)

}  // namespace pixelia::testdorf
