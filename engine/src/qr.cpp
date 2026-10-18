#include "pixelia/engine/qr.h"

#include <algorithm>
#include <cstdlib>
#include <string>

namespace pixelia::qr {

namespace {

constexpr int kMaxVersion = 10;

// Tabellen der Norm für Version 1–10 (Index 0 unbenutzt), Reihenfolge L, M, Q, H
constexpr int8_t kEccPerBlock[4][kMaxVersion + 1] = {
    {-1, 7, 10, 15, 20, 26, 18, 20, 24, 30, 18},
    {-1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26},
    {-1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24},
    {-1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28},
};
constexpr int8_t kBlocks[4][kMaxVersion + 1] = {
    {-1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4},
    {-1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5},
    {-1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8},
    {-1, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8},
};
// Kennbits der Stufe im Formatfeld
constexpr int kFormatBits[4] = {1, 0, 3, 2};

// Alle Datenmodule (ohne Funktionsmuster) einer Version
int raw_data_modules(int ver) {
    int result = (16 * ver + 128) * ver + 64;
    if (ver >= 2) {
        int n = ver / 7 + 2;
        result -= (25 * n - 10) * n - 55;
        if (ver >= 7) result -= 36;
    }
    return result;
}

int data_codewords(int ver, Ecc ecc) {
    int e = static_cast<int>(ecc);
    return raw_data_modules(ver) / 8 - kEccPerBlock[e][ver] * kBlocks[e][ver];
}

// Mittelpunkte der Ausrichtungsmuster
std::vector<int> alignment_positions(int ver) {
    if (ver == 1) return {};
    int n = ver / 7 + 2;
    int step = (ver * 8 + n * 3 + 5) / (n * 4 - 4) * 2;
    std::vector<int> pos(n);
    pos[0] = 6;
    for (int i = n - 1, p = ver * 4 + 10; i >= 1; --i, p -= step) pos[i] = p;
    return pos;
}

// Galois-Feld GF(256) mit dem Polynom 0x11D
uint8_t gf_mul(uint8_t x, uint8_t y) {
    int z = 0;
    for (int i = 7; i >= 0; --i) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return static_cast<uint8_t>(z);
}

std::vector<uint8_t> rs_divisor(int degree) {
    std::vector<uint8_t> result(degree);
    result[degree - 1] = 1;
    uint8_t root = 1;
    for (int i = 0; i < degree; ++i) {
        for (int j = 0; j < degree; ++j) {
            result[j] = gf_mul(result[j], root);
            if (j + 1 < degree) result[j] ^= result[j + 1];
        }
        root = gf_mul(root, 0x02);
    }
    return result;
}

std::vector<uint8_t> rs_remainder(const std::vector<uint8_t>& data, const std::vector<uint8_t>& divisor) {
    std::vector<uint8_t> result(divisor.size());
    for (uint8_t b : data) {
        uint8_t factor = b ^ result[0];
        result.erase(result.begin());
        result.push_back(0);
        for (size_t i = 0; i < result.size(); ++i) result[i] ^= gf_mul(divisor[i], factor);
    }
    return result;
}

class Builder {
public:
    Builder(int ver, Ecc ecc) : ver_(ver), ecc_(ecc), size_(ver * 4 + 17), mod_(size_ * size_), fn_(size_ * size_) {}

    void draw_function_patterns() {
        for (int i = 0; i < size_; ++i) {
            set_fn(6, i, i % 2 == 0);
            set_fn(i, 6, i % 2 == 0);
        }
        finder(3, 3);
        finder(size_ - 4, 3);
        finder(3, size_ - 4);
        auto pos = alignment_positions(ver_);
        int n = static_cast<int>(pos.size());
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                if (!((i == 0 && j == 0) || (i == 0 && j == n - 1) || (i == n - 1 && j == 0))) alignment(pos[i], pos[j]);
        draw_format(0);  // Platz reservieren, echte Bits nach der Maskenwahl
        draw_version();
    }

    void draw_codewords(const std::vector<uint8_t>& data) {
        size_t i = 0;
        for (int right = size_ - 1; right >= 1; right -= 2) {
            if (right == 6) right = 5;
            for (int vert = 0; vert < size_; ++vert) {
                for (int j = 0; j < 2; ++j) {
                    int x = right - j;
                    bool upward = ((right + 1) & 2) == 0;
                    int y = upward ? size_ - 1 - vert : vert;
                    if (!fn(x, y) && i < data.size() * 8) {
                        mod(x, y) = (data[i >> 3] >> (7 - (i & 7))) & 1;
                        ++i;
                    }
                }
            }
        }
    }

    void apply_mask(int mask) {
        for (int y = 0; y < size_; ++y) {
            for (int x = 0; x < size_; ++x) {
                bool invert = false;
                switch (mask) {
                    case 0: invert = (x + y) % 2 == 0; break;
                    case 1: invert = y % 2 == 0; break;
                    case 2: invert = x % 3 == 0; break;
                    case 3: invert = (x + y) % 3 == 0; break;
                    case 4: invert = (x / 3 + y / 2) % 2 == 0; break;
                    case 5: invert = x * y % 2 + x * y % 3 == 0; break;
                    case 6: invert = (x * y % 2 + x * y % 3) % 2 == 0; break;
                    default: invert = ((x + y) % 2 + x * y % 3) % 2 == 0; break;
                }
                if (!fn(x, y) && invert) mod(x, y) ^= 1;
            }
        }
    }

    void draw_format(int mask) {
        int data = kFormatBits[static_cast<int>(ecc_)] << 3 | mask;
        int rem = data;
        for (int i = 0; i < 10; ++i) rem = (rem << 1) ^ ((rem >> 9) * 0x537);
        int bits = (data << 10 | rem) ^ 0x5412;
        auto bit = [&](int i) { return ((bits >> i) & 1) != 0; };
        for (int i = 0; i <= 5; ++i) set_fn(8, i, bit(i));
        set_fn(8, 7, bit(6));
        set_fn(8, 8, bit(7));
        set_fn(7, 8, bit(8));
        for (int i = 9; i < 15; ++i) set_fn(14 - i, 8, bit(i));
        for (int i = 0; i < 8; ++i) set_fn(size_ - 1 - i, 8, bit(i));
        for (int i = 8; i < 15; ++i) set_fn(8, size_ - 15 + i, bit(i));
        set_fn(8, size_ - 8, true);  // immer dunkles Modul
    }

    // Strafpunkte nach der Norm (Regeln N1–N4): kleiner = besser lesbar
    long penalty() const {
        long result = 0;
        auto line_penalty = [&](bool horizontal) {
            for (int a = 0; a < size_; ++a) {
                int run = 0;
                uint8_t color = 2;
                for (int b = 0; b < size_; ++b) {
                    uint8_t c = horizontal ? at(b, a) : at(a, b);
                    if (c == color) {
                        ++run;
                        if (run == 5) result += 3;
                        else if (run > 5) result += 1;
                    } else {
                        color = c;
                        run = 1;
                    }
                }
                // Muster dunkel-hell-dunkel-dunkel-dunkel-hell-dunkel mit vier hellen Modulen davor oder danach;
                // außerhalb des Codes zählt die helle Ruhezone mit
                static constexpr uint8_t p1[11] = {1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0};
                static constexpr uint8_t p2[11] = {0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1};
                for (int s = -4; s + 10 < size_ + 4; ++s) {
                    bool m1 = true, m2 = true;
                    for (int k = 0; k < 11 && (m1 || m2); ++k) {
                        int pos = s + k;
                        uint8_t c = (pos < 0 || pos >= size_) ? 0 : (horizontal ? at(pos, a) : at(a, pos));
                        if (c != p1[k]) m1 = false;
                        if (c != p2[k]) m2 = false;
                    }
                    if (m1) result += 40;
                    if (m2) result += 40;
                }
            }
        };
        line_penalty(true);
        line_penalty(false);
        for (int y = 0; y + 1 < size_; ++y)
            for (int x = 0; x + 1 < size_; ++x) {
                uint8_t c = at(x, y);
                if (c == at(x + 1, y) && c == at(x, y + 1) && c == at(x + 1, y + 1)) result += 3;
            }
        long dark = 0;
        for (uint8_t m : mod_) dark += m;
        long total = static_cast<long>(size_) * size_;
        long k = (std::labs(dark * 20 - total * 10) + total - 1) / total - 1;
        result += k * 10;
        return result;
    }

    int size() const { return size_; }
    std::vector<uint8_t>& modules() { return mod_; }

private:
    uint8_t at(int x, int y) const { return mod_[static_cast<size_t>(y) * size_ + x]; }
    uint8_t& mod(int x, int y) { return mod_[static_cast<size_t>(y) * size_ + x]; }
    bool fn(int x, int y) const { return fn_[static_cast<size_t>(y) * size_ + x] != 0; }
    void set_fn(int x, int y, bool dark) {
        mod(x, y) = dark ? 1 : 0;
        fn_[static_cast<size_t>(y) * size_ + x] = 1;
    }

    void finder(int cx, int cy) {
        for (int dy = -4; dy <= 4; ++dy)
            for (int dx = -4; dx <= 4; ++dx) {
                int x = cx + dx, y = cy + dy;
                if (x < 0 || y < 0 || x >= size_ || y >= size_) continue;
                int dist = std::max(std::abs(dx), std::abs(dy));
                set_fn(x, y, dist != 2 && dist != 4);
            }
    }

    void alignment(int cx, int cy) {
        for (int dy = -2; dy <= 2; ++dy)
            for (int dx = -2; dx <= 2; ++dx) set_fn(cx + dx, cy + dy, std::max(std::abs(dx), std::abs(dy)) != 1);
    }

    void draw_version() {
        if (ver_ < 7) return;
        int rem = ver_;
        for (int i = 0; i < 12; ++i) rem = (rem << 1) ^ ((rem >> 11) * 0x1F25);
        long bits = static_cast<long>(ver_) << 12 | rem;
        for (int i = 0; i < 18; ++i) {
            bool b = ((bits >> i) & 1) != 0;
            int a = size_ - 11 + i % 3, c = i / 3;
            set_fn(a, c, b);
            set_fn(c, a, b);
        }
    }

    int ver_;
    Ecc ecc_;
    int size_;
    std::vector<uint8_t> mod_;
    std::vector<uint8_t> fn_;
};

// Datenbits: Modus „Byte“, Länge, Nutzdaten, Abschluss, Füllbytes; danach Blöcke mit Fehlerkorrektur verschränken
std::vector<uint8_t> build_codewords(std::string_view text, int ver, Ecc ecc) {
    std::vector<uint8_t> data;
    uint32_t acc = 0;
    int nbits = 0;
    size_t total_bits = 0;
    auto put = [&](uint32_t value, int bits) {
        for (int i = bits - 1; i >= 0; --i) {
            acc = (acc << 1) | ((value >> i) & 1);
            ++total_bits;
            if (++nbits == 8) {
                data.push_back(static_cast<uint8_t>(acc));
                acc = 0;
                nbits = 0;
            }
        }
    };
    const size_t capacity_bits = static_cast<size_t>(data_codewords(ver, ecc)) * 8;
    put(0x4, 4);
    put(static_cast<uint32_t>(text.size()), ver <= 9 ? 8 : 16);
    for (char c : text) put(static_cast<uint8_t>(c), 8);
    put(0, static_cast<int>(std::min<size_t>(4, capacity_bits - total_bits)));
    if (nbits) put(0, 8 - nbits);
    for (uint8_t pad = 0xEC; data.size() * 8 < capacity_bits; pad ^= 0xEC ^ 0x11) data.push_back(pad);

    const int e = static_cast<int>(ecc);
    const int num_blocks = kBlocks[e][ver];
    const int ecc_len = kEccPerBlock[e][ver];
    const int raw = raw_data_modules(ver) / 8;
    const int num_short = num_blocks - raw % num_blocks;
    const int short_len = raw / num_blocks;
    auto divisor = rs_divisor(ecc_len);
    std::vector<std::vector<uint8_t>> blocks;
    for (int i = 0, k = 0; i < num_blocks; ++i) {
        int len = short_len - ecc_len + (i < num_short ? 0 : 1);
        std::vector<uint8_t> block(data.begin() + k, data.begin() + k + len);
        k += len;
        auto rem = rs_remainder(block, divisor);
        if (i < num_short) block.push_back(0);
        block.insert(block.end(), rem.begin(), rem.end());
        blocks.push_back(std::move(block));
    }
    std::vector<uint8_t> result;
    for (size_t i = 0; i < blocks[0].size(); ++i)
        for (int j = 0; j < num_blocks; ++j)
            if (i != static_cast<size_t>(short_len - ecc_len) || j >= num_short) result.push_back(blocks[j][i]);
    return result;
}

}  // namespace

bool encode(std::string_view data, Ecc ecc, Code* out, int mask) {
    int ver = 1;
    for (; ver <= kMaxVersion; ++ver) {
        int header_bits = 4 + (ver <= 9 ? 8 : 16);
        if (header_bits + static_cast<int>(data.size()) * 8 <= data_codewords(ver, ecc) * 8) break;
    }
    if (ver > kMaxVersion) return false;

    auto codewords = build_codewords(data, ver, ecc);
    Builder b(ver, ecc);
    b.draw_function_patterns();
    b.draw_codewords(codewords);
    if (mask < 0 || mask > 7) {
        long best = -1;
        for (int m = 0; m < 8; ++m) {
            b.apply_mask(m);
            b.draw_format(m);
            long p = b.penalty();
            if (best < 0 || p < best) {
                best = p;
                mask = m;
            }
            b.apply_mask(m);  // XOR macht die Maske wieder rückgängig
        }
    }
    b.apply_mask(mask);
    b.draw_format(mask);
    out->version = ver;
    out->size = b.size();
    out->mask = mask;
    out->modules = std::move(b.modules());
    return true;
}

std::string wifi_payload(std::string_view ssid, std::string_view password) {
    auto escape = [](std::string_view s) {
        std::string r;
        for (char c : s) {
            if (c == '\\' || c == ';' || c == ',' || c == ':' || c == '"') r += '\\';
            r += c;
        }
        return r;
    };
    std::string r = "WIFI:T:";
    r += password.empty() ? "nopass" : "WPA";
    r += ";S:" + escape(ssid) + ";";
    if (!password.empty()) r += "P:" + escape(password) + ";";
    r += ";";
    return r;
}

}  // namespace pixelia::qr
