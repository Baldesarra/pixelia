// Live-Status vom Gerät: WebSocket /ws (1/s), sonst Abfrage /api/status alle 2 s.
// Ist die Seite länger als 60 s unsichtbar, wird die Verbindung geschlossen (spart einen der 7 Sockets am Gerät).
// Über denselben WebSocket meldet sich der Browser für „Verbundene Geräte“ an (Geräte-ID, Browser, aktuelle Seite).

import { agentLabel, clientId } from './client';

export type Status = {
  v: string;
  built: string;
  idf: string;
  part: string;
  uptime: number;
  host: string;
  wifi: {
    state: string;
    ssid: string;
    rssi: number;
    ch: number;
    ip: string;
    ap: boolean;
    apClients: number;
    retry: number;
    err: string;
    saved: number;
  };
  time: { valid: boolean; local: string };
  perf: {
    fps: number;
    render: number;
    isr: number;
    lcd: number;
    cpu0: number;
    cpu1: number;
    desync: number;
    ram: number;
    ramMin: number;
    ramBlk: number;
    psram: number;
    psramMin: number;
  };
  ota: {
    busy: boolean;
    src?: 'upload' | 'github'; // seit 0.4.1
    bytes: number;
    total: number;
    newV: string;
    err: string;
    trial: boolean;
    trialS: number;
    trialNeed: number;
    trialNote: string;
    passed: boolean;
    // seit 0.5.11 (V08): Zustand des Selbsttests, erwartete Panelrate, Neustart steht bevor (Update geschrieben)
    trialState?: TrialState;
    trialHz?: number;
    reboot?: boolean;
    avail?: string; // seit 0.4.2: neuere Version auf GitHub (Prüfung im Hintergrund), leer = keine
    ghSeq?: number; // seit 0.4.2: ändert sich mit jedem neuen Stand von /api/update
  };
  rollback: { v: string; why: string } | null;
  set: {
    rot: number;
    bright: number;
    lp: number;
    ui?: string;
    exp?: boolean;
    // seit 0.5.6: Pixel-Uhr (Ecke 0 oben rechts, 1 oben links, 2 unten rechts, 3 unten links) und Kamera
    clk?: boolean;
    clkCorner?: number;
    clkSec?: boolean;
    camHome?: number;
    camFig?: number;
    camPlace?: number;
    camRet?: number;
    night?: boolean;   // Nachtmodus (seit 0.5.7)
    nightBr?: number;
    nightNow?: boolean;
    nightFrom?: number;
    nightTo?: number;
    pend?: boolean;    // seit 0.5.11: übernommen, aber noch nicht dauerhaft gespeichert (neuer Versuch alle 30 s)
  };
  auth?: { pin: boolean; lockS: number };
  diag: {
    boot: number;
    reset: string;
    errors: number;
    warnings: number;
    logLines: number;
    crash: { task: string; reason: string; v: string } | null;
  };
};

// Selbsttest nach einem Update (firmware: trial_phase_name in engine/src/device_flow.cpp)
export type TrialState = 'läuft' | 'bestätigen' | 'unbestätigt' | 'bestanden' | 'nicht bestätigt' | 'Rückfall' | 'Rückfall gescheitert';

export type Link ='verbinde' | 'live' | 'abfrage' | 'getrennt';

class Device {
  data = $state<Status | null>(null);
  link = $state<Link>('verbinde');
  /** Zeitpunkt der letzten Daten (ms, performance.now) */
  seen = $state(0);

  #ws: WebSocket | null = null;
  #fails = 0;
  #retry = 0;
  #poll = 0;
  #req: AbortController | null = null;
  #hidden = 0;
  #page = '';

  start() {
    this.#connect();
    this.#schedule(0); // erste Werte sofort, nicht erst mit der ersten WebSocket-Nachricht
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) {
        this.#hidden = window.setTimeout(() => this.#close(), 60_000);
        clearTimeout(this.#poll);
        this.#req?.abort();
      } else {
        clearTimeout(this.#hidden);
        if (!this.#ws) this.#connect();
        this.#schedule(0);
      }
    });
  }

  // B21 (seit 0.5.11): höchstens eine Abfrage zur Zeit, die nächste erst 2 s nach dem Ende der vorigen
  #schedule(ms: number) {
    clearTimeout(this.#poll);
    this.#poll = window.setTimeout(() => void this.#watch(), ms);
  }

  #connect() {
    clearTimeout(this.#retry);
    let ws: WebSocket;
    try {
      ws = new WebSocket(`${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws`);
    } catch {
      this.#scheduleRetry();
      return;
    }
    this.#ws = ws;
    ws.onopen = () => {
      this.#fails = 0;
      ws.send(new URLSearchParams({ hello: clientId, ua: agentLabel(), page: this.#page }).toString());
    };
    ws.onmessage = (e) => {
      try {
        this.#take(JSON.parse(e.data), 'live');
      } catch {
        /* unvollständige Nachricht ignorieren */
      }
    };
    ws.onclose = () => {
      if (this.#ws === ws) this.#ws = null;
      this.#fails++;
      if (!document.hidden) this.#scheduleRetry();
    };
  }

  /** Aktuelle Seite ans Gerät melden (Liste „Verbundene Geräte“) */
  setPage(page: string) {
    this.#page = page;
    if (this.#ws?.readyState === WebSocket.OPEN) this.#ws.send(new URLSearchParams({ page }).toString());
  }

  #scheduleRetry() {
    // 2 s, 4 s, 8 s … höchstens 15 s
    const delay = Math.min(15_000, 2000 * 2 ** Math.min(3, this.#fails - 1));
    this.#retry = window.setTimeout(() => this.#connect(), delay);
  }

  #close() {
    clearTimeout(this.#retry);
    const ws = this.#ws;
    this.#ws = null;
    ws?.close();
  }

  #take(d: Status, via: Link) {
    this.data = d;
    this.link = via;
    this.seen = performance.now();
  }

  // Keine Live-Daten seit 3,5 s: einmal abfragen (z. B. Captive-Portal-Fenster ohne WebSocket). Frist 5 s; eine
  // Antwort, die erst nach neueren Live-Daten ankommt, wird verworfen (sonst spränge die Anzeige zurück)
  async #watch() {
    if (document.hidden || this.#req) return;
    if (this.data && performance.now() - this.seen < 3500) return this.#schedule(2000);
    const ctl = new AbortController();
    this.#req = ctl;
    const timer = window.setTimeout(() => ctl.abort(), 5000);
    const asked = performance.now();
    try {
      const r = await fetch('/api/status', { cache: 'no-store', signal: ctl.signal });
      if (!r.ok) throw new Error(String(r.status));
      const d = (await r.json()) as Status;
      if (this.seen <= asked) this.#take(d, this.#ws?.readyState === WebSocket.OPEN ? 'live' : 'abfrage');
    } catch {
      if (!document.hidden && this.seen <= asked) this.link = this.data ? 'getrennt' : 'verbinde';
    } finally {
      clearTimeout(timer);
      this.#req = null;
      if (!document.hidden) this.#schedule(2000);
    }
  }
}

export const device = new Device();
