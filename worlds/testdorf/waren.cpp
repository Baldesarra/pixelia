// Waren, Lagerplätze und Startvorrat des Testdorfs (seit 0.5.10, ledger.h). Nur Prüfinhalt mit Testwerten: Die
// echten Waren (WIRTSCHAFT.md Kapitel 4) und ihre Zahlen kommen mit 0.6.x und dem Zeitraffer-Bericht.
//
// Holzkette (Wahl des Users 2026-10-29): Auf der Wiese wird Holz in den Holzstapel gespalten, die Holzlege neben dem
// Herd im Raum holt Nachschub (Korb: 10 Trageeinheiten), der Herd verbrennt das Holz (StateDef::fuel in orte/raum.cpp).
// Brot im Regal: einzelne Laibe, Nachschub aus der Vorratskiste am Platz, die ihr Brot vom Backofen daneben holt (seit
// Teil 2, Platzhalter: dort wird gebacken wie am Holzstapel gespalten). Bett-Projekt in der Kammer (seit Teil 3,
// projects.h): drei Teilaufgaben mit Bretter, Nägel und Stroh aus der Vorratskiste, Mengen nach WIRTSCHAFT 7 (Bett 8
// Bretter, 6 Nägel, Strohsack 4 Stroh, ohne Stoff), Dauer nach SETTING 8 (Bett 1 Tag ≈ 8 Arbeitsstunden); arbeiten darf
// nur, wer Tischlern kann (Wahl des Users), den Strohsack füllen kann jeder (seit Teil 3b). Die Säge ist ein
// Einzelstück in der Vorratskiste; Gestell und Liegefläche brauchen sie als Werkzeug (seit Teil 4, projects.h).
#include "testdorf/testdorf.h"

#include "pixelia/engine/projects.h"

namespace pixelia::testdorf {
namespace {

// Gewicht in Zehntel-Trageeinheiten (Korb trägt 100); Soll und Obergrenze in Tagen Reichweite (alles Testwerte)
constexpr GoodDef kGoods[] = {
    {.id = "brennholz", .title = "Brennholz", .weight = 10, .soll_days = 2, .max_days = 6, .one = "Scheit",
     .many = "Scheite"},
    {.id = "brot", .title = "Brot", .weight = 5, .soll_days = 1, .max_days = 4, .one = "Laib", .many = "Laibe"},
    {.id = "bretter", .title = "Bretter", .weight = 20},
    {.id = "naegel", .title = "Nägel", .weight = 1},
    {.id = "stroh", .title = "Stroh", .weight = 20, .one = "Bund Stroh", .many = "Bunde Stroh"},
    {.id = "saege", .title = "Säge", .weight = 20, .unique = true, .the = "die Säge"},
};

constexpr StockRule kWoodpile[] = {{"brennholz", 20, 40}};  // Erzeuger: unter 20 spalten, bis 40
constexpr StockRule kWoodshed[] = {{"brennholz", 4, 12}};   // Hausvorrat: unter 4 Nachschub holen, bis 12
constexpr StockRule kBread[] = {{"brot", 2, 8}};
constexpr StockRule kOven[] = {{"brot", 8, 16}};   // Erzeuger (Platzhalter, Wahl des Users 2026-10-29): backt bis 16
constexpr StockRule kStore[] = {{"brot", 12, 24}};  // Vorratskiste: Brot vom Backofen

// part: Teil der Raumliste, an dessen Benutzungspunkt geholt und gebracht wird; seit 0.5.10 Teil 2b eigene Teile mit
// einem Stellplatz je Stück (Holzstapel Wiese Teil 6 mit Hackklotz Teil 10, Holzlege Raum Teil 18, Brotregal Raum
// Teil 12, Lehmbackofen Platz Teil 14, seit Teil 3b Materialstapel Kammer Teil 6); die Vorratskiste (Platz Teil 13)
// bleibt eine Kiste
constexpr StoreDef kStores[] = {
    {.id = "wiese_holzstapel", .title = "Holzstapel", .place = "testort-wiese", .kind = StoreKind::Producer,
     .capacity = 40, .part = 6, .rules = kWoodpile, .rule_count = 1, .work_part = 10},
    {.id = "raum_holzlege", .title = "Holzlege", .place = "testort-raum", .kind = StoreKind::House, .capacity = 12,
     .part = 18, .rules = kWoodshed, .rule_count = 1, .source = "wiese_holzstapel"},
    {.id = "raum_brotregal", .title = "Brotregal", .place = "testort-raum", .kind = StoreKind::House, .capacity = 8,
     .part = 12, .rules = kBread, .rule_count = 1, .source = "platz_vorrat"},
    {.id = "platz_vorrat", .title = "Vorratskiste", .place = "testort-platz", .kind = StoreKind::Community,
     .capacity = 200, .part = 13, .rules = kStore, .rule_count = 1, .source = "platz_backofen"},
    {.id = "kammer_baustelle", .title = "Baustelle", .place = "testort-kammer", .kind = StoreKind::Site,
     .capacity = 60, .part = 6},  // Materialstapel neben dem Bett (seit Teil 3b)
    {.id = "platz_backofen", .title = "Backofen", .place = "testort-platz", .kind = StoreKind::Producer,
     .capacity = 20, .part = 14, .rules = kOven, .rule_count = 1},
};

// Bett in der Kammer: Teil 1 der Raumliste (orte/kammer.cpp), Baustelle kammer_baustelle
// Säge (seit Teil 4, Wahl des Users 2026-10-30): Werkzeug für Gestell und Liegefläche, geholt, nicht verbaut
constexpr ProjectNeed kFrame[] = {{"bretter", 6}, {"naegel", 4}, {"saege", 1, true}};
constexpr ProjectNeed kSlats[] = {{"bretter", 2}, {"naegel", 2}, {"saege", 1, true}};
constexpr ProjectNeed kSack[] = {{"stroh", 4}};
constexpr ProjectStepDef kBedSteps[] = {
    {.id = "gestell", .title = "Gestell zimmern", .label = "Gestell", .needs = kFrame, .need_count = 3,
     .skill = Skill::Joinery, .level = SkillLevel::Beginner, .minutes = 240},
    {.id = "liegeflaeche", .title = "Liegefläche legen", .label = "Liegefläche", .needs = kSlats, .need_count = 3,
     .skill = Skill::Joinery, .level = SkillLevel::Beginner, .minutes = 150},
    // Strohsack füllen kann jeder (seit Teil 3b, Wahl des Users: Helfer ohne Tischlern)
    {.id = "strohsack", .title = "Strohsack füllen", .label = "Strohsack", .needs = kSack, .need_count = 1,
     .skill = Skill::Joinery, .level = SkillLevel::Unknown, .minutes = 90},
};
constexpr ProjectDef kProjects[] = {
    {.id = "kammer_bett", .title = "Bett bauen", .kind = ProjectKind::Furniture, .place = "testort-kammer",
     .site = "kammer_baustelle", .source = "platz_vorrat", .part = 1, .steps = kBedSteps, .step_count = 3,
     .start = true},
};

constexpr StartStock kStart[] = {
    {"wiese_holzstapel", "brennholz", 24}, {"raum_holzlege", "brennholz", 8}, {"raum_brotregal", "brot", 6},
    {"platz_vorrat", "brot", 24},          {"platz_vorrat", "bretter", 12},   {"platz_vorrat", "naegel", 60},
    {"platz_vorrat", "stroh", 10},         {"platz_vorrat", "saege", 1, 820},     {"platz_backofen", "brot", 12},
};

}  // namespace

const EconomyDef kEconomy = {kGoods,    sizeof(kGoods) / sizeof(kGoods[0]),      kStores,
                             sizeof(kStores) / sizeof(kStores[0]),     kStart,    sizeof(kStart) / sizeof(kStart[0]),
                             kProjects, sizeof(kProjects) / sizeof(kProjects[0])};

}  // namespace pixelia::testdorf
