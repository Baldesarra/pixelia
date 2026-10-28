#pragma once
// Figuren-Baukasten (seit 0.5.9 Teil 3; Stil des Users 2026-10-27: Chibi wie Skizze v3, „Mehr Tiefe“). Je Ebene
// (figure.h, LookLayer) gibt es Varianten als Bildstreifen mit gleichem Aufbau (Ansicht vorn, hinten, Seite zu je
// kFigurePoses Posen), gezeichnet mit der Platzhalter-Rampe (tools/figure_art.py). FigureArt legt für ein Aussehen
// (FigureLook) die Ebenen in fester Reihenfolge mit ihrer Farbreihe übereinander und zieht danach außen eine farbige
// Kontur (Tabelle: Farbe → dunkle Fassung). Die fertigen Bilder liegen im Grafik-Speicher, je Aussehen einmal, und
// entstehen mit Budget über mehrere Schritte (Scene::prepare), zuletzt Gewünschtes zuerst.

#include <cstdint>

#include "pixelia/engine/figure.h"
#include "pixelia/engine/sprite.h"

namespace pixelia {

inline constexpr int kFigureViews = 3;  // vorn, hinten, Seite (nach links gespiegelt)
inline constexpr int kFigurePoses = 5;  // stehen, Schritt, Mitte, Schritt, atmen

// Messung am Gerät (seit 0.5.9 Teil 4, Konsole „bench figuren“): über alle Baukästen; ohne Uhr (set_figure_art_clock)
// wird nicht gemessen. first_us/done_us: erstes Bild bzw. zuletzt alles fertig (Uhr der Firmware, µs seit dem Start)
struct FigureArtStats {
    uint32_t frames = 0;
    int64_t us = 0, max_us = 0;
    int64_t first_us = 0, done_us = 0;
    int64_t first_done_us = 0;  // zum ersten Mal alles fertig (so lange war nach dem Start die Testfigur zu sehen)
};
void set_figure_art_clock(int64_t (*now_us)());
const FigureArtStats& figure_art_stats();

class FigureArt {
public:
    static constexpr int kVariants = 8;  // je Ebene Varianten 1 … kVariants (0 = keine)
    static constexpr int kSlots = 24;    // fertige Aussehen zugleich (älteste ungenutzte werden ersetzt)
    static constexpr int kFrameCost = 1500;  // Aufwand je Bild im Budget (Einheiten wie light_row_cost)

    FigureArt() = default;
    FigureArt(const FigureArt&) = delete;
    FigureArt& operator=(const FigureArt&) = delete;
    ~FigureArt();

    // Bild der Variante (1 …) einer Ebene; alle Streifen gleich groß. Verwirft fertige Bilder
    void set_part(LookLayer layer, int variant, const SpriteSheet& sheet);
    // Farbreihen (look.row zeigt hinein, 0 = keine) und Kontur (256 Byte); Zeiger bleiben gültig
    void set_rows(const Recolor* rows, int count, const uint8_t* outline);
    bool has_parts() const { return width_ > 0; }

    // Bald gebraucht (betretene Orte, je Schritt): kommt beim Zusammensetzen zuerst dran
    void want(const FigureLook& look);
    // Fertiges Bild (kFigureViews × kFigurePoses) oder nullptr, solange es entsteht
    const SpriteSheet* sheet(const FigureLook& look) const;
    // Zusammensetzen mit Budget; gibt den Aufwand zurück
    int prepare(int budget);
    int ready() const;  // fertige Aussehen (Prüfungen)
    static bool any_part(const FigureLook& look);  // hat überhaupt eine Ebene (sonst zeichnet der Ort die Testfigur)

private:
    struct Slot {
        FigureLook look{};
        uint8_t* pixels = nullptr;
        SpriteSheet sheet{};
        uint32_t used = 0;  // zuletzt gewünscht (Zähler)
        int8_t frame = -1;  // nächstes Bild, -1 = leer, kFigureViews * kFigurePoses = fertig
    };
    Slot* find(const FigureLook& look);
    const Slot* find(const FigureLook& look) const;
    void compose_frame(Slot& s, int frame);
    void clear();

    SpriteSheet parts_[kLookLayers][kVariants]{};
    const Recolor* rows_ = nullptr;
    int row_count_ = 0;
    const uint8_t* outline_ = nullptr;
    int width_ = 0, height_ = 0, pivot_x_ = 0, pivot_y_ = 0;
    Slot slots_[kSlots]{};
    uint32_t tick_ = 0;
    uint8_t* scratch_ = nullptr;  // Kopie eines Bildes für die Kontur
};

}  // namespace pixelia
