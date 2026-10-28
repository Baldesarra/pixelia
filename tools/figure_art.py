# -*- coding: utf-8 -*-
"""Figuren-Baukasten (seit 0.5.9 Teil 3): Chibi-Figuren aus Ebenen, nach der Skizze v3 (docs/skizzen/figuren-v3.html).
Stil „Mehr Tiefe“ (Wahl des Users 2026-10-27): Augen mit Glanzpunkt, Lichtkante an der Kleidung, farbige Kontur.

Ebenen in fester Reihenfolge (figure.h, LookLayer): Körper (Kopf mit Gesicht, Hände), Beine, Oberteil (mit Ärmeln),
Haare (mit Bart), Kopfbedeckung, Werkzeug. Jede Variante ist ein Bildstreifen mit 15 Bildern: Ansicht vorn, hinten,
Seite (nach rechts; links = gespiegelt) zu je 5 Posen (stehen, Schritt, Mitte, Schritt, atmen), 24 × 36, Fußpunkt
(11, 33). Was eine Farbreihe bekommt, ist mit der Platzhalter-Rampe gezeichnet (20 hell, 21 Grund, 22 Schatten);
feste Farben (Augen, Wangen, Leinen, Holz, Eisen, Schuhe) direkt. Die Kontur zieht die Engine erst nach dem
Übereinanderlegen (FigureArt, Tabelle „farbreihe/kontur“), so hat jede Figur nur außen eine Kontur.

  python tools/figure_art.py bauen                       Ebenen nach assets/testorte/figuren/ schreiben
  python tools/figure_art.py gif <ziel.gif> --bg <png>   Vorschau: Figuren laufen durch einen Raum (wie am Gerät)
  python tools/figure_art.py bogen <ziel.png>            Kontaktbogen: alle Figuren vorn, Seite, hinten
  python tools/figure_art.py gehen <ziel.gif>            Gehbilder aller Figuren in alle Richtungen

Statur (seit 0.5.9 Teil 5, Ansatz A „Chibi mit Statur“): jede Ebene wird einmal gezeichnet; beim Zusammensetzen
streckt oder staucht die Statur Rumpf und Beine (stature_rows, stature_cols, gleiche Rechnung wie figure_art.cpp),
der Kopf bleibt. Fertige Bilder sind 24 × 40, Fußpunkt (11, 37).

Indizes wie assets/paletten/testorte.gpl.
"""
import argparse
import math
import os
import sys
from PIL import Image

SW, SH = 32, 40        # Zeichenfläche je Bild
CX, Y = 15, 6          # Mitte, oberste Zeile des Kopfes
FW, FH, OX = 24, 36, 4  # Bild im Paket: Ausschnitt ab x = OX
FOOT = Y + 13 + 9 + 5  # letzte Zeile der Schuhe (Fußpunkt y)
POSES = ((0, 0), (1, 0), (0, 0), (3, 0), (0, 1))  # (Gehbild, atmen)

PH = (20, 21, 22)      # Platzhalter: hell, Grund, Schatten
INK, PINK, GOLD, WHITE, MOUTH = 54, 24, 32, 10, 26
LINEN = (10, 16, 17)
WOOD, IRON = (64, 65, 66), (58, 59, 61)
SHOES = (51, 52, 53)
RIBBON = 25

# Varianten je Ebene (Nummer = Platz in der Liste + 1, figure.h: 0 = keine)
VARIANTS = {
    'koerper': ['normal', 'brille'],
    'beine': ['hose', 'rock'],
    'oberteil': ['hemd', 'kleid', 'kleid-schuerze', 'weste', 'mantel', 'lederschurz', 'kleid-tuch', 'gehrock',
                 'wirtsschuerze'],
    'haare': ['kurz', 'dutt', 'zoepfe', 'lang', 'glatze', 'kurz-bart', 'kurz-schnurrbart', 'glatze-bart',
              'glatze-schnurrbart'],
    'kopf': ['haube', 'schlapphut', 'muetze', 'kopftuch', 'strohhut'],
    'werkzeug': ['schieber', 'hammer', 'hobel', 'stock'],
}
LAYERS = list(VARIANTS)


def read_gpl(path='assets/paletten/testorte.gpl'):
    cols = []
    for line in open(path, encoding='utf-8'):
        t = line.split()
        if len(t) >= 3 and all(v.isdigit() for v in t[:3]):
            cols.append(tuple(int(v) for v in t[:3]))
    return cols


PAL = read_gpl()


class Spr:
    def __init__(self):
        self.d = [[0] * SW for _ in range(SH)]

    def r(self, x, y, w, h, c):
        x, y, w, h = int(round(x)), int(round(y)), int(w), int(h)
        for yy in range(max(0, y), min(SH, y + h)):
            for xx in range(max(0, x), min(SW, x + w)):
                self.d[yy][xx] = c

    def p(self, x, y, c):
        self.r(x, y, 1, 1, c)


INS = [3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2]  # Kopf: Einzug je Zeile (rund)


def geom(v, fr, breathe):
    by = Y + 13
    return dict(by=by, ly=by + 9, a0=by + 1, bob=1 if fr in (1, 3) or breathe else 0)


def draw_layer(layer, kind, v, fr, breathe=0, stand=False):
    """Eine Ebene in Ansicht v ('f', 'b', 's'), Gehbild fr (0–3); stand: Standbild oder atmen (nicht Gehen)."""
    g = geom(v, fr, breathe)
    by, ly, a0, bob = g['by'], g['ly'], g['a0'], g['bob']
    yb, ey = Y + bob, Y + bob + 7
    s = Spr()
    if layer == 'koerper':
        for r in range(13):
            s.r(CX - 7 + INS[r], yb + r, 14 - INS[r] * 2, 1, PH[0])
            if v != 'b':
                s.p(CX + 6 - INS[r], yb + r, PH[1])  # Schattenseite
        if v == 'f':
            for ex in (CX - 5, CX + 3):
                s.r(ex, ey - 1, 2, 3, INK)
                s.p(ex, ey - 1, WHITE)
            s.p(CX - 6, ey + 3, PINK); s.p(CX + 5, ey + 3, PINK)
            s.r(CX - 1, ey + 4, 2, 1, PH[2])
            if kind == 'brille':
                for gx in (CX - 6, CX + 2):
                    s.r(gx, ey - 2, 4, 1, GOLD); s.r(gx, ey + 2, 4, 1, GOLD)
                    s.r(gx, ey - 1, 1, 3, GOLD); s.r(gx + 3, ey - 1, 1, 3, GOLD)
                s.r(CX - 2, ey, 4, 1, GOLD)
        elif v == 's':
            s.r(CX + 3, ey - 1, 2, 3, INK); s.p(CX + 4, ey - 1, WHITE)
            s.p(CX + 4, ey + 3, PINK)
            s.p(CX + 6, ey + 4, PH[2])
            if kind == 'brille':
                s.r(CX + 2, ey - 2, 4, 1, GOLD); s.r(CX + 2, ey + 2, 4, 1, GOLD); s.r(CX - 1, ey, 3, 1, GOLD)
        # Hände (die Ärmel gehören zum Oberteil)
        if v == 's':
            s.r(CX - 1 + [0, 1, 0, -1][fr], a0 + 6 + bob, 2, 2, PH[1])
        else:
            sh1, sh3 = (1 if fr == 1 and v == 'f' else 0), (1 if fr == 3 and v == 'f' else 0)
            s.r(CX - 6, a0 + 6 - sh1 + breathe, 2, 2, PH[0]); s.r(CX + 4, a0 + 6 - sh3 + breathe, 2, 2, PH[1])
    elif layer == 'beine':
        if v != 's':
            l0, l1, lh = (1 if fr == 1 else 0), (1 if fr == 3 else 0), 4
            if kind == 'hose':
                s.r(CX - 4, ly, 4, lh - l0, PH[1]); s.r(CX, ly, 4, lh - l1, PH[1]); s.r(CX - 1, ly, 1, lh, PH[2])
            s.r(CX - 4, ly + lh - l0, 4, 2, SHOES[1]); s.r(CX, ly + lh - l1, 4, 2, SHOES[1])
            s.p(CX - 3, ly + lh - l0, SHOES[0]); s.p(CX + 1, ly + lh - l1, SHOES[0])
        else:
            f = [[0, 0], [2, -2], [0, 0], [-2, 2]][fr]
            if kind == 'hose':
                s.r(CX - 2 + f[1] * .5, ly, 3, 2, PH[2]); s.r(CX - 2 + f[1], ly + 2, 3, 2, PH[2])
            s.r(CX - 2 + f[1], ly + 4, 4, 2, SHOES[2])
            if kind == 'hose':
                s.r(CX - 1 + f[0] * .5, ly, 3, 2, PH[1]); s.r(CX - 1 + f[0], ly + 2, 3, 2, PH[1])
            s.r(CX - 1 + f[0], ly + 4, 4, 2, SHOES[1]); s.p(CX + f[0] + 1, ly + 4, SHOES[0])
    elif layer == 'oberteil':
        tw, tx, th = (6, CX - 3, 9) if v == 's' else (8, CX - 4, 9)
        dress = kind.startswith('kleid')
        s.r(tx, by + breathe, tw, th - breathe, PH[1])
        s.r(tx if v == 's' else tx + tw - 1, by + breathe, 1, th - breathe, PH[2])
        if v != 's':
            s.r(tx, by + breathe, 1, th - breathe, PH[0])        # Lichtkante
            s.r(tx + 1, by + breathe, tw - 2, 1, PH[2])          # Schatten unter dem Kopf
        if dress:
            for k in range(5):
                s.r(tx - 1 - (k > 1), by + th - 1 + k, tw + 2 + (2 if k > 1 else 0), 1, PH[1])
            s.r(tx - 1, by + th + 3, tw + 2, 1, PH[2])
            if v != 's':
                s.r(tx - 2, by + th + 1, 1, 3, PH[0])
        if kind == 'mantel':
            s.r(tx - (0 if v == 's' else 1), by + th - 1, tw + (1 if v == 's' else 2), 4, PH[1])
            if v == 'f':
                s.r(CX, by, 1, th + 3, PH[2]); s.p(CX, by + 2, GOLD); s.p(CX, by + 5, GOLD)
        if kind == 'gehrock':  # bis über die Knie, vorn offen (die Beine sieht man im Schlitz), hinten mit Falte
            s.r(tx - 1, by + th - 1, tw + 2, 6, PH[1])
            if v == 'f':
                s.r(tx - 1, by + th - 1, 1, 6, PH[0])
                s.r(CX - 1, by + 3, 2, th + 3, PH[2]); s.r(CX - 1, by + th, 2, 5, 0)
                s.r(CX - 2, by, 1, 3, LINEN[0]); s.r(CX + 1, by, 1, 3, LINEN[1])   # Kragen, Hemd darunter
                s.p(CX - 2, by + 4, GOLD); s.p(CX - 2, by + 6, GOLD)
            elif v == 's':
                s.r(tx - 2, by + th, 2, 5, PH[2])                                   # Schoß hinten
            else:
                s.r(CX - 1, by + th - 1, 1, 6, PH[2]); s.p(CX - 3, by + 6, GOLD); s.p(CX + 2, by + 6, GOLD)
        if kind == 'wirtsschuerze':  # Hemd (Farbreihe), lange Leinenschürze bis über die Knie
            if v == 'f':
                s.r(tx + 1, by + 4, tw - 2, th + 1, LINEN[0]); s.r(tx + tw - 2, by + 4, 1, th + 1, LINEN[1])
                s.r(tx, by + 4, tw, 1, LINEN[1])
            elif v == 's':
                s.r(tx + tw - 2, by + 4, 3, th + 1, LINEN[0]); s.r(tx, by + 4, tw, 1, LINEN[1])
            else:
                s.r(tx, by + 4, tw, 1, LINEN[1]); s.r(CX - 2, by + 4, 4, 2, LINEN[0])  # Schleife
        sleeve = PH[2]
        if kind == 'weste':  # Hemd aus Leinen, darüber die Weste (Platzhalter)
            s.r(tx, by + breathe, tw, th - 1 - breathe, LINEN[1])
            if v == 'f':
                s.r(tx, by + breathe, 3, th - 1 - breathe, PH[1]); s.r(tx + tw - 3, by + breathe, 3, th - 1 - breathe, PH[2])
                for k in range(2, th - 1, 2):
                    s.p(tx + 2, by + k, GOLD)
            elif v == 's':
                s.r(tx, by + breathe, 4, th - 1 - breathe, PH[1])
            else:
                s.r(tx, by + breathe, tw, th - 1 - breathe, PH[1])
            sleeve = LINEN[1]
        if kind in ('kleid-schuerze', 'lederschurz'):
            ac = LINEN if kind == 'kleid-schuerze' else WOOD
            if v == 'f':
                s.r(tx + 1, by + 2, tw - 2, th + (3 if dress else 1), ac[0])
                s.r(tx + tw - 2, by + 2, 1, th + (3 if dress else 1), ac[1])
            elif v == 's':
                s.r(tx + tw - 2, by + 2, 3, th + 2, ac[0])
            else:
                s.r(tx, by + 4, tw, 1, ac[0])
        if kind == 'kleid-tuch':
            s.r(tx - 1, by, tw + 2, 2, MOUTH)
            if v == 'f':
                s.r(tx + 1, by + 2, 2, 3, MOUTH)
        # Ärmel
        if v == 's':
            sw = [0, 1, 0, -1][fr]
            s.r(CX - 1, a0 + bob, 2, 3, sleeve); s.r(CX - 1 + sw, a0 + 3 + bob, 2, 3, sleeve)
        else:
            sh1, sh3 = (1 if fr == 1 and v == 'f' else 0), (1 if fr == 3 and v == 'f' else 0)
            s.r(CX - 6, a0 + breathe, 2, 6 - sh1, sleeve); s.r(CX + 4, a0 + breathe, 2, 6 - sh3, sleeve)
    elif layer == 'haare':
        base, beard = kind.split('-')[0], kind.split('-')[1] if '-' in kind else ''

        def full(r0, r1):
            for r in range(r0, r1 + 1):
                s.r(CX - 7 + INS[r], yb + r, 14 - INS[r] * 2, 1, PH[1])

        if v == 'b':
            if base == 'glatze':
                s.r(CX - 6, yb + 9, 12, 2, PH[1])
            else:
                full(0, 11)
                if base == 'lang':
                    s.r(CX - 7, yb + 11, 14, 8, PH[1])
                for k in range(4):
                    s.p(CX - 4 + k * 3, yb + 1 + (k & 1), PH[0])
                for r in (3, 6, 9):
                    s.p(CX - 3 + (r % 2) * 5, yb + r, PH[2])
        elif v == 'f':
            if base != 'glatze':
                full(0, 3)
                for x in range(CX - 7, CX + 7):
                    if (x * 7) % 3:
                        s.p(x, yb + 4, PH[1])
                s.r(CX - 7, yb + 4, 2, 4, PH[1]); s.r(CX + 5, yb + 4, 2, 4, PH[2])
                for k in range(4):
                    s.p(CX - 4 + k * 3, yb + 1, PH[0])
            else:
                s.r(CX - 7, yb + 5, 2, 4, PH[1]); s.r(CX + 5, yb + 5, 2, 4, PH[1])
            if base == 'lang':
                s.r(CX - 8, yb + 4, 2, 14, PH[1]); s.r(CX + 6, yb + 4, 2, 14, PH[2])
            if beard == 'bart':
                s.r(CX - 5, ey + 4, 10, 2, PH[1]); s.r(CX - 4, ey + 6, 8, 1, PH[1]); s.r(CX - 1, ey + 4, 2, 1, MOUTH)
            if beard == 'schnurrbart':
                s.r(CX - 2, ey + 3, 4, 1, PH[1])
        else:
            if base != 'glatze':
                full(0, 3)
                s.r(CX - 7, yb + 2, 8, 9, PH[1]); s.r(CX + 1, yb + 4, 4, 1, PH[1]); s.p(CX + 5, yb + 4, PH[1])
                for k in range(3):
                    s.p(CX - 3 + k * 3, yb + 1, PH[0])
            else:
                s.r(CX - 7, yb + 6, 5, 3, PH[1])
            if base == 'lang':
                s.r(CX - 8, yb + 4, 6, 15, PH[1])
            if beard == 'bart':
                s.r(CX + 1, ey + 4, 6, 2, PH[1]); s.r(CX + 2, ey + 6, 4, 1, PH[1])
            if beard == 'schnurrbart':
                s.r(CX + 4, ey + 3, 3, 1, PH[1])
        if base == 'dutt':
            bx = CX - 6 if v == 's' else CX - 2
            s.r(bx, yb - 3, 4, 3, PH[1]); s.p(bx + 1, yb - 3, PH[0])
        if base == 'zoepfe':
            for bx in ((CX - 10, CX + 7) if v != 's' else (CX - 9,)):
                s.r(bx, yb + 7, 3, 5, PH[1]); s.p(bx + 1, yb + 6, RIBBON); s.p(bx + 1, yb + 12, PH[1])
    elif layer == 'kopf':
        if kind == 'haube':
            s.r(CX - 6, yb - 2, 12, 4, PH[0]); s.r(CX - 7, yb + 1, 14, 2, PH[0])
            if v != 'b':
                for x in range(CX - 7, CX + 7, 2):
                    s.p(x, yb + 3, PH[1])
            else:
                s.r(CX - 6, yb + 2, 12, 6, PH[0])
            s.r(CX + 4, yb - 1, 2, 3, PH[1])
        elif kind == 'schlapphut':
            s.r(CX - 5, yb - 4, 10, 5, PH[1]); s.r(CX - 5, yb, 10, 1, PH[2])
            s.r(CX - 10, yb + 1, 20, 2, PH[1]); s.r(CX - 10, yb + 2, 20, 1, PH[2])
            s.p(CX - 3, yb - 4, PH[0])
        elif kind == 'kopftuch':  # über die Haare, an den Seiten bis zum Kinn, hinten mit Zipfel
            s.r(CX - 7, yb - 1, 14, 4, PH[1]); s.r(CX - 6, yb - 2, 12, 1, PH[1]); s.p(CX - 4, yb - 1, PH[0])
            if v == 'f':
                s.r(CX - 8, yb + 2, 2, 9, PH[1]); s.r(CX + 6, yb + 2, 2, 9, PH[2])
                s.r(CX - 7, yb + 3, 14, 1, PH[2])
            elif v == 's':
                s.r(CX - 8, yb + 2, 8, 9, PH[1]); s.r(CX + 2, yb + 3, 3, 1, PH[2]); s.r(CX - 9, yb + 10, 3, 3, PH[2])
            else:
                s.r(CX - 8, yb + 2, 16, 10, PH[1]); s.r(CX - 2, yb + 12, 4, 2, PH[1]); s.p(CX - 1, yb + 14, PH[2])
                for x in range(CX - 6, CX + 6, 3):
                    s.p(x, yb + 5, PH[0])
        elif kind == 'strohhut':  # flacher Kopf, breite Krempe (Stroh mit Band)
            s.r(CX - 5, yb - 4, 10, 4, PH[0]); s.r(CX - 5, yb - 1, 10, 1, PH[2])
            s.r(CX - 9, yb, 18, 2, PH[1]); s.r(CX - 9, yb + 1, 18, 1, PH[2])
            for x in range(CX - 8, CX + 8, 3):
                s.p(x, yb, PH[0])
            s.p(CX - 3, yb - 4, PH[1]); s.p(CX + 1, yb - 3, PH[1])
        elif kind == 'muetze':
            s.r(CX - 7, yb - 2, 14, 5, PH[1]); s.r(CX - 7, yb + 2, 14, 1, PH[0])
            for x in range(CX - 6, CX + 6, 2):
                s.p(x, yb, PH[2])
            if v == 'f':
                s.r(CX - 6, yb + 3, 12, 1, PH[2])
            elif v == 's':
                s.r(CX + 3, yb + 3, 6, 1, PH[2])
    elif layer == 'werkzeug' and v != 'b':
        hx, hy = (CX - 6, a0 + 6 + breathe) if v == 'f' else (CX - 1 + [0, 1, 0, -1][fr], a0 + 6 + bob)
        if kind == 'schieber' and v == 'f':  # Brotschieber neben dem Körper aufgestellt, oben das Blatt
            s.r(CX - 8, hy - 13, 1, 16, WOOD[1]); s.r(CX - 10, hy - 18, 5, 5, WOOD[0]); s.r(CX - 6, hy - 18, 1, 5, WOOD[1])
        elif kind == 'schieber':  # von der Seite waagrecht getragen, Blatt vorn
            s.r(hx, hy, 9, 1, WOOD[1]); s.r(hx + 8, hy - 2, 4, 5, WOOD[0]); s.r(hx + 8, hy + 2, 4, 1, WOOD[1])
        elif kind == 'hammer':
            s.r(hx + (0 if v == 'f' else 1), hy - 1, 1, 5, WOOD[2]); s.r(hx - 2, hy + 3, 5, 2, IRON[1]); s.p(hx - 2, hy + 3, IRON[0])
        elif kind == 'stock':  # Gehstock: im Stand schräg vor sich aufgesetzt, die Hand oben auf dem Griff (Wunsch des
            # Users 2026-10-28); beim Gehen pendelt er mit dem Schritt
            if v == 'f':
                top, tip = CX - 7, CX - 9 if stand else CX - 8
            else:
                top, tip = hx + 1, hx + 5 if stand else hx + 1 + [2, 4, 2, 0][fr]
            n = FOOT - hy
            for k in range(n):
                s.p(top + round((tip - top) * (k + 1) / n), hy + 1 + k, WOOD[1])
            s.r(top - 1, hy - (1 if stand else 0), 3, 1, WOOD[0])
        elif kind == 'hobel':
            s.r(hx - 3, hy + 1, 7, 3, WOOD[1]); s.r(hx - 3, hy + 1, 7, 1, WOOD[0]); s.r(hx, hy - 1, 2, 2, WOOD[2]); s.p(hx - 2, hy + 3, IRON[0])
    return s


# --- Statur (seit 0.5.9 Teil 5, Ansatz A „Chibi mit Statur“): wie FigureArt beim Zusammensetzen ---
# Jede Ebene wird einmal gezeichnet; die Statur streckt oder staucht sie zeilen- und spaltenweise (Zeilen und Spalten
# im Inneren verdoppeln oder weglassen, so bleiben Lichtkante, Schatten und Kontur erhalten). Der Kopf bleibt gleich.
# Statur: (Größe -1…2, Breite -1…1, Bauch 0…2, gebeugt 0/1), (0, 0, 0, 0) = Standard (alte Speicherstände)
TOP = 4                  # Zeilen über dem Kopf für die größte Statur
BY, LY = Y + 13, Y + 22  # Beginn Rumpf, Beginn Beine (Zeichenfläche)


def stature_rows(st):
    """Quellzeile je Zeile der Ausgabe (SH + TOP hoch, Fußpunkt bleibt unten): Rumpf und Beine länger oder kürzer."""
    h, _, _, stoop = st
    torso, legs = h - stoop, h
    src = list(range(SH))

    def fit(rows, at, n):  # n > 0: Zeilen ab „at“ verdoppeln, n < 0: weglassen
        i = rows.index(at)
        if n > 0:
            return rows[:i] + [r for r in rows[i:i + n] for _ in (0, 1)] + rows[i + n:]
        return rows[:i] + rows[i - n:]

    src = fit(src, LY + 1, legs)       # Oberschenkel (nicht die Schuhe)
    src = fit(src, BY + 4, torso)      # Mitte des Rumpfes (Ärmel werden mit länger)
    off = src.index(FOOT) - (FOOT + TOP)
    return [src[y + off] if 0 <= y + off < len(src) else None for y in range(SH + TOP)]


def belly_at(sy, b):
    """Bauch: zusätzliche Spalten je Seite in Zeile sy des Rumpfes (rund, unten am stärksten)."""
    if not b or not BY + 2 <= sy < LY + 2:
        return 0
    k = sy - BY
    return min(b, 1 if k in (2, 9, 10) else b)


def stature_cols(v, sy, st, head):
    """Quellspalte je Spalte der Ausgabe für Quellzeile sy (wie ColMap in figure_art.cpp): innen links und rechts je
    eine Spalte verdoppeln (Breite, Bauch) oder weglassen; gebeugt von der Seite Kopf zwei und Schultern eine vor.
    head: Zeile gehört zum Kopf (Haare und Kopfbedeckung ganz, beim Körper bis zum Kinn)."""
    _, w, b, stoop = st
    if st == (0, 0, 0, 0):
        return list(range(SW))
    if head:
        la = ra = CX
        nl = nr = 0
        shift = 2 if stoop and v == 's' else 0
    elif v == 's':
        la, nl, ra, nr = CX - 2, w, CX + 2, belly_at(sy, b)   # Rücken, Bauch nach vorn
        shift = 1 if stoop and sy < BY + 3 else 0
    else:
        e = belly_at(sy, b)
        la, nl, ra, nr, shift = CX - 3, w + e, CX + 2, w + e, 0

    def src(x):
        x -= shift
        if x < 0:
            return None
        if x < la - nl:
            return x + nl
        if x <= la:
            return la
        if x < ra:
            return x
        if x <= ra + nr:
            return ra
        return x - nr
    return [src(x) for x in range(SW)]


def warp(s, v, st, chin, tool=False):
    """Spr (SW × SH) → Raster SW × (SH + TOP) mit Statur; Zeilen über chin gehören zum Kopf, ein Werkzeug folgt starr
    der Hand (Spalten wie in der Zeile der Hände)."""
    out = [[0] * SW for _ in range(SH + TOP)]
    hand = stature_cols(v, BY + 7, st, False)
    for y, sy in enumerate(stature_rows(st)):
        if sy is None:
            continue
        cmap = hand if tool else stature_cols(v, sy, st, sy < chin)
        for x, sx in enumerate(cmap):
            if sx is not None and 0 <= sx < SW:
                out[y][x] = s.d[sy][sx]
    return out


def strip(layer, kind):
    """15 Bilder (vorn, hinten, Seite × 5 Posen) als Index-Raster FW*15 × FH."""
    out = [[0] * (FW * 15) for _ in range(FH)]
    for vi, v in enumerate('fbs'):
        for pi, (fr, br) in enumerate(POSES):
            s = draw_layer(layer, kind, v, fr, br, pi in (0, 4))
            for y in range(FH):
                for x in range(FW):
                    out[y][(vi * 5 + pi) * FW + x] = s.d[y][x + OX]
    return out


def save_png(rows, path):
    h, w = len(rows), len(rows[0])
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    for y in range(h):
        for x in range(w):
            if rows[y][x]:
                im.putpixel((x, y), PAL[rows[y][x]] + (255,))
    im.save(path)


def build(target='assets/testorte/figuren'):
    os.makedirs(target, exist_ok=True)
    for layer, kinds in VARIANTS.items():
        for kind in kinds:
            save_png(strip(layer, kind), f'{target}/{layer}-{kind}.png')
    print(f'{sum(len(k) for k in VARIANTS.values())} Ebenen-Bilder nach {target}')


# --- Vorschau: Farbreihen und Zusammensetzen wie FigureArt (engine/src/figure_art.cpp) ---

ROWS = {  # Platzhalter 20–22 → Farben (wie die Farbreihen in assets/testorte/pakete.json)
    'haut-hell': (48, 49, 50), 'haut-mittel': (64, 65, 66), 'haut-dunkel': (13, 14, 15),
    'haar-braun': (51, 52, 53), 'haar-blond': (32, 33, 34), 'haar-rot': (25, 26, 40), 'haar-grau': (58, 59, 60),
    'haar-schwarz': (52, 53, 1), 'rot': (24, 25, 26), 'blau': (28, 29, 30), 'gelb': (32, 33, 34),
    'lavendel': (36, 37, 38), 'holz': (64, 65, 66), 'leinen': (10, 16, 17), 'schwarz': (52, 53, 1),
    'gruen': (2, 3, 5), 'stein': (58, 59, 60),
}

PEOPLE = {  # (Variante, Farbreihe) je Ebene, Statur (Größe, Breite, Bauch, gebeugt); wie kTestLooks im Testdorf
    'greta': dict(koerper=('normal', 'haut-hell'), beine=('rock', None), oberteil=('kleid-schuerze', 'blau'),
                  haare=('dutt', 'haar-blond'), kopf=('haube', 'leinen'), werkzeug=('schieber', None), statur=(0, 1, 1, 0)),
    'jakob': dict(koerper=('normal', 'haut-hell'), beine=('hose', 'schwarz'), oberteil=('weste', 'schwarz'),
                  haare=('kurz-bart', 'haar-braun'), kopf=('schlapphut', 'schwarz'), werkzeug=('hobel', None),
                  statur=(1, -1, 0, 0)),
    'ida': dict(koerper=('normal', 'haut-mittel'), beine=('rock', None), oberteil=('kleid', 'gruen'),
                haare=('zoepfe', 'haar-rot'), statur=(-1, -1, 0, 0)),
    'konrad': dict(koerper=('normal', 'haut-dunkel'), beine=('hose', 'holz'), oberteil=('lederschurz', 'leinen'),
                   haare=('kurz-schnurrbart', 'haar-schwarz'), werkzeug=('hammer', None), statur=(1, 1, 1, 0)),
    'martha': dict(koerper=('brille', 'haut-hell'), beine=('rock', None), oberteil=('kleid-tuch', 'lavendel'),
                   haare=('dutt', 'haar-grau'), kopf=('kopftuch', 'holz'), statur=(-1, 0, 0, 1)),
    'otto': dict(koerper=('normal', 'haut-hell'), beine=('hose', 'holz'), oberteil=('hemd', 'gelb'),
                 haare=('kurz', 'haar-blond'), kopf=('strohhut', 'gelb'), statur=(2, 0, 0, 0)),
    'lene': dict(koerper=('normal', 'haut-mittel'), beine=('rock', None), oberteil=('kleid', 'rot'),
                 haare=('dutt', 'haar-grau'), statur=(-1, 0, 1, 0)),
    'emil': dict(koerper=('normal', 'haut-hell'), beine=('hose', 'blau'), oberteil=('gehrock', 'blau'),
                 haare=('kurz-schnurrbart', 'haar-braun'), kopf=('muetze', 'blau')),
    'gustav': dict(koerper=('normal', 'haut-hell'), beine=('hose', 'schwarz'), oberteil=('wirtsschuerze', 'rot'),
                   haare=('glatze-schnurrbart', 'haar-grau'), statur=(0, 1, 2, 0)),
    'wilhelm': dict(koerper=('normal', 'haut-hell'), beine=('hose', 'haar-grau'), oberteil=('mantel', 'gruen'),
                    haare=('glatze-bart', 'haar-grau'), werkzeug=('stock', None), statur=(0, -1, 0, 1)),
}


def outline_table():
    ink = PAL[INK]
    t = list(range(256))
    for i in range(1, len(PAL)):
        c = PAL[i]
        want = [c[k] * .4 + ink[k] * .3 for k in range(3)]
        best, bd = 1, 1e9
        for j in range(1, len(PAL)):
            if j in (56, 72, 73, 78, 79):  # leuchtend, Himmel
                continue
            d = sum((PAL[j][k] - want[k]) ** 2 for k in range(3))
            if d < bd:
                best, bd = j, d
        t[i] = best
    return t


OUTC = outline_table()


def compose(who, v, fr, breathe=0, stand=False):
    """Fertiges Bild (SW × (SH + TOP)) wie FigureArt: Ebenen mit Statur und Farbreihe, dann Kontur außen."""
    look = PEOPLE[who]
    st = look.get('statur', (0, 0, 0, 0))
    H2 = SH + TOP
    out = [[0] * SW for _ in range(H2)]
    for layer in LAYERS:
        if layer not in look:
            continue
        kind, row = look[layer]
        g = geom(v, fr, breathe)
        chin = SH if layer in ('haare', 'kopf') else BY + g['bob']
        px = warp(draw_layer(layer, kind, v, fr, breathe, stand), v, st, chin, layer == 'werkzeug')
        for y in range(H2):
            for x in range(SW):
                c = px[y][x]
                if c:
                    out[y][x] = ROWS[row][c - 20] if row and 20 <= c <= 22 else c
    o = [row[:] for row in out]
    for y in range(H2):
        for x in range(SW):
            if o[y][x]:
                continue
            for dx, dy in ((0, 1), (-1, 0), (1, 0), (0, -1)):
                xx, yy = x + dx, y + dy
                if 0 <= xx < SW and 0 <= yy < H2 and o[yy][xx]:
                    out[y][x] = OUTC[o[yy][xx]]
                    break
    return out


def sprite(who, v, fr, flip=False, stand=None):
    idx = compose(who, v, fr, stand=fr == 0 if stand is None else stand)
    im = Image.new('RGBA', (SW, SH + TOP), (0, 0, 0, 0))
    for y in range(SH + TOP):
        for x in range(SW):
            if idx[y][x]:
                im.putpixel((SW - 1 - x if flip else x, y), PAL[idx[y][x]] + (255,))
    return im


def sheet(target, who_list, labels=None, bg=(214, 202, 178), z=4):
    """Kontaktbogen: je Figur vorn, Seite, hinten in z-facher Größe, darunter alle in Display-Größe (2×)."""
    from PIL import ImageDraw, ImageFont
    try:
        font = ImageFont.truetype('arial.ttf', 15)
    except OSError:
        font = ImageFont.load_default()
    cw, chh = FW, SH + TOP - 2
    n = len(who_list)
    big = Image.new('RGB', (n * (68 * z + 8) + 20, chh * z + 30 + chh * 2 + 20), (238, 236, 240))
    d = ImageDraw.Draw(big)
    for i, who in enumerate(who_list):
        x0 = 10 + i * (68 * z + 8)
        tile = Image.new('RGBA', (68, chh), bg + (255,))
        for j, v in enumerate('fsb'):
            sp = sprite(who, v, 0).crop((OX, 2, OX + FW, SH + TOP))
            tile.alpha_composite(sp, (j * 22, 0))
        big.paste(tile.resize((tile.width * z, chh * z), Image.NEAREST), (x0, 10))
        lab = (labels or {}).get(who, who)
        d.text((x0 + 4, chh * z + 12), lab, fill=(60, 50, 60), font=font)
    row = Image.new('RGBA', (n * (FW + 4) + 8, chh), bg + (255,))
    for i, who in enumerate(who_list):
        row.alpha_composite(sprite(who, 'f', 0).crop((OX, 2, OX + FW, SH + TOP)), (4 + i * (FW + 4), 0))
    big.paste(row.resize((row.width * 2, chh * 2), Image.NEAREST), (10, chh * z + 34))
    big.save(target)
    print('Bogen:', target, big.size)


def gif(target, bg_path):
    bg = Image.open(bg_path).convert('RGBA')
    bg.paste(bg.crop((230, 140, 290, 210)), (294, 140))  # alte Testfigur der Kammer überdecken
    paths = {'greta': [(60, 92)], 'jakob': [(150, 88)], 'ida': [(40, 70), (120, 70), (120, 100), (40, 100)],
             'konrad': [(170, 64), (170, 104), (100, 104), (100, 64)], 'martha': [(80, 110), (160, 110)]}
    frames, n = [], 48
    for f in range(n):
        im, items = bg.copy(), []
        for who, pts in paths.items():
            if len(pts) == 1:
                (x, y), v, flip, fr = pts[0], ('s' if who == 'jakob' else 'f'), who == 'jakob', 0
            else:
                seg = [math.dist(pts[i], pts[(i + 1) % len(pts)]) for i in range(len(pts))]
                d, i = (f / n) * sum(seg), 0
                while d > seg[i]:
                    d -= seg[i]
                    i += 1
                a, b = pts[i], pts[(i + 1) % len(pts)]
                x, y = a[0] + (b[0] - a[0]) * d / seg[i], a[1] + (b[1] - a[1]) * d / seg[i]
                dx, dy = b[0] - a[0], b[1] - a[1]
                v, flip = ('s', dx < 0) if abs(dx) > abs(dy) else (('f' if dy > 0 else 'b'), False)
                fr = (f // 2) % 4
            items.append((y, who, x, v, fr, flip))
        for y, who, x, v, fr, flip in sorted(items):
            sp = sprite(who, v, fr, flip, len(paths[who]) == 1).resize((SW * 2, (SH + TOP) * 2), Image.NEAREST)
            cx = (SW - 1 - CX) if flip else CX
            im.alpha_composite(sp, (int(round(x * 2 - cx * 2)), int(round(y * 2 - (FOOT + TOP + 1) * 2))))
        frames.append(im.resize((800, 480), Image.NEAREST).convert('RGB').quantize(colors=255))
    frames[0].save(target, save_all=True, append_images=frames[1:], duration=110, loop=0)
    print('GIF:', target)


def walk(target, bg=(214, 202, 178), z=3):
    """Gehbilder aller Figuren auf der Stelle: vorn, Seite, hinten, Seite gespiegelt (je 8 Schritte)."""
    who = list(PEOPLE)
    frames = []
    for f in range(32):
        v, flip = (('f', False), ('s', False), ('b', False), ('s', True))[f // 8]
        im = Image.new('RGBA', (len(who) * 26 + 4, SH + TOP - 2), bg + (255,))
        for i, w in enumerate(who):
            im.alpha_composite(sprite(w, v, (f // 2) % 4, flip, False).crop((OX - 2, 2, OX + FW + 2, SH + TOP)), (2 + i * 26, 0))
        frames.append(im.resize((im.width * z, im.height * z), Image.NEAREST).convert('RGB').quantize(colors=255))
    frames[0].save(target, save_all=True, append_images=frames[1:], duration=130, loop=0)
    print('GIF:', target)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('bauen')
    sub.add_parser('bogen').add_argument('ziel')
    sub.add_parser('gehen').add_argument('ziel')
    p = sub.add_parser('gif')
    p.add_argument('ziel')
    p.add_argument('--bg', required=True)
    a = ap.parse_args()
    if a.cmd == 'bauen':
        build()
    elif a.cmd == 'bogen':
        sheet(a.ziel, list(PEOPLE))
    elif a.cmd == 'gehen':
        walk(a.ziel)
    else:
        gif(a.ziel, a.bg)
    return 0


if __name__ == '__main__':
    sys.exit(main())
