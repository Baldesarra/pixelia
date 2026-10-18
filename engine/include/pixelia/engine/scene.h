#pragma once
// Schnittstelle für alles, was die Engine zeigt (Testszenen, später die Welten).
// Gerät und Simulator nutzen denselben Szenen-Code: Logik-Schritt, Zeichnen in 400×240, Palette.

#include <memory>

#include "pixelia/engine/canvas.h"
#include "pixelia/engine/world_clock.h"

namespace pixelia {

class Scene {
public:
    virtual ~Scene() = default;
    virtual const char* name() const = 0;
    // Gleicher Startwert = gleicher Ablauf (deterministisch)
    virtual void init(uint64_t seed) = 0;
    // Ein Logik-Schritt; die Uhr ist bereits weitergezählt
    virtual void tick(const WorldClock& clock) = 0;
    virtual void render(Canvas& canvas) const = 0;
    virtual const Palette& palette() const = 0;
    // Lastregler für Messungen (z. B. Zahl der Sprites); Szenen ohne Regler ignorieren das
    virtual void set_load(int /*load*/) {}
};

// Szene nach Namen anlegen; nullptr, wenn es sie nicht gibt
std::unique_ptr<Scene> make_scene(const char* name);
// Namen aller Szenen, durch Komma getrennt (für Hilfetexte)
const char* scene_names();

}  // namespace pixelia
