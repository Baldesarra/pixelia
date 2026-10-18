# pages/ – Seite auf GitHub Pages

Öffentliche Seite unter **https://baldesarra.github.io/pixelia/**:

- **Web-Flasher** (ESP Web Tools): Pixelia per USB direkt aus Chrome oder Edge installieren, ohne Werkzeuge
- aktuelle **Simulator-Bilder**
- später (0.5.x) der **Browser-Simulator**

Gebaut und veröffentlicht vom Workflow `.github/workflows/pages.yml`: nach jedem Release, bei Änderungen in `pages/`
und auf Knopfdruck. Der Workflow holt das Factory-Image der neuesten Version (`pixelia-vX.Y.Z-factory.bin` aus dem
GitHub-Release), schreibt `manifest.json` und `version.txt` und erzeugt die Simulator-Bilder.

Nicht verwechseln mit `web/`: Das ist die Weboberfläche, die das Gerät selbst ausliefert.
