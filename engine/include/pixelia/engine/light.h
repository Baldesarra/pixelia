#pragma once
// Farbe und Licht (seit 0.5.2). Wirkt nur auf die Szenenfarben 0–239; 240–255 gehören der System-Oberfläche und
// bleiben bei Tag und Nacht gleich (Einblendungen sind nachts lesbar). Aufbau der Palette: docs/STIL.md.
//
// - Grundfarben 0 … base−1: damit wird gezeichnet (Index 0 = durchsichtig), Farbreihen tauschen Rampen darin.
// - Lichtstufen: Stufe k liegt bei k × base + i (base = 240 / Stufen, bei drei Stufen 80). Stufe 0 = nur Umgebungs-
//   licht, die höheren = zusätzlich Lampenlicht (halb, voll). Licht = Index verschieben: Die Lichtkarte (je grobem
//   Bildpunkt die Stufen der vier feinen, Raster schon ausgerechnet) wird beim Verdoppeln angewendet (stage.h), so ist
//   das Raster fein (2 × 2 Displaypunkte). Figuren bekommen gleichmäßig die Stufe an ihrem Fußpunkt (depth.h).
//   Tagsüber sind alle Stufen gleich hell: dann wird gar nicht gerechnet (Room::set_light_on).
// - Tageslicht (Daylight): Umgebungslicht der Tageszeit aus Stützpunkten (Farbe, Sättigung, Lampenwirkung) und
//   Himmelsfarben. Die Tageszeit kommt von außen über WorldClock (bis 0.5.6 die Testuhr, dann die Weltzeit).
//   Die Dunkelheit der Palette ist nicht die Hintergrundbeleuchtung des Displays (Helligkeitskurve kommt mit 0.5.6).
// - Rollen einzelner Grundfarben: leuchtend (Fenster am Abend, Flamme: wird bei Dunkelheit zur Nachtfarbe statt
//   dunkler), Himmel (Verlauf oben → unten nach Tageszeit), fest (immer gleich).
// - Farbrotation (ColorCycle): Wasser, Feuer; eine Rampe dreht sich alle n Logik-Schritte um einen Platz.
// - Farbtabellen: Index → Index (Schatten, Tönung, später Nebel) und 50-%-Mischung (Transparenz), gesucht unter den
//   normalen Grundfarben, Lichtstufen bleiben erhalten.
// - Raster (dither4): geordnetes 4×4-Raster für weiche Übergänge (Lichtkreise, Himmel).

#include <array>
#include <cstdint>

#include "pixelia/engine/canvas.h"

namespace pixelia {

struct SunTimes;  // calendar.h

// Geordnetes 4×4-Raster: true, wenn der Punkt (x, y) beim Anteil level16 (0 … 16) gesetzt ist
inline bool dither4(int x, int y, int level16) {
    static constexpr uint8_t kBayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    return kBayer[y & 3][x & 3] < level16;
}

// Stützpunkt des Tageslichts
struct LightKey {
    uint16_t minute;  // Tageszeit 0 … 1439
    Color tint;       // Umgebungslicht je Kanal (255 = volles Tageslicht)
    uint8_t sat;      // Sättigung (255 = unverändert)
    uint8_t lamp;     // Wirkung von Lampen und leuchtenden Fenstern (tagsüber 0)
};
struct SkyKey {
    uint16_t minute;
    Color top, bottom;
};

// Umgebungslicht zu einer Tageszeit, Faktoren in 1/256
struct Ambient {
    int r = 256, g = 256, b = 256, sat = 256, lamp = 0;
    Color sky_top{110, 170, 240}, sky_bottom{190, 220, 250};
    // Dunst (seit 0.5.3, Wetter): alle Farben außer „fest“ um haze/256 Richtung haze_color; Lampenlicht und
    // leuchtende Farben scheinen nachts durch (schwächer gedämpft)
    int haze = 0;
    Color haze_color{200, 204, 210};
};

class Daylight {
public:
    static constexpr int kMaxKeys = 16;
    Daylight() = default;
    Daylight(const LightKey* keys, int n, const SkyKey* sky, int sky_n);
    // Mindesthelligkeit 0 … 256 (Mittel der drei Kanäle): dunkler wird die Nacht nicht (Lesbarkeit am Display)
    void set_floor(int k256) { floor_ = k256; }
    int floor() const { return floor_; }
    // Tageszeit in Mikrosekunden seit Mitternacht (WorldClock::us_of_day); zwischen den Stützpunkten linear.
    // Seit 0.5.7 mit Sonnenzeiten (calendar.h, nach der Weltjahreszeit): Die Stützpunkte gelten für Aufgang 6:30 und
    // Untergang 19:45; Morgen- und Abenddämmerung wandern mit Auf- und Untergang und behalten ihre Länge, Tag und Nacht
    // werden gestreckt bzw. gestaucht. nullptr = wie bis 0.5.6.
    Ambient at(int64_t us_of_day, const SunTimes* sun = nullptr) const;

private:
    std::array<LightKey, kMaxKeys> keys_{};
    std::array<SkyKey, kMaxKeys> sky_{};
    int n_ = 0, sky_n_ = 0;
    int floor_ = 0;
};

// Vorgaben des Tageslichts: 0 = Standard. Für Vergleiche (Befehl „tageslicht n“) gibt es weitere Stimmungen.
// Alle starten mit der Mindesthelligkeit kDefaultNightFloor.
inline constexpr int kDaylightPresets = 3;
inline constexpr int kDefaultNightFloor = 85;
Daylight daylight_preset(int i);
const char* daylight_preset_name(int i);

enum class ColorRole : uint8_t { Normal, Glow, Sky, Fixed };

struct ColorCycle {
    uint8_t first = 0, count = 0;
    uint16_t ticks = 1;  // Logik-Schritte je Platz
};

// Palette eines Ortes: Grundfarben mit Rollen, Lichtstufen, Farbrotation; apply() rechnet sie zur Tageszeit aus
class PlacePalette {
public:
    static constexpr int kMaxCycles = 4;
    explicit PlacePalette(int levels = 3) { set_levels(levels); }
    void set_levels(int levels);  // 1 … 3
    int levels() const { return levels_; }
    int base() const { return base_; }
    int used() const { return base_ * levels_; }  // belegte Szenenfarben

    void set(int i, Color day);
    Color get(int i) const { return day_[i]; }
    ColorRole role(int i) const { return role_[i]; }
    void set_glow(int i, Color night);     // bei Dunkelheit diese Farbe, nicht abgedunkelt
    void set_sky(int first, int count);    // Himmelsverlauf: first = oben
    void set_fixed(int i);                 // immer gleich
    // Leuchten an/aus (seit 0.5.3): aus = normale Farbe, die Nachtfarbe von set_glow bleibt gemerkt
    void set_glow_on(int i, bool on);
    bool add_cycle(const ColorCycle& c);   // false = voll oder ungültig
    void set_lamp(Color tint) { lamp_ = tint; }      // Farbe des Lampenlichts (oberste Stufe voll)
    void set_exposure(int k256) { exposure_ = k256; }  // Umgebungslicht dieses Ortes (Innenräume < 256)

    // Szenenfarben 0 … used() − 1 zur Tageszeit; tick für die Farbrotation. 240–255 bleiben unberührt.
    void apply(const Ambient& a, uint64_t tick, Palette& out) const;

    // Farbtabellen (Grundfarben am Tag, nur normale Rolle): jede Farbe um amount/256 Richtung target verschoben,
    // nächste Grundfarbe gesucht; Index 0 bleibt 0, Lichtstufen bleiben erhalten (k × base + i → k × base + j)
    void tint_table(Color target, int amount, uint8_t out[256]) const;
    // 50-%-Mischung zweier Grundfarben a, b < base: out[a * base + b] (base × base Byte)
    void blend_table(uint8_t* out) const;
    int nearest(Color c) const;  // nächste normale Grundfarbe 1 … base − 1

private:
    std::array<Color, kSceneColors> day_{};
    std::array<Color, kSceneColors> night_{};
    std::array<ColorRole, kSceneColors> role_{};
    std::array<bool, kSceneColors> used_{};  // gesetzt (unbenutzte Einträge zählen bei Farbtabellen nicht)
    std::array<ColorCycle, kMaxCycles> cycles_{};
    int cycle_count_ = 0;
    int sky_first_ = 0, sky_count_ = 0;
    int levels_ = 3, base_ = kSceneColors / 3;
    Color lamp_{255, 196, 120};
    int exposure_ = 256;
};

// Lichtkreis am Boden (3/4-Sicht: Ellipse), in groben Bildpunkten. Bis inner/255 des Halbmessers volles Licht,
// danach weicher Abfall bis zum Rand; strength 255 = bis zur obersten Stufe
struct LightCircle {
    int16_t x, y;
    uint8_t rx, ry;
    uint8_t strength = 255;
    uint8_t inner = 128;  // Standard: hell bis zur Hälfte (Entscheidung des Users 2026-10-21)
};

// Wie der Übergang zwischen den Lichtstufen aussieht (angewendet beim Verdoppeln, stage.h)
enum class LightStyle : uint8_t {
    Fine,     // feines Raster: 2 × 2 Displaypunkte, wirkt aus Betrachtungsabstand weich
    Coarse,   // grobes Raster: 4 × 4 Displaypunkte (ein grober Bildpunkt), klassischer Pixel-Look
    Stepped,  // hart gestuft: Ringe ohne Raster
};
// Lichtkarte im groben Raster w × h: je Bildpunkt die Stufen (0 … 3) seiner vier feinen Bildpunkte, je 2 Bit:
// Bit 0–1 oben links, 2–3 oben rechts, 4–5 unten links, 6–7 unten rechts. Größter Wert aller Kreise, Raster nach style.
void build_light_map(uint8_t* map, int w, int h, const LightCircle* lights, int n, int levels, LightStyle style);
// Stufe am Fußpunkt einer Figur: Mittel der Karte über das 4 × 4-Feld um (x, y), gerundet
int light_level_at(const uint8_t* map, int w, int h, int x, int y);

}  // namespace pixelia
