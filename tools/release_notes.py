"""Release-Notes für GitHub aus CHANGELOG.json erzeugen und das Changelog-Format prüfen.

CHANGELOG.json ist die einzige Quelle für den Versionsverlauf (neuester Eintrag oben).

Aufruf:
  python tools/release_notes.py 0.2.0           Markdown-Text der Release-Notes
  python tools/release_notes.py 0.2.0 --title   nur der Titel („Pixelia 0.2.0 – Vernetzt“)
  python tools/release_notes.py --check         Format aller Einträge prüfen (läuft in der CI)
  python tools/release_notes.py --embed OUT.cpp Changelog-Block für die Firmware erzeugen (läuft im Firmware-Build)

Exit 1, wenn die Version fehlt, unvollständig ist (Datum, Titel, Untertitel) oder die Prüfung scheitert.

Changelog-Block (seit 0.4.0): CHANGELOG.json ohne Leerraum, gzip, mit festem Kopf als ein konstantes Feld in der
Firmware. Es landet im Flash-Segment für Konstanten (DROM) und steht damit am Stück in der .bin. So kann die
Weboberfläche den Verlauf einer gewählten Update-Datei vor dem Hochladen lesen (web/src/lib/changelog.ts), und
/api/changelog liefert die gzip-Daten direkt aus (Content-Encoding: gzip). Kopf, 24 Byte:
  0  16  Marker „PIXELIA:CHLOG:v1“ (ASCII)
  16  4  Länge der gzip-Daten (uint32, little endian)
  20  4  CRC-32 der gzip-Daten (uint32, little endian; schützt die Suche vor zufälligen Treffern)
  24  …  gzip-Daten (JSON-Liste wie CHANGELOG.json)
"""

import gzip
import hashlib
import json
import pathlib
import re
import struct
import sys
import zlib

REPO_URL = "https://github.com/Baldesarra/pixelia"

# Reihenfolge der Abschnitte in Release-Notes und Versionsverlauf
SECTIONS = [
    ("n", "Neu"),
    ("i", "Verbessert"),
    ("f", "Behoben"),
    ("r", "Entfernt"),
    ("g", "Geprüft"),
    ("h", "Hinweise"),
]
TEXT_KEYS = {"v", "d", "t", "s"}
ALLOWED_KEYS = TEXT_KEYS | {key for key, _ in SECTIONS} | {"pr"}

VERSION_RE = re.compile(r"^(\d+)\.(\d+)\.(\d+)(?:-([0-9A-Za-z.-]+))?$")
DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")
# Jeder Stichpunkt beginnt mit dem Bereich, z. B. „WLAN: …“, „Doku: …“
AREA_RE = re.compile(r"^[A-ZÄÖÜ][A-Za-zÄÖÜäöüß0-9-]*: \S")
# Kurz und konkret: Details gehören in die PR-Beschreibung. Gilt ab 0.2.1 (ältere Einträge bleiben unverändert).
MAX_ITEM_CHARS = 220
MAX_ITEMS = 24
LIMITS_FROM = (0, 2, 1)


def load_entries():
    root = pathlib.Path(__file__).resolve().parent.parent
    return json.loads((root / "CHANGELOG.json").read_text(encoding="utf-8"))


def version_key(version):
    """Sortierschlüssel: Vorabversion (X.Y.Z-beta.N) kommt vor der fertigen Version X.Y.Z."""
    m = VERSION_RE.match(version)
    major, minor, patch, pre = m.groups()
    return (int(major), int(minor), int(patch), 0 if pre else 1, pre or "")


def check(entries):
    errors = []
    if not isinstance(entries, list) or not entries:
        return ["CHANGELOG.json muss eine nicht leere Liste sein"]
    seen = set()
    for index, entry in enumerate(entries):
        where = f"Eintrag {index + 1} ({entry.get('v', '?')})"
        unknown = set(entry) - ALLOWED_KEYS
        if unknown:
            errors.append(f"{where}: unbekannte Felder {sorted(unknown)}")
        for key in TEXT_KEYS:
            if not isinstance(entry.get(key), str):
                errors.append(f"{where}: Feld „{key}“ fehlt oder ist kein Text")
        version = entry.get("v", "")
        if not VERSION_RE.match(version):
            errors.append(f"{where}: Version „{version}“ ist nicht X.Y.Z bzw. X.Y.Z-beta.N")
        elif version in seen:
            errors.append(f"{where}: Version doppelt")
        seen.add(version)
        released = bool(entry.get("d"))
        if released:
            if not DATE_RE.match(entry["d"]):
                errors.append(f"{where}: Datum „{entry['d']}“ ist nicht JJJJ-MM-TT")
            if not entry.get("t") or not entry.get("s"):
                errors.append(f"{where}: veröffentlichte Version braucht Titel (t) und Untertitel (s)")
        elif index != 0:
            errors.append(f"{where}: nur der oberste Eintrag darf unveröffentlicht sein (Datum leer)")
        for key, name in SECTIONS:
            items = entry.get(key, [])
            if not isinstance(items, list):
                errors.append(f"{where}: „{key}“ ({name}) muss eine Liste sein")
                continue
            for item in items:
                if not isinstance(item, str) or not AREA_RE.match(item):
                    errors.append(f"{where}, {name}: Stichpunkt beginnt nicht mit „Bereich: “ → {item!r}")
        if VERSION_RE.match(version) and version_key(version)[:3] >= LIMITS_FROM:
            all_items = [x for key, _ in SECTIONS for x in entry.get(key, []) if isinstance(x, str)]
            if len(all_items) > MAX_ITEMS:
                errors.append(f"{where}: {len(all_items)} Stichpunkte, höchstens {MAX_ITEMS} (zusammenfassen)")
            for item in all_items:
                if len(item) > MAX_ITEM_CHARS:
                    errors.append(f"{where}: Stichpunkt mit {len(item)} Zeichen, höchstens {MAX_ITEM_CHARS} → {item[:60]!r}…")
        prs = entry.get("pr", [])
        if not isinstance(prs, list) or not all(isinstance(n, int) and n > 0 for n in prs):
            errors.append(f"{where}: „pr“ muss eine Liste von PR-Nummern sein")
    versions = [e.get("v", "") for e in entries if VERSION_RE.match(e.get("v", ""))]
    if versions != sorted(versions, key=version_key, reverse=True):
        errors.append("Versionen sind nicht absteigend sortiert (neueste oben)")
    return errors


def notes(entry, version):
    title = f"Pixelia {version} – {entry['t']}"
    lines = [f"## {title}", "", f"*{entry['s']}* · {entry['d']}", ""]
    for key, name in SECTIONS:
        items = entry.get(key) or []
        if items:
            lines.append(f"### {name}")
            lines += [f"- {item}" for item in items]
            lines.append("")
    prs = entry.get("pr") or []
    if prs:
        lines.append("### Pull Requests")
        lines.append(", ".join(f"[#{n}]({REPO_URL}/pull/{n})" for n in prs))
        lines.append("")
    lines += [
        "### Installation",
        "",
        "- **Update (ab 0.2.0):** auf der Einrichtungsseite des Geräts unter „Update“ die Datei "
        f"`pixelia-v{version}.bin` hochladen.",
        "- **Erstinstallation:** Web-Flasher https://baldesarra.github.io/pixelia/ öffnen (Chrome oder Edge am PC), "
        "Board per USB-Datenkabel anschließen, auf „Installieren“ klicken.",
        f"- **Von Hand:** `pixelia-v{version}-factory.bin` mit esptool ab Adresse `0x0` schreiben.",
        "",
    ]
    return title, "\n".join(lines)


EMBED_MAGIC = b"PIXELIA:CHLOG:v1"


def embed(entries, out):
    """C++-Quelle mit dem Changelog-Block schreiben (Aufbau siehe oben); nur bei Änderung, damit nichts neu baut."""
    compact = json.dumps(entries, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    packed = gzip.compress(compact, 9, mtime=0)
    block = EMBED_MAGIC + struct.pack("<II", len(packed), zlib.crc32(packed)) + packed
    etag = hashlib.sha1(packed).hexdigest()[:12]
    body = ",".join(str(b) for b in block)
    text = "\n".join([
        "// Erzeugt von tools/release_notes.py --embed aus CHANGELOG.json, nicht von Hand ändern.",
        '#include "web_assets.h"',
        "namespace app::web::assets {",
        "namespace {",
        # used: der Block muss auch dann in der .bin bleiben, wenn der Linker ungenutzte Daten entfernt
        f"__attribute__((used)) alignas(4) const uint8_t kBlock[] = {{{body}}};",
        "}  // namespace",
        f'const Asset kChangelog = {{"/api/changelog", "application/json", kBlock + {len(block) - len(packed)}, '
        f'{len(packed)}, true, "\\"{etag}\\""}};',
        "}  // namespace app::web::assets",
        "",
    ])
    out = pathlib.Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if not out.exists() or out.read_text(encoding="utf-8") != text:
        out.write_text(text, encoding="utf-8")
    print(f"release_notes: Changelog {len(entries)} Versionen, {len(compact) // 1024} KB -> "
          f"{len(packed) // 1024} KB eingebettet")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    entries = load_entries()

    if args[0] == "--embed" and len(args) == 2:
        # Formfehler halten den Firmware-Build nicht auf (die CI prüft mit --check), sie werden nur gemeldet
        for error in check(entries):
            print(f"release_notes: Warnung: {error}", file=sys.stderr)
        embed(entries, args[1])
        return 0

    if args[0] == "--check":
        errors = check(entries)
        for error in errors:
            print(f"FEHLER: {error}", file=sys.stderr)
        if errors:
            return 1
        top = entries[0]
        state = "unveröffentlicht" if not top.get("d") else f"veröffentlicht {top['d']}"
        print(f"CHANGELOG.json ok: {len(entries)} Einträge, oberster {top['v']} ({state})")
        return 0

    version = args[0].lstrip("v")
    entry = next((e for e in entries if e.get("v") == version), None)
    if entry is None:
        print(f"FEHLER: Version {version} fehlt in CHANGELOG.json", file=sys.stderr)
        return 1
    if not entry.get("d") or not entry.get("t") or not entry.get("s"):
        print(f"FEHLER: Version {version} ist noch nicht fertig (Datum, Titel und Untertitel setzen)", file=sys.stderr)
        return 1
    title, text = notes(entry, version)
    print(title if "--title" in args else text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
