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
    bytes: number;
    total: number;
    newV: string;
    err: string;
    trial: boolean;
    trialS: number;
    trialNeed: number;
    trialNote: string;
    passed: boolean;
  };
  rollback: { v: string; why: string } | null;
  set: { rot: number; bright: number; lp: number; ui?: string };
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

export type Link = 'verbinde' | 'live' | 'abfrage' | 'getrennt';

class Device {
  data = $state<Status | null>(null);
  link = $state<Link>('verbinde');
  /** Zeitpunkt der letzten Daten (ms, performance.now) */
  seen = $state(0);

  #ws: WebSocket | null = null;
  #fails = 0;
  #retry = 0;
  #poll = 0;
  #hidden = 0;
  #page = '';

  start() {
    this.#connect();
    void this.#watch(); // erste Werte sofort, nicht erst mit der ersten WebSocket-Nachricht
    this.#poll = window.setInterval(() => this.#watch(), 2000);
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) {
        this.#hidden = window.setTimeout(() => this.#close(), 60_000);
      } else {
        clearTimeout(this.#hidden);
        if (!this.#ws) this.#connect();
      }
    });
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

  // Keine Live-Daten seit 3,5 s: einmal abfragen (z. B. Captive-Portal-Fenster ohne WebSocket)
  async #watch() {
    if (document.hidden || (this.data && performance.now() - this.seen < 3500)) return;
    try {
      const r = await fetch('/api/status', { cache: 'no-store' });
      if (!r.ok) throw new Error(String(r.status));
      this.#take(await r.json(), this.#ws?.readyState === WebSocket.OPEN ? 'live' : 'abfrage');
    } catch {
      this.link = this.data ? 'getrennt' : 'verbinde';
    }
  }
}

export const device = new Device();
