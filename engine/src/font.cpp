#include "pixelia/engine/font.h"

#include <algorithm>
#include <cstddef>

#include "pixelia/engine/assets.h"
#include "pixelia/engine/memory.h"

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

size_t index_of(char32_t cp) {
    if (cp >= 32 && cp < 32 + kAsciiCount) return cp - 32;
    for (size_t i = kAsciiCount; i < kGlyphCount; ++i)
        if (kGlyphs[i].cp == cp) return i;
    return kGlyphCount - 1;
}

// Schnitt aus dem Paket „system“ (seit 0.5.6): Breite und Zeilen je Zeichen in der Reihenfolge von kGlyphs,
// im Grafik-Speicher. Die Systemschrift (Normal) steht in kGlyphs und braucht nichts davon.
struct Face {
    int cell_h = 0, cap_top = 0, cap_h = 0, line_h = 0;
    uint8_t* widths = nullptr;
    uint16_t* rows = nullptr;  // je Zeichen cell_h Zeilen, Bit (Breite − 1) = linke Spalte
};
Face s_faces[3];

const Face* face_of(Style st) {
    const Face& f = s_faces[static_cast<int>(st)];
    return st != Style::Normal && f.rows ? &f : nullptr;
}

struct GlyphRef {
    int width;
    const uint8_t* r8;    // Systemschrift
    const uint16_t* r16;  // Schnitt aus dem Paket
    int rows;
};

GlyphRef find(char32_t cp, Style st) {
    const size_t i = index_of(cp);
    if (const Face* f = face_of(st)) return {f->widths[i], nullptr, f->rows + i * f->cell_h, f->cell_h};
    return {kGlyphs[i].width, kGlyphs[i].rows, nullptr, kCellHeight};
}

bool build_face(Face& f, const SpriteSheet& sheet, int cap_top, int cap_h) {
    if (!sheet.pixels || sheet.frames != static_cast<int>(kGlyphCount) || sheet.height < 2 || sheet.width > 16)
        return false;
    const int cell = sheet.height - 1;
    auto* mem = static_cast<uint8_t*>(asset_alloc(kGlyphCount * (1 + cell * 2) + 2));
    if (!mem) return false;
    f.widths = mem;
    f.rows = reinterpret_cast<uint16_t*>(mem + ((kGlyphCount + 1) & ~size_t{1}));
    for (size_t i = 0; i < kGlyphCount; ++i) {
        const Sprite fr = sheet.frame(static_cast<int>(i));
        int w = 0;
        while (w < fr.width && fr.pixels[cell * fr.width + w] == 2) ++w;  // Breite: Zeile unter der Zelle
        f.widths[i] = static_cast<uint8_t>(w);
        for (int r = 0; r < cell; ++r) {
            uint16_t bits = 0;
            for (int x = 0; x < w; ++x) bits = static_cast<uint16_t>(bits << 1 | (fr.pixels[r * fr.width + x] == 1));
            f.rows[i * cell + r] = bits;
        }
    }
    f.cell_h = cell;
    f.cap_top = cap_top;
    f.cap_h = cap_h;
    f.line_h = cell + 1;
    return true;
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

int draw_glyph(Canvas& c, int x, int y, const GlyphRef& g, uint8_t color, int scale) {
    for (int r = 0; r < g.rows; ++r) {
        const unsigned bits = g.r8 ? g.r8[r] : g.r16[r];
        if (!bits) continue;
        for (int col = 0; col < g.width; ++col)
            if (bits & (1u << (g.width - 1 - col))) plot(c, x + col * scale, y + r * scale, scale, color);
    }
    return g.width * scale;
}

// Breite eines Wortes ab i bis zum nächsten Leerzeichen/Zeilenumbruch (ohne Abstand danach)
int word_width(std::string_view s, size_t i, int scale, Style st, size_t* end) {
    int w = 0;
    bool first = true;
    while (i < s.size() && s[i] != ' ' && s[i] != '\n') {
        const GlyphRef g = find(next_cp(s, i), st);
        w += (first ? 0 : scale) + g.width * scale;
        first = false;
    }
    *end = i;
    return w;
}

// Gemeinsamer Umbruch für Zählen und Zeichnen
template <typename Line>
int wrap(std::string_view s, int max_width, int scale, Style st, Line&& line) {
    const int space = (find(U' ', st).width + 1) * scale;
    int lines = 0;
    size_t i = 0;
    while (i <= s.size()) {
        size_t start = i, line_end = i;
        int w = 0;
        bool any = false;
        while (i < s.size() && s[i] != '\n') {
            size_t wend;
            int ww = word_width(s, i, scale, st, &wend);
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
            word_width(s, i, scale, st, &wend);
            i = wend;
        }
    }
    return lines;
}

}  // namespace

bool load_faces(AssetStore& store) {
    if (s_faces[1].rows && s_faces[2].rows) return true;
    if (!store.load_base("system")) return false;
    ImageAsset img;
    bool ok = true;
    if (!s_faces[1].rows) ok &= store.image(asset_id("schrift/fett"), &img) && build_face(s_faces[1], img.sheet, kCapTop, kCapHeight);
    if (!s_faces[2].rows) ok &= store.image(asset_id("schrift/klein"), &img) && build_face(s_faces[2], img.sheet, 1, 5);
    return ok;
}

bool has_face(Style style) { return style == Style::Normal || face_of(style); }

int line_height(Style style, int scale) {
    const Face* f = face_of(style);
    return (f ? f->line_h : kLineHeight) * scale;
}
int cap_top(Style style) {
    const Face* f = face_of(style);
    return f ? f->cap_top : kCapTop;
}
int cap_height(Style style) {
    const Face* f = face_of(style);
    return f ? f->cap_h : kCapHeight;
}

int text_width(std::string_view s, int scale, Style style) {
    int w = 0;
    bool first = true;
    for (size_t i = 0; i < s.size();) {
        const GlyphRef g = find(next_cp(s, i), style);
        w += (first ? 0 : scale) + g.width * scale;
        first = false;
    }
    return w;
}

std::string ellipsize(std::string_view s, int max_width, int scale, Style style) {
    if (text_width(s, scale, style) <= max_width) return std::string(s);
    std::string r(s);
    while (!r.empty()) {
        // letztes Zeichen (inkl. UTF-8-Folgebytes) entfernen
        size_t cut = r.size() - 1;
        while (cut > 0 && (static_cast<unsigned char>(r[cut]) & 0xC0) == 0x80) --cut;
        r.erase(cut);
        while (!r.empty() && r.back() == ' ') r.pop_back();
        if (text_width(r + "…", scale, style) <= max_width) break;
    }
    return r + "…";
}

int draw(Canvas& canvas, int x, int y, std::string_view s, uint8_t color, int scale, Style style) {
    const int x0 = x;
    bool first = true;
    for (size_t i = 0; i < s.size();) {
        const GlyphRef g = find(next_cp(s, i), style);
        if (!first) x += scale;
        x += draw_glyph(canvas, x, y, g, color, scale);
        first = false;
    }
    return x - x0;
}

void draw_centered(Canvas& canvas, int cx, int y, std::string_view s, uint8_t color, int scale, Style style) {
    draw(canvas, cx - text_width(s, scale, style) / 2, y, s, color, scale, style);
}

int draw_wrapped(Canvas& canvas, int x, int y, int max_width, std::string_view s, uint8_t color, int scale,
                 int line_h, int max_lines, Style style) {
    const int step = line_h > 0 ? line_h : line_height(style, scale);
    return wrap(s, max_width, scale, style, [&](int n, std::string_view line) {
        if (max_lines > 0 && n >= max_lines) return;
        draw(canvas, x, y + n * step, line, color, scale, style);
    });
}

int wrapped_lines(int max_width, std::string_view s, int scale, int* widest, Style style) {
    int w = 0;
    int n = wrap(s, max_width, scale, style,
                 [&](int, std::string_view line) { w = std::max(w, text_width(line, scale, style)); });
    if (widest) *widest = w;
    return n;
}

}  // namespace pixelia::font
