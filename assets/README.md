# assets/ – Grafiken, Paletten und Inhalte

Hier liegen die Quellen für alles, was die Welten zeigen und erzählen:

- **Zeichenpaletten** (`paletten/*.gpl`, GIMP-Palette): Grundfarben eines Ortes mit benannten Rampen; lassen sich in
  Aseprite, LibreSprite, GIMP und Krita laden
- **Grafiken** als PNG-Streifen (alle Bilder eines Teils gleich groß nebeneinander) und je Ordner eine `pakete.json`,
  die sie zu Paketen zusammenfasst (seit 0.5.5, Beispiel `testorte/`). `tools/asset_pack.py` baut daraus die Pakete
  (`*.pxa`), die Firmware und Simulator einbetten bzw. aus LittleFS oder von der SD-Karte laden
- später **Datendateien** für Inhalte: Ereignisse, Aktivitäten, Berufe, Gegenstände, Gedanken … (datengetrieben, damit
  jede Patch-Version eine neue „Inhaltswelle“ bringen kann)

Format, Namensregeln der Palette und das Werkzeug: [docs/STIL.md](../docs/STIL.md) Kapitel 9.

Regeln:

- Fremde Grafiken **nur mit CC0- oder CC-BY-Lizenz**, abgelegt unter `fremd/<Quelle>/`, mit Eintrag in
  [CREDITS.md](CREDITS.md) (Datei, Urheber, Quelle, Lizenz).
- Eigene Dateien brauchen ebenfalls eine Zeile in CREDITS.md (Lizenz „eigen“, Platzhalter `*` erlaubt).
- `python tools/asset_pack.py check` prüft beides (läuft in der CI).
