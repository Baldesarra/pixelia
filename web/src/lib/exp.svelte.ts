// Zustand von /api/exp für die Seiten Experimentell und Gerätetests (seit 0.5.4 getrennt): Laden, Abfrage-Schleife,
// Aktionen mit Meldung. Ein Objekt für beide Seiten, damit der Stand beim Wechsel erhalten bleibt.
import { get, post, type ExpState } from './api';

class ExpStore {
  ex = $state<ExpState | null>(null);
  failed = $state(false);
  msg = $state<{ ok: boolean; text: string } | null>(null);
  busy = $state(false);

  load = async () => {
    const r = await get<ExpState>('/api/exp').catch(() => null);
    if (r) [this.ex, this.failed] = [r, false];
    else this.failed = true;
  };

  /** Abfrage: 4×/s beim Touch-Test, 1×/s während Probe oder Testbild, sonst alle 5 s; gibt die Stopp-Funktion zurück */
  poll = () => {
    void this.load();
    let stop = false;
    const loop = async () => {
      while (!stop) {
        const ex = this.ex;
        const wait = ex?.mode === 'touch' ? 250 : ex && (ex.trialS >= 0 || ex.testS >= 0) ? 1000 : 5000;
        await new Promise((r) => setTimeout(r, wait));
        if (!document.hidden && !stop) await this.load();
      }
    };
    void loop();
    return () => (stop = true);
  };

  act = async (data: Record<string, string | number>, okText?: string) => {
    this.busy = true;
    this.msg = null;
    const r = await post('/api/exp', data).catch(() => null);
    this.busy = false;
    if (!r?.ok) this.msg = { ok: false, text: r?.msg ?? 'Pixelia antwortet nicht' };
    else if (okText) this.msg = { ok: true, text: okText };
    await this.load();
    return !!r?.ok;
  };
}

export const exp = new ExpStore();
