#pragma once
// Webserver auf Port 80 (Kern 0): einfache Einrichtungsseite, JSON-API, WebSocket für den Live-Status,
// Update per .bin-Upload und die Weiterleitungen des Captive Portals im Setup-Modus.
// Die richtige Weboberfläche (Svelte, M3 Expressive) folgt in 0.3.x.

namespace app::web {

void start();
int ws_clients();

}  // namespace app::web
