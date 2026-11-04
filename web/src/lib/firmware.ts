// Update-Datei schon im Browser prüfen: Kopf der .bin lesen (ESP-Image, Chip, App-Beschreibung, Projekt).
// Gleiche Prüfung wie in firmware/main/web/setup.html; die Firmware prüft beim Hochladen noch einmal (ota.cpp).

import { auth } from './auth.svelte';

export type FirmwareHeader = { err: string } | { v: string; date: string; time: string; idf: string };

export const HEADER_BYTES = 288;

export function parseHeader(buf: ArrayBuffer): FirmwareHeader {
  const u8 = new Uint8Array(buf);
  const d = new DataView(buf);
  if (u8.length < HEADER_BYTES || u8[0] !== 0xe9) return { err: 'Keine ESP32-Firmware (.bin)' };
  if (d.getUint16(12, true) !== 9) return { err: 'Firmware für einen anderen Chip (kein ESP32-S3)' };
  if (d.getUint32(32, true) !== 0xabcd5432) {
    return {
      err:
        u8[32] === 0x50
          ? 'Das ist ein Factory-Image für den Web-Flasher. Für Updates die .bin ohne „factory“ nehmen.'
          : 'Keine App-Beschreibung im Image',
    };
  }
  const str = (o: number, n: number) => new TextDecoder().decode(u8.slice(o, o + n)).replace(/\0[\s\S]*$/, '');
  const project = str(80, 32);
  if (project !== 'pixelia') return { err: `Das ist keine Pixelia-Firmware (Projekt „${project}“)` };
  return { v: str(48, 32), time: str(112, 16), date: str(128, 16), idf: str(144, 32) };
}

/** Ausgang eines Uploads (B20, seit 0.5.11): „ok“ und „abgelehnt“ hat das Gerät selbst gemeldet; „unklar“ heißt, der
 *  Browser weiß es nicht (Verbindung weg, Antwort ausgeblieben, abgebrochen). Dann fragt die Seite den Status ab und
 *  sagt erst danach, ob installiert wurde */
export type UploadResult =
  | { kind: 'ok'; msg: string; v?: string }
  | { kind: 'abgelehnt'; msg: string }
  | { kind: 'unklar'; msg: string; sent: boolean };

export type UploadEvents = {
  progress: (sent: number, total: number) => void;
  sent: () => void; // alles übertragen, das Gerät prüft und antwortet
  done: (r: UploadResult) => void;
};

const STALL_MS = 30_000; // Übertragung steht (schwacher Empfang)
const ANSWER_MS = 60_000; // nach dem Senden: Prüfen und Abschließen dauert am Gerät wenige Sekunden

/** Hochladen per XHR (Fortschritt) mit Fristen fürs Senden und für die Antwort */
export function upload(file: File, ev: UploadEvents): () => void {
  const x = new XMLHttpRequest();
  let last = Date.now();
  let sentAt = 0;
  let finished = false;
  const finish = (r: UploadResult) => {
    if (finished) return;
    finished = true;
    clearInterval(watchdog);
    ev.done(r);
  };
  const unclear = (msg: string) => finish({ kind: 'unklar', msg, sent: sentAt > 0 });
  const watchdog = setInterval(() => {
    if (!sentAt && Date.now() - last >= STALL_MS) {
      x.abort();
      unclear('Die Übertragung hängt seit 30 Sekunden (schwacher WLAN-Empfang?).');
    } else if (sentAt && Date.now() - sentAt >= ANSWER_MS) {
      x.abort();
      unclear('Die Datei ist angekommen, aber Pixelia hat nach 60 Sekunden noch nicht geantwortet.');
    }
  }, 2000);
  x.open('POST', '/api/ota');
  x.setRequestHeader('Content-Type', 'application/octet-stream');
  for (const [k, v] of Object.entries(auth.headers())) x.setRequestHeader(k, v);
  x.upload.onload = () => {
    sentAt = Date.now();
    ev.sent();
  };
  x.upload.onprogress = (e) => {
    last = Date.now();
    if (e.lengthComputable) ev.progress(e.loaded, e.total);
  };
  x.onload = () => {
    let j: { ok?: boolean; msg?: string; v?: string } = {};
    try {
      j = JSON.parse(x.responseText);
    } catch {
      /* leer */
    }
    if (x.status === 200 && j.ok) finish({ kind: 'ok', msg: j.msg ?? 'Installiert', v: j.v });
    else if (j.msg) finish({ kind: 'abgelehnt', msg: j.msg });
    else unclear(`Unerwartete Antwort (${x.status}).`);
  };
  x.onerror = () => unclear('Die Verbindung ist abgebrochen.');
  x.send(file);
  return () => {
    x.abort();
    unclear('Abgebrochen.');
  };
}
