#!/usr/bin/env python3
"""Grafik-Werkzeug (seit 0.5.5): PNG → Paletten-Indizes → Asset-Pakete (*.pxa, Format PXA1).

Ohne Fremdbibliotheken (nur zlib aus Python). Format, Palette und Regeln: docs/STIL.md Kapitel 9.

Befehle:
  build [--cpp DATEI] [--out ORDNER]   alle Pakete aus assets/**/pakete.json bauen; --cpp schreibt die
                                       eingebetteten Pakete für Firmware und Simulator, --out die .pxa-Dateien
  check                                Pakete bauen und prüfen, dazu assets/CREDITS.md (läuft in der CI)
  info PAKET.pxa                       Einträge eines Pakets auflisten
  zeige PAKET.pxa NAME --out BILD.png [--palette X.gpl]
                                       ein Bild aus einem Paket als PNG (Vorschau, Zustände nebeneinander)
  wandeln BILD.png --palette X.gpl [--farben streng|naechste|eigene] [--bilder N] [--vorschau BILD.png]
                                       ein PNG prüfen und umrechnen (eigene Möbel): streng = nur Pixelia-Farben,
                                       naechste = fremde Farben auf die nächste Pixelia-Farbe, eigene = fremde Farben
                                       behalten (höchstens 16 je Bild); die Vorschau zeigt das Ergebnis
"""

from __future__ import annotations

import argparse
import fnmatch
import json
import struct
import sys
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
ASSETS = REPO / "assets"

TYPE_IMAGE, TYPE_TILES, TYPE_RECOLOR, TYPE_PALETTE, TYPE_DATA = 1, 2, 3, 4, 5
TYPE_NAMES = {1: "Bild", 2: "Kacheln", 3: "Farbreihe", 4: "Palette", 5: "Daten"}
OWN_BASE, MAX_OWN = 224, 16  # eigene Farben eines Bildes: Indizes 224 … 239
TILE = 8


class AssetError(Exception):
    pass


def asset_id(name: str) -> int:
    """FNV-1a 32 Bit, gleich wie pixelia::asset_id (engine/assets.h)."""
    h = 2166136261
    for b in name.encode("ascii"):
        h ^= b
        h = (h * 16777619) & 0xFFFFFFFF
    return h


# --- PNG lesen und schreiben ---------------------------------------------------------------------------------------

def read_png(path: Path) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    """PNG → (Breite, Höhe, Pixel als RGBA). Alle Farbtypen mit 8 Bit, Palette auch 1/2/4 Bit, ohne Interlace."""
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssetError(f"{path}: kein PNG")
    pos, idat, plte, trns = 8, b"", None, None
    w = h = depth = ctype = 0
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
            if interlace:
                raise AssetError(f"{path}: Interlace wird nicht unterstützt (ohne Interlace speichern)")
        elif kind == b"PLTE":
            plte = [tuple(body[i:i + 3]) for i in range(0, len(body), 3)]
        elif kind == b"tRNS":
            trns = body
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(ctype)
    if channels is None or (depth != 8 and not (ctype == 3 and depth in (1, 2, 4))):
        raise AssetError(f"{path}: Farbtyp {ctype} mit {depth} Bit wird nicht unterstützt (8 Bit speichern)")
    bpp = max(1, channels * depth // 8)
    stride = (w * channels * depth + 7) // 8
    raw = zlib.decompress(idat)
    rows, prev = [], bytearray(stride)
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if f == 1:
                line[x] = (line[x] + a) & 255
            elif f == 2:
                line[x] = (line[x] + b) & 255
            elif f == 3:
                line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(line)
        prev = line
    px = []
    for line in rows:
        for x in range(w):
            if ctype == 3:
                bit = x * depth
                v = (line[bit // 8] >> (8 - depth - bit % 8)) & ((1 << depth) - 1)
                r, g, b = plte[v]
                alpha = trns[v] if trns is not None and v < len(trns) else 255
                px.append((r, g, b, alpha))
            elif ctype == 0:
                v = line[x]
                px.append((v, v, v, 0 if trns is not None and struct.unpack(">H", trns)[0] == v else 255))
            elif ctype == 4:
                v = line[x * 2]
                px.append((v, v, v, line[x * 2 + 1]))
            elif ctype == 2:
                r, g, b = line[x * 3:x * 3 + 3]
                alpha = 0 if trns is not None and struct.unpack(">HHH", trns) == (r, g, b) else 255
                px.append((r, g, b, alpha))
            else:
                px.append(tuple(line[x * 4:x * 4 + 4]))
    return w, h, px


def write_png_indexed(path: Path, w: int, h: int, indices: bytes | list[int], palette: list[tuple[int, int, int]]):
    """8-Bit-Palettenbild mit Index 0 durchsichtig (lässt sich mit der Palette in Aseprite/GIMP weiterzeichnen)."""
    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))
    pal = list(palette) + [(0, 0, 0)] * (256 - len(palette))
    raw = b"".join(b"\0" + bytes(indices[y * w:(y + 1) * w]) for y in range(h))
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 3, 0, 0, 0))
    png += chunk(b"PLTE", b"".join(bytes(c) for c in pal[:256])) + chunk(b"tRNS", b"\0")
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


# --- Palette (.gpl) ----------------------------------------------------------------------------------------------

class Palette:
    """GIMP-Palette: Zeile i = Index i. Benannte Einträge sind Zeichenfarben; „name n“ bildet eine Rampe."""

    def __init__(self, path: Path):
        self.path = path
        self.colors: list[tuple[int, int, int]] = []
        self.names: list[str] = []
        lines = path.read_text(encoding="utf-8").splitlines()
        if not lines or lines[0].strip() != "GIMP Palette":
            raise AssetError(f"{path}: keine GIMP-Palette")
        for line in lines[1:]:
            s = line.strip()
            if not s or s.startswith("#") or ":" in s.split()[0]:
                continue
            parts = s.split(None, 3)
            self.colors.append((int(parts[0]), int(parts[1]), int(parts[2])))
            self.names.append(parts[3].strip() if len(parts) > 3 else "")
        self.lookup: dict[tuple[int, int, int], int] = {}
        for i, (c, n) in enumerate(zip(self.colors, self.names)):
            if i == 0 or not n or n == "-":
                continue  # 0 = durchsichtig, unbenannt = frei
            if c in self.lookup:
                raise AssetError(f"{path}: Farbe {c} doppelt (Index {self.lookup[c]} und {i}); jede Zeichenfarbe "
                                 "braucht einen eigenen RGB-Wert")
            self.lookup[c] = i
        self.ramps: dict[str, tuple[int, int]] = {}
        for i, n in enumerate(self.names):
            parts = n.rsplit(" ", 1)
            if len(parts) == 2 and parts[1].isdigit():
                start, length = self.ramps.get(parts[0], (i, 0))
                if start + length != i:
                    raise AssetError(f"{path}: Rampe „{parts[0]}“ ist nicht zusammenhängend (Index {i})")
                self.ramps[parts[0]] = (start, length + 1)

    def nearest(self, c: tuple[int, int, int]) -> int:
        def dist(p):
            rm = (c[0] + p[0]) / 2
            dr, dg, db = c[0] - p[0], c[1] - p[1], c[2] - p[2]
            return (2 + rm / 256) * dr * dr + 4 * dg * dg + (2 + (255 - rm) / 256) * db * db
        return min(self.lookup.values(), key=lambda i: dist(self.colors[i]))


def to_indices(px, w, h, pal: Palette, mode: str, label: str):
    """RGBA → Indizes. Gibt (Indizes, eigene Farben, Meldungen) zurück; streng: Fehler bei fremden Farben."""
    out, own, foreign, notes = bytearray(len(px)), [], {}, []
    for i, (r, g, b, a) in enumerate(px):
        if a < 128:
            continue
        c = (r, g, b)
        k = pal.lookup.get(c)
        if k is None:
            foreign.setdefault(c, []).append((i % w, i // w))
            if mode == "naechste":
                k = pal.nearest(c)
            elif mode == "eigene":
                if c not in own:
                    own.append(c)
                k = OWN_BASE + own.index(c) if len(own) <= MAX_OWN else pal.nearest(c)
            else:
                k = 0
        out[i] = k
    if foreign:
        lines = [f"{c} an {len(pos)} Stelle{'' if len(pos) == 1 else 'n'}, z. B. {pos[0]}" + (f" → {pal.names[pal.nearest(c)]}" if mode != "eigene" else "")
                 for c, pos in list(foreign.items())[:8]]
        if mode == "streng":
            raise AssetError(f"{label}: {len(foreign)} Farben nicht in der Palette {pal.path.name}:\n  " + "\n  ".join(lines))
        if mode == "eigene" and len(own) > MAX_OWN:
            notes.append(f"{label}: {len(own)} eigene Farben, nur {MAX_OWN} erlaubt; der Rest geht auf die nächste Farbe")
            own = own[:MAX_OWN]
        notes.append(f"{label}: {len(foreign)} fremde Farben ({mode}):\n  " + "\n  ".join(lines))
    return bytes(out), own, notes


# --- Einträge -----------------------------------------------------------------------------------------------------

def image_entry(spec: dict, base: Path, pal: Palette, label: str):
    w, h, px = read_png(base / spec["datei"])
    frames = int(spec.get("bilder", 1))
    states = int(spec.get("zustaende", 1))
    if w % frames or frames % states:
        raise AssetError(f"{label}: Breite {w} nicht durch {frames} Bilder teilbar bzw. Bilder nicht durch Zustände")
    fw = w // frames
    idx, own, notes = to_indices(px, w, h, pal, spec.get("farben", "streng"), label)
    # Streifen → Bild für Bild hintereinander
    pixels = bytearray()
    for f in range(frames):
        for y in range(h):
            pixels += idx[y * w + f * fw:y * w + (f + 1) * fw]
    fx, fy = spec.get("fuss", [fw // 2, h - 1])
    head = struct.pack("<HHHBBhh", fw, h, frames, states, len(own), fx, fy)
    return head + b"".join(bytes(c) for c in own) + bytes(pixels), notes


def tiles_entry(spec: dict, base: Path, pal: Palette, label: str):
    w, h, px = read_png(base / spec["datei"])
    if w % TILE or h % TILE:
        raise AssetError(f"{label}: Kachelbild muss ein Vielfaches von 8 × 8 sein")
    idx, own, notes = to_indices(px, w, h, pal, "streng", label)
    cols, count = w // TILE, (w // TILE) * (h // TILE)
    flags = bytearray(count)
    for t in spec.get("sperren", []):
        flags[t] |= 1  # kTileBlocks (tiles.h)
    pixels = bytearray()
    for t in range(count):
        tx, ty = (t % cols) * TILE, (t // cols) * TILE
        for y in range(TILE):
            pixels += idx[(ty + y) * w + tx:(ty + y) * w + tx + TILE]
    return struct.pack("<H", count) + bytes(flags) + bytes(pixels), notes


def color_index(pal: Palette, name: str, label: str) -> int:
    if name not in pal.names:
        raise AssetError(f"{label}: Farbe „{name}“ fehlt in {pal.path.name}")
    return pal.names.index(name)


def recolor_entry(spec: dict, pal: Palette, label: str):
    table = bytearray(range(256))
    if "kontur" in spec:
        # Kontur der Figuren (seit 0.5.9 Teil 3, wie Skizze v3): je Farbe die nächste zu 0,4 · Farbe + 0,3 · Tinte,
        # ohne die Farben unter „ohne“ (leuchtend, Himmel)
        ink = pal.colors[color_index(pal, spec["kontur"], label)]
        skip = {color_index(pal, n, label) for n in spec.get("ohne", [])}
        cand = [j for j in range(1, len(pal.colors)) if pal.names[j] not in ("", "-") and j not in skip]
        for i in range(1, len(pal.colors)):
            c = pal.colors[i]
            want = [c[k] * .4 + ink[k] * .3 for k in range(3)]
            table[i] = min(cand, key=lambda j: sum((pal.colors[j][k] - want[k]) ** 2 for k in range(3)))
        return bytes(table)
    for src, dst in spec.items():
        if src not in pal.ramps:
            raise AssetError(f"{label}: Rampe „{src}“ fehlt in {pal.path.name}")
        if isinstance(dst, list):  # seit 0.5.9 Teil 3: Rampe auf eine Liste benannter Farben (Haut, Haare)
            a, n = pal.ramps[src]
            if len(dst) != n:
                raise AssetError(f"{label}: „{src}“ hat {n} Farben, die Liste {len(dst)}")
            for i, name in enumerate(dst):
                table[a + i] = color_index(pal, name, label)
            continue
        if dst not in pal.ramps:
            raise AssetError(f"{label}: Rampe „{dst}“ fehlt in {pal.path.name}")
        (a, n), (b, m) = pal.ramps[src], pal.ramps[dst]
        if n != m:
            raise AssetError(f"{label}: Rampen „{src}“ ({n}) und „{dst}“ ({m}) sind verschieden lang")
        for i in range(n):
            table[a + i] = b + i
    return bytes(table)


def palette_entry(pal: Palette) -> bytes:
    if len(pal.colors) > 255:
        raise AssetError(f"{pal.path}: höchstens 255 Farben")
    return bytes([len(pal.colors)]) + b"".join(bytes(c) for c in pal.colors)


# --- Paket schreiben (PXA1, engine/assets.h) ----------------------------------------------------------------------

def packbits(data: bytes) -> bytes:
    out, i, n = bytearray(), 0, len(data)
    while i < n:
        j = i + 1
        while j < n and j - i < 128 and data[j] == data[i]:
            j += 1
        if j - i >= 3:
            out += bytes([257 - (j - i), data[i]])
            i = j
            continue
        start = i
        while i < n and i - start < 128:
            if i + 2 < n and data[i] == data[i + 1] == data[i + 2]:
                break
            i += 1
        out += bytes([i - start - 1]) + data[start:i]
    return bytes(out)


def write_pack(entries: list[tuple[str, int, bytes]]) -> bytes:
    names, directory, payload = bytearray(), bytearray(), bytearray()
    unpacked = 0
    for name, kind, body in entries:
        packed = packbits(body)
        codec, stored = (1, packed) if len(packed) < len(body) else (0, body)
        directory += struct.pack("<IBBHIII", asset_id(name), kind, codec, len(names), len(payload), len(stored),
                                 len(body))
        names += name.encode("ascii") + b"\0"
        payload += stored
        unpacked += len(body)
    rest = bytes(directory) + bytes(names) + bytes(payload)
    head = b"PXA1" + struct.pack("<HHIII", len(entries), len(names), len(payload), unpacked, zlib.crc32(rest))
    return head + rest


def read_pack(data: bytes) -> list[dict]:
    if data[:4] != b"PXA1":
        raise AssetError("kein PXA1-Paket")
    count, nbytes, dbytes, _, crc = struct.unpack("<HHIII", data[4:20])
    if zlib.crc32(data[20:]) != crc:
        raise AssetError("Prüfsumme falsch")
    names_at = 20 + count * 20
    data_at = names_at + nbytes
    out = []
    for i in range(count):
        ident, kind, codec, name, off, packed, size = struct.unpack("<IBBHIII", data[20 + i * 20:40 + i * 20])
        raw = data[data_at + off:data_at + off + packed]
        if codec == 1:
            body, k = bytearray(), 0
            while k < len(raw):
                c = raw[k]
                k += 1
                if c < 128:
                    body += raw[k:k + c + 1]
                    k += c + 1
                elif c > 128:
                    body += bytes([raw[k]]) * (257 - c)
                    k += 1
            raw = bytes(body)
        label = data[names_at + name:data.index(b"\0", names_at + name)].decode("ascii")
        out.append({"id": ident, "kind": kind, "codec": codec, "name": label, "packed": packed, "body": raw})
    return out


# --- Pakete aus assets/**/pakete.json ------------------------------------------------------------------------------

def build_all(verbose: bool = False) -> tuple[dict[str, bytes], list[str]]:
    packs, notes, seen = {}, [], {}
    for desc in sorted(ASSETS.rglob("pakete.json")):
        base = desc.parent
        spec = json.loads(desc.read_text(encoding="utf-8"))
        pal = Palette((base / spec["palette"]).resolve())
        for pack, content in spec["pakete"].items():
            if pack in packs:
                raise AssetError(f"Paket „{pack}“ doppelt ({desc})")
            entries = []
            for name, file in content.get("paletten", {}).items():
                entries.append((name, TYPE_PALETTE, palette_entry(Palette((base / file).resolve()))))
            for name, s in content.get("kacheln", {}).items():
                body, n = tiles_entry(s, base, pal, name)
                entries.append((name, TYPE_TILES, body))
                notes += n
            for name, s in content.get("farbreihen", {}).items():
                entries.append((name, TYPE_RECOLOR, recolor_entry(s, pal, name)))
            for name, s in content.get("bilder", {}).items():
                body, n = image_entry(s, base, pal, name)
                entries.append((name, TYPE_IMAGE, body))
                notes += n
            for name, _, _ in entries:
                ident = asset_id(name)
                if ident in seen and seen[ident] != name:
                    raise AssetError(f"Kennung von „{name}“ gleich wie „{seen[ident]}“: anders benennen")
                if ident in seen:
                    raise AssetError(f"Name „{name}“ doppelt (Pakete teilen sich die Kennungen)")
                seen[ident] = name
            packs[pack] = write_pack(entries)
            if verbose:
                raw = sum(len(b) for _, _, b in entries)
                print(f"  {pack}.pxa: {len(entries)} Einträge, {raw / 1024:.1f} KB → {len(packs[pack]) / 1024:.1f} KB")
    return packs, notes


def write_cpp(packs: dict[str, bytes], path: Path):
    lines = ["// Erzeugt von tools/asset_pack.py aus assets/**/pakete.json – nicht von Hand ändern.",
             '#include "pixelia/engine/assets.h"', "", "namespace pixelia {", "namespace {"]
    for i, (name, data) in enumerate(packs.items()):
        lines.append(f"alignas(4) const uint8_t kPack{i}[] = {{  // {name}.pxa, {len(data)} Byte")
        for k in range(0, len(data), 24):
            lines.append("    " + ",".join(str(b) for b in data[k:k + 24]) + ",")
        lines.append("};")
    lines += ["}  // namespace", "", "const EmbeddedAsset kEmbeddedAssets[] = {"]
    lines += [f'    {{"{name}.pxa", kPack{i}, sizeof(kPack{i})}},' for i, name in enumerate(packs)]
    lines += ["};", f"const int kEmbeddedAssetCount = {len(packs)};", "", "}  // namespace pixelia", ""]
    text = "\n".join(lines)
    if not path.exists() or path.read_text(encoding="utf-8") != text:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")


# --- Credits (CI) ---------------------------------------------------------------------------------------------------

ALLOWED = ("CC0", "CC-BY")
FORBIDDEN = ("SA", "NC", "ND")


def check_credits() -> list[str]:
    """Jede Datei in assets/ (außer *.md) braucht eine Zeile in CREDITS.md: fremd mit CC0/CC-BY, eigen mit „eigen“.

    Fremde Dateien liegen unter assets/fremd/<Quelle>/; dort zählt eine Zeile mit „eigen“ nicht.
    """
    rows = []
    for line in (ASSETS / "CREDITS.md").read_text(encoding="utf-8").splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 3 or not cells[0].startswith("`"):
            continue
        pattern = cells[0].strip("`")
        rows.append((pattern, cells[-1]))
    errors = []
    for f in sorted(ASSETS.rglob("*")):
        if f.is_dir() or f.suffix.lower() == ".md":
            continue
        rel = f.relative_to(REPO).as_posix()
        foreign = rel.startswith("assets/fremd/")
        match = [lic for pattern, lic in rows
                 if fnmatch.fnmatch(rel, pattern) and not (foreign and lic.lower() == "eigen")]
        if not match:
            errors.append(f"{rel}: keine Zeile in assets/CREDITS.md (fremd: Urheber, Quelle, Lizenz CC0/CC-BY; eigen: „eigen“)")
            continue
        lic = match[0]
        ok = lic.lower() == "eigen" or (any(a in lic for a in ALLOWED) and not any(
            f"-{x}" in lic or f" {x}" in lic for x in FORBIDDEN))
        if not ok:
            errors.append(f"{rel}: Lizenz „{lic}“ ist nicht erlaubt (nur CC0 oder CC-BY)")
    return errors


# --- Befehle --------------------------------------------------------------------------------------------------------

def cmd_build(args):
    packs, notes = build_all(verbose=args.verbose)
    for n in notes:
        print("Hinweis:", n)
    if args.cpp:
        write_cpp(packs, Path(args.cpp))
    if args.out:
        out = Path(args.out)
        out.mkdir(parents=True, exist_ok=True)
        for name, data in packs.items():
            (out / f"{name}.pxa").write_bytes(data)
    return 0


def cmd_check(args):
    try:
        packs, notes = build_all(verbose=True)
    except AssetError as e:
        print("Fehler:", e)
        return 1
    for n in notes:
        print("Hinweis:", n)
    for name, data in packs.items():
        read_pack(data)  # Gegenprobe: lässt sich wieder lesen
    errors = check_credits()
    for e in errors:
        print("Fehler:", e)
    total = sum(len(d) for d in packs.values())
    print(f"{len(packs)} Pakete, {total / 1024:.1f} KB; Credits: {'in Ordnung' if not errors else f'{len(errors)} Fehler'}")
    return 1 if errors else 0


def cmd_info(args):
    for e in read_pack(Path(args.pack).read_bytes()):
        extra = ""
        if e["kind"] == TYPE_IMAGE:
            w, h, frames, states, own, fx, fy = struct.unpack("<HHHBBhh", e["body"][:12])
            extra = f"{w}×{h}, {frames} Bilder, {states} Zustände, Fuß ({fx}, {fy})" + (f", {own} eigene Farben" if own else "")
        elif e["kind"] == TYPE_TILES:
            extra = f"{struct.unpack('<H', e['body'][:2])[0]} Kacheln"
        print(f"{e['id']:08x} {TYPE_NAMES.get(e['kind'], '?'):9} {e['name']:24} {len(e['body']):6} B "
              f"(gepackt {e['packed']}) {extra}")
    return 0


def cmd_show(args):
    entries = {e["name"]: e for e in read_pack(Path(args.pack).read_bytes())}
    e = entries.get(args.name)
    if not e or e["kind"] not in (TYPE_IMAGE, TYPE_TILES):
        raise AssetError(f"kein Bild „{args.name}“ im Paket")
    pal = Palette(Path(args.palette)) if args.palette else None
    colors = list(pal.colors) if pal else [(i, i, i) for i in range(256)]
    colors += [(0, 0, 0)] * (256 - len(colors))
    body = e["body"]
    if e["kind"] == TYPE_TILES:
        n = struct.unpack("<H", body[:2])[0]
        pix, fw, h, frames = body[2 + n:], TILE, TILE, n
    else:
        fw, h, frames, _, own, _, _ = struct.unpack("<HHHBBhh", body[:12])
        for k in range(own):
            colors[OWN_BASE + k] = tuple(body[12 + k * 3:15 + k * 3])
        pix = body[12 + own * 3:]
    w = fw * frames
    out = bytearray(w * h)
    for f in range(frames):
        for y in range(h):
            out[y * w + f * fw:y * w + (f + 1) * fw] = pix[(f * h + y) * fw:(f * h + y + 1) * fw]
    write_png_indexed(Path(args.out), w, h, bytes(out), colors[:256])
    print(f"{args.out}: {frames} Bilder à {fw}×{h}")
    return 0


def cmd_convert(args):
    pal = Palette(Path(args.palette))
    w, h, px = read_png(Path(args.bild))
    if w % args.bilder:
        raise AssetError(f"Breite {w} nicht durch {args.bilder} Bilder teilbar")
    idx, own, notes = to_indices(px, w, h, pal, args.farben, Path(args.bild).name)
    for n in notes:
        print(n)
    used = sorted({i for i in idx if i})
    print(f"{Path(args.bild).name}: {w // args.bilder}×{h}, {args.bilder} Bilder, {len(used)} Farben"
          + (f", davon {len(own)} eigene" if own else "") + "; in Ordnung")
    if args.vorschau:
        colors = list(pal.colors) + [(0, 0, 0)] * (256 - len(pal.colors))
        for k, c in enumerate(own):
            colors[OWN_BASE + k] = c
        write_png_indexed(Path(args.vorschau), w, h, idx, colors)
        print(f"Vorschau: {args.vorschau}")
    return 0


def main() -> int:
    for stream in (sys.stdout, sys.stderr):  # Windows mit Umleitung: sonst cp1252 (Umlaute, Pfeile)
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("--cpp")
    b.add_argument("--out")
    b.add_argument("-v", "--verbose", action="store_true")
    sub.add_parser("check")
    i = sub.add_parser("info")
    i.add_argument("pack")
    s = sub.add_parser("zeige")
    s.add_argument("pack")
    s.add_argument("name")
    s.add_argument("--out", required=True)
    s.add_argument("--palette")
    c = sub.add_parser("wandeln")
    c.add_argument("bild")
    c.add_argument("--palette", required=True)
    c.add_argument("--farben", choices=["streng", "naechste", "eigene"], default="streng")
    c.add_argument("--bilder", type=int, default=1)
    c.add_argument("--vorschau")
    args = ap.parse_args()
    try:
        return {"build": cmd_build, "check": cmd_check, "info": cmd_info, "zeige": cmd_show,
                "wandeln": cmd_convert}[args.cmd](args)
    except AssetError as e:
        print("Fehler:", e, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
