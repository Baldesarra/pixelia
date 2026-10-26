#!/usr/bin/env python3
"""Schriften und Symbole der Einblendungen als PNG-Streifen (seit 0.5.6) für das Paket „system“.

Die Systemschrift 5×7 steht im Code (engine/src/font.cpp) und bleibt die Grundlage. Daraus entstehen hier:
- assets/system/schrift-fett.png: fette Fassung (jede Spalte nach rechts verbreitert), für Namen, Titel und die Uhr
- assets/system/schrift-klein.png: kleine 3×5-Schrift (Großbuchstaben, Umlaute), Glyphen unten in KLEIN
- assets/system/symbole.png: Symbole der Karten und Kurzmeldungen (9 × 9), Bilder unten in SYMBOLE

Format einer Schrift (docs/STIL.md Kapitel 9): ein Bild je Zeichen in der Reihenfolge der Tabelle in font.cpp
(ASCII 32–126, dann Sonderzeichen), Bildhöhe = Zellhöhe + 1. Die unterste Zeile markiert die Breite des Zeichens
(Farbe „breite“ in den Spalten 0 … Breite − 1), darüber die Glyphe in der Farbe „tinte“. So lassen sich die PNGs
in Aseprite oder GIMP mit assets/paletten/system.gpl direkt weiterzeichnen; dieses Werkzeug nur nach Änderungen an
font.cpp oder den Tabellen hier erneut ausführen (überschreibt die PNGs).

Aufruf:  python tools/font_png.py
"""
import re
import struct
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "engine/src/font.cpp"
DST = ROOT / "assets/system"
MAKE_RE = re.compile(r"make\(U'(\\.|[^'])', \"([#. ]+)\"\)")
CELL = 11          # Zellhöhe der 5×7 (Zeile 0–1 Umlautpunkte, 2–8 Großbuchstaben, 9–10 Unterlängen)
FRAME_W = 10       # Bildbreite je Zeichen (breiteste Glyphe 6 + fett 1, Luft für eigene Zeichen)
SMALL_CELL = 7     # klein: Zeile 0 Umlautpunkte, 1–5 Glyphe, 6 frei
INK, WIDTH = 1, 2  # Indizes in system.gpl

# Kleine Schrift 3×5 (Großbuchstaben; Kleinbuchstaben werden auf Großbuchstaben abgebildet)
KLEIN = {
    " ": "... ... ... ... ...",
    "!": "# # # . #", "\"": "#.# #.# ... ... ...", "#": "#.# ### #.# ### #.#", "%": "#.# ..# .#. #.. #.#",
    "&": ".#. #.# .#. #.# .##", "'": "# # . . .", "(": ".# #. #. #. .#", ")": "#. .# .# .# #.",
    "*": "... #.# .#. #.# ...", "+": "... .#. ### .#. ...", ",": ". . . # #", "-": "... ... ### ... ...",
    ".": ". . . . #", "/": "..# ..# .#. #.. #..", ":": ". # . # .", ";": ". # . # #",
    "<": "..# .#. #.. .#. ..#", "=": "... ### ... ### ...", ">": "#.. .#. ..# .#. #..", "?": "##. ..# .#. ... .#.",
    "@": ".#. #.# ### #.. .##", "[": "## #. #. #. ##", "]": "## .# .# .# ##", "_": "... ... ... ... ###",
    "0": "### #.# #.# #.# ###", "1": ".#. ##. .#. .#. ###", "2": "##. ..# .#. #.. ###", "3": "##. ..# .#. ..# ##.",
    "4": "#.# #.# ### ..# ..#", "5": "### #.. ##. ..# ##.", "6": ".## #.. ### #.# ###", "7": "### ..# .#. .#. .#.",
    "8": "### #.# ### #.# ###", "9": "### #.# ### ..# ##.",
    "A": ".#. #.# ### #.# #.#", "B": "##. #.# ##. #.# ##.", "C": ".## #.. #.. #.. .##", "D": "##. #.# #.# #.# ##.",
    "E": "### #.. ##. #.. ###", "F": "### #.. ##. #.. #..", "G": ".## #.. #.# #.# .##", "H": "#.# #.# ### #.# #.#",
    "I": "### .#. .#. .#. ###", "J": "..# ..# ..# #.# .#.", "K": "#.# #.# ##. #.# #.#", "L": "#.. #.. #.. #.. ###",
    "M": "#...# ##.## #.#.# #...# #...#", "N": "#..# ##.# #.## #..# #..#", "O": ".#. #.# #.# #.# .#.",
    "P": "##. #.# ##. #.. #..", "Q": ".#. #.# #.# #.# .##", "R": "##. #.# ##. #.# #.#", "S": ".## #.. .#. ..# ##.",
    "T": "### .#. .#. .#. .#.", "U": "#.# #.# #.# #.# ###", "V": "#.# #.# #.# #.# .#.",
    "W": "#...# #...# #.#.# ##.## #...#", "X": "#.# #.# .#. #.# #.#", "Y": "#.# #.# .#. .#. .#.",
    "Z": "### ..# .#. #.. ###",
    "ß": "##. #.# ##. #.# ##.", "°": "### #.# ### ... ...", "·": ". . # . .", "…": "..... ..... ..... ..... #.#.#",
    "–": "... ... ### ... ...", "—": "..... ..... ##### ..... .....", "„": "... ... ... #.# #.#",
    "“": "#.# #.# ... ... ...", "”": "#.# #.# ... ... ...", "‚": ". . . # #", "‘": "# # . . .", "’": "# # . . .",
    "€": ".## ### #.. ### .##", "→": ".... ..#. #### ..#. ....", "←": ".... .#.. #### .#.. ....",
    "✓": "... ..# ..# #.# .#.", "●": "... .#. ### .#. ...", "×": "... #.# .#. #.# ...", "�": "### #.# #.# #.# ###",
    "|": "# # # # #", "\\": "#.. #.. .#. ..# ..#", "^": ".#. #.# ... ... ...", "`": "#. .# .. .. ..",
    "{": ".## .#. ##. .#. .##", "}": "##. .#. .## .#. ##.", "~": "... .## ##. ... ...", "$": ".## ##. .#. .## ##.",
}
UMLAUT = {"Ä": "A", "Ö": "O", "Ü": "U"}

# Symbole 9 × 9 („#“ = tinte), Reihenfolge = Bildnummer (engine/overlay.h, ui::Symbol)
SYMBOLE = [
    ("gedanke", ".#####... #.....#.. #.....#.. #.....#.. .#####... ......... ...##.... ......... .##......"),
    ("folgen", "......... ..###.... .#...#... #..#..#.. #.###.#.. #..#..#.. .#...#... ..###.... ........."),
    ("haus", "....#.... ...#.#... ..#...#.. .#.....#. ######### .#.....#. .#.##..#. .#.##..#. .#######."),
    ("fund", "....#.... ....#.... ..#####.. ...###... ..##.##.. .#.....#. ......... ......... ........."),
    ("neu", "......... ...###... ..#####.. .#######. .#######. .#######. ..#####.. ...###... ........."),
    ("ort", "...###... ..#...#.. ..#.#.#.. ..#...#.. ...#.#... ...#.#... ....#.... ......... ........."),
    # seit 0.5.7: Jahreszeiten neben der Pixel-Uhr (Keimling, Sonne, Blatt, Schneeflocke)
    ("fruehling", "......... .##...##. #..#.#..# #...#...# .#..#..#. ..#.#.#.. ....#.... ....#.... ..#####.."),
    ("sommer", "....#.... .#..#..#. ..#...#.. ...###... ##.###.## ...###... ..#...#.. .#..#..#. ....#...."),
    ("herbst", "......##. ....####. ...#####. ..######. ..#####.. ..####... .#.##.... #........ ........."),
    ("winter", "....#.... .#..#..#. ..#.#.#.. ...###... ######### ...###... ..#.#.#.. .#..#..#. ....#...."),
]


def glyphs_5x7():
    out = []
    for m in MAKE_RE.finditer(SRC.read_text(encoding="utf-8")):
        ch = m.group(1)[-1]
        rows = m.group(2).split(" ")
        if len(rows) == 9:
            rows = ["." * len(rows[0])] * 2 + rows
        out.append((ch, rows))
    return out


def bold(rows):
    """Fett: jede gesetzte Spalte auch eine nach rechts (Breite + 1)."""
    w = len(rows[0])
    return ["".join("#" if (x < w and r[x] == "#") or (x > 0 and r[x - 1] == "#") else "." for x in range(w + 1))
            for r in rows]


def small(ch):
    """Glyphe der kleinen Schrift: Zeile 0 Umlautpunkte, 1–5 Zeichen, 6 frei."""
    up = ch.upper() if ch not in "ß" else ch
    base = UMLAUT.get(up)
    art = KLEIN.get(base or up) or KLEIN["�"]
    rows = art.split(" ")
    w = len(rows[0])
    dots = ("#.#" + "." * (w - 3)) if base else "." * w
    return [dots] + rows + ["." * w]


def write_png(path: Path, w: int, h: int, px: bytearray, pal):
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    raw = b"".join(b"\0" + bytes(px[y * w:(y + 1) * w]) for y in range(h))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 3, 0, 0, 0))
    png += chunk(b"PLTE", b"".join(bytes(c) for c in pal)) + chunk(b"tRNS", b"\0")
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    path.write_bytes(png)


def strip(cells, cell_h, frame_w, pal, path, width_row=True):
    h = cell_h + (1 if width_row else 0)
    w = frame_w * len(cells)
    px = bytearray(w * h)
    for i, rows in enumerate(cells):
        cw = len(rows[0])
        if cw > frame_w:
            sys.exit(f"{path.name}: Zeichen {i} ist {cw} breit, Bild nur {frame_w}")
        for y, r in enumerate(rows):
            for x, c in enumerate(r):
                if c == "#":
                    px[y * w + i * frame_w + x] = INK
        if width_row:
            for x in range(cw):
                px[cell_h * w + i * frame_w + x] = WIDTH
    write_png(path, w, h, px, pal)
    print(f"{path.relative_to(ROOT)}: {len(cells)} Bilder à {frame_w} × {h}")


def main():
    pal = [(0, 0, 0), (255, 255, 255), (230, 60, 60)]
    g = glyphs_5x7()
    DST.mkdir(parents=True, exist_ok=True)
    strip([bold(r) for _, r in g], CELL, FRAME_W, pal, DST / "schrift-fett.png")
    strip([small(ch) for ch, _ in g], SMALL_CELL, 6, pal, DST / "schrift-klein.png")
    strip([s.split(" ") for _, s in SYMBOLE], 9, 9, pal, DST / "symbole.png", width_row=False)
    print(f"{len(g)} Zeichen in der Reihenfolge von font.cpp; Symbole: {', '.join(n for n, _ in SYMBOLE)}")


if __name__ == "__main__":
    main()
