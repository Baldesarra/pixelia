#include "pixelia/engine/test_places.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <new>
#include <string_view>

#include "pixelia/engine/font.h"
#include "pixelia/engine/memory.h"
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

constexpr int kFigW = 16, kFigH = 32, kPoses = 5;  // Posen: stehen, Schritt, Mitte, Schritt, atmen

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

// Maße je Teil-Stufe: Bild w × h, davon Vorderseite z (Rest = Oberseite), Stellfläche fw × fh, Bilder
struct StageSpec {
    uint8_t part, w, h, z, fw, fh, frames;
    const UsePoint* uses;
    uint8_t use_count;
};
constexpr StageSpec kStages[] = {
    {kPDoor, 16, 24, 0, 2, 3, 1, nullptr, 0},
    {kPWindow, 16, 16, 0, 2, 2, 1, kUseWindow, 2},
    {kPPicture, 8, 8, 0, 1, 1, 1, nullptr, 0},
    {kPWardrobe, 16, 32, 24, 2, 1, 1, kUseWardrobe, 2},
    {kPBed, 16, 27, 3, 2, 3, 1, kUseBed, 1},  // Stufe 0: Lager am Boden
    {kPBed, 16, 32, 6, 2, 3, 1, kUseBed, 1},  // Stufe 1: Bett mit Kopfteil
    {kPTable, 16, 22, 10, 2, 2, 1, kUseTable, 2},  // Stufe 0: zwei Kisten
    {kPTable, 16, 24, 8, 2, 2, 1, kUseTable, 2},   // Stufe 1: Tisch
    {kPChair, 8, 14, 6, 1, 1, 1, kUseChair, 1},
    {kPCrate, 16, 16, 8, 2, 1, 1, kUseCrate, 1},
    {kPRug, 32, 24, 0, 4, 3, 1, nullptr, 0},
    {kPLamp, 8, 20, 12, 1, 1, 2, nullptr, 0},
    {kPPlant, 8, 14, 5, 1, 1, 1, nullptr, 0},
    {kPToken, 8, 12, 5, 1, 1, 1, kUseToken, 1},
    {kPBush, 16, 18, 6, 2, 2, 1, nullptr, 0},
    {kPBench, 16, 12, 5, 2, 1, 1, kUseBench, 2},
    {kPBedFrame, 16, 30, 5, 2, 3, 1, kUseBuild, 1},  // im Bau: Rahmen ohne Matratze
    {kPSite, 16, 11, 5, 1, 1, 1, nullptr, 0},        // Materialstapel: Bretter
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

constexpr int kPoolBytes = 6144;
constexpr int pool_needed() {
    int n = 0;
    for (const StageSpec& s : kStages) n += s.w * s.h * s.frames;
    return n;
}
static_assert(pool_needed() <= kPoolBytes, "Bilder der Teile passen in den Vorrat");

// Gemeinsame Grafiken und Katalog aller Testorte (einmal beim ersten init() gezeichnet, ~16 KB Grafik-Speicher)
struct Kit {
    uint8_t figure[3 * kPoses * kFigW * kFigH];
    uint8_t tiles[kTileCount][kTilePixels];
    uint8_t tile_flags[kTileCount];
    uint8_t pool[kPoolBytes];
    PartStage stages[kStageCount];
    PartDef parts[kPartCount];
    const PartStage* site;
    Recolor rows[kRowCount];
    TileSet tileset;
    PartCatalog catalog;
};
Kit* s_kit = nullptr;

struct Paint {
    uint8_t* px;
    int w, h;
    void dot(int x, int y, uint8_t c) {
        if (x >= 0 && y >= 0 && x < w && y < h) px[y * w + x] = c;
    }
    void rect(int x, int y, int rw, int rh, uint8_t c) {
        for (int yy = y; yy < y + rh; ++yy)
            for (int xx = x; xx < x + rw; ++xx) dot(xx, yy, c);
    }
    void frame(int x, int y, int rw, int rh, uint8_t c) {
        rect(x, y, rw, 1, c);
        rect(x, y + rh - 1, rw, 1, c);
        rect(x, y, 1, rh, c);
        rect(x + rw - 1, y, 1, rh, c);
    }
    // Umriss: jeder durchsichtige Punkt neben einem gezeichneten wird dunkel (ohne Hilfspuffer: erst markieren)
    void outline(uint8_t c) {
        constexpr uint8_t kMark = 255;
        auto solid = [&](int x, int y) {
            if (x < 0 || y < 0 || x >= w || y >= h) return false;
            const uint8_t v = px[y * w + x];
            return v != 0 && v != kMark;
        };
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (!px[y * w + x] && (solid(x - 1, y) || solid(x + 1, y) || solid(x, y - 1) || solid(x, y + 1)))
                    px[y * w + x] = kMark;
        for (int i = 0; i < w * h; ++i)
            if (px[i] == kMark) px[i] = c;
    }
};

// Kopf 12 × 13, abgerundet (Einzug je Zeile)
constexpr int8_t kHeadInset[13] = {2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2};

// view: 0 vorn, 1 hinten, 2 Seite (Blick nach rechts); Fußpunkt (8, 30), 28 Bildpunkte hoch
void paint_figure(uint8_t* px, int view, int pose) {
    Paint p{px, kFigW, kFigH};
    std::memset(px, 0, kFigW * kFigH);
    const int lift = pose == 2 ? -1 : 0;  // Mitte des Schritts: Körper eine Zeile höher
    const int drop = pose == 4 ? 1 : 0;   // Atmen: Kopf sinkt eine Zeile

    const int top = 27 + lift;
    if (view == 2) {
        int front = 6, back = 6;
        if (pose == 1) front = 8, back = 4;
        if (pose == 3) front = 4, back = 8;
        p.rect(back, top, 3, 31 - top, kHair + 2);
        p.rect(front, top, 3, 31 - top, kHair + 1);
        p.rect(front, 30, 3, 1, kHair + 2);
    } else {
        const int l_end = pose == 1 ? 29 : 30, r_end = pose == 3 ? 29 : 30;
        p.rect(4, top, 3, l_end - top + 1, kHair + 1);
        p.rect(9, top, 3, r_end - top + 1, kHair + 1);
        p.rect(4, l_end, 3, 1, kHair + 2);
        p.rect(9, r_end, 3, 1, kHair + 2);
    }

    // Körper, Zeilen 16–26, in der Platzhalter-Rampe (hell links, dunkel rechts, Saum unten)
    const int by = 16 + lift;
    if (view == 2) {
        p.rect(5, by, 6, 1, kRamp + 1);
        p.rect(4, by + 1, 8, 10, kRamp + 1);
        p.rect(4, by + 1, 2, 10, kRamp);
        p.rect(10, by + 1, 2, 10, kRamp + 2);
        p.rect(4, by + 10, 8, 1, kRamp + 3);
        const int ax = pose == 1 ? 8 : pose == 3 ? 6 : 7;  // Arm schwingt mit
        p.rect(ax, by + 1, 2, 7, kRamp + 2);
        p.rect(ax, by + 8, 2, 1, kSkin + 1);
    } else {
        p.rect(4, by, 8, 1, kRamp + 1);
        p.rect(3, by + 1, 10, 10, kRamp + 1);
        p.rect(3, by + 1, 2, 10, kRamp);
        p.rect(11, by + 1, 2, 10, kRamp + 2);
        p.rect(3, by + 10, 10, 1, kRamp + 3);
        const int swing = pose == 1 ? -1 : pose == 3 ? 1 : 0;
        p.rect(2, by + 1, 1, 7 + swing, kRamp + 2);
        p.dot(2, by + 8 + swing, kSkin + 1);
        p.rect(13, by + 1, 1, 7 - swing, kRamp + 2);
        p.dot(13, by + 8 - swing, kSkin + 1);
    }

    // Kopf, Zeilen 3–15 (≈ 45 % der Figur)
    const int hy = 3 + lift + drop;
    auto head_row = [&](int r, uint8_t c) { p.rect(2 + kHeadInset[r], hy + r, 12 - 2 * kHeadInset[r], 1, c); };
    for (int r = 0; r < 13; ++r) head_row(r, kSkin);
    for (int r = 2; r < 11; ++r) p.dot(13, hy + r, kSkin + 1);
    p.rect(4, hy + 12, 8, 1, kSkin + 1);
    if (view == 0) {
        for (int r = 0; r < 4; ++r) head_row(r, kHair);
        p.rect(2, hy + 4, 2, 4, kHair);
        p.rect(12, hy + 4, 2, 4, kHair + 1);
        p.rect(5, hy + 4, 2, 1, kHair);
        p.rect(5, hy + 7, 1, 2, kEye);
        p.rect(10, hy + 7, 1, 2, kEye);
    } else if (view == 1) {
        for (int r = 0; r < 12; ++r) head_row(r, kHair);
        for (int r = 3; r < 12; ++r) p.dot(13, hy + r, kHair + 1);
        head_row(12, kHair + 1);
    } else {
        for (int r = 0; r < 4; ++r) head_row(r, kHair);
        p.rect(2, hy + 4, 5, 7, kHair);
        p.rect(3, hy + 11, 3, 1, kHair + 1);
        p.rect(11, hy + 7, 1, 2, kEye);
    }
    p.outline(kOutline);
}

// Teil-Stufe s, Bild f in der Draufsicht von schräg oben: Oberseite hell, Vorderseite dunkler
void paint_stage(uint8_t* px, int s, int f) {
    const StageSpec& sp = kStages[s];
    Paint p{px, sp.w, sp.h};
    std::memset(px, 0, static_cast<size_t>(sp.w) * sp.h);
    const int W = sp.w, H = sp.h, top = sp.h - sp.z;
    const bool flat = sp.part < kPartCount && kParts[sp.part].layer != PartLayer::Standing;
    const uint8_t r0 = kRamp, r1 = kRamp + 1, r2 = kRamp + 2, r3 = kRamp + 3;
    switch (sp.part) {
        case kPDoor:
            p.rect(0, 0, W, H, r3);
            p.rect(2, 2, W - 4, H - 2, r1);
            p.rect(2, 2, 1, H - 2, r0);
            p.frame(4, 4, 8, 7, r2);
            p.frame(4, 13, 8, 9, r2);
            p.dot(12, 13, r0);
            break;
        case kPWindow:
            p.rect(0, 0, W, H, r2);
            // Glas: Verlauf aus zwei Farben im 4×4-Raster (oben kSky, unten kSky + 1)
            for (int y = 2; y < H - 3; ++y)
                for (int x = 2; x < W - 2; ++x)
                    p.dot(x, y, dither4(x, y, (y - 2) * 16 / (H - 6)) ? kSky + 1 : kSky);
            p.rect(7, 2, 2, H - 5, r2);
            p.rect(2, 7, W - 4, 1, r2);
            p.dot(3, 3, kBloom);
            p.dot(4, 3, kBloom);
            p.rect(0, H - 3, W, 3, r1);
            p.rect(0, H - 1, W, 1, r3);
            break;
        case kPPicture:
            p.frame(0, 0, W, H, r3);
            p.rect(1, 1, W - 2, H - 2, kGlass);
            p.rect(1, 5, W - 2, 2, kGrass + 1);
            p.dot(5, 2, kBloom + 1);
            break;
        case kPWardrobe:
            p.rect(0, 0, W, top, r0);
            p.rect(0, top, W, sp.z, r1);
            p.rect(7, top + 1, 2, sp.z - 3, r2);
            p.dot(5, top + 11, r3);
            p.dot(10, top + 11, r3);
            p.rect(0, H - 2, W, 2, r3);
            break;
        case kPBed:
            if (H == 27) {  // Lager: flacher Sack mit Nähten
                p.rect(1, 1, W - 2, top - 1, r0);
                for (int y = 6; y < top; y += 6) p.rect(2, y, W - 4, 1, r1);
                p.rect(1, top, W - 2, sp.z, r2);
            } else {        // Bett: Kopfteil, Kissen, Decke, Füße
                p.rect(0, 0, W, 7, r3);
                p.rect(1, 1, W - 2, 2, r2);
                p.rect(2, 7, W - 4, 5, kBloom);
                p.rect(1, 12, W - 2, top - 12, r0);
                p.rect(1, 12, W - 2, 1, r1);
                p.rect(0, top, W, sp.z - 2, r1);
                p.rect(0, H - 2, 2, 2, r3);
                p.rect(W - 2, H - 2, 2, 2, r3);
            }
            break;
        case kPTable:
            if (H == 22) {  // zwei Kisten
                for (int b = 0; b < 2; ++b) {
                    const int x = b * 8, y = b ? 8 : 6;
                    p.rect(x, y, 8, 5, r0);
                    p.rect(x, y + 5, 8, H - y - 5, r1);
                    p.rect(x + 3, y + 6, 1, H - y - 7, r2);
                    p.rect(x, H - 1, 8, 1, r3);
                }
            } else {        // Tisch: Platte, Zarge, Beine
                p.rect(0, 0, W, top, r0);
                p.rect(0, top - 1, W, 1, r1);
                p.rect(0, top, W, 3, r2);
                p.rect(1, top + 3, 2, sp.z - 3, r3);
                p.rect(W - 3, top + 3, 2, sp.z - 3, r3);
            }
            break;
        case kPChair:
            p.rect(1, 0, 6, 7, r1);
            p.rect(2, 1, 4, 1, r0);
            p.rect(0, 7, 8, 3, r0);
            p.rect(1, 10, 1, 4, r3);
            p.rect(6, 10, 1, 4, r3);
            break;
        case kPCrate:
            p.rect(1, 1, 14, 6, r0);
            p.rect(1, 7, 14, 7, r1);
            p.rect(1, 7, 14, 1, r2);
            p.rect(5, 8, 1, 6, r2);
            p.rect(10, 8, 1, 6, r2);
            p.rect(1, 14, 14, 1, r3);
            break;
        case kPRug:
            p.rect(0, 0, W, H, r1);
            p.frame(2, 2, W - 4, H - 4, r0);
            for (int i = 0; i < 4; ++i) p.rect(10 + i, 8 + i, 12 - 2 * i, 8 - 2 * i, i & 1 ? r2 : r0);
            break;
        case kPLamp:
            p.rect(3, 7, 2, 11, r3);
            p.rect(1, 18, 6, 2, r2);
            p.rect(2, 1, 4, 1, r1);
            // Schirm leuchtet bei Dunkelheit (Rolle „leuchtend“), Kern flackert zwischen den Bildern
            p.rect(1, 2, 6, 5, kFlame);
            for (int x = 1; x < 7; x += 2) p.dot(x + f, 3, kFlame + 1);
            p.rect(2 + f, 4, 3, 2, kGlow);
            p.rect(1, 6, 6, 1, r2);
            break;
        case kPPlant:
            p.rect(1, 9, 6, 5, r1);
            p.rect(1, 9, 6, 1, r0);
            p.rect(5, 10, 2, 4, r2);
            p.rect(2, 2, 4, 7, kGrass + 1);
            p.rect(0, 4, 3, 3, kGrass);
            p.rect(5, 3, 3, 4, kGrass + 2);
            p.dot(3, 1, kGrass);
            break;
        case kPToken:  // Sockel mit Raute: Platzhalter für ein Erkennungsstück
            p.rect(1, 7, 6, 5, r2);
            p.rect(1, 7, 6, 1, r1);
            for (int y = 0; y < 7; ++y) {
                const int hw = y < 3 ? y + 1 : 6 - y;
                p.rect(4 - hw, y, 2 * hw, 1, y < 3 ? r0 : r1);
            }
            break;
        case kPBush:  // runder Busch: Kreis über der Stellfläche, oben hell, unten dunkel
            for (int y = 0; y < 16; ++y)
                for (int x = 0; x < W; ++x) {
                    const int dx = 2 * x - 15, dy = 2 * y - 15;
                    if (dx * dx + dy * dy < 15 * 15 + 12) p.dot(x, y, dy + dx / 2 < -6 ? kGrass : dy > 6 ? kGrass + 3 : kGrass + 1);
                }
            p.rect(6, 15, 4, 3, kHair + 1);
            p.dot(5, 5, kTuft);
            p.dot(9, 9, kTuft);
            p.dot(11, 4, kTuft);
            break;
        case kPBedFrame:  // Kopfteil und Seitenbretter, dazwischen Boden sichtbar
            p.rect(0, 0, W, 7, r3);
            p.rect(1, 1, W - 2, 2, r2);
            p.rect(0, 7, 2, top - 7, r2);
            p.rect(W - 2, 7, 2, top - 7, r2);
            p.rect(2, top - 2, W - 4, 2, r1);
            p.rect(0, top, W, sp.z - 2, r1);
            p.rect(0, H - 2, 2, 2, r3);
            p.rect(W - 2, H - 2, 2, 2, r3);
            break;
        case kPSite:  // Bretterstapel in Holzfarben (ohne Farbreihe), ein Brett schräg angelehnt
            for (int b = 0; b < 3; ++b) {
                const int y = 4 + b * 2;
                p.rect(1 + b, y, W - 2 - 2 * b, 2, b & 1 ? kWood + 1 : kWood);
                p.rect(1 + b, y + 1, W - 2 - 2 * b, 1, kWood + 2);
            }
            p.rect(1, H - 2, W - 2, 2, kWood + 3);
            for (int i = 0; i < 5; ++i) p.rect(W - 4 - i, i, 2, 1, kWood + 1);
            break;
        case kPBench:
            p.rect(0, 0, W, 2, r1);
            p.rect(0, 3, W, 1, r1);
            p.rect(0, 5, W, 3, r0);
            p.rect(0, 7, W, 1, r2);
            p.rect(1, 8, 2, 4, r3);
            p.rect(W - 3, 8, 2, 4, r3);
            break;
        default:
            break;
    }
    if (!flat) p.outline(kOutline);
}

void paint_tiles(uint8_t (*t)[kTilePixels], uint8_t* flags) {
    auto fill = [&](int i, uint8_t c) { std::memset(t[i], c, kTilePixels); };
    auto dot = [&](int i, int x, int y, uint8_t c) { t[i][y * 8 + x] = c; };
    auto hline = [&](int i, int y, uint8_t c) { std::memset(t[i] + y * 8, c, 8); };
    // Gras im Schachbrett (Raster der Kacheln bleibt sichtbar), Büschel, Blüte, Weg
    fill(kTGrassA, kGrass + 1);
    dot(kTGrassA, 1, 2, kGrass);
    dot(kTGrassA, 5, 6, kGrass + 2);
    fill(kTGrassB, kGrass + 2);
    dot(kTGrassB, 2, 5, kGrass + 1);
    dot(kTGrassB, 6, 1, kGrass + 3);
    std::memcpy(t[kTTuft], t[kTGrassA], kTilePixels);
    dot(kTTuft, 2, 3, kTuft);
    dot(kTTuft, 4, 3, kTuft);
    dot(kTTuft, 3, 4, kTuft);
    dot(kTTuft, 3, 5, kGrass + 3);
    std::memcpy(t[kTBloom], t[kTGrassA], kTilePixels);
    dot(kTBloom, 4, 3, kBloom);
    dot(kTBloom, 3, 4, kBloom);
    dot(kTBloom, 5, 4, kBloom);
    dot(kTBloom, 4, 4, kBloom + 1);
    dot(kTBloom, 4, 5, kGrass + 3);
    fill(kTPath, kPath + 1);
    dot(kTPath, 1, 1, kPath);
    dot(kTPath, 5, 4, kPath);
    dot(kTPath, 3, 6, kPath + 2);
    dot(kTPath, 6, 2, kPath + 2);
    // Dielen (versetzte Stöße)
    for (int i = kTPlankA; i <= kTPlankB; ++i) {
        fill(i, kFloor + 1);
        hline(i, 0, kFloor);
        hline(i, 3, kFloor + 3);
        hline(i, 4, kFloor);
        hline(i, 7, kFloor + 3);
        const int a = i == kTPlankA ? 5 : 1, b = i == kTPlankA ? 2 : 6;
        for (int y = 0; y < 3; ++y) dot(i, a, y, kFloor + 2);
        for (int y = 4; y < 7; ++y) dot(i, b, y, kFloor + 2);
    }
    // Pflaster: zwei versetzte Steinreihen mit Fugen
    for (int i = kTStoneA; i <= kTStoneB; ++i) {
        fill(i, i == kTStoneA ? kStone + 1 : kStone + 2);
        hline(i, 3, kStone + 3);
        hline(i, 7, kStone + 3);
        const int a = i == kTStoneA ? 3 : 7, b = i == kTStoneA ? 7 : 3;
        for (int y = 0; y < 3; ++y) dot(i, a, y, kStone + 3);
        for (int y = 4; y < 7; ++y) dot(i, b, y, kStone + 3);
        dot(i, 1, 1, kStone);
        dot(i, 5, 5, kStone);
    }
    // Wand: Fläche (Tapete mit Streifen), Sockel mit Leiste, Band unter der Decke, Mauerkrone von oben
    fill(kTWallFace, kWall + 1);
    for (int y = 0; y < 8; ++y) {
        dot(kTWallFace, 3, y, kWall + 2);
        dot(kTWallFace, 7, y, kWall);
    }
    std::memcpy(t[kTWallBase], t[kTWallFace], kTilePixels);
    hline(kTWallBase, 5, kWall + 2);
    hline(kTWallBase, 6, kWall + 3);
    hline(kTWallBase, 7, kWall + 3);
    std::memcpy(t[kTWallBand], t[kTWallFace], kTilePixels);
    for (int y = 0; y < 3; ++y) hline(kTWallBand, y, kWall + 3);
    hline(kTWallBand, 3, kWall + 2);
    fill(kTWallTop, kWallTop);
    hline(kTWallTop, 0, kOutline);
    dot(kTWallTop, 2, 3, kDark);
    dot(kTWallTop, 6, 5, kDark);
    // Hecke: dichter Busch von oben
    fill(kTHedge, kGrass + 3);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            if ((x * 5 + y * 3) % 7 < 3) dot(kTHedge, x, y, (x + y) % 3 ? kGrass + 2 : kGrass + 1);
    std::memcpy(t[kTHedgeFace], t[kTHedge], kTilePixels);
    for (int x = 0; x < 8; x += 2) dot(kTHedgeFace, x, 6, kGrass + 3);
    hline(kTHedgeFace, 7, kOutline);
    // Deko-Ebene (durchsichtig): Kiesel, Blumen
    fill(kTPebbles, 0);
    dot(kTPebbles, 2, 2, kStone);
    dot(kTPebbles, 3, 2, kStone + 2);
    dot(kTPebbles, 5, 5, kStone + 1);
    dot(kTPebbles, 6, 5, kStone + 3);
    fill(kTFlowers, 0);
    dot(kTFlowers, 1, 2, kBloom);
    dot(kTFlowers, 5, 1, kBloom + 1);
    dot(kTFlowers, 3, 5, kBloom);
    dot(kTFlowers, 6, 6, kRows + 8);
    // Wasser (seit 0.5.2): schräge Wellen aus vier Farben, die Farbrotation lässt sie wandern
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) dot(kTWater, x, y, static_cast<uint8_t>(kWater + ((x + y * 3) / 3 & 3)));
    std::memset(flags, 0, kTileCount);
    for (int i : {kTWallFace, kTWallBase, kTWallBand, kTWallTop, kTHedge, kTHedgeFace, kTWater}) flags[i] = kTileBlocks;
}

// Erkennungsstück: in den Testorten bekommt jeder Besitzer denselben neutralen Sockel (Farbe aus der Raumliste)
int token_for(uint8_t owner) { return owner ? kPToken : -1; }

const Kit& kit() {
    if (!s_kit) {
        void* mem = asset_alloc(sizeof(Kit));
        Kit* k = mem ? new (mem) Kit : new Kit;
        for (int v = 0; v < 3; ++v)
            for (int pose = 0; pose < kPoses; ++pose)
                paint_figure(k->figure + (v * kPoses + pose) * kFigW * kFigH, v, pose);
        std::memset(k->tiles[0], 0, kTilePixels);
        paint_tiles(k->tiles, k->tile_flags);
        int used = 0;
        for (int s = 0; s < kStageCount; ++s) {
            const StageSpec& sp = kStages[s];
            uint8_t* px = k->pool + used;
            const int bytes = sp.w * sp.h;
            for (int f = 0; f < sp.frames; ++f) paint_stage(px + f * bytes, s, f);
            used += bytes * sp.frames;
            k->stages[s] = PartStage{{px, sp.w, sp.h, sp.frames, sp.w / 2, sp.h - 1},
                                     sp.fw, sp.fh, 0, 0, 45, sp.uses, sp.use_count};
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
        for (int r = 0; r < kFigureRows; ++r)
            k->rows[1 + r].ramp(kRamp, static_cast<uint8_t>(kRows + r * 4), 4);
        k->rows[kRowWood].ramp(kRamp, kWood, 4);
        k->rows[kRowPaper].ramp(kWall, kPastel, 4);
        k->tileset = TileSet{&k->tiles[0][0], k->tile_flags, kTileCount};
        k->catalog = PartCatalog{k->parts, kPartCount, k->rows, kRowCount, token_for, k->site};
        s_kit = k;  // bleibt bis zum Ende (wird nie freigegeben)
    }
    return *s_kit;
}

constexpr SpriteSheet figure_sheet(const Kit& k) { return {k.figure, kFigW, kFigH, 3 * kPoses, 8, 30}; }

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
constexpr LightCircle kFieldLights[] = {{80, 37, 22, 9, 190}, {120, 37, 22, 9, 190}};
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
    std::array<Color, kColors> c{};
    c[kOutline] = {28, 26, 36};
    const Color grass[4] = {{112, 150, 110}, {96, 134, 98}, {82, 118, 86}, {66, 100, 74}};
    const Color path[3] = {{186, 172, 138}, {166, 152, 120}, {142, 128, 100}};
    const Color floor[4] = {{164, 124, 90}, {146, 108, 78}, {126, 92, 66}, {104, 74, 54}};
    const Color wall[4] = {{206, 194, 178}, {186, 174, 158}, {160, 148, 134}, {128, 116, 106}};
    const Color ramp[4] = {{200, 200, 204}, {164, 164, 170}, {128, 128, 136}, {92, 92, 100}};
    const Color stone[4] = {{184, 180, 172}, {164, 160, 152}, {142, 138, 132}, {112, 108, 104}};
    const Color wood[4] = {{188, 140, 96}, {160, 114, 76}, {128, 88, 60}, {96, 64, 46}};
    const Color pastel[4] = {{214, 200, 222}, {194, 178, 206}, {166, 150, 184}, {130, 116, 150}};
    const Color water[4] = {{92, 150, 186}, {72, 128, 170}, {60, 112, 156}, {80, 140, 178}};
    // Farbreihen der Figuren: rot, blau, gelb, lavendel, grün, türkis
    const Color rows[kFigureRows][4] = {
        {{228, 106, 98}, {198, 80, 76}, {162, 60, 62}, {122, 46, 52}},
        {{106, 152, 224}, {80, 122, 198}, {62, 96, 162}, {48, 72, 122}},
        {{238, 206, 98}, {216, 176, 70}, {182, 142, 54}, {142, 106, 44}},
        {{178, 150, 222}, {150, 120, 198}, {124, 92, 168}, {96, 70, 132}},
        {{134, 202, 122}, {102, 172, 98}, {78, 140, 82}, {58, 106, 66}},
        {{110, 202, 198}, {82, 170, 172}, {62, 136, 142}, {46, 102, 110}},
    };
    for (int i = 0; i < 4; ++i) {
        c[kGrass + i] = grass[i];
        c[kFloor + i] = floor[i];
        c[kWall + i] = wall[i];
        c[kRamp + i] = ramp[i];
        c[kStone + i] = stone[i];
        c[kWood + i] = wood[i];
        c[kPastel + i] = pastel[i];
        c[kWater + i] = water[i];
        for (int r = 0; r < kFigureRows; ++r) c[kRows + r * 4 + i] = rows[r][i];
    }
    for (int i = 0; i < 3; ++i) c[kPath + i] = path[i];
    c[kTuft] = {130, 168, 118};
    c[kBloom] = {238, 234, 222};
    c[kBloom + 1] = {222, 180, 86};
    c[kSkin] = {238, 216, 192};
    c[kSkin + 1] = {216, 188, 162};
    c[kSkin + 2] = {182, 152, 130};
    c[kHair] = {84, 66, 60};
    c[kHair + 1] = {64, 50, 46};
    c[kHair + 2] = {46, 36, 34};
    c[kEye] = {32, 28, 38};
    c[kDark] = {52, 42, 40};
    c[kWallTop] = {84, 74, 72};
    c[kGlass] = {168, 198, 212};
    c[kShadow] = kind_ == Kind::Field ? Color{70, 100, 78} : kind_ == Kind::Room ? Color{100, 74, 56}
                                                                                  : Color{118, 114, 108};
    c[kSky] = {176, 204, 216};      // Fensterglas am Tag (Spiegelung)
    c[kSky + 1] = {150, 180, 196};
    c[kFlame] = {214, 200, 168};    // Lampenschirm am Tag
    c[kFlame + 1] = {200, 184, 150};
    c[kGlow] = {226, 214, 184};
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

void TestPlace::spawn_at(Figure& f, int tx, int ty, Facing facing, uint8_t row, bool guide) {
    f = Figure{};
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
    const LightCircle* l = kind_ == Kind::Field ? kFieldLights : kind_ == Kind::Room ? kRoomLights : kSquareLights;
    const int n = kind_ == Kind::Field ? 2 : 1;
    std::array<LightCircle, Room::kMaxLights> c{};
    for (int i = 0; i < n; ++i) {
        c[i] = l[i];
        if (light_inner_ >= 0) c[i].inner = static_cast<uint8_t>(light_inner_);
    }
    room_.set_lights(c.data(), n, pal_.base(), pal_.levels(), light_style_);
}

void TestPlace::init(uint64_t seed) {
    kit();
    rng_.reseed(seed);
    build_palette();
    pal_.apply(daylight_.at(12 * 3600 * 1000000LL), 0, palette_);
    build_room();
    count_ = 0;
    out_count_ = 0;
    load_pending_ = false;
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
            t = Travel{e.to, e.entry, static_cast<uint8_t>(f.exit), e.transition, f.guide,
                       static_cast<uint16_t>(f.row | (f.guide ? 0x100 : 0))};
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
    // Palette zur Tageszeit (Tageslicht, Lampen, Fenster, Himmel) und Farbrotation
    const Ambient a = daylight_.at(clock.us_of_day());
    pal_.apply(a, clock.ticks(), palette_);
    room_.set_light_on(a.lamp > 0);
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
    const bool guide = t.who & 0x100;
    const uint8_t row = static_cast<uint8_t>(std::clamp<int>(t.who & 0xFF, 1, kFigureRows));
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
    spawn_at(f, tx, ty, facing, row, guide);
    f.wait = static_cast<uint16_t>(rng_.range(10, 40));
}

bool TestPlace::command(const char* cmd) {
    if (!cmd) return false;
    int n = 0;
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
    const SpriteSheet sheet = figure_sheet(k);
    for (int i = 0; i < count_; ++i) {
        const Figure& f = figures_[i];
        const View v = view_of(f.facing);
        list.add(sheet.frame(v.index * kPoses + f.anim.frame()), f.x >> 8, f.y >> 8, sheet.pivot_x, sheet.pivot_y,
                 v.flip, k.rows[f.row].table(), f.dz, 5);
    }
    list.draw(c);
}

void TestPlace::render(Canvas& c) const { draw(c, true); }

void TestPlace::render_overlay(Canvas& fine) const {
    if (inspect_) room_.draw_inspect(fine, ui::kTextDim, ui::kPrimary, ui::kSuccess, ui::kWarning);
    // Feine Beschriftung über dem groben Bild (Farben der System-Oberfläche)
    const char* t = title();
    fine.fill_rect(4, 4, font::text_width(t) + 8, 13, ui::kSurface);
    font::draw(fine, 8, 5, t, ui::kText);
    // kleiner Pfeil über der Begleiterin (ihr folgt der Blick durch die Ausgänge)
    for (int i = 0; i < count_; ++i) {
        const Figure& f = figures_[i];
        if (!f.guide) continue;
        const int x = (f.x >> 8) * kCoarseScale, y = ((f.y >> 8) - 34) * kCoarseScale;
        for (int r = 0; r < 5; ++r) fine.fill_rect(x - 5 + r, y + r, 11 - 2 * r, 1, ui::kBg);
        for (int r = 0; r < 3; ++r) fine.fill_rect(x - 3 + r, y + 1 + r, 7 - 2 * r, 1, ui::kWhite);
    }
}

}  // namespace pixelia
