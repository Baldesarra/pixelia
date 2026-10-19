#pragma once
// Systemanzeigen auf dem Display (gezeichnet im Grafik-Task, siehe engine/system_ui.h).
// Vorrang: Update > Zurücksetzen > Meldung > Setup-Modus > Welt. Kurzmeldungen erscheinen über der Welt,
// die Info-Karte und die Halte-Anzeige über allem außer dem Update.
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

// Info-Karte (langer Druck): show() startet die Anzeigedauer neu, refresh_info() ändert nur die Daten
void show_info(const pixelia::ui::InfoCardData& data, int seconds);
void refresh_info(const pixelia::ui::InfoCardData& data);
void hide_info();
bool info_visible();
bool update_visible();

// Zurücksetzen am Gerät (über allem außer dem Update): zeigen bzw. aktualisieren, Anzeigedauer startet neu
void show_reset(const pixelia::ui::ResetInfo& info, int seconds);
void hide_reset();
bool reset_visible();

// Halte-Anzeige während eines langen Drucks: 0..1, negativ = ausblenden
void set_hold_progress(float progress);

}  // namespace app::ui
