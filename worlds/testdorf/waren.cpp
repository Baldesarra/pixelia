// Waren, Lagerplätze und Startvorrat des Testdorfs (seit 0.5.10, ledger.h). Nur Prüfinhalt mit Testwerten: Die
// echten Waren (WIRTSCHAFT.md Kapitel 4) und ihre Zahlen kommen mit 0.6.x und dem Zeitraffer-Bericht.
//
// Holzkette (Wahl des Users 2026-10-29): Auf der Wiese wird Holz in den Holzstapel gespalten, die Holzlege neben dem
// Herd im Raum holt Nachschub (Korb: 10 Trageeinheiten), der Herd verbrennt das Holz (StateDef::fuel in orte/raum.cpp).
// Brot im Regal: einzelne Laibe, Nachschub aus der Vorratskiste am Platz. Bett-Projekt in der Kammer: Bretter, Nägel
// und Stroh aus der Vorratskiste, die Säge ist ein Einzelstück.
#include "testdorf/testdorf.h"

namespace pixelia::testdorf {
namespace {

// Gewicht in Zehntel-Trageeinheiten (Korb trägt 100); Soll und Obergrenze in Tagen Reichweite (alles Testwerte)
constexpr GoodDef kGoods[] = {
    {.id = "brennholz", .title = "Brennholz", .weight = 10, .soll_days = 2, .max_days = 6},
    {.id = "brot", .title = "Brot", .weight = 5, .soll_days = 1, .max_days = 4},
    {.id = "bretter", .title = "Bretter", .weight = 20},
    {.id = "naegel", .title = "Nägel", .weight = 1},
    {.id = "stroh", .title = "Stroh", .weight = 20},
    {.id = "saege", .title = "Säge", .weight = 20, .unique = true},
};

constexpr StockRule kWoodpile[] = {{"brennholz", 20, 40}};  // Erzeuger: unter 20 spalten, bis 40
constexpr StockRule kWoodshed[] = {{"brennholz", 4, 12}};   // Hausvorrat: unter 4 Nachschub holen, bis 12
constexpr StockRule kBread[] = {{"brot", 2, 8}};

constexpr StoreDef kStores[] = {
    {.id = "wiese_holzstapel", .title = "Holzstapel", .place = "testort-wiese", .kind = StoreKind::Producer,
     .capacity = 40, .rules = kWoodpile, .rule_count = 1},
    {.id = "raum_holzlege", .title = "Holzlege", .place = "testort-raum", .kind = StoreKind::House, .capacity = 12,
     .rules = kWoodshed, .rule_count = 1, .source = "wiese_holzstapel"},
    {.id = "raum_brotregal", .title = "Brotregal", .place = "testort-raum", .kind = StoreKind::House, .capacity = 8,
     .rules = kBread, .rule_count = 1, .source = "platz_vorrat"},
    {.id = "platz_vorrat", .title = "Vorratskiste", .place = "testort-platz", .kind = StoreKind::Community,
     .capacity = 200},
    {.id = "kammer_baustelle", .title = "Baustelle", .place = "testort-kammer", .kind = StoreKind::Site,
     .capacity = 60},
};

constexpr StartStock kStart[] = {
    {"wiese_holzstapel", "brennholz", 24}, {"raum_holzlege", "brennholz", 8}, {"raum_brotregal", "brot", 6},
    {"platz_vorrat", "brot", 24},          {"platz_vorrat", "bretter", 12},   {"platz_vorrat", "naegel", 60},
    {"platz_vorrat", "stroh", 10},         {"platz_vorrat", "saege", 1, 820},
};

}  // namespace

const EconomyDef kEconomy = {kGoods, sizeof(kGoods) / sizeof(kGoods[0]), kStores, sizeof(kStores) / sizeof(kStores[0]),
                             kStart, sizeof(kStart) / sizeof(kStart[0])};

}  // namespace pixelia::testdorf
