#pragma once
// System-Schrift: 5×7 Pixel, proportional, mit Kleinbuchstaben, Umlauten, ß und deutschen Anführungszeichen.
// Für Systemanzeigen (Setup-Modus, Update, Meldungen). Die gestalteten Pixel-Schriften der Welten folgen in 0.5.x.
//
// Eine Zelle ist 11 Zeilen hoch: Zeile 0–1 Punkte der großen Umlaute, 2–8 Großbuchstaben (Grundlinie unter
// Zeile 8), 9–10 Unterlängen. Text wird als UTF-8 übergeben; unbekannte Zeichen erscheinen als Kästchen.

#include <cstdint>
#include <string_view>

#include "pixelia/engine/canvas.h"

namespace pixelia::font {

inline constexpr int kCellHeight = 11;
inline constexpr int kLineHeight = 12;  // Zeilenabstand bei Vergrößerung 1
inline constexpr int kCapTop = 2;       // erste Zeile der Großbuchstaben in der Zelle
inline constexpr int kCapHeight = 7;

// Breite in Pixeln (inklusive Abstand zwischen den Zeichen, ohne Abstand nach dem letzten)
int text_width(std::string_view utf8, int scale = 1);

// Zeichnet ab (x, y) = linke obere Ecke der Zelle, schneidet am Rand ab. Gibt die Breite zurück.
int draw(Canvas& canvas, int x, int y, std::string_view utf8, uint8_t color, int scale = 1);

// Wie draw, aber horizontal zentriert um cx
void draw_centered(Canvas& canvas, int cx, int y, std::string_view utf8, uint8_t color, int scale = 1);

// Bricht an Leerzeichen um (und hart an '\n'). line_height in Pixeln, 0 = kLineHeight × scale.
// Gibt die Zahl der Zeilen zurück.
int draw_wrapped(Canvas& canvas, int x, int y, int max_width, std::string_view utf8, uint8_t color, int scale = 1,
                 int line_height = 0);

// Zahl der Zeilen, die draw_wrapped bräuchte
int wrapped_lines(int max_width, std::string_view utf8, int scale = 1);

}  // namespace pixelia::font
