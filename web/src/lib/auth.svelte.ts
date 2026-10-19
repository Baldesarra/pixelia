// Anmeldung bei gesetztem PIN (firmware/main/auth.h): Das Gerät gibt nach dem PIN ein Sitzungs-Token heraus, das
// hier im localStorage liegt und bei jeder ändernden Anfrage im Kopf „X-Pixelia-Token“ mitgeht. Nach einem Neustart
// des Geräts oder einem PIN-Wechsel ist es ungültig; dann fragt die Oberfläche beim nächsten Mal wieder nach dem PIN.
import { promptDialog } from './m3';

const KEY = 'pixelia.token';

function read(): string {
  try {
    return localStorage.getItem(KEY) ?? '';
  } catch {
    return '';
  }
}

export type AuthState = { pin: boolean; ok: boolean; lockS: number; left: number };

class Auth {
  token = $state(read());

  headers(): Record<string, string> {
    return this.token ? { 'X-Pixelia-Token': this.token } : {};
  }

  save(token: string) {
    this.token = token;
    try {
      if (token) localStorage.setItem(KEY, token);
      else localStorage.removeItem(KEY);
    } catch {
      /* privates Fenster: Token gilt dann nur bis zum Neuladen */
    }
  }

  /** Stand beim Gerät: PIN an? Token gültig? */
  async state(): Promise<AuthState> {
    const r = await fetch('/api/auth', { cache: 'no-store', headers: this.headers() });
    return (await r.json()) as AuthState;
  }

  /** PIN abfragen und anmelden; false = abgebrochen */
  async login(reason = 'Diese Aktion ist mit einem PIN geschützt.'): Promise<boolean> {
    const pin = await promptDialog({
      title: 'PIN eingeben',
      text: reason,
      icon: 'lock',
      ok: 'Anmelden',
      field: {
        label: 'PIN oder Passwort',
        type: 'password',
        autocomplete: 'current-password',
        supporting: 'PIN vergessen? Am Gerät lange drücken, dann „Zurücksetzen …“.',
        check: async (v) => {
          if (!v) return 'Bitte den PIN eingeben';
          const r = await fetch('/api/login', { method: 'POST', body: new URLSearchParams({ pin: v }) });
          const j = (await r.json()) as { ok: boolean; msg?: string; token?: string };
          if (!j.ok || !j.token) return j.msg ?? 'PIN falsch';
          this.save(j.token);
          return null;
        },
      },
    });
    return pin !== null;
  }

  /** Vor großen Uploads: angemeldet sein, ohne dass die Firmware 6 MB ablehnen muss */
  async ensure(reason?: string): Promise<boolean> {
    const s = await this.state().catch(() => null);
    if (!s || !s.pin || s.ok) return true;
    return this.login(reason);
  }

  async logout() {
    await fetch('/api/logout', { method: 'POST', headers: this.headers() }).catch(() => null);
    this.save('');
  }
}

export const auth = new Auth();
