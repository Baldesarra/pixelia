// Farbwahl auf dem Gerät (seit 0.3.2, Entscheidung 2026-10-19): Das Gerät ist die Quelle, alle Browser zeigen
// dieselben Farben und die Wahl steckt in der Sicherung. Der Browser startet mit seiner zuletzt gesehenen Wahl
// (localStorage) und übernimmt dann die des Geräts. Hat das Gerät noch keine, bekommt es die des Browsers.
// Mit PIN und ohne Anmeldung gilt eine Änderung nur in diesem Browser (unsaved), bis jemand sie speichert.
import { post } from './api';
import { theme } from './m3';

class ThemeSync {
  /** zuletzt vom Gerät gemeldeter bzw. dort gespeicherter Wert; undefined = noch kein Status */
  private device: string | undefined = undefined;
  private timer = 0;
  unsaved = $state(false);

  /** Wert aus dem Status (App.svelte): nur Änderungen am Gerät übernehmen */
  fromStatus(ui: string | undefined) {
    if (ui === undefined || ui === this.device) return;
    this.device = ui;
    if (ui) {
      theme.adopt(ui);
      this.unsaved = false;
    } else if (theme.hadChoice) {
      this.changed(theme.json());  // Gerät hat noch keine Wahl: die gespeicherte des Browsers übernehmen
    }
  }

  /** Auswahl hat sich geändert (App.svelte): kurz warten, dann aufs Gerät, ohne nach dem PIN zu fragen */
  changed(json: string) {
    if (this.device === undefined || json === this.device) return;
    clearTimeout(this.timer);
    this.timer = window.setTimeout(() => void this.save(false), 600);
  }

  /** ask: bei fehlender Anmeldung nach dem PIN fragen (Knopf auf der Seite Darstellung) */
  async save(ask: boolean) {
    const ui = theme.json();
    const r = await post('/api/settings', { ui }, { quiet: !ask }).catch(() => null);
    if (r?.ok) {
      this.device = ui;
      this.unsaved = false;
    } else {
      this.unsaved = true;
    }
  }
}

export const themeSync = new ThemeSync();
