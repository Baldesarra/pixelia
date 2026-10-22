#!/usr/bin/env python3
"""Pixelschrift des Displays als Webschrift (seit 0.5.4, für das Design „Pixelia“ der Weboberfläche).

Liest die Glyphen aus engine/src/font.cpp (make(U'x', "#.# ...")) und baut daraus eine kleine TrueType-Schrift:
jedes Pixel ein Quadrat, waagerechte Läufe und gleiche Läufe darunter zu Rechtecken zusammengefasst. 1 Pixel =
100 Einheiten, 1200 Einheiten pro em: Bei font-size 12px ist ein Schriftpixel 1px groß, bei 24px 2px (scharf nur bei
Vielfachen von 12). Abstand wie auf dem Display: Breite + 1 Pixel.
Ergebnis: web/src/assets/pixelia-pixel.woff2 (im Repo, die CI braucht fonttools daher nicht).

Aufruf:  pip install fonttools brotli
         python tools/pixel_font.py
"""
import re
from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "engine/src/font.cpp"
DST = ROOT / "web/src/assets/pixelia-pixel.woff2"
PX, UPM, CELL = 100, 1200, 11
BASE = 9  # Grundlinie unter Zeile 9 der Zelle (Großbuchstaben Zeile 2–8)

MAKE_RE = re.compile(r"make\(U'(\\.|[^'])', \"([#. ]+)\"\)")


def glyphs():
    for m in MAKE_RE.finditer(SRC.read_text(encoding="utf-8")):
        ch = m.group(1)[-1]  # \' und \\ ohne Rückstrich
        rows = m.group(2).split(" ")
        if len(rows) == 9:
            rows = ["." * len(rows[0])] * 2 + rows  # 9 Zeilen = Zeile 2–10 der Zelle
        yield ch, rows


def rects(rows):
    """Pixel zu Rechtecken (x, y, w, h) in Zellzeilen zusammenfassen."""
    todo = [[c == "#" for c in r] for r in rows]
    out = []
    for y, row in enumerate(todo):
        x = 0
        while x < len(row):
            if not row[x]:
                x += 1
                continue
            w = 1
            while x + w < len(row) and row[x + w]:
                w += 1
            h = 1
            while y + h < len(todo) and all(todo[y + h][x:x + w]) and (x + w == len(row) or not todo[y + h][x + w]) \
                    and (x == 0 or not todo[y + h][x - 1]):
                h += 1
            for yy in range(y, y + h):
                for xx in range(x, x + w):
                    todo[yy][xx] = False
            out.append((x, y, w, h))
            x += w
    return out


def draw(rows):
    pen = TTGlyphPen(None)
    for x, y, w, h in rects(rows):
        x0, x1 = x * PX, (x + w) * PX
        y1, y0 = (BASE - y) * PX, (BASE - y - h) * PX
        pen.moveTo((x0, y0))  # im Uhrzeigersinn (TrueType-Außenkontur)
        pen.lineTo((x0, y1))
        pen.lineTo((x1, y1))
        pen.lineTo((x1, y0))
        pen.closePath()
    return pen.glyph()


def main() -> None:
    order, cmap, metrics, shapes = [".notdef"], {}, {".notdef": (600, 0)}, {".notdef": TTGlyphPen(None).glyph()}
    for ch, rows in glyphs():
        name = f"u{ord(ch):04X}"
        order.append(name)
        cmap[ord(ch)] = name
        shapes[name] = draw(rows)
        metrics[name] = ((len(rows[0]) + 1) * PX, 0)
    cmap[0xA0] = cmap[0x20]  # geschütztes Leerzeichen
    fb = FontBuilder(UPM, isTTF=True)
    fb.setupGlyphOrder(order)
    fb.setupCharacterMap(cmap)
    fb.setupGlyf(shapes)
    fb.setupHorizontalMetrics(metrics)
    fb.setupHorizontalHeader(ascent=1000, descent=-200)
    fb.setupOS2(sTypoAscender=1000, sTypoDescender=-200, sTypoLineGap=0, usWinAscent=1000, usWinDescent=200,
                sxHeight=500, sCapHeight=700)
    fb.setupNameTable({"familyName": "Pixelia Pixel", "styleName": "Regular"})
    fb.setupPost()
    fb.font.flavor = "woff2"
    fb.save(DST)
    print(f"{DST.relative_to(ROOT)}: {len(order) - 1} Zeichen, {DST.stat().st_size} Bytes")


if __name__ == "__main__":
    main()
