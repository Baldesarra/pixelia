#pragma once
// Schnittstelle für alles, was die Engine zeigt (Testszenen, später die Orte der Welten).
// Gerät und Simulator nutzen denselben Szenen-Code: Logik-Schritt, Zeichnen, Palette. Mehrere Szenen hält die
// Bühne (stage.h). Jede Szene hat ihren eigenen Zustand (Palette, später Kamera, Licht, Wetter).
// Regeln: tick() rechnet, render() zeichnet nur und ändert nichts (deshalb const); gleicher Startwert und gleiche
// Schritte ergeben dasselbe Bild, egal wie oft dazwischen gezeichnet wurde.

#include <memory>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

class Scene {
public:
    virtual ~Scene() = default;
    virtual const char* name() const = 0;    // Kennung (Befehle, Dateinamen), ASCII
    virtual const char* title() const { return name(); }  // sichtbarer Name (Weboberfläche)
    // Gleicher Startwert = gleicher Ablauf (deterministisch)
    virtual void init(uint64_t seed) = 0;
    // Ein Logik-Schritt; die Uhr ist bereits weitergezählt
    virtual void tick(const WorldClock& clock) = 0;
    // Zeichnet in 400×240 (scale() == 1) oder ins grobe Raster 200×120 (scale() == 2)
    virtual void render(Canvas& canvas) const = 0;
    virtual const Palette& palette() const = 0;
    // Lastregler für Messungen (z. B. Zahl der Sprites); Szenen ohne Regler ignorieren das
    virtual void set_load(int /*load*/) {}
    virtual int scale() const { return 1; }
    // Benutzte Paletteneinträge 0 … colors() − 1 (für die gemeinsame Palette während einer Überblendung)
    virtual int colors() const { return kSceneColors; }
    // Feine Einblendungen der Szene (Text, Markierungen) in 400×240, nach dem Verdoppeln gezeichnet
    virtual void render_overlay(Canvas& /*fine*/) const {}
};

// Szene nach Namen anlegen; nullptr, wenn es sie nicht gibt
std::unique_ptr<Scene> make_scene(const char* name);
// Namen aller Szenen, durch Komma getrennt (für Hilfetexte)
const char* scene_names();

}  // namespace pixelia
