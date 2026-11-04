<script lang="ts">
  // Seite „Sicherung“ (System → Wartung): Anzeige, Farbwahl und Gerätenamen als Datei herunterladen und einspielen;
  // seit 0.5.9 auch der Speicherstand der Welt (.pxw, Einspielen mit Neustart)
  import { Button, Card, confirmDialog, Icon, LoadingIndicator } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { post, type Backup } from '../lib/api';
  import { num } from '../lib/format';

  const s = $derived(device.data);

  let importInput = $state<HTMLInputElement>();
  let backupMsg = $state<{ ok: boolean; text: string } | null>(null);
  let importing = $state(false);

  function describe(b: Backup): string {
    const parts = [
      `Bilddrehung ${b.set.rot}°`,
      `Helligkeit ${b.set.bright} %`,
      `langer Druck ${num(b.set.lp / 1000, 1)} s`,
    ];
    if (b.set.ui) parts.push('Farbwahl der Oberfläche');
    if (b.names.length) parts.push(`${b.names.length} Gerätename${b.names.length === 1 ? '' : 'n'}`);
    return parts.join(', ');
  }

  // B19 (seit 0.5.11): Datei als unknown lesen und Feld für Feld prüfen; ein Fehler ergibt einen Satz statt einer
  // halben Übernahme. Format 1 seit 0.3.2; fehlt „format“, ist es eine Sicherung vor 0.4 (gleicher Aufbau)
  const isObj = (x: unknown): x is Record<string, unknown> => typeof x === 'object' && x !== null && !Array.isArray(x);
  const isInt = (x: unknown, lo: number, hi: number): x is number => Number.isInteger(x) && (x as number) >= lo && (x as number) <= hi;
  function parseBackup(x: unknown): Backup | string {
    if (!isObj(x) || x.pixelia !== 'settings') return 'Das ist keine Pixelia-Sicherung (.json aus „Sicherung herunterladen“).';
    const format = x.format ?? 1;
    if (format !== 1) return `Diese Sicherung hat ein neueres Format (${String(format)}). Bitte zuerst Pixelia aktualisieren.`;
    const set = x.set;
    if (!isObj(set)) return 'Die Sicherung enthält keine Einstellungen.';
    if (set.rot !== 0 && set.rot !== 180) return 'Bilddrehung in der Sicherung ist ungültig (erlaubt: 0 oder 180).';
    if (!isInt(set.bright, 1, 100)) return 'Helligkeit in der Sicherung ist ungültig (erlaubt: 1 bis 100 %).';
    if (!isInt(set.lp, 1000, 10000)) return 'Dauer des langen Drucks in der Sicherung ist ungültig (erlaubt: 1 bis 10 s).';
    if (set.ui !== undefined && typeof set.ui !== 'string') return 'Farbwahl in der Sicherung ist ungültig.';
    const str = (v: unknown) => (typeof v === 'string' ? v : '');
    const names = (Array.isArray(x.names) ? x.names : [])
      .filter((n): n is { id: string; name: string } => isObj(n) && typeof n.id === 'string' && typeof n.name === 'string')
      .map((n) => ({ id: n.id, name: n.name }));
    return {
      pixelia: 'settings',
      format: 1,
      v: str(x.v),
      date: str(x.date),
      host: str(x.host),
      set: { rot: set.rot, bright: set.bright, lp: set.lp, ui: str(set.ui) },
      names,
      wifi: (Array.isArray(x.wifi) ? x.wifi : []).filter((w): w is string => typeof w === 'string'),
      pin: x.pin === true,
    };
  }

  async function importFile() {
    const f = importInput?.files?.[0];
    if (importInput) importInput.value = '';
    if (!f) return;
    backupMsg = null;
    try {
      await importBackup(f);
    } catch (e) {
      importing = false;
      backupMsg = { ok: false, text: `Einspielen nicht möglich: ${(e as Error).message || 'unbekannter Fehler'}` };
    }
  }

  async function importBackup(f: File) {
    let raw: unknown;
    try {
      raw = JSON.parse(await f.text());
    } catch {
      raw = null;
    }
    const b = parseBackup(raw);
    if (typeof b === 'string') {
      backupMsg = { ok: false, text: b };
      return;
    }
    const extra = [
      b.wifi.length ? `WLAN-Netze (${b.wifi.join(', ')}) stehen nur zur Erinnerung drin, ohne Passwörter: bei Bedarf auf der Seite WLAN neu eintragen.` : '',
      b.pin ? 'Ein PIN war gesetzt; er steckt nie in der Sicherung und wird hier nicht geändert.' : '',
    ].filter(Boolean);
    const ok = await confirmDialog({
      title: 'Sicherung übernehmen?',
      text: `Von ${b.host || 'Pixelia'} ${b.v ? `(${b.v}${b.date ? `, ${b.date}` : ''})` : ''}: ${describe(b)}. ${extra.join(' ')}`,
      ok: 'Übernehmen',
      icon: 'settings_backup_restore',
    });
    if (!ok) return;
    importing = true;
    const data: Record<string, string | number> = { rot: b.set.rot === 180 ? 180 : 0, bright: b.set.bright, lp: b.set.lp };
    if (typeof b.set.ui === 'string' && b.set.ui) data.ui = b.set.ui;
    const r = await post('/api/settings', data).catch(() => null);
    let names = 0;
    if (r?.ok) {
      for (const n of b.names) {
        const nr = await post('/api/clients/name', { id: n.id, name: n.name, create: 1 }).catch(() => null);
        if (nr?.ok) names++;
      }
    }
    importing = false;
    // Antwort „noch nicht dauerhaft gespeichert“ (B23) mit anzeigen; Namen zählen, falls einer scheitert
    backupMsg = r?.ok
      ? {
          ok: true,
          text: `Übernommen: ${describe(b)}${b.names.length ? ` (${names} von ${b.names.length} Namen)` : ''}.${r.msg && r.msg !== 'Gespeichert' ? ` ${r.msg}.` : ''}`,
        }
      : { ok: false, text: `Nichts übernommen: ${r?.msg ?? 'keine Antwort vom Gerät'}` };
  }

  // Speicherstand der Welt (seit 0.5.9): Kopf der Datei lesen (PXW1, Generation, Weltzeit), dann einspielen
  let worldInput = $state<HTMLInputElement>();
  let worldMsg = $state<{ ok: boolean; text: string } | null>(null);
  let worldBusy = $state(false);
  async function importWorld() {
    const f = worldInput?.files?.[0];
    if (worldInput) worldInput.value = '';
    if (!f) return;
    worldMsg = null;
    const head = new DataView(await f.slice(0, 32).arrayBuffer());
    if (head.byteLength < 32 || head.getUint32(0, true) !== 0x31575850 || f.size > 96 * 1024) {
      worldMsg = { ok: false, text: 'Das ist kein Pixelia-Speicherstand (.pxw aus „Speicherstand herunterladen“).' };
      return;
    }
    // Weltzeit = Ortszeit in µs seit 1970
    const when = new Date(Number(head.getBigInt64(16, true) / 1000n));
    const at = `${when.getUTCDate()}.${when.getUTCMonth() + 1}.${when.getUTCFullYear()}, ${String(when.getUTCHours()).padStart(2, '0')}:${String(when.getUTCMinutes()).padStart(2, '0')}`;
    const ok = await confirmDialog({
      title: 'Speicherstand einspielen?',
      text: `Stand vom ${at} (Nr. ${head.getUint32(12, true)}, ${num(f.size / 1024, 1)} KB). Die Welt auf dem Gerät wird ersetzt, Pixelia startet neu und holt die Zeit seitdem nach (höchstens drei Tage).`, // Stage::kCatchUpMinutes
      ok: 'Einspielen',
      icon: 'upload',
    });
    if (!ok) return;
    worldBusy = true;
    const r = await post('/api/world?a=load', f).catch(() => null);
    worldBusy = false;
    worldMsg = r?.ok ? { ok: true, text: 'Eingespielt. Pixelia startet neu, die Seite verbindet sich gleich wieder.' } : { ok: false, text: r?.msg ?? 'Keine Antwort vom Gerät' };
  }
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    <Card class="wide">
      <h2 class="card-title"><Icon name="settings_backup_restore" size={20} />Sicherung</h2>
      <p class="muted">
        Anzeige, Farben und Gerätenamen als Datei sichern, z. B. vor einem Werksreset. Ohne WLAN-Passwörter und PIN.
      </p>
      <input class="hidden" type="file" accept=".json,application/json" bind:this={importInput} onchange={importFile} />
      <div class="actions">
        <Button icon="download" href="/api/backup" download>Sicherung herunterladen</Button>
        <Button variant="tonal" icon="upload" disabled={importing} onclick={() => importInput?.click()}>Einspielen</Button>
      </div>
      {#if backupMsg}<p class={backupMsg.ok ? 'ok-text' : 'err-text'}>{backupMsg.text}</p>{/if}
    </Card>
    <Card class="wide">
      <h2 class="card-title"><Icon name="save" size={20} />Speicherstand der Welt</h2>
      <p class="muted">
        Alles, was in Pixelia passiert ist: Figuren, Chronik, Orte, Kalender. Einspielen ersetzt die Welt und startet neu.
      </p>
      <input class="hidden" type="file" accept=".pxw,application/octet-stream" bind:this={worldInput} onchange={importWorld} />
      <div class="actions">
        <Button icon="download" href="/api/world?datei=1" download>Speicherstand herunterladen</Button>
        <Button variant="tonal" icon="upload" disabled={worldBusy} onclick={() => worldInput?.click()}>Einspielen</Button>
      </div>
      {#if worldMsg}<p class={worldMsg.ok ? 'ok-text' : 'err-text'}>{worldMsg.text}</p>{/if}
    </Card>
  </div>
{/if}

<style>
  .waiting {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  p {
    margin: 12px 0 0;
  }
  .card-title + p {
    margin-top: 0;
  }
  .hidden {
    display: none;
  }
</style>
