#pragma once
// Himmel (seit 0.5.7): Sonnenzeiten nach der Weltjahreszeit und der echte Mond (SETTING.md Kapitel 9).
//
// - Mondphase aus dem Datum ohne Internet (Meeus, „Astronomical Algorithms“ Kap. 48, gekürzt: auf wenige Stunden
//   genau). Der Mond geht wie der echte durch: Neumond steht mittags im Süden, Vollmond um Mitternacht, dazwischen
//   je Tag etwa 50 min später; über dem Horizont etwa zwölf Stunden. Sichtbar ist er nur, wenn es dunkel und klar ist.
// - Vollmond hebt die Nachthelligkeit an (kühles Licht, apply()), bei Neumond bleibt es bei der Mindesthelligkeit.
// - Gezeichnet wird der Mond von der Bühne fein nach dem Verdoppeln, nur über Farben mit der Rolle Himmel (draußen
//   der Himmel, drinnen das Fensterglas, STIL.md Kapitel 7), in den Farben der System-Oberfläche: Er leuchtet selbst
//   und bleibt bei Nacht hell. Während eines Ortswechsels wird er nicht gezeichnet.

#include <cstdint>

#include "pixelia/engine/calendar.h"
#include "pixelia/engine/canvas.h"
#include "pixelia/engine/light.h"
#include "pixelia/engine/scene.h"  // SkyView

namespace pixelia {

struct WeatherState;

struct MoonInfo {
    double phase = 0;        // 0 … 1: 0 Neumond, 0,25 erstes Viertel, 0,5 Vollmond, 0,75 letztes Viertel
    int illum = 0;           // beleuchteter Anteil in Promille
    bool waxing = true;
    int up = 0;              // Höhe über dem Horizont 0 … 256 (0 = untergegangen)
    int x = 128;             // Lage am Himmel 0 … 256 (0 = Osten links, 256 = Westen rechts)
};
// unix_s: echte Zeit (UTC); minute: Ortszeit des Tages (für die Lage am Himmel)
MoonInfo moon_at(int64_t unix_s, int minute);
const char* moon_phase_title(const MoonInfo& m);  // Neumond, zunehmende Sichel, erstes Viertel …
// Phase in acht Abschnitten (0 Neumond … 4 Vollmond … 7 abnehmende Sichel)
int moon_octant(const MoonInfo& m);

// Aussehen (Vergleichsbilder, Befehl „mond a|b|c“)
enum class MoonStyle : uint8_t { Halo, Plain, Craters };

class Sky {
public:
    // Einmal je Weltminute (Stage::tick): Sonne aus dem Kalender, Mond aus der echten Zeit, Wetter fürs Klarsein
    void update(const CalendarDate& date, int64_t unix_s, const WeatherState& w);
    const SunTimes& sun() const { return sun_; }
    const MoonInfo& moon() const { return moon_; }
    int dark() const { return dark_; }         // 0 Tag … 256 Nacht (nach den Sonnenzeiten)
    int moonlight() const { return light_; }   // 0 … 256: Phase × Höhe × Dunkelheit × klarer Himmel
    bool moon_visible() const { return visible_; }

    // Nacht aufhellen: Umgebungslicht Richtung kühles Mondlicht, nur wenn es dunkel ist (a.lamp); drinnen halb
    void apply(Ambient& a, bool outdoor = true) const;

    void set_style(MoonStyle s) { style_ = s; }
    MoonStyle style() const { return style_; }
    // Prüfungen und Vergleichsbilder: Phase, Höhe und Lage fest vorgeben (phase < 0 = wieder echt)
    void force(double phase, int up = 200, int x = 160);

    // Mond in die Himmelsflächen zeichnen (fein, nach dem Verdoppeln). Seit 0.5.8 steht er in einem Fenster ganz in
    // einer Scheibe (Lage nach seinem Stand am Himmel, bei kleinen Scheiben kleiner). Während einer Überblendung:
    // remap = Farben des Ortes → gemeinsame Palette, owns(ctx, x, y) = zeigt dieser Bildpunkt den Ort?
    using OwnsFn = bool (*)(const void* ctx, int x, int y);
    void draw(Canvas& fine, const SkyView& view, const uint8_t* remap = nullptr, OwnsFn owns = nullptr,
              const void* ctx = nullptr) const;
    // Wo der Mond gezeichnet würde (Prüfungen): Mitte und Halbmesser fein; false = nicht zu sehen
    bool place(const SkyView& view, int* cx, int* cy, int* r, Rect* pane = nullptr) const;

private:
    struct DiscColors {
        uint8_t lit, sea, dark, halo;
    };
    void draw_disc(Canvas& fine, int cx, int cy, int r, const Rect& clip, const uint8_t* sky_lut,
                   const DiscColors& col, OwnsFn owns, const void* ctx) const;

    SunTimes sun_{};
    MoonInfo moon_{};
    int dark_ = 0, light_ = 0;
    bool visible_ = false;
    MoonStyle style_ = MoonStyle::Craters;  // Entscheidung des Users 2026-10-26: groß mit Kratern
    double forced_ = -1;
    int forced_up_ = 200, forced_x_ = 160;
};

}  // namespace pixelia
