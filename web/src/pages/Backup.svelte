<script lang="ts">
  // Seite „Sicherung“ (System → Wartung): Anzeige, Farbwahl und Gerätenamen als Datei herunterladen und einspielen
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

  async function importFile() {
    const f = importInput?.files?.[0];
    if (importInput) importInput.value = '';
    if (!f) return;
    backupMsg = null;
    let b: Backup;
    try {
      b = JSON.parse(await f.text()) as Backup;
      if (b.pixelia !== 'settings' || typeof b.set !== 'object') throw new Error();
      b.names = Array.isArray(b.names) ? b.names.filter((n) => typeof n?.id === 'string' && typeof n?.name === 'string') : [];
      b.wifi = Array.isArray(b.wifi) ? b.wifi : [];
    } catch {
      backupMsg = { ok: false, text: 'Das ist keine Pixelia-Sicherung (.json aus „Sicherung herunterladen“).' };
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
    backupMsg = r?.ok
      ? { ok: true, text: `Übernommen: ${describe(b)}${b.names.length ? ` (${names} von ${b.names.length} Namen)` : ''}.` }
      : { ok: false, text: r?.msg ?? 'Keine Antwort vom Gerät' };
  }
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    <Card class="wide">
      <h2 class="card-title"><Icon name="settings_backup_restore" size={20} />Sicherung</h2>
      <p class="muted">
        Anzeige, Farbwahl und Gerätenamen als Datei sichern und wieder einspielen, z. B. nach einem Werksreset. Die Datei
        enthält nie WLAN-Passwörter oder den PIN.
      </p>
      <input class="hidden" type="file" accept=".json,application/json" bind:this={importInput} onchange={importFile} />
      <div class="actions">
        <Button icon="download" href="/api/backup" download>Sicherung herunterladen</Button>
        <Button variant="tonal" icon="upload" disabled={importing} onclick={() => importInput?.click()}>Einspielen</Button>
      </div>
      {#if backupMsg}<p class={backupMsg.ok ? 'ok-text' : 'err-text'}>{backupMsg.text}</p>{/if}
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
