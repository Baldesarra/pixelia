"""Release-Notes für GitHub aus CHANGELOG.json erzeugen (einzige Quelle für den Versionsverlauf).

Aufruf: python tools/release_notes.py 0.1.0 [--title]
  ohne --title: Markdown-Text der Release-Notes
  mit  --title: nur der Titel („Pixelia 0.1.0 – Fundament“)
Exit 1, wenn die Version im Changelog fehlt (dann soll kein Release entstehen).
"""

import json
import pathlib
import sys

SECTIONS = [("n", "Neu"), ("i", "Verbessert"), ("f", "Behoben"), ("r", "Entfernt")]


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    version = sys.argv[1].lstrip("v")
    root = pathlib.Path(__file__).resolve().parent.parent
    entries = json.loads((root / "CHANGELOG.json").read_text(encoding="utf-8"))
    entry = next((e for e in entries if e.get("v") == version), None)
    if entry is None:
        print(f"FEHLER: Version {version} fehlt in CHANGELOG.json", file=sys.stderr)
        return 1
    title = f"Pixelia {version} – {entry['t']}"
    if "--title" in sys.argv:
        print(title)
        return 0

    lines = [f"## {title}", "", f"*{entry['s']}* · {entry['d']}", ""]
    for key, name in SECTIONS:
        items = entry.get(key) or []
        if items:
            lines.append(f"### {name}")
            lines += [f"- {item}" for item in items]
            lines.append("")
    lines += [
        "### Installation",
        "",
        "- **Web-Flasher:** https://baldesarra.github.io/pixelia/ öffnen (Chrome oder Edge am PC), Board per "
        "USB-Datenkabel anschließen, auf „Installieren“ klicken.",
        f"- **Von Hand:** `pixelia-v{version}-factory.bin` mit esptool ab Adresse `0x0` schreiben.",
        f"- `pixelia-v{version}.bin` ist das App-Image für Updates (OTA ab Version 0.2.x).",
        "",
    ]
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
