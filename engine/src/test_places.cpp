#include "pixelia/engine/test_places.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <new>
#include <string_view>
#include <vector>

#include "pixelia/engine/assets.h"
#include "pixelia/engine/font.h"
#include "pixelia/engine/memory.h"
#include "pixelia/engine/overlay.h"
#include "pixelia/engine/system_ui.h"

namespace pixelia {

namespace {

// Palette: alle Orte gleich aufgebaut, getönt wird am Ende (Wiese kühl, Raum warm, Platz neutral)
constexpr uint8_t kOutline = 1;
constexpr uint8_t kGrass = 2;   // 2..5
constexpr uint8_t kPath = 6;    // 6..8
constexpr uint8_t kTuft = 9;
constexpr uint8_t kBloom = 10;  // 10..11
constexpr uint8_t kFloor = 12;  // 12..15
constexpr uint8_t kWall = 16;   // 16..19
constexpr uint8_t kRamp = 20;   // 20..23 graue Platzhalter-Rampe, wird per Farbreihe ersetzt
constexpr uint8_t kRows = 24;   // 24..47: sechs Farbreihen zu je 4 Farben
constexpr int kFigureRows = 6;
constexpr uint8_t kSkin = 48;  // 48..50
constexpr uint8_t kHair = 51;  // 51..53
constexpr uint8_t kEye = 54;
constexpr uint8_t kDark = 55;   // Türöffnung, Fugen
constexpr uint8_t kGlow = 56;   // Lampenlicht
constexpr uint8_t kShadow = 57;
constexpr uint8_t kStone = 58;  // 58..61
constexpr uint8_t kWallTop = 62;
constexpr uint8_t kGlass = 63;
constexpr uint8_t kWood = 64;     // 64..67 Farbreihe Holz
constexpr uint8_t kPastel = 68;   // 68..71 Tapete (Farbreihe für die Wand-Ebene)
// seit 0.5.2 (Farbe und Licht): Rollen und Farbrotation, light.h
constexpr uint8_t kSky = 72;    // 72..73 Fensterglas: Himmelsverlauf (Raum) bzw. Licht im Fenster am Abend (Wiese)
constexpr uint8_t kWater = 74;  // 74..77 Wasser (Farbrotation)
constexpr uint8_t kFlame = 78;  // 78..79 Schirm der Lampe, leuchtet bei Dunkelheit (Farbrotation), Kern kGlow
static_assert(kFlame + 2 <= 80, "Grundfarben passen in eine Lichtstufe");

// Farbreihen des Katalogs: 0 wie gezeichnet, 1–6 rot, blau, gelb, lavendel, grün, türkis, 7 Holz, 8 Tapete
constexpr int kRowWood = 7, kRowPaper = 8, kRowCount = 9;

constexpr int kPoses = 5;  // Figur (Bild figur/test): Posen stehen, Schritt, Mitte, Schritt, atmen; drei Ansichten

constexpr AnimFrame kWalkFrames[] = {{1, 6}, {2, 6}, {3, 6}, {2, 6}};
constexpr AnimFrame kIdleFrames[] = {{0, 54}, {4, 18}};
constexpr Animation kWalk{kWalkFrames, 4, AnimMode::Loop};
constexpr Animation kIdle{kIdleFrames, 2, AnimMode::Loop};

// Kacheln: Legende Zeichen → Nummer (tiles.h); 0 = leer
enum Tile : uint8_t {
    kTGrassA = 1, kTGrassB, kTTuft, kTBloom, kTPath, kTPlankA, kTPlankB, kTStoneA, kTStoneB,
    kTWallFace, kTWallBase, kTWallBand, kTWallTop, kTHedge, kTHedgeFace, kTPebbles, kTFlowers, kTWater, kTileCount
};
constexpr const char* kLegend = " .,\"*=_-ocWST#hHsf~";
static_assert(kTileCount == 19, "Legende passt zu den Kacheln");

// Teile des Katalogs
enum Part : uint8_t {
    kPDoor, kPWindow, kPPicture, kPWardrobe, kPBed, kPTable, kPChair, kPCrate, kPRug, kPLamp, kPPlant, kPToken,
    kPBush, kPBench, kPartCount,
    kPBedFrame = kPartCount, kPSite  // Bilder „im Bau“: halbfertiges Bett, Materialstapel (keine eigenen Teile)
};

// Benutzungspunkte (Kachel relativ zur Lage des Teils)
constexpr UsePoint kUseWardrobe[] = {{0, 1, Facing::Up, UsePose::Stand, 0}, {1, 1, Facing::Up, UsePose::Stand, 0}};
constexpr UsePoint kUseWindow[] = {{0, 3, Facing::Up, UsePose::Stand, 0}, {1, 3, Facing::Up, UsePose::Stand, 0}};
constexpr UsePoint kUseBed[] = {{-1, 1, Facing::Right, UsePose::Stand, 0}};
constexpr UsePoint kUseTable[] = {{0, 2, Facing::Up, UsePose::Stand, 0}, {1, 2, Facing::Up, UsePose::Stand, 0}};
constexpr UsePoint kUseChair[] = {{0, 0, Facing::Down, UsePose::Sit, 4}};
constexpr UsePoint kUseCrate[] = {{0, 1, Facing::Up, UsePose::Stand, 0}};
constexpr UsePoint kUseToken[] = {{0, 1, Facing::Up, UsePose::Stand, 0}};
constexpr UsePoint kUseBench[] = {{0, 0, Facing::Down, UsePose::Sit, 4}, {1, 0, Facing::Down, UsePose::Sit, 4}};
constexpr UsePoint kUseBuild[] = {{-1, 1, Facing::Right, UsePose::Stand, 0}};  // am halbfertigen Bett arbeiten

// Teil-Stufen: Bild (Kennung im Asset-Paket), Stellfläche fw × fh, Benutzungspunkte. Die Bilder liegen seit 0.5.5 als
// PNG in assets/testorte/ (tools/asset_pack.py) und kommen über den Grafik-Speicher (assets.h)
struct StageSpec {
    uint8_t part;
    const char* image;
    uint8_t fw, fh;
    const UsePoint* uses;
    uint8_t use_count;
};
constexpr StageSpec kStages[] = {
    {kPDoor, "teil/tuer", 2, 3, nullptr, 0},
    {kPWindow, "teil/fenster", 2, 2, kUseWindow, 2},
    {kPPicture, "teil/bild", 1, 1, nullptr, 0},
    {kPWardrobe, "teil/schrank", 2, 1, kUseWardrobe, 2},
    {kPBed, "teil/bett/0", 2, 3, kUseBed, 1},       // Stufe 0: Lager am Boden
    {kPBed, "teil/bett/1", 2, 3, kUseBed, 1},       // Stufe 1: Bett mit Kopfteil
    {kPTable, "teil/tisch/0", 2, 2, kUseTable, 2},  // Stufe 0: zwei Kisten
    {kPTable, "teil/tisch/1", 2, 2, kUseTable, 2},  // Stufe 1: Tisch
    {kPChair, "teil/stuhl", 1, 1, kUseChair, 1},
    {kPCrate, "teil/kiste", 2, 1, kUseCrate, 1},
    {kPRug, "teil/teppich", 4, 3, nullptr, 0},
    {kPLamp, "teil/lampe", 1, 1, nullptr, 0},  // Zustände aus, an (nach der Lichtquelle); Schirm flackert
    {kPPlant, "teil/pflanze", 1, 1, nullptr, 0},
    {kPToken, "teil/zeichen", 1, 1, kUseToken, 1},
    {kPBush, "teil/busch", 2, 2, nullptr, 0},
    {kPBench, "teil/bank", 2, 1, kUseBench, 2},
    {kPBedFrame, "teil/bett/bau", 2, 3, kUseBuild, 1},  // im Bau: Rahmen ohne Matratze
    {kPSite, "teil/stapel", 1, 1, nullptr, 0},          // Materialstapel: Bretter
};
constexpr int kStageCount = sizeof(kStages) / sizeof(kStages[0]);

struct PartSpec {
    const char* id;
    PartKind kind;
    PartLayer layer;
    bool blocks;
};
constexpr PartSpec kParts[kPartCount] = {
    {"tuer", PartKind::Door, PartLayer::Wall, false},
    {"fenster", PartKind::Window, PartLayer::Wall, false},
    {"bild", PartKind::Deco, PartLayer::Wall, false},
    {"schrank", PartKind::Storage, PartLayer::Standing, true},
    {"bett", PartKind::Bed, PartLayer::Standing, true},
    {"tisch", PartKind::Table, PartLayer::Standing, true},
    {"stuhl", PartKind::Seat, PartLayer::Standing, true},
    {"kiste", PartKind::Storage, PartLayer::Standing, true},
    {"teppich", PartKind::Deco, PartLayer::Floor, false},
    {"lampe", PartKind::Light, PartLayer::Standing, true},
    {"pflanze", PartKind::Deco, PartLayer::Standing, true},
    {"zeichen", PartKind::Deco, PartLayer::Standing, true},
    {"busch", PartKind::Deco, PartLayer::Standing, true},
    {"bank", PartKind::Seat, PartLayer::Standing, true},
};

// Asset-Pakete (assets/testorte/pakete.json): Grundausstattung immer geladen, gemeinsame Möbel und Ortspakete beim
// Betreten (TestPlace::packs)
constexpr const char* kBasePack = "grund";
constexpr const char* kRowNames[kRowCount] = {nullptr,          "farbreihe/rot",   "farbreihe/blau",
                                              "farbreihe/gelb", "farbreihe/lavendel", "farbreihe/gruen",
                                              "farbreihe/tuerkis", "farbreihe/holz", "farbreihe/tapete"};

// Gemeinsamer Katalog aller Testorte. Figur, Kacheln, Palette und Farbreihen kommen aus der Grundausstattung, die
// Bilder der Teile aus den Paketen der Orte
struct Kit {
    SpriteSheet figure;
    TileSet tileset;
    const uint8_t* colors = nullptr;  // Grundfarben RGB (palette/testorte)
    int color_count = 0;
    PartStage stages[kStageCount];
    PartDef parts[kPartCount];
    const PartStage* site;
    Recolor rows[kRowCount];
    PartCatalog catalog;
    uint32_t generation = ~0u;  // Stand des Grafik-Speichers beim letzten Auflösen
};
Kit* s_kit = nullptr;

// Bilder der Teile neu auflösen, wenn sich der Grafik-Speicher geändert hat (beim Betreten eines Ortes). Gültig sind
// danach die Bilder geladener Pakete; ein Ort zeichnet nur seine eigenen, und die sind angeheftet, solange er
// betreten ist (stage.h). Fehlt ein Bild, bleibt das Teil unsichtbar.
void resolve(Kit& k) {
    const AssetStore& store = asset_store();
    const uint32_t gen = store.generation();
    if (k.generation == gen) return;
    k.generation = gen;
    for (int s = 0; s < kStageCount; ++s) {
        ImageAsset img;
        k.stages[s].sheet = store.image(asset_id(kStages[s].image), &img) ? img.sheet : SpriteSheet{};
    }
}

// Erkennungsstück: in den Testorten bekommt jeder Besitzer denselben neutralen Sockel (Farbe aus der Raumliste)
int token_for(uint8_t owner) { return owner ? kPToken : -1; }

Kit& kit() {
    if (!s_kit) {
        AssetStore& store = asset_store();
        store.load_base(kBasePack);  // sofort und für immer (auch auf dem Gerät beim Start)
        void* mem = asset_alloc(sizeof(Kit));
        Kit* k = mem ? new (mem) Kit : new Kit;
        ImageAsset fig;
        if (store.image(asset_id("figur/test"), &fig)) k->figure = fig.sheet;
        store.tiles(asset_id("kacheln/testorte"), &k->tileset);
        k->colors = store.palette(asset_id("palette/testorte"), &k->color_count);
        for (int s = 0; s < kStageCount; ++s) {
            const StageSpec& sp = kStages[s];
            k->stages[s] = PartStage{SpriteSheet{}, sp.fw, sp.fh, 0, 0, 45, sp.uses, sp.use_count};
        }
        auto stage_of = [&](int part, int* n) {
            int first = 0;
            *n = 0;
            for (int s = 0; s < kStageCount; ++s)
                if (kStages[s].part == part && (*n)++ == 0) first = s;
            return &k->stages[first];
        };
        int n = 0;
        for (int i = 0; i < kPartCount; ++i) {
            const PartStage* first = stage_of(i, &n);
            k->parts[i] = PartDef{kParts[i].id, kParts[i].kind, kParts[i].layer, kParts[i].blocks, first,
                                  static_cast<uint8_t>(n)};
        }
        k->parts[kPBed].building = stage_of(kPBedFrame, &n);
        k->site = stage_of(kPSite, &n);
        for (int r = 1; r < kRowCount; ++r) k->rows[r].load(store.recolor(asset_id(kRowNames[r])));
        k->catalog = PartCatalog{k->parts, kPartCount, k->rows, kRowCount, token_for, k->site};
        resolve(*k);
        s_kit = k;  // bleibt bis zum Ende (wird nie freigegeben)
    }
    return *s_kit;
}



Color tint(Color c, int r256, int g256, int b256) {
    auto ch = [](int v, int k) { return static_cast<uint8_t>(std::min(255, v * k >> 8)); };
    return {ch(c.r, r256), ch(c.g, g256), ch(c.b, b256)};
}

// Grundstimmung der Testorte (Vergleich für docs/STIL.md): 0 warm und gemütlich, 1 kräftig und bunt, 2 gedeckt
Color grade(Color c, int mood) {
    const int lum = (77 * c.r + 150 * c.g + 29 * c.b) >> 8;
    const int sat = mood == 1 ? 330 : mood == 2 ? 170 : 232;
    int r = lum + ((c.r - lum) * sat >> 8), g = lum + ((c.g - lum) * sat >> 8), b = lum + ((c.b - lum) * sat >> 8);
    if (mood == 0) {
        // wärmer, Schatten leicht ins Violettbraune angehoben
        r = (r * 266 >> 8) + ((255 - lum) * 8 >> 8);
        b = (b * 236 >> 8) + ((255 - lum) * 12 >> 8);
    } else if (mood == 1) {
        r = 128 + ((r - 128) * 280 >> 8);
        g = 128 + ((g - 128) * 280 >> 8);
        b = 128 + ((b - 128) * 280 >> 8);
    } else if (mood == 2) {
        r = (r * 204 + (lum * 276 >> 8) * 52) >> 8;
        b = (b * 204 + (lum * 214 >> 8) * 52) >> 8;
    }
    auto cl = [](int v) { return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v); };
    return {cl(r), cl(g), cl(b)};
}

// --- Orte als Daten: Kachelzeilen (25 × 15), Raumliste, Ausgänge ---

constexpr const char* kFieldFloor[kTilesY] = {
    "hhhhhhhh.........hhhhhhhh", "hhhhhhhh.........hhhhhhhh", "HHHHHHHH.........HHHHHHHH",
    ".........................", "...........==............", "...........==............",
    "...........==............", "...........==............", "...........==............",
    "...........==============", "...........==============", ".........................",
    ".........................", ".........................", ".........................",
};
constexpr const char* kFieldWalls[kTilesY] = {
    "        #########        ", "        TTTTTTTTT        ", "        WWWWWWWWW        ",
    "        SSSSSSSSS        ",
};
constexpr const char* kFieldDeco[kTilesY] = {
    "", "", "", "", "   f                 s   ", "", "      s          f       ", "", "",
    "", "", "  f            s        ", "", "                     f   ", "",
};
constexpr RoomPart kFieldParts[] = {
    {kPDoor, 11, 1, 0, kRowWood, 0, 0},  {kPWindow, 9, 1, 0, kRowWood, 0, 0},
    {kPWindow, 14, 1, 0, kRowWood, 0, 0}, {kPBush, 2, 4, 0, 0, 0, 0},
    {kPBush, 20, 4, 0, 0, 0, 0},          {kPBush, 4, 12, 0, 0, 0, 0},
    {kPCrate, 15, 4, 0, kRowWood, 0, 0},  {kPBench, 18, 12, 0, kRowWood, 0, 0},
};
constexpr Exit kFieldExits[] = {
    {11, 3, 2, 1, 11, 4, Facing::Down, ExitKind::Door, "testort-raum", 0, Transition::Fade},
    {24, 9, 1, 2, 23, 9, Facing::Left, ExitKind::Edge, "testort-platz", 0, Transition::Fade},
};

constexpr const char* kRoomFloor[kTilesY] = {
    "_________________________", "-------------------------", "_________________________",
    "-------------------------", "_________________________", "-------------------------",
    "_________________________", "-------------------------", "_________________________",
    "-------------------------", "_________________________", "-------------------------",
    "_________________________", "-------------------------", "_________________________",
};
constexpr const char* kRoomWalls[kTilesY] = {
    "#########################", "#TTTTTTTTTTTTTTTTTTTTTTT#", "#WWWWWWWWWWWWWWWWWWWWWWW#",
    "#WWWWWWWWWWWWWWWWWWWWWWW#", "#SSSSSSSSSSSSSSSSSSSSSSS#", "#                       #",
    "#                       #", "#                       #", "#                       #",
    "#                       #", "#                       #", "#                       #",
    "#                       #", "#                       #", "##########  #############",
};
constexpr RoomPart kRoomParts[] = {
    {kPWindow, 6, 2, 0, kRowWood, 0, 0},   {kPWindow, 16, 2, 0, kRowWood, 0, 0},
    {kPPicture, 11, 2, 0, kRowWood, 0, 0}, {kPPicture, 13, 2, 0, kRowWood, 0, 0},
    {kPRug, 8, 8, 0, 4, 0, 0},             {kPWardrobe, 2, 5, 0, kRowWood, 0, 0},
    {kPBed, 21, 5, 0, 3, 0, 0},            {kPTable, 10, 8, 0, kRowWood, 0, 0},
    {kPChair, 9, 9, 0, kRowWood, 0, 0},    {kPChair, 12, 9, 0, kRowWood, 0, kPartFlipped},
    {kPLamp, 23, 12, 0, 3, 0, 0},          {kPPlant, 1, 12, 0, 1, 0, 0},
    {kPCrate, 3, 12, 0, kRowWood, 0, 0},   {kSignaturePart, 17, 12, 0, 6, 1, 0},
};
constexpr Exit kRoomExits[] = {
    {10, 14, 2, 1, 10, 13, Facing::Up, ExitKind::Door, "testort-wiese", 0, Transition::Fade},
};

constexpr const char* kSquareFloor[kTilesY] = {
    "hhhhhhhhhhhhhhhhhhhhhhhhh", "hhhhhhhhhhhhhhhhhhhhhhhhh", "HHHHHHHHHHHHHHHHHHHHHHHHH",
    "..ococococococococococo..", "..cocococococococococoo..", "..ococococococococococo..",
    "..cocococococococococoo..", "..ococococococococococo..", "..cocococ~~~~~cocococoo..",
    "==ocococo~~~~~ococococo..", "==cocococococococococoo..", "..ococococococococococo..",
    "..cocococococococococoo..", "..ococococococococococo..", "hhhhhhhhhhhhhhhhhhhhhhhhh",
};
constexpr const char* kSquareDeco[kTilesY] = {
    "", "", "", " f                     s ", "", "", "", "", "", "", "", "", "", " s                     f ", "",
};
constexpr RoomPart kSquareParts[] = {
    {kPBush, 4, 3, 0, 0, 0, 0},           {kPBush, 19, 3, 0, 0, 0, 0},
    {kPBush, 4, 11, 0, 0, 0, 0},          {kPBush, 19, 11, 0, 0, 0, 0},
    {kPBench, 8, 4, 0, kRowWood, 0, 0},   {kPBench, 15, 4, 0, kRowWood, 0, 0},
    {kPBench, 11, 11, 0, kRowWood, 0, 0}, {kPLamp, 12, 6, 0, 0, 0, 0},
    {kPPlant, 7, 7, 0, 1, 0, 0},          {kPPlant, 17, 7, 0, 5, 0, 0},
};
constexpr Exit kSquareExits[] = {
    {0, 9, 1, 2, 1, 9, Facing::Right, ExitKind::Edge, "testort-wiese", 1, Transition::Fade},
};

// Lichtkreise (seit 0.5.2): Wiese vor den Fenstern, Raum und Platz um die Lampe (Mitte am Fuß, grobe Bildpunkte)
// Seit 0.5.3 gehören die Fenster der Wiese zum Raum dahinter (leuchten nur, wenn dort Licht brennt)
constexpr const char* kRoomName = "testort-raum";
constexpr WindowLight kFieldWindows[] = {{{80, 37, 22, 9, 190}, kRoomName}, {{120, 37, 22, 9, 190}, kRoomName}};
constexpr LightCircle kRoomLights[] = {{188, 100, 58, 38, 255}};
constexpr LightCircle kSquareLights[] = {{100, 56, 64, 40, 255}};

constexpr RoomDef kRooms[3] = {
    {{kFieldFloor, kFieldWalls, kFieldDeco}, kLegend, {0, 0, 0}, kFieldParts, 8, kFieldExits, 2},
    {{kRoomFloor, kRoomWalls, nullptr}, kLegend, {0, kRowPaper, 0}, kRoomParts, 14, kRoomExits, 1},
    {{kSquareFloor, nullptr, kSquareDeco}, kLegend, {0, 0, 0}, kSquareParts, 10, kSquareExits, 1},
};

int center_x(int tx) { return (tx * kTileSize + kTileSize / 2) << 8; }
int center_y(int ty) { return (ty * kTileSize + kTileSize / 2) << 8; }

}  // namespace

void* TestPlace::operator new(size_t bytes) {
    void* p = asset_alloc(bytes);
    if (!p) p = std::malloc(bytes);  // asset_free gibt beides frei (Gerät: heap_caps_free, PC: free)
    if (!p) std::abort();
    return p;
}

void TestPlace::operator delete(void* p) { asset_free(p); }

const char* TestPlace::name() const {
    return kind_ == Kind::Field ? "testort-wiese" : kind_ == Kind::Room ? "testort-raum" : "testort-platz";
}

const char* TestPlace::title() const {
    return kind_ == Kind::Field ? "Testort Wiese" : kind_ == Kind::Room ? "Testort Raum" : "Testort Platz";
}

void TestPlace::build_palette() {
    PlacePalette& p = pal_;
    p = PlacePalette(3);
    // Grundfarben aus der Zeichenpalette (seit 0.5.5 assets/paletten/testorte.gpl, Paket „grund“)
    const Kit& k = kit();
    std::array<Color, kColors> c{};
    for (int i = 1; i < kColors && i < k.color_count; ++i)
        c[i] = {k.colors[i * 3], k.colors[i * 3 + 1], k.colors[i * 3 + 2]};
    // Schatten der Figuren je Ort (die Zeichenpalette enthält den der Wiese)
    if (kind_ != Kind::Field) c[kShadow] = kind_ == Kind::Room ? Color{100, 74, 56} : Color{118, 114, 108};
    // Grundstimmung, dann Licht des Ortes: Wiese kühl, Raum warm, Platz neutral (verschiedene Paletten)
    for (int i = 1; i < kColors; ++i) {
        const Color g = grade(c[i], mood_);
        p.set(i, kind_ == Kind::Field  ? tint(g, 246, 256, 268)
                 : kind_ == Kind::Room ? tint(g, 268, 248, 222)
                                       : tint(g, 258, 254, 246));
    }
    // Rollen: Lampe leuchtet bei Dunkelheit, Fenster zeigen innen den Himmel, außen am Abend Licht
    p.set_glow(kFlame, {255, 210, 128});
    p.set_glow(kFlame + 1, {255, 186, 96});
    p.set_glow(kGlow, {255, 244, 204});
    if (kind_ == Kind::Room) {
        p.set_sky(kSky, 2);
    } else if (kind_ == Kind::Field) {
        p.set_glow(kSky, {255, 214, 130});
        p.set_glow(kSky + 1, {246, 178, 92});
    }
    p.add_cycle({kWater, 4, 7});   // Wasser: alle 7 Schritte ein Platz weiter
    p.add_cycle({kFlame, 2, 5});   // Lampe flackert
    p.set_lamp({255, 190, 112});
    p.set_exposure(kind_ == Kind::Room ? 220 : 256);
    // Schatten durchscheinend: jede Farbe ein Stück Richtung Dunkelviolett (Farbtabelle)
    p.tint_table({24, 18, 34}, 96, shadow_);
    // Wetter (seit 0.5.3): Regen hellblau wie Glas, Schnee und Spritzer weiß, Blätter gelb und holzbraun; drinnen
    // nur in den Fenstern (Glas mit Himmelsverlauf)
    WeatherLook look;
    look.outdoor = outdoor();
    look.rain = kGlass;
    look.splash = kBloom;
    look.snow = kBloom;
    look.leaf[0] = kBloom + 1;
    look.leaf[1] = kWood + 1;
    if (kind_ == Kind::Room) {
        look.window_first = kSky;
        look.window_count = 2;
    }
    weather_.configure(look, p);
    apply_glow();
}

bool TestPlace::lights_on() const {
    if (!env_) return true;
    return kind_ == Kind::Field ? env_->lights.any_on(kRoomName) : env_->lights.on(name(), 0);
}

void TestPlace::apply_glow() {
    // Licht aus: Lampenschirm bzw. Fensterglas leuchtet nachts nicht
    const bool on = lights_on();
    if (kind_ == Kind::Field) {
        pal_.set_glow_on(kSky, on);
        pal_.set_glow_on(kSky + 1, on);
    } else {
        pal_.set_glow_on(kFlame, on);
        pal_.set_glow_on(kFlame + 1, on);
        pal_.set_glow_on(kGlow, on);
    }
}

void TestPlace::build_room() {
    const Kit& k = kit();
    if (!room_.init(&k.tileset, &k.catalog)) return;
    room_.load(kRooms[static_cast<int>(kind_)]);
    room_.set_shadow_color(kShadow);
    room_.set_shadow_table(shadow_);
    apply_lights();
    if (kind_ != Kind::Field) return;
    // Wiese: Gras im Schachbrett, dazu zufällig Büschel und Blüten (aus dem Startwert)
    TileMap& m = room_.tiles();
    for (int ty = 0; ty < kTilesY; ++ty)
        for (int tx = 0; tx < kTilesX; ++tx) {
            if (m.at(kFloorLayer, tx, ty) != kTGrassA) continue;
            const uint32_t r = rng_.below(100);
            m.set(kFloorLayer, tx, ty, r < 7 ? kTTuft : r < 10 ? kTBloom : ((tx + ty) & 1) ? kTGrassB : kTGrassA);
        }
    room_.invalidate();
}

bool TestPlace::random_free_tile(int* tx, int* ty) {
    for (int n = 0; n < 64; ++n) {
        const int x = static_cast<int>(rng_.below(kTilesX)), y = static_cast<int>(rng_.below(kTilesY));
        if (room_.walkable(x, y) && room_.exit_at(x, y) < 0) {
            *tx = x;
            *ty = y;
            return true;
        }
    }
    return false;
}

void TestPlace::spawn_at(Figure& f, int tx, int ty, Facing facing, uint8_t row, bool guide, uint16_t id) {
    f = Figure{};
    f.id = id ? id : next_id_++;
    f.cx = f.gx = static_cast<uint8_t>(tx);
    f.cy = f.gy = static_cast<uint8_t>(ty);
    f.x = center_x(tx);
    f.y = center_y(ty);
    f.wait = static_cast<uint16_t>(rng_.range(20, 120));
    f.speed = static_cast<uint8_t>(rng_.range(150, 210));
    f.row = row;
    f.guide = guide;
    f.use_part = f.use_k = f.exit = -1;
    f.facing = facing;
    f.anim.play(&kIdle);
}

void TestPlace::apply_lights() {
    // Lichtkreise der brennenden Quellen; die Fenster der Wiese nach dem Licht im Raum dahinter (environment.h)
    std::array<LightCircle, Room::kMaxLights> c{};
    int n = 0;
    auto add = [&](LightCircle l) {
        if (light_inner_ >= 0) l.inner = static_cast<uint8_t>(light_inner_);
        c[n++] = l;
    };
    if (kind_ == Kind::Field) {
        for (const WindowLight& w : kFieldWindows)
            if (!env_ || env_->lights.any_on(w.room)) add(w.circle);
    } else if (lights_on()) {
        add(kind_ == Kind::Room ? kRoomLights[0] : kSquareLights[0]);
    }
    room_.set_lights(c.data(), n, pal_.base(), pal_.levels(), light_style_);
    // Zustandsbild (seit 0.5.5): Lampe und Laterne aus bzw. an wie ihre Lichtquelle
    const int lamp = kind_ != Kind::Field && lights_on() ? 1 : 0;
    for (int i = 0; i < room_.part_count(); ++i)
        if (room_.part(i).part == kPLamp) room_.set_state(i, lamp);
    apply_glow();
    lights_version_ = env_ ? env_->lights.version() : 0;
}

void TestPlace::init(uint64_t seed) {
    kit();
    rng_.reseed(seed);
    seed_ = seed;
    enters_ = 0;
    ticks_ = 0;
    if (env_ && kind_ != Kind::Field) env_->lights.declare(name(), 1);  // Lampe bzw. Laterne
    build_palette();
    pal_.apply(daylight_.at(12 * 3600 * 1000000LL), 0, palette_);
    build_room();
    count_ = 0;
    out_count_ = 0;
    load_pending_ = false;
    next_id_ = static_cast<uint16_t>(1000 * (static_cast<int>(kind_) + 1));  // Wiese 1000 (Begleiterin), Raum 2000 …
    set_load(0);
    while (count_ < wanted_) {
        int tx = 0, ty = 0;
        if (!random_free_tile(&tx, &ty)) break;
        // Begleiterin: die erste Figur der Wiese
        spawn_at(figures_[count_], tx, ty, static_cast<Facing>(rng_.below(4)),
                 static_cast<uint8_t>(1 + count_ % kFigureRows), kind_ == Kind::Field && count_ == 0);
        ++count_;
    }
    load_pending_ = false;
}

void TestPlace::set_load(int figures) {
    wanted_ = figures <= 0 ? (kind_ == Kind::Field ? 6 : kind_ == Kind::Room ? 3 : 4)
                           : std::min(figures, kMaxFigures);
    load_pending_ = true;
}

void TestPlace::apply_load() {
    load_pending_ = false;
    while (count_ < wanted_) {
        int tx = 0, ty = 0;
        if (!random_free_tile(&tx, &ty)) return;
        spawn_at(figures_[count_], tx, ty, static_cast<Facing>(rng_.below(4)),
                 static_cast<uint8_t>(1 + count_ % kFigureRows), false);
        ++count_;
    }
    // zu viele: von hinten entfernen, die Begleiterin bleibt
    for (int i = count_ - 1; i >= 0 && count_ > wanted_; --i)
        if (!figures_[i].guide) remove_figure(i);
}

bool TestPlace::has_guide() const {
    for (int i = 0; i < count_; ++i)
        if (figures_[i].guide) return true;
    return false;
}

void TestPlace::remove_figure(int i) {
    for (int k = i + 1; k < count_; ++k) figures_[k - 1] = figures_[k];
    --count_;
}

bool TestPlace::use_taken(int part, int k, const Figure* self) const {
    for (int i = 0; i < count_; ++i) {
        const Figure& o = figures_[i];
        if (&o != self && o.goal == Goal::Use && o.use_part == part && o.use_k == k) return true;
    }
    return false;
}

void TestPlace::decide(Figure& f) {
    f.goal = Goal::None;
    f.use_part = f.use_k = f.exit = -1;
    const uint32_t r = rng_.below(100);
    const int exits = room_.exit_count();
    const bool leave = exits > 0 && (f.guide ? f.here > 600 && r < 50 : r < 5);
    int gx = f.cx, gy = f.cy;
    if (leave) {
        const int e = static_cast<int>(rng_.below(static_cast<uint32_t>(exits)));
        const Exit& x = room_.exit(e);
        gx = x.x + static_cast<int>(rng_.below(x.w));
        gy = x.y + static_cast<int>(rng_.below(x.h));
        f.goal = Goal::Exit;
        f.exit = static_cast<int8_t>(e);
    } else if (r < 55 && room_.part_count() > 0) {
        const int part = static_cast<int>(rng_.below(static_cast<uint32_t>(room_.part_count())));
        const PartStage* s = room_.look(part);  // im Bau: nur die Punkte des Bau-Bildes
        if (s && s->use_count > 0) {
            const int k = static_cast<int>(rng_.below(s->use_count));
            if (!use_taken(part, k, &f) && room_.use_point(part, k, &gx, &gy, nullptr)) {
                f.goal = Goal::Use;
                f.use_part = static_cast<int8_t>(part);
                f.use_k = static_cast<int8_t>(k);
            }
        }
    }
    if (f.goal == Goal::None && random_free_tile(&gx, &gy)) f.goal = Goal::Wander;
    if (f.goal == Goal::None) {
        f.wait = 30;
        return;
    }
    f.gx = static_cast<uint8_t>(gx);
    f.gy = static_cast<uint8_t>(gy);
    Facing d;
    if (!(gx == f.cx && gy == f.cy) && !room_.step_toward(f.cx, f.cy, gx, gy, &d)) {
        f.goal = Goal::None;  // kein Weg: später neu überlegen
        f.wait = static_cast<uint16_t>(rng_.range(20, 60));
    }
}

void TestPlace::reached(int i) {
    Figure& f = figures_[i];
    switch (f.goal) {
        case Goal::Exit: {
            if (out_count_ >= static_cast<int>(out_.size())) {
                f.wait = 10;  // Ausgang voll: kurz warten
                return;
            }
            const Exit& e = room_.exit(f.exit);
            Travel& t = out_[out_count_++];
            t = Travel{e.to, e.entry, static_cast<uint8_t>(f.exit), e.transition, f.guide, f.id, f.row};
            remove_figure(i);
            return;
        }
        case Goal::Use: {
            UsePoint u{};
            room_.use_point(f.use_part, f.use_k, nullptr, nullptr, &u);
            f.facing = u.facing;
            f.dz = u.dz;
            f.busy = true;
            f.wait = static_cast<uint16_t>(rng_.range(90, 300));
            return;
        }
        default:
            f.goal = Goal::None;
            f.wait = static_cast<uint16_t>(rng_.range(40, 200));
            return;
    }
}

void TestPlace::tick(const WorldClock& clock) {
    // Palette zur Tageszeit (Tageslicht, Lampen, Fenster, Himmel), Wetter und Farbrotation
    ticks_ = clock.ticks();
    if (env_ && env_->lights.version() != lights_version_) apply_lights();  // Licht an/aus, auch unsichtbar
    Ambient a = daylight_.at(clock.us_of_day());
    if (env_) env_->weather.apply(a, outdoor());
    pal_.apply(a, clock.ticks(), palette_);
    room_.set_light_on(a.lamp > 0);
    if (env_) weather_.tick(env_->weather);  // Partikel nur, solange der Ort betreten ist
    if (load_pending_) apply_load();
    room_.tick();
    for (int i = 0; i < count_;) {
        Figure& f = figures_[i];
        if (f.here < 0xFFFF) ++f.here;
        const int32_t tx = center_x(f.cx), ty = center_y(f.cy);
        if (f.x != tx || f.y != ty) {
            // unterwegs zur nächsten Kachel: immer nur eine Achse, so passen die vier Blickrichtungen
            const int32_t dx = tx - f.x, dy = ty - f.y;
            const int32_t s = f.speed;
            if (dx) f.x += dx > 0 ? std::min(s, dx) : std::max(-s, dx);
            if (dy) f.y += dy > 0 ? std::min(s, dy) : std::max(-s, dy);
            f.anim.play(&kWalk);
        } else if (f.goal != Goal::None && !f.busy && !(f.cx == f.gx && f.cy == f.gy)) {
            Facing d;
            if (room_.step_toward(f.cx, f.cy, f.gx, f.gy, &d)) {
                constexpr int8_t sx[4] = {0, 0, -1, 1}, sy[4] = {1, -1, 0, 0};
                f.cx = static_cast<uint8_t>(f.cx + sx[static_cast<int>(d)]);
                f.cy = static_cast<uint8_t>(f.cy + sy[static_cast<int>(d)]);
                f.facing = d;
                f.anim.play(&kWalk);
            } else {
                f.goal = Goal::None;  // Weg versperrt (z. B. nach einem Umbau)
                f.wait = 30;
            }
        } else if (f.goal != Goal::None && !f.busy) {
            const int before = count_;
            reached(i);
            if (count_ < before) continue;  // durch den Ausgang gegangen
            f.anim.play(&kIdle);
        } else {
            f.anim.play(&kIdle);
            if (f.wait > 0) --f.wait;
            if (f.wait == 40 && !f.busy && rng_.below(3) == 0) f.facing = static_cast<Facing>(rng_.below(4));
            if (f.wait == 0) {
                f.busy = false;
                f.dz = 0;
                decide(f);
            }
        }
        f.anim.tick();
        ++i;
    }
}

bool TestPlace::take_travel(Travel* out) {
    if (out_count_ == 0) return false;
    *out = out_[0];
    for (int k = 1; k < out_count_; ++k) out_[k - 1] = out_[k];
    --out_count_;
    return true;
}

void TestPlace::arrive(const Travel& t) {
    const bool guide = t.follow;
    const uint8_t row = static_cast<uint8_t>(std::clamp<int>(t.look, 1, kFigureRows));
    if (count_ >= kMaxFigures) {
        if (!guide) return;  // voll: wer ankommt, bleibt draußen (nur die Begleiterin verdrängt jemanden)
        for (int i = count_ - 1; i >= 0; --i)
            if (!figures_[i].guide) {
                remove_figure(i);
                break;
            }
        if (count_ >= kMaxFigures) return;
    }
    int tx = 0, ty = 0;
    Facing facing = Facing::Down;
    if (t.entry < room_.exit_count()) {
        const Exit& e = room_.exit(t.entry);
        tx = e.ax;
        ty = e.ay;
        facing = e.facing;
    } else if (!random_free_tile(&tx, &ty)) {
        return;
    }
    Figure& f = figures_[count_++];
    spawn_at(f, tx, ty, facing, row, guide, t.who);
    f.wait = static_cast<uint16_t>(rng_.range(10, 40));
}

void TestPlace::enter() {
    resolve(kit());  // Bilder der eben geladenen Pakete (assets.h)
    room_.enter();
    // eigener Startwert je Betreten: deterministisch, ändert den Zufall der Figuren nicht
    if (env_) weather_.enter(env_->weather, seed_ ^ (0xA5A5ULL + enters_++) * 0x9E3779B97F4A7C15ULL);
}

int TestPlace::packs(const char** out, int max) const {
    // gemeinsame Möbel, dazu das eigene Paket (der Platz kommt mit den gemeinsamen aus); „grund“ ist immer geladen
    const char* own = kind_ == Kind::Field ? "wiese" : kind_ == Kind::Room ? "raum" : nullptr;
    int n = 0;
    if (n < max) out[n++] = "moebel";
    if (own && n < max) out[n++] = own;
    return n;
}

int TestPlace::neighbors(const char** out, int max) const {
    int n = 0;
    for (int i = 0; i < room_.exit_count() && n < max; ++i) out[n++] = room_.exit(i).to;
    return n;
}

void TestPlace::leave() {
    room_.leave();
    weather_.leave();
}

bool TestPlace::command(const char* cmd) {
    if (!cmd) return false;
    int n = 0;
    char word[12] = {};
    if (std::sscanf(cmd, "innenlicht %11s", word) == 1) {
        const std::string_view w(word);
        if (!env_ || (w != "an" && w != "aus" && w != "um")) return false;
        env_->lights.set_all(kRoomName, w == "um" ? !env_->lights.any_on(kRoomName) : w == "an");
        return true;
    }
    if (std::sscanf(cmd, "regen %11s", word) == 1) {
        const std::string_view w(word);
        if (w != "striche" && w != "tropfen" && w != "fein") return false;
        weather_.set_rain_style(w == "striche" ? RainStyle::Streaks
                                : w == "tropfen" ? RainStyle::Drops
                                                 : RainStyle::Fine);
        return true;
    }
    if (std::sscanf(cmd, "schnee %11s", word) == 1) {
        const std::string_view w(word);
        if (w != "gemischt" && w != "klein" && w != "gross") return false;
        weather_.set_snow_style(w == "gemischt" ? SnowStyle::Mixed
                                : w == "klein"  ? SnowStyle::Small
                                                : SnowStyle::Large);
        return true;
    }
    if (std::sscanf(cmd, "dichte %d", &n) == 1) {
        weather_.set_density(n);
        return true;
    }
    if (std::sscanf(cmd, "nebelstil %11s", word) == 1) {
        const std::string_view w(word);
        if (w != "a" && w != "b" && w != "c") return false;
        weather_.set_fog_style(w == "a" ? FogStyle::Drift : w == "b" ? FogStyle::Layers : FogStyle::Billow);
        return true;
    }
    if (std::sscanf(cmd, "nebelbaenke %d", &n) == 1) {
        weather_.set_fog_scale(n);
        return true;
    }
    if (std::sscanf(cmd, "stimmung %d", &n) == 1) {
        mood_ = std::clamp(n, 0, 2);
        build_palette();
        room_.invalidate();
        return true;
    }
    if (std::sscanf(cmd, "tageslicht %d", &n) == 1) {
        const int floor = daylight_.floor();
        daylight_ = daylight_preset(std::clamp(n, 0, kDaylightPresets - 1));
        daylight_.set_floor(floor);
        return true;
    }
    if (std::sscanf(cmd, "nacht %d", &n) == 1) {
        daylight_.set_floor(std::clamp(n, 0, 256));
        return true;
    }
    // „licht fein|grob|hart [innen 0–100 %]“
    char style[8] = {};
    if (std::sscanf(cmd, "licht %7s %d", style, &n) >= 1) {
        const std::string_view st(style);
        if (st != "fein" && st != "grob" && st != "hart") return false;
        light_style_ = st == "fein" ? LightStyle::Fine : st == "grob" ? LightStyle::Coarse : LightStyle::Stepped;
        light_inner_ = std::sscanf(cmd, "licht %*s %d", &n) == 1 ? std::clamp(n, 0, 100) * 255 / 100 : -1;
        apply_lights();
        return true;
    }
    if (std::strcmp(cmd, "ausbau") == 0) {
        for (int i = 0; i < room_.part_count(); ++i) {
            const int n = room_.stage_count(i);
            if (n > 1) room_.set_stage(i, (room_.part(i).stage + 1) % n);
        }
        return true;
    }
    if (std::strcmp(cmd, "baustelle") == 0) {
        // Möbel (Bett, Tisch, Stühle, Schrank, Kisten, Bänke) wechseln zwischen fertig und „im Bau“
        bool any = false, on = true;
        for (int pass = 0; pass < 2; ++pass)
            for (int i = 0; i < room_.part_count(); ++i) {
                const PartDef* d = room_.def(i);
                if (!d || d->layer != PartLayer::Standing || d->kind == PartKind::Deco || d->kind == PartKind::Light)
                    continue;
                if (pass == 0) {
                    any = true;
                    if (room_.building(i)) on = false;  // schon im Bau: alles wieder fertig
                } else {
                    room_.set_building(i, on);
                }
            }
        // wer gerade ein Teil benutzt, das nun im Bau ist, steht auf
        for (int i = 0; i < count_; ++i)
            if (figures_[i].goal == Goal::Use) {
                figures_[i].goal = Goal::None;
                figures_[i].busy = false;
                figures_[i].dz = 0;
                figures_[i].wait = 1;
            }
        return any;
    }
    return false;
}

void TestPlace::draw(Canvas& c, bool with_background, CoarseLayers* layers) const {
    if (layers) layers->base = nullptr;
    if (!room_.ready()) {
        c.clear(0);
        return;
    }
    const Kit& k = kit();
    DepthList& list = room_.begin(c, with_background, layers);
    const SpriteSheet& sheet = k.figure;
    for (int i = 0; sheet.pixels && i < count_; ++i) {
        const Figure& f = figures_[i];
        bool flip = false;
        const Sprite fr = figure_frame(f, &flip);
        list.add(fr, f.x >> 8, f.y >> 8, sheet.pivot_x, sheet.pivot_y, flip, k.rows[f.row].table(), f.dz, 5);
    }
    list.draw(c);
}

void TestPlace::render(Canvas& c) const { draw(c, true); }

void TestPlace::render_layers(Canvas& work, CoarseLayers& out) const {
    draw(work, true, &out);
    if (env_) weather_.layers(out, env_->weather, ticks_);
}

void TestPlace::render_fine(Canvas& fine, const CoarseLayers& layers, const uint8_t* remap) const {
    weather_.draw(fine, layers, remap);
}

void TestPlace::render_overlay(Canvas& fine) const {
    if (inspect_) room_.draw_inspect(fine, ui::kTextDim, ui::kPrimary, ui::kSuccess, ui::kWarning);
    // Der Ortsname steht seit 0.5.6 nicht mehr dauerhaft im Bild: die Anzeige blendet ihn nach jedem Wechsel 3 s ein
    // Kein Dauer-Pfeil über der Begleiterin mehr (Wunsch des Users 2026-10-22): einen Pfeil zeigt nur die offene
    // Karte über ihrer Figur (overlay.h)
}

Sprite TestPlace::figure_frame(const Figure& f, bool* flip) const {
    const View v = view_of(f.facing);
    *flip = v.flip;
    return kit().figure.frame(v.index * kPoses + f.anim.frame());
}

const TestPlace::Figure* TestPlace::figure_by_id(uint16_t id) const {
    for (int i = 0; i < count_; ++i)
        if (figures_[i].id == id) return &figures_[i];
    return nullptr;
}

Hit TestPlace::hit(int fx, int fy) const {
    Hit best;
    if (!room_.ready()) return best;
    const int x = fx / kCoarseScale, y = fy / kCoarseScale;
    int depth = 0;
    const int part = room_.hit_part(x, y, &depth);
    if (part >= 0) {
        best.kind = Hit::Kind::Part;
        best.id = static_cast<uint16_t>(part);
        best.depth = static_cast<int16_t>(depth);
        const Rect r = room_.part_rect(part);
        best.x = static_cast<int16_t>((r.x + r.w / 2) * kCoarseScale);
        best.y = static_cast<int16_t>((r.y + r.h) * kCoarseScale);
    }
    // Figuren nach den Teilen: bei gleicher Tiefe liegt die Figur vorn (wie beim Zeichnen)
    const SpriteSheet& sheet = kit().figure;
    for (int i = 0; sheet.pixels && i < count_; ++i) {
        const Figure& f = figures_[i];
        bool flip = false;
        const Sprite s = figure_frame(f, &flip);
        const int footx = f.x >> 8, footy = f.y >> 8;
        const int sx = x - (footx - (flip ? s.width - 1 - sheet.pivot_x : sheet.pivot_x));
        const int sy = y - (footy - sheet.pivot_y);
        if (sx < 0 || sy < 0 || sx >= s.width || sy >= s.height) continue;
        if (!s.pixels[sy * s.width + (flip ? s.width - 1 - sx : sx)]) continue;  // durchsichtig
        const int d = footy + f.dz;
        if (best.kind != Hit::Kind::None && d < best.depth) continue;
        best.kind = Hit::Kind::Figure;
        best.id = f.id;
        best.depth = static_cast<int16_t>(d);
        best.x = static_cast<int16_t>(footx * kCoarseScale);
        best.y = static_cast<int16_t>(footy * kCoarseScale);
        best.h = static_cast<int16_t>(s.height * kCoarseScale);
    }
    // Ausgänge ohne Tür-Bild (Türschwelle, Weg am Bildrand): eine Kachel Spielraum rundherum, damit der Finger trifft
    if (best.kind == Hit::Kind::None) {
        const int tx = x / kTileSize, ty = y / kTileSize;
        for (int e = 0; e < room_.exit_count(); ++e) {
            const Exit& ex = room_.exit(e);
            if (tx < ex.x - 1 || tx > ex.x + ex.w || ty < ex.y - 1 || ty > ex.y + ex.h) continue;
            best.kind = Hit::Kind::Exit;
            best.id = static_cast<uint16_t>(e);
            best.x = static_cast<int16_t>((ex.x * kTileSize + ex.w * kTileSize / 2) * kCoarseScale);
            best.y = static_cast<int16_t>((ex.y + ex.h) * kTileSize * kCoarseScale);
            break;
        }
    }
    return best;
}

namespace {
void figure_name(const TestPlace::FigureView& v, char* out, size_t n) {
    if (v.guide) std::snprintf(out, n, "Begleiterin");
    else std::snprintf(out, n, "Testfigur %u", static_cast<unsigned>(v.id / 1000 * 100 + v.id % 1000));
}

// Testgedanken, bis die Simulation ab 0.6.x echte Gründe liefert
constexpr const char* kThoughts[] = {
    "Ob es heute noch regnet? Die Wolken über der Hecke sehen schwer aus.",
    "Erst einmal hinsetzen. Der Weg war weiter als gedacht.",
    "Hier riecht es nach frischem Holz.",
    "Wer wohl in dem Haus mit der grünen Tür wohnt?",
    "Ich sollte mir die Bank am Platz genauer ansehen.",
    "Testgedanke: Hier steht später, was die Figur wirklich denkt und warum.",
};
constexpr const char* kPartNames[] = {"Dekoration", "Sitzplatz", "Bett",  "Tisch",  "Schrank",
                                      "Werkbank",   "Lampe",     "Tür",   "Fenster"};
}  // namespace

bool TestPlace::describe(const Hit& hit, ui::Card* card) const {
    using Kind = ui::Card::Kind;
    if (hit.kind == Hit::Kind::Figure) {
        const Figure* f = figure_by_id(hit.id);
        if (!f) return false;
        card->kind = Kind::Figure;
        card->figure = f->id;
        char name[24];
        figure_name({f->id, f->guide}, name, sizeof(name));
        card->title = name;
        const bool walking = f->x != center_x(f->cx) || f->y != center_y(f->cy);
        card->subtitle = f->busy                  ? "sitzt und ruht"
                         : f->goal == Goal::Exit  ? "geht hinaus"
                         : walking                ? "spaziert herum"
                                                  : "steht und schaut";
        card->where = title();
        card->quote = std::string("„") + kThoughts[f->id % (sizeof(kThoughts) / sizeof(kThoughts[0]))] + "“";
        card->hint = "Folgen";
        card->hint_symbol = ui::Symbol::Follow;
        card->picture.sprite = kit().figure.frame(view_of(Facing::Down).index * kPoses);
        card->picture.remap = kit().rows[f->row].table();
        card->picture.scale = 2;
        card->picture.light_base = static_cast<uint8_t>(pal_.base());  // hellste Lichtstufe: auch nachts erkennbar
        card->picture.light_add = static_cast<uint8_t>(pal_.base() * (pal_.levels() - 1));
        figure_mark(f->id, &card->mark_x, &card->mark_y, &card->mark_h);
        return true;
    }
    if (hit.kind == Hit::Kind::Exit) {
        if (hit.id >= room_.exit_count()) return false;
        const Exit& e = room_.exit(hit.id);
        const bool door = e.kind == ExitKind::Door;
        card->kind = Kind::Building;
        card->title = door ? "Tür" : "Weg";
        card->place = e.to;  // die Bühne setzt das Ziel („→ …“) und wer dort ist
        card->hint = door ? "Betreten" : "Hingehen";
        card->hint_symbol = door ? ui::Symbol::House : ui::Symbol::Place;
        return true;
    }
    if (hit.kind == Hit::Kind::Part) {
        const PartDef* d = room_.def(hit.id);
        const PartStage* l = room_.look(hit.id);
        if (!d) return false;
        card->kind = d->kind == PartKind::Door ? Kind::Building : Kind::Exhibit;
        card->title = kPartNames[static_cast<int>(d->kind)];
        card->subtitle = room_.building(hit.id) ? "im Bau" : "";
        card->where = title();
        if (l && l->sheet.pixels) {
            card->picture.sprite = l->sheet.frame(room_.part(hit.id).state, 0);
            card->picture.scale = l->sheet.width <= 24 && l->sheet.height <= 24 ? 3 : 2;
            card->picture.light_base = static_cast<uint8_t>(pal_.base());
            card->picture.light_add = static_cast<uint8_t>(pal_.base() * (pal_.levels() - 1));
            const uint8_t row = room_.part(hit.id).row;
            card->picture.remap = row > 0 && row < kRowCount ? kit().rows[row].table() : nullptr;
        }
        card->hint = "Tippen: schließen";
        if (d->kind == PartKind::Door) {
            // Ort hinter der Tür: nächster Ausgang (die Bühne ergänzt, wer drin ist)
            const Rect r = room_.part_rect(hit.id);
            int best = -1, dist = 1 << 30;
            for (int e = 0; e < room_.exit_count(); ++e) {
                const Exit& x = room_.exit(e);
                const int dx = x.x * kTileSize - r.x, dy = x.y * kTileSize - r.y;
                if (dx * dx + dy * dy < dist) {
                    dist = dx * dx + dy * dy;
                    best = e;
                }
            }
            if (best >= 0) card->place = room_.exit(best).to;
            card->hint = "Betreten";
            card->hint_symbol = ui::Symbol::House;
        } else {
            card->quote = "Testkarte: Hier stehen später Name, Herkunft und Angaben (Museum ab 0.7.x).";
        }
        return true;
    }
    return false;
}

int TestPlace::figures(FigureInfo* out, int max) const {
    int n = 0;
    for (int pass = 0; pass < 2; ++pass)  // Begleiterin zuerst
        for (int i = 0; i < count_ && n < max; ++i) {
            const Figure& f = figures_[i];
            if (f.guide != (pass == 0)) continue;
            out[n].id = f.id;
            out[n].lead = f.guide;
            figure_name({f.id, f.guide}, out[n].name, sizeof(out[n].name));
            ++n;
        }
    return n;
}

bool TestPlace::has_figure(uint16_t id) const { return figure_by_id(id) != nullptr; }

bool TestPlace::figure_mark(uint16_t id, int* x, int* y, int* h) const {
    const Figure* f = figure_by_id(id);
    if (!f) return false;
    *x = (f->x >> 8) * kCoarseScale;
    *y = (f->y >> 8) * kCoarseScale;
    *h = (kit().figure.pixels ? kit().figure.pivot_y : 28) * kCoarseScale;
    return true;
}

}  // namespace pixelia
