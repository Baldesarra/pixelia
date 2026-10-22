<script lang="ts">
  // Seite „Zugangsschutz“ (Einstellungen → Gerät): optionaler PIN festlegen, ändern, ausschalten, abmelden
  import { Button, Card, Chip, confirmDialog, Icon, LoadingIndicator, promptDialog } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { auth } from '../lib/auth.svelte';
  import { post, type Reply } from '../lib/api';

  const s = $derived(device.data);

  const pinOn = $derived(!!s?.auth?.pin);
  let loggedIn = $state<boolean | null>(null);
  let pinMsg = $state<{ ok: boolean; text: string } | null>(null);
  async function refreshAuth() {
    loggedIn = (await auth.state().catch(() => null))?.ok ?? null;
  }
  // Nach PIN-Wechsel, Neustart oder Anmeldung in einem Dialog neu fragen
  $effect(() => {
    void pinOn;
    void auth.token;
    void refreshAuth();
  });

  async function setPin() {
    pinMsg = null;
    let first = '';
    const pin = await promptDialog({
      title: pinOn ? 'Neuer PIN' : 'PIN festlegen',
      text: 'Schützt Einstellungen, Updates, Neustart, Werksreset, Befehle und Gerätenamen. Ansehen bleibt ohne PIN möglich.',
      icon: 'password',
      ok: 'Weiter',
      field: {
        label: 'PIN oder Passwort',
        type: 'password',
        autocomplete: 'new-password',
        supporting: '4 bis 32 Zeichen, Ziffern oder Wörter',
        check: async (v) => (v.length < 4 || v.length > 32 ? '4 bis 32 Zeichen' : ((first = v), null)),
      },
    });
    if (pin === null) return;
    const again = await promptDialog({
      title: 'PIN wiederholen',
      icon: 'password',
      ok: 'Speichern',
      field: {
        label: 'PIN noch einmal',
        type: 'password',
        autocomplete: 'new-password',
        check: async (v) => (v === first ? null : 'Stimmt nicht mit dem ersten überein'),
      },
    });
    if (again === null) return;
    const r = await post<Reply & { token?: string }>('/api/pin', { new: pin }).catch(() => null);
    if (r?.ok && r.token) auth.save(r.token);
    pinMsg = { ok: !!r?.ok, text: r?.msg ?? 'Keine Antwort vom Gerät' };
  }

  async function pinOff() {
    const ok = await confirmDialog({
      title: 'PIN ausschalten?',
      text: 'Danach kann jeder im Heimnetz Einstellungen ändern, Updates installieren und Pixelia zurücksetzen.',
      ok: 'Ausschalten',
      icon: 'lock_open',
      danger: true,
    });
    if (!ok) return;
    const r = await post('/api/pin', { new: '' }).catch(() => null);
    if (r?.ok) auth.save('');
    pinMsg = { ok: !!r?.ok, text: r?.msg ?? 'Keine Antwort vom Gerät' };
  }

  async function logout() {
    await auth.logout();
    pinMsg = { ok: true, text: 'Abgemeldet. Dieser Browser fragt beim nächsten Ändern wieder nach dem PIN.' };
  }
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    <Card class="wide">
      <h2 class="card-title">
        <Icon name="shield_lock" size={20} />Schutz
        <span class="end">
          <Chip tone={pinOn ? 'primary' : 'neutral'} icon={pinOn ? 'lock' : 'lock_open'}>
            {pinOn ? (loggedIn ? 'PIN an · angemeldet' : 'PIN an') : 'kein PIN'}
          </Chip>
        </span>
      </h2>
      <p class="muted">
        {#if pinOn}
          Einstellungen, Updates, Neustart, Werksreset, Befehle und Gerätenamen brauchen den PIN. Nach 5 Fehlversuchen
          sperrt Pixelia 30 Sekunden, danach jedes Mal doppelt so lange.
        {:else}
          Optional: Ein PIN oder Passwort schützt Einstellungen, Updates, Neustart, Werksreset, Befehle und Gerätenamen.
          Ansehen bleibt für alle im Heimnetz möglich.
        {/if}
      </p>
      {#if s.auth?.lockS}<p class="err-text">Gesperrt nach Fehlversuchen, noch {s.auth.lockS} s.</p>{/if}
      <div class="actions">
        <Button variant={pinOn ? 'tonal' : 'filled'} icon="password" onclick={setPin}>{pinOn ? 'PIN ändern' : 'PIN festlegen'}</Button>
        {#if pinOn}
          <Button variant="text" class="danger" icon="lock_open" onclick={pinOff}>Ausschalten</Button>
          {#if loggedIn && auth.token}<Button variant="text" icon="logout" onclick={logout}>Abmelden</Button>{/if}
        {/if}
      </div>
      {#if pinMsg}<p class={pinMsg.ok ? 'ok-text' : 'err-text'}>{pinMsg.text}</p>{/if}
      <p class="muted small">
        PIN vergessen? Am Gerät lange aufs Display drücken, auf der Info-Karte „Zurücksetzen …“ tippen und „PIN
        entfernen“ 3 Sekunden halten. Über das Netz geht das bewusst nicht.
      </p>
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
