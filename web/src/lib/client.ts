// Dieser Browser für „Verbundene Geräte“: zufällige Geräte-ID (localStorage) und Kurzname aus der Browser-Kennung.
// Ein echter Gerätename ist im Browser nicht verfügbar (Projektplan 4.12).

const KEY = 'pixelia.client';

function makeId(): string {
  const b = new Uint8Array(6);
  crypto.getRandomValues(b);
  return Array.from(b, (x) => x.toString(16).padStart(2, '0')).join('');
}

export const clientId: string = (() => {
  try {
    const old = localStorage.getItem(KEY);
    if (old && /^[0-9a-f]{12}$/.test(old)) return old;
    const id = makeId();
    localStorage.setItem(KEY, id);
    return id;
  } catch {
    return makeId(); // privater Modus, Captive-Portal-Fenster: ID gilt nur für diese Sitzung
  }
})();

/** z. B. „Chrome · Android“, „Edge · Windows“, „Safari · iOS“ */
export function agentLabel(ua = navigator.userAgent): string {
  const browser = /Edg\//.test(ua)
    ? 'Edge'
    : /OPR\/|Opera/.test(ua)
      ? 'Opera'
      : /SamsungBrowser/.test(ua)
        ? 'Samsung Internet'
        : /Firefox\/|FxiOS/.test(ua)
          ? 'Firefox'
          : /Chrome\/|CriOS/.test(ua)
            ? 'Chrome'
            : /Safari\//.test(ua)
              ? 'Safari'
              : 'Browser';
  const os = /Android/.test(ua)
    ? 'Android'
    : /iPhone|iPod/.test(ua)
      ? 'iOS'
      : /iPad/.test(ua) || (/Macintosh/.test(ua) && navigator.maxTouchPoints > 1)
        ? 'iPadOS'
        : /Windows/.test(ua)
          ? 'Windows'
          : /CrOS/.test(ua)
            ? 'ChromeOS'
            : /Mac OS X/.test(ua)
              ? 'macOS'
              : /Linux/.test(ua)
                ? 'Linux'
                : '';
  // Android-Captive-Portal-Fenster (WebView) kennzeichnet sich mit „; wv)“
  const wv = /; wv\)/.test(ua) ? ' (WebView)' : '';
  return os ? `${browser}${wv} · ${os}` : browser + wv;
}

/** Handy oder Rechner (für das Symbol in der Geräteliste) */
export function isPhone(agent: string): boolean {
  return /Android|iOS|iPadOS/.test(agent);
}
