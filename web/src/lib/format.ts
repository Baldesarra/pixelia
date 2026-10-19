// Anzeige-Hilfen (deutsch)
const nf = (digits: number) => new Intl.NumberFormat('de-DE', { maximumFractionDigits: digits, minimumFractionDigits: digits });
const n0 = nf(0);
const n1 = nf(1);

export const num = (v: number, digits = 0) => (digits === 1 ? n1 : digits === 0 ? n0 : nf(digits)).format(v);

export function duration(s: number): string {
  const d = Math.floor(s / 86400);
  const h = Math.floor((s % 86400) / 3600);
  const m = Math.floor((s % 3600) / 60);
  if (d > 0) return `${d} T ${h} h`;
  if (h > 0) return `${h} h ${m} min`;
  if (m > 0) return `${m} min ${Math.floor(s % 60)} s`;
  return `${Math.floor(s)} s`;
}

export function kb(v: number): string {
  return v >= 1024 ? `${num(v / 1024, 1)} MB` : `${num(v)} KB`;
}

/** WLAN-Signal: Bewertung und Balken 0–4 */
export function signal(rssi: number): { bars: number; text: string } {
  if (rssi === 0) return { bars: 0, text: 'kein Signal' };
  if (rssi >= -55) return { bars: 4, text: 'sehr gut' };
  if (rssi >= -67) return { bars: 3, text: 'gut' };
  if (rssi >= -75) return { bars: 2, text: 'mittel' };
  return { bars: 1, text: 'schwach' };
}

// Zustände aus wifi::state_name() in der Firmware
export const WIFI_STATE: Record<string, string> = {
  aus: 'Aus',
  verbinde: 'Verbinde …',
  verbunden: 'Verbunden',
  setup: 'Setup-Modus',
};
