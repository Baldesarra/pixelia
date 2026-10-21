#pragma once
// Kacheln (seit 0.5.1): Boden, Wände und Deko eines Ortes als Kachelebenen im groben Raster.
//
// - Eine Kachel hat 8 × 8 grobe Bildpunkte (Paletten-Indizes, 0 = durchsichtig); 25 × 15 Kacheln füllen einen Ort.
// - Kachelsatz: Kacheln hintereinander (je 64 Byte, Kachel 0 = leer und wird nie gezeichnet), je Kachel
//   Eigenschaften wie „versperrt den Weg“. Die Grafiken liegen im Grafik-Speicher (memory.h).
// - Kachelkarte: drei Ebenen übereinander. Boden deckt, Wand und Deko lassen durchsichtige Punkte offen. Jede Ebene
//   bekommt beim Zeichnen ihre eigene Farbreihe (Tapete und Boden je Haus aus denselben Kacheln).
// - Datengetrieben: Eine Ebene kommt als 15 Textzeilen mit je 25 Zeichen und einer Legende (Zeichen → Kachel).
// Gezeichnet wird meist nur einmal beim Betreten in das Hintergrundbild eines Raums (room.h).

#include <cstdint>

#include "pixelia/engine/canvas.h"

namespace pixelia {

inline constexpr int kTileSize = 8;
inline constexpr int kTilePixels = kTileSize * kTileSize;
inline constexpr int kTilesX = kCoarseWidth / kTileSize;   // 25
inline constexpr int kTilesY = kCoarseHeight / kTileSize;  // 15
inline constexpr int kTileCells = kTilesX * kTilesY;
static_assert(kCoarseWidth % kTileSize == 0 && kCoarseHeight % kTileSize == 0, "Ort = ganze Kacheln");

// Eigenschaften einer Kachel (Bits)
inline constexpr uint8_t kTileBlocks = 1;  // versperrt den Weg (Wand, Zaun, Wasser)

struct TileSet {
    const uint8_t* pixels = nullptr;  // count × 64 Indizes
    const uint8_t* flags = nullptr;   // count Einträge oder nullptr
    int count = 0;

    const uint8_t* tile(int i) const { return pixels + i * kTilePixels; }
    uint8_t flag(int i) const { return flags && i > 0 && i < count ? flags[i] : 0; }
};

enum TileLayer : uint8_t { kFloorLayer, kWallLayer, kDecoLayer, kTileLayers };

class TileMap {
public:
    void clear();
    uint8_t at(int layer, int tx, int ty) const;  // außerhalb: 0
    void set(int layer, int tx, int ty, uint8_t tile);
    void fill(int layer, int tx, int ty, int w, int h, uint8_t tile);
    // Textzeilen (höchstens kTilesY, je bis zu kTilesX Zeichen); legend[i] ist das Zeichen für Kachel i,
    // legend[0] steht für leer. Unbekannte Zeichen sind leer.
    void load(int layer, const char* const* rows, int count, const char* legend);
    bool blocks(const TileSet& set, int tx, int ty) const;  // eine Ebene versperrt (außerhalb: ja)
    // Ebene zeichnen; Kachel 0 bleibt frei. opaque: Kacheln decken (Boden), sonst bleiben Index-0-Punkte offen.
    void draw(Canvas& c, const TileSet& set, int layer, bool opaque, const uint8_t* remap = nullptr) const;

private:
    uint8_t t_[kTileLayers][kTilesY][kTilesX]{};
};

}  // namespace pixelia
