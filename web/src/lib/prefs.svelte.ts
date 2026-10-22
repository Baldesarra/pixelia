// Darstellung pro Browser (seit 0.5.4, localStorage, nicht auf dem Gerät): Ansicht schmal wie am Handy (Standard) oder
// breit mit Schiene und Spalten, Schriftgröße, Bewegung reduzieren, Startseite. Die Farbwahl liegt weiter auf dem Gerät
// (theme-sync.svelte.ts), weil sie für alle Browser gelten soll; Ansicht und Schrift passen dagegen zum jeweiligen Gerät.
const KEY = 'pixelia.prefs';

export type FontSize = 's' | 'm' | 'l';
export type StartPage = 'uebersicht' | 'welt';

class Prefs {
  wide = $state(false);
  font = $state<FontSize>('m');
  reduce = $state(false);
  start = $state<StartPage>('uebersicht');
  /** Version, deren Karte „Neu in dieser Version“ schon weggeklickt wurde */
  seen = $state('');

  constructor() {
    try {
      const p = JSON.parse(localStorage.getItem(KEY) ?? '{}') as Partial<Prefs>;
      this.wide = p.wide === true;
      if (p.font === 's' || p.font === 'l') this.font = p.font;
      this.reduce = p.reduce === true;
      if (p.start === 'welt') this.start = 'welt';
      if (typeof p.seen === 'string') this.seen = p.seen;
    } catch {
      /* ohne Speicher (privates Fenster): Standardwerte */
    }
  }

  /** Auf die Seite anwenden (html-Klassen) und merken; App.svelte ruft das in einem Effekt auf */
  apply() {
    const html = document.documentElement;
    html.classList.toggle('wide', this.wide);
    html.classList.toggle('reduce', this.reduce);
    html.dataset.font = this.font;
    try {
      localStorage.setItem(KEY, JSON.stringify({ wide: this.wide, font: this.font, reduce: this.reduce, start: this.start, seen: this.seen }));
    } catch {
      /* nicht speicherbar */
    }
  }
}

export const prefs = new Prefs();
