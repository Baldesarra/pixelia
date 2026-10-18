#pragma once
// Eingabe: Touch-Daten → Gesten (engine/gesture.h) → Aktionen.
//
// Der Touch-Task (hwtest.cpp) liefert die Rohdaten, hier wird nur erkannt und verteilt. Heute gehören die Gesten
// der System-Oberfläche (langer Druck = Info-Karte, Tippen schließt sie). Wenn Welten eigene Gesten bekommen,
// erhalten sie hier alles, was die System-Oberfläche nicht verbraucht.

#include <cstdint>

namespace app::input {

// Neue Touch-Daten in Panel-Koordinaten (800×480, Bildausrichtung): fingers = 0 beim Loslassen
void on_touch(int fingers, int x, int y);
// Regelmäßig aufrufen (alle 20 ms), auch ohne neue Daten: langer Druck, Halte-Anzeige, Info-Karte aktualisieren
void tick();
void set_long_press_ms(uint32_t ms);

}  // namespace app::input

namespace app::infocard {

inline constexpr int kSeconds = 20;  // Anzeigedauer, Tippen schließt früher

void show();
void hide();
bool visible();
// Alle 2 s neue Daten (Signal, Zustand), solange die Karte sichtbar ist
void tick();

}  // namespace app::infocard
