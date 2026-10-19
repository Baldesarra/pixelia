// Update-Datei schon im Browser prüfen: Kopf der .bin lesen (ESP-Image, Chip, App-Beschreibung, Projekt).
// Gleiche Prüfung wie in firmware/main/web/setup.html; die Firmware prüft beim Hochladen noch einmal (ota.cpp).

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

export type UploadEvents = {
  progress: (sent: number, total: number) => void;
  done: (ok: boolean, msg: string, version?: string) => void;
};

/** Hochladen per XHR (Fortschritt); bricht ab, wenn 30 s nichts vorangeht (schwacher Empfang) */
export function upload(file: File, ev: UploadEvents): () => void {
  const x = new XMLHttpRequest();
  let last = Date.now();
  let sent = false;
  let finished = false;
  const finish = (ok: boolean, msg: string, v?: string) => {
    if (finished) return;
    finished = true;
    clearInterval(watchdog);
    ev.done(ok, msg, v);
  };
  const watchdog = setInterval(() => {
    if (sent || Date.now() - last < 30_000) return;
    x.abort();
    finish(false, 'Die Übertragung hängt seit 30 Sekunden (schwacher WLAN-Empfang?). Es wurde nichts installiert, bitte noch einmal versuchen.');
  }, 2000);
  x.open('POST', '/api/ota');
  x.setRequestHeader('Content-Type', 'application/octet-stream');
  x.upload.onload = () => (sent = true);
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
    if (x.status === 200 && j.ok) finish(true, j.msg ?? 'Installiert', j.v);
    else finish(false, j.msg || `Fehler ${x.status}`);
  };
  x.onerror = () => finish(false, 'Verbindung abgebrochen, es wurde nichts installiert. Bitte noch einmal versuchen.');
  x.send(file);
  return () => {
    x.abort();
    finish(false, 'Abgebrochen, es wurde nichts installiert.');
  };
}
