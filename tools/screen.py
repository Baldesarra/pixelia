"""Bildschirmfoto vom Display per WLAN (seit 0.5.1): holt /api/screen und speichert ein PNG.

Aufruf:  python tools/screen.py [--target http://pixelia] [--scale 2] DATEI.png
Antwort von /api/screen (Little Endian): "PXS1", Breite u16, Höhe u16, Bildnummer u32, Verfahren u8 (1 = PackBits),
3 Byte frei, Palette 256 × RGB888, dann die gepackten Paletten-Indizes. Ohne Zusatzpakete (eigener PNG-Schreiber).
"""
import argparse
import struct
import sys
import urllib.request
import zlib


def unpack_bits(data, size):
    out = bytearray()
    i = 0
    while i < len(data) and len(out) < size:
        n = data[i] - 256 if data[i] > 127 else data[i]
        i += 1
        if n >= 0:
            out += data[i:i + n + 1]
            i += n + 1
        elif n != -128:
            out += bytes([data[i]]) * (1 - n)
            i += 1
    return bytes(out[:size])


def write_png(path, w, h, rows):
    def chunk(kind, body):
        c = struct.pack('>I', len(body)) + kind + body
        return c + struct.pack('>I', zlib.crc32(kind + body) & 0xFFFFFFFF)
    raw = b''.join(b'\x00' + r for r in rows)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(png)


def main():
    ap = argparse.ArgumentParser(description='Bildschirmfoto vom Pixelia-Display per WLAN')
    ap.add_argument('file')
    ap.add_argument('--target', default='http://pixelia')
    ap.add_argument('--scale', type=int, default=2, help='Vergrößerung (2 = wie die Simulator-Screenshots)')
    a = ap.parse_args()
    data = urllib.request.urlopen(a.target.rstrip('/') + '/api/screen', timeout=10).read()
    if data[:4] != b'PXS1' or data[12] != 1:
        sys.exit('FEHLER: unbekannte Antwort von /api/screen')
    w, h, frame = struct.unpack_from('<HHI', data, 4)
    pal = data[16:16 + 768]
    idx = unpack_bits(data[16 + 768:], w * h)
    s = max(1, a.scale)
    rows = []
    for y in range(h):
        line = bytearray()
        for x in range(w):
            k = idx[y * w + x] * 3
            line += pal[k:k + 3] * s
        rows += [bytes(line)] * s
    write_png(a.file, w * s, h * s, rows)
    print(f'{a.file}: {w * s}×{h * s}, Bild {frame}')


if __name__ == '__main__':
    main()
