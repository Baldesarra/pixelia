#pragma once
// Kleiner PNG-Encoder ohne Fremdbibliothek (Deflate mit festen Huffman-Codes und LZ77).
// Pixel-Art komprimiert damit gut. Genutzt vom Simulator (Screenshots, Kontaktbogen),
// später auch vom Gerät (Screenshot und Live-Vorschau in der Weboberfläche).

#include <cstddef>
#include <cstdint>
#include <vector>

#include "pixelia/engine/canvas.h"

namespace pixelia {

// 8-Bit-Palettenbild (PNG-Farbtyp 3), genau das interne Bildformat der Engine
std::vector<uint8_t> encode_png_indexed(const uint8_t* pixels, int width, int height, const Palette& palette);

// RGB-Bild, 3 Byte pro Pixel (PNG-Farbtyp 2)
std::vector<uint8_t> encode_png_rgb(const uint8_t* rgb, int width, int height);

// CRC-32 wie PNG und zlib (seit 0.5.5 öffentlich: Asset-Pakete, assets.h); crc = Zwischenstand zum Fortsetzen
uint32_t crc32(const uint8_t* data, size_t size, uint32_t crc = 0);

// zlib-Datenstrom (Deflate + Adler-32), z. B. für eigene Formate
std::vector<uint8_t> zlib_compress(const uint8_t* data, size_t size);

}  // namespace pixelia
