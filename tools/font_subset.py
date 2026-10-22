#!/usr/bin/env python3
"""Roboto Flex für die Weboberfläche reduzieren (einmalig bzw. bei neuen Zeichen ausführen).

Quelle: @fontsource-variable/roboto-flex (OFL 1.1) aus web/node_modules (npm ci in web/).
Behalten werden nur die Achsen wght (400–800) und wdth (100–120, seit 0.5.4 enger: 33 → 21 KB); opsz und slnt
werden festgelegt.
Zeichen: druckbares ASCII plus deutsche Sonderzeichen und ein paar Satzzeichen (ZEICHEN unten).
Ergebnis: web/src/assets/roboto-flex-pixelia.woff2 (im Repo, die CI braucht fonttools daher nicht).

Aufruf:  pip install fonttools brotli
         python tools/font_subset.py
"""
from pathlib import Path

from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "web/node_modules/@fontsource-variable/roboto-flex/files/roboto-flex-latin-standard-normal.woff2"
DST = ROOT / "web/src/assets/roboto-flex-pixelia.woff2"

# Neue Zeichen in der Oberfläche? Hier ergänzen und das Skript neu ausführen.
ZEICHEN = "".join(chr(c) for c in range(0x20, 0x7F)) + "ÄÖÜäöüß°–—„“”‚‘’…·•×€µ±→←↑↓≤≥≈½  "


def main() -> None:
    font = TTFont(SRC, lazy=False)
    opts = subset.Options()
    opts.flavor = "woff2"
    opts.layout_features = ["kern", "liga", "calt", "tnum", "lnum", "case"]
    opts.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14]  # Copyright und Lizenz bleiben drin
    opts.notdef_outline = True
    sub = subset.Subsetter(opts)
    sub.populate(text=ZEICHEN)
    sub.subset(font)
    # erst reduzieren, dann Achsen festlegen (umgekehrt scheitert fonttools an .notdef)
    font = instancer.instantiateVariableFont(
        font, {"wght": (400, 800), "wdth": (100, 120), "opsz": 14, "slnt": 0})
    DST.parent.mkdir(parents=True, exist_ok=True)
    font.flavor = "woff2"
    font.save(DST)
    print(f"{DST.relative_to(ROOT)}: {DST.stat().st_size} Byte, {len(font.getGlyphOrder())} Glyphen")


if __name__ == "__main__":
    main()
