#pragma once
// System-Schrift: 5×7 Pixel, proportional, mit Kleinbuchstaben, Umlauten, ß und deutschen Anführungszeichen.
// Für Systemanzeigen (Setup-Modus, Update, Meldungen). Sie steht im Code und geht immer.
// Seit 0.5.6 weitere Schnitte als Grafik aus dem Paket „system“ (Grundausstattung, assets/system, erzeugt mit
// tools/font_png.py): fett (Namen, Titel, Uhr) und klein (3×5, Großbuchstaben). Fehlt das Paket, zeichnen alle
// Schnitte in der Systemschrift. Format: ein Bild je Zeichen in der Reihenfolge der Tabelle in font.cpp, unterste
// Zeile = Breite (docs/STIL.md Kapitel 9).
//
// Eine Zelle ist 11 Zeilen hoch: Zeile 0–1 Punkte der großen Umlaute, 2–8 Großbuchstaben (Grundlinie unter
// Zeile 8), 9–10 Unterlängen. Text wird als UTF-8 übergeben; unbekannte Zeichen erscheinen als Kästchen.

#include <cstdint>
#include <string>
#include <string_view>

#include "pixelia/engine/canvas.h"

namespace pixelia {
class AssetStore;
}

namespace pixelia::font {

enum class Style : uint8_t { Normal, Bold, Small };

inline constexpr int kCellHeight = 11;
inline constexpr int kLineHeight = 12;  // Zeilenabstand bei Vergrößerung 1
inline constexpr int kCapTop = 2;       // erste Zeile der Großbuchstaben in der Zelle
inline constexpr int kCapHeight = 7;

// Schnitte fett und klein aus dem Paket „system“ übernehmen (lädt es als Grundausstattung); false = fehlt
bool load_faces(AssetStore& store);
bool has_face(Style style);
// Zeilenabstand und erste Zeile der Großbuchstaben in der Zelle (klein: 8 bzw. 1)
int line_height(Style style, int scale = 1);
int cap_top(Style style);
int cap_height(Style style);

// Breite in Pixeln (inklusive Abstand zwischen den Zeichen, ohne Abstand nach dem letzten)
int text_width(std::string_view utf8, int scale = 1, Style style = Style::Normal);

// Text auf max_width kürzen, abgeschnitten wird mit „…“ (zeichenweise, UTF-8-sicher)
std::string ellipsize(std::string_view utf8, int max_width, int scale = 1, Style style = Style::Normal);

// Zeichnet ab (x, y) = linke obere Ecke der Zelle, schneidet am Rand ab. Gibt die Breite zurück.
int draw(Canvas& canvas, int x, int y, std::string_view utf8, uint8_t color, int scale = 1,
         Style style = Style::Normal);

// Wie draw, aber horizontal zentriert um cx
void draw_centered(Canvas& canvas, int cx, int y, std::string_view utf8, uint8_t color, int scale = 1,
                   Style style = Style::Normal);

// Bricht an Leerzeichen um (und hart an '\n'). line_height in Pixeln, 0 = kLineHeight × scale.
// Gibt die Zahl der Zeilen zurück.
// max_lines > 0: weitere Zeilen werden weggelassen.
int draw_wrapped(Canvas& canvas, int x, int y, int max_width, std::string_view utf8, uint8_t color, int scale = 1,
                 int line_height = 0, int max_lines = 0, Style style = Style::Normal);

// Zahl der Zeilen, die draw_wrapped bräuchte; widest = Breite der breitesten Zeile (optional)
int wrapped_lines(int max_width, std::string_view utf8, int scale = 1, int* widest = nullptr,
                  Style style = Style::Normal);

}  // namespace pixelia::font
