#include "testdorf/testdorf.h"

#include "testdorf/test_place.h"

namespace pixelia::testdorf {
namespace {

// Orte des Testdorfs: neuer Ort = eigene Datei in orte/ und eine Zeile hier. Die Reihenfolge bleibt (Nummern der
// Orte in Konsole, Weboberfläche und Einstellungen)
constexpr const PlaceDef* kPlaces[] = {
    &kWiese,
    &kRaum,
    &kPlatz,
    &kKammer,
};

std::unique_ptr<Scene> make(const PlaceDef& def, int index) { return std::make_unique<TestPlace>(def, index); }

}  // namespace

const AreaDef kArea = {"testdorf", "Testdorf", kPlaces, sizeof(kPlaces) / sizeof(kPlaces[0]), make,
                       nullptr,  0,          &kEconomy};

}  // namespace pixelia::testdorf
