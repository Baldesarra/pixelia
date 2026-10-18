# web/ – Weboberfläche des Geräts

Ab Phase 0.3.x entsteht hier die Weboberfläche, die das Gerät selbst ausliefert (`http://pixelia.local`).

- **Svelte 5 + TypeScript + Vite**, eigene Komponenten im Material-3-Expressive-Stil (hell/dunkel, dynamische Farben)
- **Komplett offline:** keine CDNs, Schriften und Icons eingebettet; gzip-komprimiert in die Firmware eingebaut
- Budget: gzip ≤ 400 KB inklusive Schriften und Icons
- Seiten: Übersicht, Info, Hardware, WLAN, Verbundene Geräte, System, später Updates, Experimentell, Welten

Details: [docs/PROJEKTPLAN.md](../docs/PROJEKTPLAN.md), Kapitel 4.8 und 7.

Nicht verwechseln mit `pages/`: Das ist die öffentliche Seite auf GitHub Pages (Web-Flasher).
