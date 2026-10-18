#include "pixelia/engine/tiny_font.h"

namespace pixelia::tiny_font {

namespace {
// Zeilen von oben nach unten, je 3 Bit (links = 4)
constexpr uint16_t g(int r0, int r1, int r2, int r3, int r4) {
    return static_cast<uint16_t>((r0 << 12) | (r1 << 9) | (r2 << 6) | (r3 << 3) | r4);
}

constexpr uint16_t kDigits[10] = {
    g(7, 5, 5, 5, 7), g(2, 6, 2, 2, 7), g(7, 1, 7, 4, 7), g(7, 1, 7, 1, 7), g(5, 5, 7, 1, 1),
    g(7, 4, 7, 1, 7), g(7, 4, 7, 5, 7), g(7, 1, 1, 2, 2), g(7, 5, 7, 5, 7), g(7, 5, 7, 1, 7),
};

constexpr uint16_t kLetters[26] = {
    g(2, 5, 7, 5, 5),  // A
    g(6, 5, 6, 5, 6),  // B
    g(3, 4, 4, 4, 3),  // C
    g(6, 5, 5, 5, 6),  // D
    g(7, 4, 6, 4, 7),  // E
    g(7, 4, 6, 4, 4),  // F
    g(3, 4, 5, 5, 3),  // G
    g(5, 5, 7, 5, 5),  // H
    g(7, 2, 2, 2, 7),  // I
    g(1, 1, 1, 5, 2),  // J
    g(5, 5, 6, 5, 5),  // K
    g(4, 4, 4, 4, 7),  // L
    g(5, 7, 7, 5, 5),  // M
    g(6, 5, 5, 5, 5),  // N
    g(2, 5, 5, 5, 2),  // O
    g(6, 5, 6, 4, 4),  // P
    g(2, 5, 5, 6, 3),  // Q
    g(6, 5, 6, 5, 5),  // R
    g(3, 4, 2, 1, 6),  // S
    g(7, 2, 2, 2, 2),  // T
    g(5, 5, 5, 5, 7),  // U
    g(5, 5, 5, 5, 2),  // V
    g(5, 5, 7, 7, 5),  // W
    g(5, 5, 2, 5, 5),  // X
    g(5, 5, 2, 2, 2),  // Y
    g(7, 1, 2, 4, 7),  // Z
};
}  // namespace

uint16_t glyph(char c) {
    if (c >= '0' && c <= '9') return kDigits[c - '0'];
    if (c >= 'A' && c <= 'Z') return kLetters[c - 'A'];
    if (c >= 'a' && c <= 'z') return kLetters[c - 'a'];
    switch (c) {
        case ':': return g(0, 2, 0, 2, 0);
        case '.': return g(0, 0, 0, 0, 2);
        case '-': return g(0, 0, 7, 0, 0);
        case '/': return g(1, 1, 2, 4, 4);
        default: return 0;
    }
}

int text_width(const char* text) {
    int n = 0;
    for (const char* p = text; *p; ++p) ++n;
    return n > 0 ? n * kAdvance - 1 : 0;
}

}  // namespace pixelia::tiny_font
