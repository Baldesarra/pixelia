#!/usr/bin/env python3
"""Bilanz des Wirtschafts-Protokolls prüfen (seit 0.5.11): Bestand am Vortag + Buchungen des Tages = Bestand am Tagesende.

Liest einen Protokoll-Ordner (Simulator `--wirtschaft DIR` oder vom Gerät geholt) mit den Dateien
`tNNNNN-bewegungen.csv` und `tNNNNN-bestand_lager_tag.csv` und vergleicht für jeden Tag mit Vortag jeden Lagerplatz und
jede Ware. Fehlt ein Tag dazwischen (Sprung beim Nachholen), gilt der letzte Tag mit Bestandsaufnahme als Vortag und
alle Buchungen dazwischen zählen mit. Tage mit Vermerk in `tNNNNN-verluste.csv` (Tag zu voll, B08) werden genannt,
aber nicht als Abweichung gezählt. Doppelte Buchungszeilen (gleicher Text) werden gemeldet.

Beispiel:
  python tools/wirtschaft_bilanz.py build-sim-headless/wirtschaft
Exit 0 = keine Abweichung.
"""

import csv
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path


def rows(path: Path):
    if not path.exists():
        return []
    with path.open(encoding="utf-8", newline="") as f:
        lines = [ln for ln in f if not ln.startswith("#")]
    return list(csv.DictReader(lines))


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = Path(sys.argv[1])
    days = sorted({int(m.group(1)) for p in d.glob("t*-*.csv") if (m := re.match(r"t(\d{5})-", p.name))})
    if not days:
        print(f"{d}: keine Protokoll-Dateien")
        return 2
    stock = {}  # Tag -> {(lager, ware): menge}
    moves = defaultdict(lambda: defaultdict(int))  # Tag -> (lager, ware) -> Summe
    lost_days = set()
    dup = 0
    for day in days:
        snap = rows(d / f"t{day:05d}-bestand_lager_tag.csv")
        if snap:
            stock[day] = {(r["lager"], r["ware"]): int(r["menge"]) for r in snap}
        lines = Counter()
        for r in rows(d / f"t{day:05d}-bewegungen.csv"):
            lines[tuple(r.values())] += 1
            q = int(r["menge"])
            if r["von"]:
                moves[day][(r["von"], r["ware"])] -= q
            if r["nach"]:
                moves[day][(r["nach"], r["ware"])] += q
        # gleiche Zeile zweimal: nur verdächtig, wenn auch Zeit und Stück gleich sind (Einzelstücke haben eine Kennung)
        dup += sum(n - 1 for k, n in lines.items() if n > 1 and k[-1])
        if (d / f"t{day:05d}-verluste.csv").exists():
            lost_days.add(day)
    bad = 0
    checked = 0
    prev = None
    for day in days:
        if day not in stock:
            continue
        if prev is not None:
            delta = defaultdict(int)
            for k in range(prev + 1, day + 1):
                for key, q in moves[k].items():
                    delta[key] += q
            keys = set(stock[prev]) | set(stock[day]) | set(delta)
            for key in sorted(keys):
                want = stock[prev].get(key, 0) + delta.get(key, 0)
                got = stock[day].get(key, 0)
                checked += 1
                if want != got:
                    note = " (Tag mit Vermerk verluste)" if any(k in lost_days for k in range(prev + 1, day + 1)) else ""
                    if not note:
                        bad += 1
                    print(f"Tag {day}: {key[0]} {key[1]}: Vortag {stock[prev].get(key, 0)} + Buchungen "
                          f"{delta.get(key, 0):+d} = {want}, Bestand {got} ({got - want:+d}){note}")
        prev = day
    print(f"{d}: {len(days)} Tage, {checked} Posten geprüft, {bad} Abweichungen, {dup} doppelte Einzelstück-Zeilen"
          + (f", Tage mit Verlust-Vermerk: {sorted(lost_days)}" if lost_days else ""))
    return 0 if bad == 0 and dup == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
