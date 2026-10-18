#include "pixelia/engine/png.h"

#include <algorithm>
#include <cstring>

namespace pixelia {

namespace {

// --- CRC-32 (PNG-Chunks) ---------------------------------------------------------------------------------------
uint32_t crc32(const uint8_t* data, size_t size, uint32_t crc = 0) {
    static uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[n] = c;
        }
        ready = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < size; ++i) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

uint32_t adler32(const uint8_t* data, size_t size) {
    uint32_t a = 1, b = 0;
    while (size > 0) {
        size_t n = std::min<size_t>(size, 5552);  // größter Block ohne Überlauf vor dem Modulo
        size -= n;
        while (n--) {
            a += *data++;
            b += a;
        }
        a %= 65521;
        b %= 65521;
    }
    return (b << 16) | a;
}

// --- Bitstrom (Deflate schreibt LSB zuerst, Huffman-Codes MSB zuerst) ------------------------------------------
class BitWriter {
public:
    explicit BitWriter(std::vector<uint8_t>& out) : out_(out) {}
    void put(uint32_t bits, int count) {
        buf_ |= static_cast<uint64_t>(bits) << count_;
        count_ += count;
        while (count_ >= 8) {
            out_.push_back(static_cast<uint8_t>(buf_));
            buf_ >>= 8;
            count_ -= 8;
        }
    }
    void put_code(uint32_t code, int length) {
        uint32_t rev = 0;
        for (int i = 0; i < length; ++i) rev |= ((code >> i) & 1u) << (length - 1 - i);
        put(rev, length);
    }
    void flush() {
        if (count_ > 0) out_.push_back(static_cast<uint8_t>(buf_));
        buf_ = 0;
        count_ = 0;
    }

private:
    std::vector<uint8_t>& out_;
    uint64_t buf_ = 0;
    int count_ = 0;
};

constexpr uint16_t kLenBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                   31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr uint8_t kLenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr uint16_t kDistBase[30] = {1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
                                    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
constexpr uint8_t kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

// Feste Huffman-Codes laut RFC 1951, Abschnitt 3.2.6
void put_litlen(BitWriter& bw, int v) {
    if (v < 144) {
        bw.put_code(0x30 + v, 8);
    } else if (v < 256) {
        bw.put_code(0x190 + (v - 144), 9);
    } else if (v < 280) {
        bw.put_code(v - 256, 7);
    } else {
        bw.put_code(0xC0 + (v - 280), 8);
    }
}

void put_match(BitWriter& bw, int length, int distance) {
    int i = 28;
    while (kLenBase[i] > length) --i;
    put_litlen(bw, 257 + i);
    if (kLenExtra[i]) bw.put(length - kLenBase[i], kLenExtra[i]);
    int j = 29;
    while (kDistBase[j] > distance) --j;
    bw.put_code(j, 5);
    if (kDistExtra[j]) bw.put(distance - kDistBase[j], kDistExtra[j]);
}

void deflate_fixed(const uint8_t* data, size_t n, std::vector<uint8_t>& out) {
    constexpr int kWindow = 32768;
    constexpr int kHashBits = 15;
    constexpr int kMaxChain = 48;
    constexpr int kMinMatch = 3;
    constexpr int kMaxMatch = 258;

    BitWriter bw(out);
    bw.put(1, 1);  // BFINAL: einziger Block
    bw.put(1, 2);  // BTYPE 01: feste Huffman-Codes

    std::vector<int32_t> head(1u << kHashBits, -1);
    std::vector<int32_t> prev(kWindow, -1);
    auto hash = [&](size_t p) {
        uint32_t v = data[p] | (data[p + 1] << 8) | (data[p + 2] << 16);
        return (v * 2654435761u) >> (32 - kHashBits);
    };
    auto insert = [&](size_t p) {
        if (p + 2 >= n) return;
        uint32_t h = hash(p);
        prev[p & (kWindow - 1)] = head[h];
        head[h] = static_cast<int32_t>(p);
    };

    size_t i = 0;
    while (i < n) {
        int best_len = 0, best_dist = 0;
        if (i + 2 < n) {
            const int max_len = static_cast<int>(std::min<size_t>(kMaxMatch, n - i));
            int32_t cand = head[hash(i)];
            for (int chain = kMaxChain; cand >= 0 && chain > 0; --chain) {
                if (i - cand > static_cast<size_t>(kWindow)) break;
                if (data[cand + best_len] == data[i + best_len]) {
                    int len = 0;
                    while (len < max_len && data[cand + len] == data[i + len]) ++len;
                    if (len > best_len) {
                        best_len = len;
                        best_dist = static_cast<int>(i - cand);
                        if (len == max_len) break;
                    }
                }
                int32_t next = prev[cand & (kWindow - 1)];
                if (next >= cand) break;  // veralteter Eintrag im Ringpuffer
                cand = next;
            }
        }
        if (best_len >= kMinMatch) {
            put_match(bw, best_len, best_dist);
            for (int k = 0; k < best_len; ++k) insert(i + k);
            i += best_len;
        } else {
            put_litlen(bw, data[i]);
            insert(i);
            ++i;
        }
    }
    put_litlen(bw, 256);  // Blockende
    bw.flush();
}

void put_u32_be(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

void put_chunk(std::vector<uint8_t>& png, const char* type, const std::vector<uint8_t>& data) {
    put_u32_be(png, static_cast<uint32_t>(data.size()));
    size_t start = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    put_u32_be(png, crc32(png.data() + start, png.size() - start));
}

std::vector<uint8_t> build_png(int width, int height, uint8_t color_type, const std::vector<uint8_t>& raw,
                               const std::vector<uint8_t>* plte) {
    static constexpr uint8_t kSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<uint8_t> png(kSignature, kSignature + 8);
    std::vector<uint8_t> ihdr;
    put_u32_be(ihdr, static_cast<uint32_t>(width));
    put_u32_be(ihdr, static_cast<uint32_t>(height));
    ihdr.push_back(8);           // Bittiefe
    ihdr.push_back(color_type);  // 2 = RGB, 3 = Palette
    ihdr.push_back(0);           // Kompression Deflate
    ihdr.push_back(0);           // Filtermethode
    ihdr.push_back(0);           // ohne Interlacing
    put_chunk(png, "IHDR", ihdr);
    if (plte) put_chunk(png, "PLTE", *plte);
    put_chunk(png, "IDAT", zlib_compress(raw.data(), raw.size()));
    put_chunk(png, "IEND", {});
    return png;
}

}  // namespace

std::vector<uint8_t> zlib_compress(const uint8_t* data, size_t size) {
    std::vector<uint8_t> out;
    out.reserve(size / 4 + 64);
    out.push_back(0x78);  // Deflate, 32-KB-Fenster
    out.push_back(0x01);  // Prüfbits passend zu 0x78, keine Voreinstellung
    deflate_fixed(data, size, out);
    put_u32_be(out, adler32(data, size));
    return out;
}

std::vector<uint8_t> encode_png_indexed(const uint8_t* pixels, int width, int height, const Palette& palette) {
    // Jede Zeile beginnt mit dem Filtertyp 0 (keiner), das ist für Palettenbilder üblich
    std::vector<uint8_t> raw(static_cast<size_t>(width + 1) * height);
    for (int y = 0; y < height; ++y) {
        uint8_t* row = raw.data() + static_cast<size_t>(y) * (width + 1);
        row[0] = 0;
        std::memcpy(row + 1, pixels + static_cast<size_t>(y) * width, width);
    }
    std::vector<uint8_t> plte;
    plte.reserve(256 * 3);
    for (const Color& c : palette) {
        plte.push_back(c.r);
        plte.push_back(c.g);
        plte.push_back(c.b);
    }
    return build_png(width, height, 3, raw, &plte);
}

std::vector<uint8_t> encode_png_rgb(const uint8_t* rgb, int width, int height) {
    const size_t stride = static_cast<size_t>(width) * 3;
    std::vector<uint8_t> raw((stride + 1) * height);
    for (int y = 0; y < height; ++y) {
        uint8_t* row = raw.data() + static_cast<size_t>(y) * (stride + 1);
        row[0] = 0;
        std::memcpy(row + 1, rgb + static_cast<size_t>(y) * stride, stride);
    }
    return build_png(width, height, 2, raw, nullptr);
}

}  // namespace pixelia
