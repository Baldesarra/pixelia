#include "pixelia/engine/tiles.h"

#include <cstring>

namespace pixelia {

void TileMap::clear() { std::memset(t_, 0, sizeof(t_)); }

uint8_t TileMap::at(int layer, int tx, int ty) const {
    if (layer < 0 || layer >= kTileLayers || tx < 0 || ty < 0 || tx >= kTilesX || ty >= kTilesY) return 0;
    return t_[layer][ty][tx];
}

void TileMap::set(int layer, int tx, int ty, uint8_t tile) {
    if (layer < 0 || layer >= kTileLayers || tx < 0 || ty < 0 || tx >= kTilesX || ty >= kTilesY) return;
    t_[layer][ty][tx] = tile;
}

void TileMap::fill(int layer, int tx, int ty, int w, int h, uint8_t tile) {
    for (int y = ty; y < ty + h; ++y)
        for (int x = tx; x < tx + w; ++x) set(layer, x, y, tile);
}

void TileMap::load(int layer, const char* const* rows, int count, const char* legend) {
    if (layer < 0 || layer >= kTileLayers) return;
    std::memset(t_[layer], 0, sizeof(t_[layer]));
    if (!rows || !legend) return;
    const size_t n = std::strlen(legend);
    for (int y = 0; y < count && y < kTilesY; ++y) {
        const char* r = rows[y];
        for (int x = 0; r && r[x] && x < kTilesX; ++x) {
            const void* hit = std::memchr(legend, r[x], n);
            t_[layer][y][x] = hit ? static_cast<uint8_t>(static_cast<const char*>(hit) - legend) : 0;
        }
    }
}

bool TileMap::blocks(const TileSet& set, int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= kTilesX || ty >= kTilesY) return true;
    for (int l = 0; l < kTileLayers; ++l)
        if (set.flag(t_[l][ty][tx]) & kTileBlocks) return true;
    return false;
}

void TileMap::draw(Canvas& c, const TileSet& set, int layer, bool opaque, const uint8_t* remap) const {
    if (layer < 0 || layer >= kTileLayers || !set.pixels) return;
    for (int ty = 0; ty < kTilesY; ++ty) {
        for (int tx = 0; tx < kTilesX; ++tx) {
            const uint8_t id = t_[layer][ty][tx];
            if (id == 0 || id >= set.count) continue;
            const uint8_t* src = set.tile(id);
            if (opaque && !remap) {
                for (int r = 0; r < kTileSize; ++r)
                    std::memcpy(c.row(ty * kTileSize + r) + tx * kTileSize, src + r * kTileSize, kTileSize);
                continue;
            }
            for (int r = 0; r < kTileSize; ++r) {
                uint8_t* dst = c.row(ty * kTileSize + r) + tx * kTileSize;
                for (int i = 0; i < kTileSize; ++i, ++src) {
                    const uint8_t v = *src;
                    if (v || opaque) dst[i] = remap && v ? remap[v] : v;
                }
            }
        }
    }
}

}  // namespace pixelia
