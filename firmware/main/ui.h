#pragma once
// Systemanzeigen auf dem Display (gezeichnet im Grafik-Task, siehe engine/system_ui.h).
// Vorrang: Update > Meldung > Setup-Modus > Welt. Kurzmeldungen erscheinen über der Welt.
// Alle Funktionen dürfen von jedem Task aus aufgerufen werden.

#include <string_view>

#include "pixelia/engine/system_ui.h"

namespace app::ui {

void show_setup(const pixelia::ui::SetupInfo& info);  // zeigen bzw. aktualisieren
void hide_setup();

void show_update(const pixelia::ui::UpdateInfo& info);
void hide_update();

void show_message(const pixelia::ui::MessageInfo& info, int seconds);
void hide_message();

void toast(std::string_view text, pixelia::ui::Tone tone, int seconds);

}  // namespace app::ui
