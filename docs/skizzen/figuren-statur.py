"""Vorschau: Figuren mit unterschiedlicher Statur, drei Ansätze (A Chibi, B Mittel, C Schlank). Nur Ansicht vorn."""
from PIL import Image, ImageDraw, ImageFont

W, H = 36, 52
SKIN = [(240, 196, 160), (214, 160, 120), (170, 112, 78), (120, 78, 52)]
OUT_DARK = (40, 26, 24)


def shade(c, f):
    return tuple(max(0, min(255, int(v * f))) for v in c)


class Pic:
    def __init__(self):
        self.d = [[None] * W for _ in range(H)]

    def r(self, x, y, w, h, c):
        for yy in range(int(y), int(y + h)):
            for xx in range(int(x), int(x + w)):
                if 0 <= xx < W and 0 <= yy < H:
                    self.d[yy][xx] = c

    def p(self, x, y, c):
        self.r(x, y, 1, 1, c)


def figure(base, ch):
    """base: Grundmaße des Ansatzes; ch: Merkmale der Person."""
    s = Pic()
    cx = W // 2
    hf, wf = ch.get('h', 1.0), ch.get('w', 1.0)
    kid = ch.get('kid', False)
    hw = round(base['hw'] * (0.9 if kid else 1))
    hh = round(base['hh'] * (0.9 if kid else 1))
    th = max(5, round(base['th'] * (0.7 if kid else hf)))
    lh = max(2, round(base['lh'] * (0.55 if kid else hf)))
    sw = max(6, round(base['sw'] * wf * (0.8 if kid else 1)))
    belly = ch.get('belly', 0)
    stoop = ch.get('stoop', 0)
    skin = SKIN[ch.get('skin', 0)]
    top, low, hair = ch['top'], ch.get('low', (70, 60, 70)), ch.get('hair', (90, 60, 40))
    foot = H - 3
    ly = foot - 2 - lh          # Beginn der Beine
    ty = ly - th                # Schultern
    hy = ty - hh + 1 + stoop    # Kopf (gebeugt: tiefer)
    hx = cx - hw // 2 + (1 if stoop else 0)
    # Schuhe und Beine
    lw = max(2, sw // 2 - 1)
    for k, x0 in enumerate((cx - lw - (0 if sw < 10 else 1), cx + (0 if sw < 10 else 1))):
        s.r(x0, ly, lw, lh, shade(low, 1.0 if k == 0 else 0.8))
        s.r(x0 - (1 if k == 0 else 0), foot - 2, lw + 1, 2, (60, 42, 36))
    # Rock/Kleid: Trapez bis über die Schuhe
    if ch.get('dress'):
        flare = ch.get('flare', 3)
        n = lh + 1
        for i in range(n):
            w = sw + 2 * round(flare * (i + 1) / n)
            s.r(cx - w // 2, ly + i - 1, w, 1, shade(ch.get('skirt', top), 1.0))
            s.p(cx + w // 2 - 1, ly + i - 1, shade(ch.get('skirt', top), 0.75))
    # Mantel: verlängert den Rumpf
    coat = ch.get('coat', 0)
    # Rumpf (Trapez, Bauch)
    for i in range(th + coat):
        t = i / max(1, th - 1)
        w = sw - (1 if i == 0 else 0) + (round(belly * (1 - abs(t - 0.65) * 2.2)) * 2 if belly and 0.2 < t < 1.05 else 0)
        if i >= th:
            w = sw + 2 + (i - th)
        w = max(4, w)
        s.r(cx - w // 2, ty + i, w, 1, top)
        s.p(cx - w // 2, ty + i, shade(top, 1.18))
        s.p(cx - w // 2 + w - 1, ty + i, shade(top, 0.72))
    if ch.get('apron'):
        s.r(cx - sw // 2 + 2, ty + 3, sw - 4, th + coat + 1, (232, 224, 204))
    if coat:
        s.r(cx, ty + 1, 1, th + coat - 1, shade(top, 0.7))
        s.p(cx - 1, ty + 3, (210, 170, 60)); s.p(cx - 1, ty + 6, (210, 170, 60))
    if ch.get('belt'):
        s.r(cx - sw // 2 - (belly or 0), ty + round(th * 0.62), sw + 2 * belly, 1, (70, 46, 34))
    # Arme mit Händen
    al = max(4, round(th * 0.85))
    for k, x0 in enumerate((cx - sw // 2 - 2 - (1 if belly > 1 else 0), cx + sw // 2 + (1 if belly > 1 else 0))):
        s.r(x0, ty + 1, 2, al, shade(top, 0.9 if k == 0 else 0.7))
        s.r(x0, ty + 1 + al, 2, 2, skin)
    if ch.get('cane'):
        s.r(cx + sw // 2 + 3, ty + al - 1, 1, foot - (ty + al - 1), (110, 74, 44))
        s.r(cx + sw // 2 + 2, ty + al - 1, 2, 1, (110, 74, 44))
    # Kopf (rund)
    ins = [2, 1] + [0] * (hh - 4) + [1, 2]
    for i in range(hh):
        s.r(hx + ins[i], hy + i, hw - 2 * ins[i], 1, skin)
        s.p(hx + hw - 1 - ins[i], hy + i, shade(skin, 0.85))
    ey = hy + round(hh * 0.55)
    big = base['hw'] >= 13
    for ex in (hx + round(hw * 0.25), hx + hw - 1 - round(hw * 0.25) - (1 if big else 0)):
        if big:
            s.r(ex, ey - 1, 2, 3, (40, 30, 40)); s.p(ex, ey - 1, (255, 255, 255))
        else:
            s.r(ex, ey, 1, 2, (40, 30, 40))
    if not kid and big:
        s.p(hx + 1, ey + 3, (230, 130, 130)); s.p(hx + hw - 2, ey + 3, (230, 130, 130))
    if ch.get('glasses'):
        for ex in (hx + round(hw * 0.25) - 1, hx + hw - 2 - round(hw * 0.25) - (1 if big else 0)):
            s.r(ex, ey - 1 - big, 3 + big, 1, (200, 170, 70)); s.r(ex, ey + 1 + big, 3 + big, 1, (200, 170, 70))
    # Haare
    hs = ch.get('hs', 'kurz')
    if hs != 'glatze':
        s.r(hx + 1, hy - 1, hw - 2, 1, hair)
        s.r(hx, hy, hw, max(2, hh // 4), hair)
        s.r(hx, hy, 1, hh // 2 + 1, hair); s.r(hx + hw - 1, hy, 1, hh // 2 + 1, shade(hair, 0.8))
        s.p(hx + 2, hy, shade(hair, 1.25)); s.p(hx + hw // 2, hy, shade(hair, 1.25))
    else:
        s.r(hx - 1, ey - 1, 1, 3, hair); s.r(hx + hw, ey - 1, 1, 3, hair)
    if hs == 'lang':
        s.r(hx - 1, hy + 1, 2, hh + 3, hair); s.r(hx + hw - 1, hy + 1, 2, hh + 3, shade(hair, 0.8))
    if hs == 'dutt':
        s.r(cx - 2, hy - 4, 4, 3, hair)
    if hs == 'zoepfe':
        for x0 in (hx - 2, hx + hw):
            s.r(x0, hy + hh // 2, 2, hh // 2 + 3, hair); s.p(x0, hy + hh + 2, (200, 60, 60))
    if ch.get('beard'):
        s.r(hx + 2, ey + 2, hw - 4, max(2, hh // 4), hair); s.r(hx + 3, ey + 2 + max(2, hh // 4), hw - 6, 1 + ch['beard'], hair)
    if ch.get('stache'):
        s.r(cx - 2, ey + 2, 4, 1, hair)
    # Kopfbedeckung
    hat, hc = ch.get('hat'), ch.get('hatc', (60, 50, 60))
    if hat == 'zylinder':
        s.r(hx - 2, hy, hw + 4, 2, hc); s.r(hx + 1, hy - 7, hw - 2, 7, hc); s.r(hx + 1, hy - 2, hw - 2, 1, (150, 40, 40))
    elif hat == 'muetze':
        s.r(hx - 1, hy - 2, hw + 2, 4, hc); s.r(hx - 2, hy + 1, hw // 2 + 2, 1, shade(hc, 0.7)); s.p(cx, hy - 3, (210, 170, 60))
    elif hat == 'haube':
        s.r(hx - 1, hy - 2, hw + 2, hh // 2 + 1, hc); s.r(hx - 1, hy - 2, 1, hh, hc); s.r(hx + hw, hy - 2, 1, hh, shade(hc, 0.85))
    elif hat == 'schlapphut':
        s.r(hx + 1, hy - 4, hw - 2, 4, hc); s.r(hx - 4, hy - 1, hw + 8, 2, shade(hc, 0.85))
    elif hat == 'kopftuch':
        s.r(hx - 1, hy - 1, hw + 2, hh // 3 + 1, hc); s.r(hx - 1, hy - 1, 2, hh - 2, hc); s.r(hx + hw - 1, hy - 1, 2, hh - 2, shade(hc, 0.8))
    if ch.get('pack'):
        s.r(cx + sw // 2 - 1, ty + 2, 4, th - 2, (130, 96, 60))
    return s


def outlined(s):
    out = [row[:] for row in s.d]
    for y in range(H):
        for x in range(W):
            if s.d[y][x] is None and any(0 <= x + dx < W and 0 <= y + dy < H and s.d[y + dy][x + dx] is not None
                                        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = OUT_DARK
    return out


PEOPLE = [
    ('Bäckerin', dict(w=1.25, belly=2, top=(196, 120, 90), apron=True, dress=True, skirt=(150, 80, 70), hs='dutt', hair=(120, 70, 40), skin=0)),
    ('Schmied', dict(h=1.15, w=1.45, top=(80, 70, 66), low=(70, 56, 46), apron=True, hs='glatze', hair=(60, 40, 30), beard=2, skin=1)),
    ('Großvater', dict(h=0.9, w=0.95, stoop=2, cane=True, top=(110, 100, 80), low=(80, 76, 70), coat=3, hs='glatze', hair=(220, 220, 220), beard=1, skin=0)),
    ('Kind', dict(kid=True, top=(80, 130, 180), low=(90, 70, 50), hs='zoepfe', hair=(200, 150, 60), skin=0)),
    ('Näherin', dict(h=1.05, w=0.85, top=(120, 70, 120), dress=True, flare=4, skirt=(110, 60, 110), hs='lang', hair=(60, 34, 24), skin=2)),
    ('Bahnwärter', dict(h=1.2, w=0.9, top=(50, 70, 110), low=(40, 50, 80), coat=4, hat='muetze', hatc=(40, 56, 96), stache=True, hair=(90, 60, 40), skin=0)),
    ('Großmutter', dict(h=0.8, w=1.15, belly=1, top=(90, 90, 110), dress=True, skirt=(70, 70, 90), hat='haube', hatc=(236, 232, 220), hs='kurz', hair=(200, 200, 200), skin=0)),
    ('Lehrer', dict(h=1.1, w=1.0, top=(70, 60, 50), low=(60, 50, 44), coat=2, glasses=True, hat='zylinder', hatc=(40, 36, 40), hair=(70, 50, 36), skin=3)),
]

APPROACHES = [
    ('A  Chibi wie bisher, mit Statur', dict(hw=14, hh=13, sw=10, th=9, lh=5)),
    ('B  Mittlere Köpfe, mehr Körper', dict(hw=11, hh=10, sw=10, th=11, lh=8)),
    ('C  Schlank, kleine Köpfe', dict(hw=9, hh=8, sw=9, th=13, lh=11)),
]

Z = 5
font = ImageFont.truetype('arialbd.ttf', 26)
small = ImageFont.truetype('arial.ttf', 17)
bg = (214, 202, 178)
SW_ = len(PEOPLE) * W
rowh = 40 + H * Z + 26 + H * 2 + 30
sheet = Image.new('RGB', (SW_ * Z + 40, len(APPROACHES) * rowh + 10), (238, 236, 240))
d = ImageDraw.Draw(sheet)
for a, (title, base) in enumerate(APPROACHES):
    y0 = 10 + a * rowh
    d.text((20, y0), title, fill=(40, 40, 60), font=font)
    strip = Image.new('RGB', (SW_, H), bg)
    for i, (name, ch) in enumerate(PEOPLE):
        px = outlined(figure(base, ch))
        for y in range(H):
            for x in range(W):
                if px[y][x] is not None:
                    strip.putpixel((i * W + x, y), px[y][x])
    sheet.paste(strip.resize((SW_ * Z, H * Z), Image.NEAREST), (20, y0 + 38))
    for i, (name, _) in enumerate(PEOPLE):
        d.text((20 + i * W * Z + W * Z // 2 - d.textlength(name, font=small) / 2, y0 + 40 + H * Z), name, fill=(70, 60, 60), font=small)
    d.text((20, y0 + 40 + H * Z + 24), 'so groß wie am Display:', fill=(70, 60, 60), font=small)
    sheet.paste(strip.resize((SW_ * 2, H * 2), Image.NEAREST), (230, y0 + 40 + H * Z + 22))
sheet.save('docs/skizzen/figuren-statur.png')  # aus dem Repo-Ordner aufrufen
print(sheet.size)
