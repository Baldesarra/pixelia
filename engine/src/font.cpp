#include "pixelia/engine/font.h"

#include <cstddef>

namespace pixelia::font {

namespace {

struct Glyph {
    char32_t cp;
    uint8_t width;
    uint8_t rows[kCellHeight];  // Bit (width-1) = linke Spalte
};

// Absichtlich nicht constexpr: Ein Aufruf beim Übersetzen bricht mit Fehlermeldung ab (Ausnahmen sind aus)
void glyph_error(const char*) {}

// Glyphen als Pixelbild: Zeilen durch Leerzeichen getrennt, '#' = Pixel, '.' = frei.
// 9 Zeilen = Zeile 2–10 der Zelle (Normalfall), 11 Zeilen = ganze Zelle (große Umlaute).
// Wird beim Übersetzen geprüft: ungleich lange Zeilen oder falsche Zeilenzahl ergeben einen Compiler-Fehler.
constexpr Glyph make(char32_t cp, const char* art) {
    Glyph g{cp, 0, {}};
    int row = 0, col = 0, width = -1;
    uint8_t bits = 0;
    uint8_t rows[kCellHeight] = {};
    for (const char* p = art;; ++p) {
        char c = *p;
        if (c == ' ' || c == 0) {
            if (width < 0) width = col;
            if (col != width || row >= kCellHeight) glyph_error("Glyphe: Zeilenlänge oder Zeilenzahl falsch");
            rows[row++] = bits;
            bits = 0;
            col = 0;
            if (c == 0) break;
            continue;
        }
        if (c != '#' && c != '.') glyph_error("Glyphe: nur # und . erlaubt");
        bits = static_cast<uint8_t>((bits << 1) | (c == '#' ? 1 : 0));
        ++col;
    }
    if (width > 8 || (row != 9 && row != kCellHeight)) glyph_error("Glyphe: höchstens 8 breit, 9 oder 11 Zeilen");
    const int offset = row == 9 ? kCapTop : 0;
    for (int i = 0; i < row; ++i) g.rows[offset + i] = rows[i];
    g.width = static_cast<uint8_t>(width);
    return g;
}

// ASCII 32–126 in Reihenfolge, danach Sonderzeichen
constexpr Glyph kGlyphs[] = {
    make(U' ', "... ... ... ... ... ... ... ... ..."),
    make(U'!', "# # # # # . # . ."),
    make(U'"', "#.# #.# ... ... ... ... ... ... ..."),
    make(U'#', ".#.#. .#.#. ##### .#.#. ##### .#.#. .#.#. ..... ....."),
    make(U'$', "..#.. .#### #.#.. .###. ..#.# ####. ..#.. ..... ....."),
    make(U'%', "##... ##..# ...#. ..#.. .#... #..## ...## ..... ....."),
    make(U'&', ".##.. #..#. #.#.. .#... #.#.# #..#. .##.# ..... ....."),
    make(U'\'', "# # . . . . . . ."),
    make(U'(', "..# .#. #.. #.. #.. .#. ..# ... ..."),
    make(U')', "#.. .#. ..# ..# ..# .#. #.. ... ..."),
    make(U'*', "..... ..#.. #.#.# .###. #.#.# ..#.. ..... ..... ....."),
    make(U'+', "..... ..#.. ..#.. ##### ..#.. ..#.. ..... ..... ....."),
    make(U',', ".. .. .. .. .. .. .# #. .."),
    make(U'-', ".... .... .... #### .... .... .... .... ...."),
    make(U'.', ". . . . . . # . ."),
    make(U'/', "....# ....# ...#. ..#.. .#... #.... #.... ..... ....."),
    make(U'0', ".###. #...# #..## #.#.# ##..# #...# .###. ..... ....."),
    make(U'1', "..#.. .##.. ..#.. ..#.. ..#.. ..#.. .###. ..... ....."),
    make(U'2', ".###. #...# ....# ...#. ..#.. .#... ##### ..... ....."),
    make(U'3', "##### ...#. ..#.. ...#. ....# #...# .###. ..... ....."),
    make(U'4', "...#. ..##. .#.#. #..#. ##### ...#. ...#. ..... ....."),
    make(U'5', "##### #.... ####. ....# ....# #...# .###. ..... ....."),
    make(U'6', "..##. .#... #.... ####. #...# #...# .###. ..... ....."),
    make(U'7', "##### ....# ...#. ..#.. .#... .#... .#... ..... ....."),
    make(U'8', ".###. #...# #...# .###. #...# #...# .###. ..... ....."),
    make(U'9', ".###. #...# #...# .#### ....# ...#. .##.. ..... ....."),
    make(U':', ". . . # . . # . ."),
    make(U';', ".. .. .. .# .. .. .# #. .."),
    make(U'<', "...# ..#. .#.. #... .#.. ..#. ...# .... ...."),
    make(U'=', "..... ..... ##### ..... ##### ..... ..... ..... ....."),
    make(U'>', "#... .#.. ..#. ...# ..#. .#.. #... .... ...."),
    make(U'?', ".###. #...# ....# ...#. ..#.. ..... ..#.. ..... ....."),
    make(U'@', ".###. #...# #.### #.#.# #.##. #.... .#### ..... ....."),
    make(U'A', ".###. #...# #...# ##### #...# #...# #...# ..... ....."),
    make(U'B', "####. #...# #...# ####. #...# #...# ####. ..... ....."),
    make(U'C', ".###. #...# #.... #.... #.... #...# .###. ..... ....."),
    make(U'D', "###.. #..#. #...# #...# #...# #..#. ###.. ..... ....."),
    make(U'E', "##### #.... #.... ####. #.... #.... ##### ..... ....."),
    make(U'F', "##### #.... #.... ####. #.... #.... #.... ..... ....."),
    make(U'G', ".###. #...# #.... #.### #...# #...# .#### ..... ....."),
    make(U'H', "#...# #...# #...# ##### #...# #...# #...# ..... ....."),
    make(U'I', "### .#. .#. .#. .#. .#. ### ... ..."),
    make(U'J', "..### ...#. ...#. ...#. ...#. #..#. .##.. ..... ....."),
    make(U'K', "#...# #..#. #.#.. ##... #.#.. #..#. #...# ..... ....."),
    make(U'L', "#.... #.... #.... #.... #.... #.... ##### ..... ....."),
    make(U'M', "#...# ##.## #.#.# #.#.# #...# #...# #...# ..... ....."),
    make(U'N', "#...# #...# ##..# #.#.# #..## #...# #...# ..... ....."),
    make(U'O', ".###. #...# #...# #...# #...# #...# .###. ..... ....."),
    make(U'P', "####. #...# #...# ####. #.... #.... #.... ..... ....."),
    make(U'Q', ".###. #...# #...# #...# #.#.# #..#. .##.# ..... ....."),
    make(U'R', "####. #...# #...# ####. #.#.. #..#. #...# ..... ....."),
    make(U'S', ".#### #.... #.... .###. ....# ....# ####. ..... ....."),
    make(U'T', "##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#.. ..... ....."),
    make(U'U', "#...# #...# #...# #...# #...# #...# .###. ..... ....."),
    make(U'V', "#...# #...# #...# #...# #...# .#.#. ..#.. ..... ....."),
    make(U'W', "#...# #...# #...# #.#.# #.#.# #.#.# .#.#. ..... ....."),
    make(U'X', "#...# #...# .#.#. ..#.. .#.#. #...# #...# ..... ....."),
    make(U'Y', "#...# #...# #...# .#.#. ..#.. ..#.. ..#.. ..... ....."),
    make(U'Z', "##### ....# ...#. ..#.. .#... #.... ##### ..... ....."),
    make(U'[', "### #.. #.. #.. #.. #.. ### ... ..."),
    make(U'\\', "#.... #.... .#... ..#.. ...#. ....# ....# ..... ....."),
    make(U']', "### ..# ..# ..# ..# ..# ### ... ..."),
    make(U'^', "..#.. .#.#. #...# ..... ..... ..... ..... ..... ....."),
    make(U'_', "..... ..... ..... ..... ..... ..... ##### ..... ....."),
    make(U'`', "#. .# .. .. .. .. .. .. .."),
    make(U'a', "..... ..... .###. ....# .#### #...# .#### ..... ....."),
    make(U'b', "#.... #.... ####. #...# #...# #...# ####. ..... ....."),
    make(U'c', ".... .... .### #... #... #... .### .... ...."),
    make(U'd', "....# ....# .#### #...# #...# #...# .#### ..... ....."),
    make(U'e', "..... ..... .###. #...# ##### #.... .###. ..... ....."),
    make(U'f', "..## .#.. #### .#.. .#.. .#.. .#.. .... ...."),
    make(U'g', "..... ..... .#### #...# #...# #...# .#### ....# .###."),
    make(U'h', "#.... #.... ####. #...# #...# #...# #...# ..... ....."),
    make(U'i', "# . # # # # # . ."),
    make(U'j', "..# ... ..# ..# ..# ..# ..# ..# ##."),
    make(U'k', "#... #... #..# #.#. ##.. #.#. #..# .... ...."),
    make(U'l', "#. #. #. #. #. #. .# .. .."),
    make(U'm', "..... ..... ##.#. #.#.# #.#.# #.#.# #.#.# ..... ....."),
    make(U'n', "..... ..... ####. #...# #...# #...# #...# ..... ....."),
    make(U'o', "..... ..... .###. #...# #...# #...# .###. ..... ....."),
    make(U'p', "..... ..... ####. #...# #...# #...# ####. #.... #...."),
    make(U'q', "..... ..... .#### #...# #...# #...# .#### ....# ....#"),
    make(U'r', ".... .... #.## ##.. #... #... #... .... ...."),
    make(U's', ".... .... .### #... .##. ...# ###. .... ...."),
    make(U't', ".#. .#. ### .#. .#. .#. ..# ... ..."),
    make(U'u', "..... ..... #...# #...# #...# #...# .#### ..... ....."),
    make(U'v', "..... ..... #...# #...# #...# .#.#. ..#.. ..... ....."),
    make(U'w', "..... ..... #...# #...# #.#.# #.#.# .#.#. ..... ....."),
    make(U'x', "..... ..... #...# .#.#. ..#.. .#.#. #...# ..... ....."),
    make(U'y', "..... ..... #...# #...# #...# #...# .#### ....# .###."),
    make(U'z', "..... ..... ##### ...#. ..#.. .#... ##### ..... ....."),
    make(U'{', "..# .#. .#. #.. .#. .#. ..# ... ..."),
    make(U'|', "# # # # # # # # ."),
    make(U'}', "#.. .#. .#. ..# .#. .#. #.. ... ..."),
    make(U'~', "..... ..... .##.# #.##. ..... ..... ..... ..... ....."),
    // Deutsch
    make(U'Ä', ".#.#. ..... .###. #...# #...# ##### #...# #...# #...# ..... ....."),
    make(U'Ö', ".#.#. ..... .###. #...# #...# #...# #...# #...# .###. ..... ....."),
    make(U'Ü', ".#.#. ..... #...# #...# #...# #...# #...# #...# .###. ..... ....."),
    make(U'ä', ".#.#. ..... .###. ....# .#### #...# .#### ..... ....."),
    make(U'ö', ".#.#. ..... .###. #...# #...# #...# .###. ..... ....."),
    make(U'ü', ".#.#. ..... #...# #...# #...# #...# .#### ..... ....."),
    make(U'ß', ".##.. #..#. #..#. #.##. #...# #...# #.##. ..... ....."),
    make(U'„', "... ... ... ... ... ... #.# #.# ..."),
    make(U'“', "#.# #.# ... ... ... ... ... ... ..."),
    make(U'”', "#.# #.# ... ... ... ... ... ... ..."),
    make(U'‚', ". . . . . . # # ."),
    make(U'‘', "# # . . . . . . ."),
    make(U'’', "# # . . . . . . ."),
    make(U'–', ".... .... .... #### .... .... .... .... ...."),
    make(U'—', "...... ...... ...... ###### ...... ...... ...... ...... ......"),
    make(U'…', "..... ..... ..... ..... ..... ..... #.#.# ..... ....."),
    make(U'°', ".#. #.# .#. ... ... ... ... ... ..."),
    make(U'·', ". . . # . . . . ."),
    make(U'×', "..... ..... #...# .#.#. ..#.. .#.#. #...# ..... ....."),
    make(U'€', "..### .#... ####. .#... ####. .#... ..### ..... ....."),
    make(U'→', "..... ..#.. ...#. ##### ...#. ..#.. ..... ..... ....."),
    make(U'←', "..... ..#.. .#... ##### .#... ..#.. ..... ..... ....."),
    make(U'✓', "..... ....# ....# ...#. #.#.. .#... ..... ..... ....."),
    make(U'●', "..... .###. ##### ##### ##### .###. ..... ..... ....."),
    // Ersatz für unbekannte Zeichen
    make(U'�', "##### #...# #...# #...# #...# #...# ##### ..... ....."),
};

constexpr int kAsciiCount = 95;
constexpr size_t kGlyphCount = sizeof(kGlyphs) / sizeof(kGlyphs[0]);
static_assert(kGlyphs[kAsciiCount - 1].cp == U'~', "ASCII-Tabelle unvollständig");

const Glyph& find(char32_t cp) {
    if (cp >= 32 && cp < 32 + kAsciiCount) return kGlyphs[cp - 32];
    for (size_t i = kAsciiCount; i < kGlyphCount; ++i)
        if (kGlyphs[i].cp == cp) return kGlyphs[i];
    return kGlyphs[kGlyphCount - 1];
}

// Nächstes Zeichen aus UTF-8; ungültige Bytes werden als Ersatzzeichen gelesen
char32_t next_cp(std::string_view s, size_t& i) {
    auto b = static_cast<unsigned char>(s[i++]);
    if (b < 0x80) return b;
    int extra = (b & 0xE0) == 0xC0 ? 1 : (b & 0xF0) == 0xE0 ? 2 : (b & 0xF8) == 0xF0 ? 3 : -1;
    if (extra < 0) return U'�';
    char32_t cp = b & (0x3F >> extra);
    for (int k = 0; k < extra; ++k) {
        if (i >= s.size() || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) return U'�';
        cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
    }
    return cp;
}

void plot(Canvas& c, int x, int y, int scale, uint8_t color) {
    if (scale == 1) {
        if (x >= 0 && y >= 0 && x < c.width() && y < c.height()) c.row(y)[x] = color;
    } else {
        c.fill_rect(x, y, scale, scale, color);
    }
}

int draw_glyph(Canvas& c, int x, int y, const Glyph& g, uint8_t color, int scale) {
    for (int r = 0; r < kCellHeight; ++r) {
        uint8_t bits = g.rows[r];
        if (!bits) continue;
        for (int col = 0; col < g.width; ++col)
            if (bits & (1u << (g.width - 1 - col))) plot(c, x + col * scale, y + r * scale, scale, color);
    }
    return g.width * scale;
}

// Breite eines Wortes ab i bis zum nächsten Leerzeichen/Zeilenumbruch (ohne Abstand danach)
int word_width(std::string_view s, size_t i, int scale, size_t* end) {
    int w = 0;
    bool first = true;
    while (i < s.size() && s[i] != ' ' && s[i] != '\n') {
        const Glyph& g = find(next_cp(s, i));
        w += (first ? 0 : scale) + g.width * scale;
        first = false;
    }
    *end = i;
    return w;
}

// Gemeinsamer Umbruch für Zählen und Zeichnen
template <typename Line>
int wrap(std::string_view s, int max_width, int scale, Line&& line) {
    const int space = (find(U' ').width + 1) * scale;
    int lines = 0;
    size_t i = 0;
    while (i <= s.size()) {
        size_t start = i, line_end = i;
        int w = 0;
        bool any = false;
        while (i < s.size() && s[i] != '\n') {
            size_t wend;
            int ww = word_width(s, i, scale, &wend);
            int add = any ? space + scale + ww : ww;  // Abstand + Leerzeichen + Abstand + Wort
            if (any && w + add > max_width) break;
            w += add;
            any = true;
            line_end = wend;
            i = wend;
            while (i < s.size() && s[i] == ' ') ++i;
        }
        line(lines++, s.substr(start, line_end - start));
        if (i < s.size() && s[i] == '\n') {
            ++i;
        } else if (i >= s.size()) {
            break;
        } else if (!any) {
            // Wort breiter als die Zeile: trotzdem setzen, damit es weitergeht
            size_t wend;
            word_width(s, i, scale, &wend);
            i = wend;
        }
    }
    return lines;
}

}  // namespace

int text_width(std::string_view s, int scale) {
    int w = 0;
    bool first = true;
    for (size_t i = 0; i < s.size();) {
        const Glyph& g = find(next_cp(s, i));
        w += (first ? 0 : scale) + g.width * scale;
        first = false;
    }
    return w;
}

int draw(Canvas& canvas, int x, int y, std::string_view s, uint8_t color, int scale) {
    const int x0 = x;
    bool first = true;
    for (size_t i = 0; i < s.size();) {
        const Glyph& g = find(next_cp(s, i));
        if (!first) x += scale;
        x += draw_glyph(canvas, x, y, g, color, scale);
        first = false;
    }
    return x - x0;
}

void draw_centered(Canvas& canvas, int cx, int y, std::string_view s, uint8_t color, int scale) {
    draw(canvas, cx - text_width(s, scale) / 2, y, s, color, scale);
}

int draw_wrapped(Canvas& canvas, int x, int y, int max_width, std::string_view s, uint8_t color, int scale,
                 int line_height) {
    return wrap(s, max_width, scale, [&](int n, std::string_view line) {
        draw(canvas, x, y + n * (line_height > 0 ? line_height : kLineHeight * scale), line, color, scale);
    });
}

int wrapped_lines(int max_width, std::string_view s, int scale) {
    return wrap(s, max_width, scale, [](int, std::string_view) {});
}

}  // namespace pixelia::font
