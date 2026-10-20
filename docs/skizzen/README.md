# Skizzen zur Darstellung

Wegwerf-Skizzen aus der Planung, aufbewahrt als Zeugnis, wie Pixelia entstanden ist. Später sollen sie als
„Easter Egg“ über einen Knopf in der Weboberfläche erreichbar sein (siehe ROADMAP, Backlog).

Alle Seiten sind eigenständige HTML-Dateien ohne Abhängigkeiten: Doppelklick genügt, sie laufen offline. Die Bilder
werden im Browser prozedural in echten 400 × 240 Pixeln mit 8-Bit-Palette gezeichnet und ohne Glättung verdoppelt,
so wie auf dem 5″-Display. Es ist kein Engine- oder Firmware-Code.

| Datei | Stand | Inhalt |
|---|---|---|
| [perspektiven-v1.html](perspektiven-v1.html) | 2026-10-20 | Fünf Perspektiven im Vergleich (Seitenansicht mit Parallax, Querschnitt, 3/4-Draufsicht, Isometrie, Bühne mit Bodentiefe), je Hof, Haus, Dorf, Zug, Tag und Nacht, Bildfolge, Vergleichstabelle |
| [szenen-v2.html](szenen-v2.html) | 2026-10-20 | Die gewählte Mischung mit fester Kamera: außen 3/4 (Hof, Dorf West, Dorf Ost), innen nahe Seitenansicht (Küche, Wohnzimmer, Zugabteil) |
| [figuren-v3.html](figuren-v3.html) | 2026-10-20 | Chibi-Figuren mit 28 px, zehn Bewohner, Küche und Wohnzimmer je in Seiten- und 3/4-Ansicht, Hof, Zugabteil, Küche mit doppelt so nahem Bild |

Die Zahlen unter den Bildern (Farben, Schreibzugriffe, sortierte Objekte, geänderte Pixel) zählt die Skizze selbst
im Browser. Sie sind Anhaltspunkte, keine Messungen am ESP32-S3.

Hinweise für die spätere Einbindung in die Weboberfläche: Die Dateien zusammen sind etwa 510 KB groß, gzip-gepackt
deutlich weniger. Ohne Internet fallen die Schriften auf Systemschriften zurück; das ist gewollt (keine CDNs).
