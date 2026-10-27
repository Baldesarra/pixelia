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
    kPBush, kPBench, kPWindowBig, kPCandle, kPPot, kPOven, kPBasin, kPPuddle, kPartCount,
};

// Farbreihen: 0 wie gezeichnet, 1–4 rot, blau, gelb, lavendel, 5 Holz, 6 Tapete (seit 0.5.8 Teil 2 ohne Grün: dessen
// vier Farben sind die Glut)
inline constexpr uint8_t kRowWood = 5, kRowPaper = 6;
inline constexpr int kRowCount = 7;

// Kacheln: Zeichen in den Kachelzeilen (tiles.h); Leerzeichen = leer
//  . , gras   " büschel   * blüte   = weg   _ - dielen   o c pflaster   W S T # wand   h H hecke   s kiesel
//  f blumen   ~ wasser
inline constexpr const char* kLegend = " .,\"*=_-ocWST#hHsf~";

extern const PlaceDef kWiese, kRaum, kKammer, kPlatz;
extern const AreaDef kArea;

}  // namespace pixelia::testdorf
