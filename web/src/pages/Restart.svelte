<script lang="ts">
  // Seite „Neustart und Werksreset“ (System → Wartung)
  import { Button, Card, confirmDialog, Icon, LoadingIndicator } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { auth } from '../lib/auth.svelte';
  import { post } from '../lib/api';
  import { duration } from '../lib/format';

  const s = $derived(device.data);

  async function reboot() {
    const ok = await confirmDialog({
      title: 'Pixelia neu starten?',
      text: 'Die Anzeige ist ein paar Sekunden dunkel, die Oberfläche verbindet sich danach von selbst neu.',
      ok: 'Neu starten',
      icon: 'restart_alt',
      danger: true,
    });
    if (ok) await post('/api/reboot').catch(() => null);
  }

  let factoryDone = $state(false);
  async function factoryReset() {
    const ok = await confirmDialog({
      title: 'Auf Werkseinstellungen zurücksetzen?',
      text: 'Löscht WLAN-Netze, Einstellungen, Farbwahl, Gerätenamen und den PIN. Pixelia startet danach im Setup-Modus und muss neu eingerichtet werden. Vorher am besten die Sicherung herunterladen.',
      ok: 'Zurücksetzen',
      icon: 'delete_forever',
      danger: true,
    });
    if (!ok) return;
    const r = await post('/api/factory-reset', { confirm: 1 }).catch(() => null);
    if (r?.ok) {
      factoryDone = true;
      auth.save('');
    }
  }
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    <Card class="wide">
      <h2 class="card-title"><Icon name="restart_alt" size={20} />Neustart und Werksreset</h2>
      {#if factoryDone}
        <p class="err-text">
          Zurückgesetzt. Pixelia startet im Setup-Modus: Mit dem WLAN „Pixelia-Setup“ verbinden (Passwort steht auf dem
          Display), dann öffnet sich die Einrichtung.
        </p>
      {:else}
        <p class="muted">Einstellungen und WLAN bleiben erhalten.</p>
        <div class="actions">
          <Button class="danger" icon="restart_alt" onclick={reboot}>Neu starten</Button>
        </div>
        <p class="muted">
          Löscht WLAN-Netze, Einstellungen, Farben, Gerätenamen und PIN. Die Neustart-Historie bleibt für die Diagnose.
        </p>
        <div class="actions">
          <Button variant="outlined" class="danger" icon="delete_forever" onclick={factoryReset}>Werksreset …</Button>
        </div>
      {/if}
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
</style>
