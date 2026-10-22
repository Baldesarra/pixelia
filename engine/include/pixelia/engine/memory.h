#pragma once
// Speicher für Grafiken und andere große, selten geschriebene Daten (seit 0.5.0).
// Auf dem Gerät setzt die Firmware den PSRAM ein (der knappe interne RAM bleibt für Puffer, die jedes Bild
// beschrieben werden); im Simulator ist es der normale Heap. Dorthin lädt der Grafik-Speicher (assets.h, seit 0.5.5).

#include <cstddef>

namespace pixelia {

using AllocFn = void* (*)(size_t bytes);
using FreeFn = void (*)(void* p);

void set_asset_allocator(AllocFn alloc, FreeFn release);
void* asset_alloc(size_t bytes);  // nullptr = kein Platz
void asset_free(void* p);

}  // namespace pixelia
