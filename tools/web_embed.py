#!/usr/bin/env python3
"""Weboberfläche (web/dist) gzip-komprimiert als C++-Quelle in die Firmware einbetten.

Läuft automatisch beim Firmware-Build (firmware/main/CMakeLists.txt). Ist web/dist älter als die Quellen, baut das
Skript die Oberfläche vorher neu (npm ci, falls node_modules fehlt, dann npm run build). Ohne Node.js (z. B. im
ESP-IDF-Container der CI, dort baut ein eigener Job web/dist vorher) wird ein vorhandenes web/dist benutzt.

Ausgabe: Tabelle app::web::assets (web_assets.h) mit Pfad, MIME-Typ, Daten, gzip-Kennung und ETag (Inhalts-Hash).
"""
import argparse
import gzip
import hashlib
import os
import shutil
import subprocess
import sys
from pathlib import Path

MIME = {
    ".html": "text/html; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".svg": "image/svg+xml",
    ".json": "application/json",
    ".woff2": "font/woff2",
    ".png": "image/png",
    ".ico": "image/x-icon",
}
COMPRESS = {".html", ".js", ".css", ".svg", ".json"}
SOURCES = ["src", "index.html", "vite.config.ts", "svelte.config.js", "package.json", "package-lock.json"]


def newest(web: Path) -> float:
    t = 0.0
    for name in SOURCES:
        p = web / name
        files = p.rglob("*") if p.is_dir() else [p]
        for f in files:
            if f.is_file():
                t = max(t, f.stat().st_mtime)
    return t


def build(web: Path) -> None:
    dist_index = web / "dist" / "index.html"
    stale = not dist_index.exists() or dist_index.stat().st_mtime < newest(web)
    if not stale:
        return
    npm = shutil.which("npm")
    if not npm:
        if dist_index.exists():
            print("web_embed: Warnung: web/dist ist älter als die Quellen, Node.js fehlt, nehme vorhandenes dist",
                  file=sys.stderr)
            return
        sys.exit("web_embed: web/dist fehlt und Node.js (npm) ist nicht installiert. "
                 "Node.js 24 installieren oder in web/ 'npm ci && npm run build' ausführen.")
    env = dict(os.environ, NODE_ENV="production")
    env.pop("NODE_OPTIONS", None)
    if not (web / "node_modules").exists():
        subprocess.run([npm, "ci", "--no-audit", "--no-fund"], cwd=web, check=True, env=env)
    print("web_embed: baue Weboberfläche (npm run build)")
    r = subprocess.run([npm, "run", "build"], cwd=web, env=env, capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    if r.returncode != 0:
        sys.stderr.write(r.stdout + r.stderr)
        sys.exit("web_embed: npm run build fehlgeschlagen")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--web", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args()
    web = args.web.resolve()
    build(web)
    dist = web / "dist"

    entries = []
    arrays = []
    total = raw_total = 0
    for i, f in enumerate(sorted(p for p in dist.rglob("*") if p.is_file())):
        rel = "/" + f.relative_to(dist).as_posix()
        data = f.read_bytes()
        etag = hashlib.sha1(data).hexdigest()[:12]
        mime = MIME.get(f.suffix, "application/octet-stream")
        gz = False
        if f.suffix in COMPRESS:
            packed = gzip.compress(data, 9, mtime=0)
            if len(packed) < len(data):
                data, gz = packed, True
        raw_total += f.stat().st_size
        total += len(data)
        body = ",".join(str(b) for b in data)
        arrays.append(f"alignas(4) const uint8_t d{i}[] = {{{body}}};")
        entries.append(f'    {{"{rel}", "{mime}", d{i}, {len(data)}, {"true" if gz else "false"}, "\\"{etag}\\""}},')

    out = [
        "// Erzeugt von tools/web_embed.py aus web/dist, nicht von Hand ändern.",
        '#include "web_assets.h"',
        "namespace app::web::assets {",
        "namespace {",
        *arrays,
        "}  // namespace",
        f"const Asset kAssets[] = {{",
        *entries,
        "};",
        f"const size_t kCount = {len(entries)};",
        f"const size_t kBytes = {total};",
        "}  // namespace app::web::assets",
        "",
    ]
    text = "\n".join(out)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    if not args.out.exists() or args.out.read_text(encoding="utf-8") != text:
        args.out.write_text(text, encoding="utf-8")
    print(f"web_embed: {len(entries)} Dateien, {raw_total // 1024} KB -> {total // 1024} KB eingebettet")


if __name__ == "__main__":
    main()
