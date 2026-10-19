# web/ – Weboberfläche des Geräts

Die Oberfläche, die Pixelia selbst ausliefert (`http://pixelia/` bzw. `http://pixelia.local`), seit 0.3.0.

- **Svelte 5 + TypeScript + Vite**, eigene Komponenten im Material-3-Expressive-Stil (`src/lib/m3/`)
- **Komplett offline:** keine CDNs; Schrift (Roboto Flex, reduziert) und Icons (Material Symbols, nur die benutzten)
  sind eingebettet. Der Firmware-Build bettet `dist/` gzip-komprimiert ein (`tools/web_embed.py`).
- Ziel: gzip ≤ 150 KB, Budget laut Projektplan 400 KB (0.3.0: 89 KB)

## Befehle (in `web/`)

| Befehl | Zweck |
|---|---|
| `npm ci` | Abhängigkeiten installieren (einmalig, Node.js 24) |
| `npm run dev` | Entwicklung mit dem echten Gerät (Proxy auf `http://pixelia`, anderes Ziel: `PIXELIA_TARGET`) |
| `npm run mock` | Entwicklung ohne Gerät, `/api/status` mit Beispielwerten |
| `npm run check` | Typprüfung (läuft auch in der CI) |
| `npm run build` | Build nach `dist/` (macht der Firmware-Build bei Bedarf selbst) |

Neue Icons: Namen in `src/lib/m3/icon-names.ts` eintragen. Neue Sonderzeichen: `tools/font_subset.py` ergänzen und
ausführen. Lizenzen: [assets/CREDITS.md](../assets/CREDITS.md).

Nicht verwechseln mit `pages/`: Das ist die öffentliche Seite auf GitHub Pages (Web-Flasher).
