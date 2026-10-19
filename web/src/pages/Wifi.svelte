<script lang="ts">
  // Seite „WLAN“: Verbindung, gespeicherte Netze, Netzwerksuche mit Signalbalken, Netz hinzufügen.
  // Passwörter verlassen das Gerät nie: /api/wifi liefert nur die Namen.
  import { onMount } from 'svelte';
  import { Button, Card, Chip, confirmDialog, Icon, IconButton, LoadingIndicator, TextField } from '../lib/m3';
  import SignalBars from '../lib/SignalBars.svelte';
  import { device } from '../lib/status.svelte';
  import { get, post, type SystemInfo, type WifiList } from '../lib/api';
  import { duration, signal, WIFI_STATE } from '../lib/format';

  const s = $derived(device.data);
  const w = $derived(s?.wifi);
  const online = $derived(w?.state === 'verbunden');

  let list = $state<WifiList>({ saved: [], scan: [] });
  let net = $state<SystemInfo['net'] | null>(null);
  let scanning = $state(false);
  let scanMsg = $state('');

  let ssid = $state('');
  let pass = $state('');
  let showPass = $state(false);
  let saving = $state(false);
  let msg = $state<{ ok: boolean; text: string } | null>(null);
  let passInput = $state<HTMLInputElement>();
  let form = $state<HTMLElement>();

  const passError = $derived(pass.length > 0 && pass.length < 8 ? 'Mindestens 8 Zeichen (oder leer für offene Netze)' : '');

  async function load(fresh = false) {
    if (fresh) {
      scanning = true;
      scanMsg = '';
    }
    try {
      list = fresh ? await post<WifiList>('/api/wifi/scan') : await get<WifiList>('/api/wifi');
      if (fresh && !list.scan.length) scanMsg = 'Keine Netze gefunden';
    } catch {
      scanMsg = 'Suche fehlgeschlagen';
    }
    scanning = false;
  }

  async function loadNet() {
    try {
      net = (await get<SystemInfo>('/api/system')).net;
    } catch {
      /* Netzdetails sind Zusatz */
    }
  }

  onMount(() => {
    void load();
    void loadNet();
  });
  // Nach einem Netzwechsel die Details neu holen
  let lastSsid = '';
  $effect(() => {
    const now = online ? (w?.ssid ?? '') : '';
    if (now !== lastSsid) {
      lastSsid = now;
      void loadNet();
    }
  });

  function pick(name: string) {
    ssid = name;
    pass = '';
    msg = null;
    form?.scrollIntoView({ behavior: 'smooth', block: 'center' });
    setTimeout(() => passInput?.focus(), 300);
  }

  async function remove(name: string) {
    const ok = await confirmDialog({
      title: 'Netz entfernen?',
      text: `„${name}“ wird mit Passwort gelöscht.${name === w?.ssid ? ' Pixelia ist gerade damit verbunden und sucht danach ein anderes gespeichertes Netz.' : ''}`,
      ok: 'Entfernen',
      icon: 'delete',
      danger: true,
    });
    if (!ok) return;
    await post('/api/wifi/remove', { ssid: name }).catch(() => null);
    void load();
  }

  async function add(e: SubmitEvent) {
    e.preventDefault();
    if (!ssid || passError) return;
    saving = true;
    msg = null;
    try {
      const r = await post('/api/wifi/add', { ssid, pass });
      msg = { ok: r.ok, text: r.msg ?? '' };
      if (r.ok) {
        pass = '';
        void load();
      }
    } catch {
      msg = {
        ok: true,
        text: 'Verbindung zum Gerät unterbrochen. Läuft die Einrichtung über das Setup-WLAN, verbindet sich Pixelia gerade mit deinem Netz. Danach unter http://pixelia.local weiter.',
      };
    }
    saving = false;
  }
</script>

{#if !s || !w}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    {#if w.ap && !online}
      <Card variant="outlined" class="wide">
        <h2 class="card-title"><Icon name="router" size={20} />Willkommen bei Pixelia</h2>
        <ol class="steps">
          <li>Unter „Netze in der Nähe“ auf <b>Suchen</b> tippen und dein WLAN auswählen.</li>
          <li>Passwort eingeben und <b>Speichern und verbinden</b>.</li>
          <li>Pixelia verbindet sich, das Setup-WLAN schließt sich. Danach erreichst du Pixelia im Heimnetz unter
            <b>http://pixelia.local</b> (oder über die Adresse auf dem Display).</li>
        </ol>
        <p class="muted small">Klappt etwas nicht? Die <a href="/setup">einfache Einrichtungsseite</a> kann dasselbe ohne Extras.</p>
      </Card>
    {/if}
    <Card variant={online ? 'primary' : w.ap ? 'tertiary' : 'error'} shape="extra-large" class="wide">
      <div class="now">
        {#if online}<SignalBars rssi={w.rssi} size={48} />{:else}<Icon name={w.ap ? 'router' : 'wifi_off'} size={48} />{/if}
        <div class="now-text">
          <div class="ssid">{online ? w.ssid : w.ap ? 'Setup-Modus' : (WIFI_STATE[w.state] ?? w.state)}</div>
          <div class="tnum">
            {#if online}
              Signal {signal(w.rssi).text} · {w.rssi} dBm · Kanal {w.ch}
            {:else if w.state === 'verbinde'}
              Verbinde mit {w.ssid} …
            {:else if w.ap}
              Eigenes WLAN „Pixelia-Setup“ aktiv, {w.apClients} Gerät(e) angemeldet{w.retry >= 0 ? ` · neuer Versuch in ${w.retry} s` : ''}
            {:else}
              Nicht verbunden{w.retry >= 0 ? ` · neuer Versuch in ${w.retry} s` : ''}
            {/if}
          </div>
          {#if w.err && !online}<div class="small">Letzter Fehler: {w.err}</div>{/if}
        </div>
      </div>
      {#if online}
        <dl class="kv facts">
          <dt>Adresse</dt>
          <dd class="tnum">{w.ip}</dd>
          {#if net?.gw}<dt>Router</dt><dd class="tnum">{net.gw}</dd>{/if}
          {#if net?.dns}<dt>DNS</dt><dd class="tnum">{net.dns}</dd>{/if}
          {#if net?.mask}<dt>Netzmaske</dt><dd class="tnum">{net.mask}</dd>{/if}
          {#if net?.bssid}<dt>Zugangspunkt</dt><dd class="tnum">{net.bssid}{net.phy ? ` · ${net.phy}` : ''}</dd>{/if}
          {#if net?.connectedS !== undefined}<dt>Verbunden seit</dt><dd class="tnum">{duration(net.connectedS)}</dd>{/if}
          <dt>Name im Netz</dt>
          <dd><a href="http://{s.host}.local/">{s.host}.local</a></dd>
        </dl>
      {/if}
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="lock" size={20} />Gespeicherte Netze <span class="end tnum">{list.saved.length} von 5</span></h2>
      {#if list.saved.length}
        <ul class="list">
          {#each list.saved as name, i (name)}
            <li class="list-item">
              <span class="lead"><Icon name="wifi" /></span>
              <span class="text">
                <span class="headline">{name}</span>
                <span class="supporting">
                  {name === w.ssid && online ? 'verbunden' : i === 0 ? 'zuletzt benutzt' : 'gespeichert'}
                </span>
              </span>
              {#if name === w.ssid && online}<Chip tone="primary" icon="check">Aktiv</Chip>{/if}
              <IconButton icon="delete" label="{name} entfernen" onclick={() => remove(name)} />
            </li>
          {/each}
        </ul>
      {:else}
        <p class="muted">Noch kein Netz gespeichert.</p>
      {/if}
      <p class="muted small">Pixelia verbindet sich immer mit dem stärksten gespeicherten Netz in Reichweite.</p>
    </Card>

    <Card>
      <h2 class="card-title">
        <Icon name="wifi_find" size={20} />Netze in der Nähe
        <span class="end">
          <Button variant="tonal" size="xs" icon="wifi_find" disabled={scanning} onclick={() => load(true)}>
            {scanning ? 'Suche …' : 'Suchen'}
          </Button>
        </span>
      </h2>
      {#if scanning}
        <div class="scanning"><LoadingIndicator size={48} label="Suche Netze" /></div>
      {:else if list.scan.length}
        <ul class="list">
          {#each list.scan as n (n.ssid + n.ch)}
            <li>
              <button class="list-item" type="button" onclick={() => pick(n.ssid)}>
                <SignalBars rssi={n.rssi} size={22} />
                <span class="text">
                  <span class="headline">{n.ssid}</span>
                  <span class="supporting tnum">
                    {n.rssi} dBm · Kanal {n.ch}{n.saved ? ' · gespeichert' : ''}
                  </span>
                </span>
                <Icon name={n.secure ? 'lock' : 'lock_open'} size={20} />
              </button>
            </li>
          {/each}
        </ul>
      {:else}
        <p class="muted">{scanMsg || 'Noch nicht gesucht. Die Suche dauert ein paar Sekunden.'}</p>
      {/if}
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="add" size={20} />Netz hinzufügen</h2>
      <form class="form" bind:this={form} onsubmit={add} autocomplete="off">
        <TextField label="WLAN-Name" bind:value={ssid} maxlength={32} required autocapitalize="none" spellcheck="false" />
        <TextField
          label="Passwort"
          type={showPass ? 'text' : 'password'}
          bind:value={pass}
          bind:input={passInput}
          maxlength={64}
          autocapitalize="none"
          spellcheck="false"
          error={passError}
          supporting="Bleibt auf dem Gerät und wird nirgends angezeigt"
        >
          {#snippet trailing()}
            <IconButton
              type="button"
              icon={showPass ? 'visibility_off' : 'visibility'}
              label={showPass ? 'Passwort verbergen' : 'Passwort anzeigen'}
              onclick={() => (showPass = !showPass)}
            />
          {/snippet}
        </TextField>
        <div class="actions">
          <Button type="submit" icon="check" disabled={saving || !ssid || !!passError}>
            {saving ? 'Speichere …' : 'Speichern und verbinden'}
          </Button>
        </div>
        {#if msg}<p class={msg.ok ? 'ok-text' : 'err-text'}>{msg.text}</p>{/if}
      </form>
    </Card>
  </div>
{/if}

<style>
  .waiting,
  .scanning {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  .scanning {
    padding: 24px 0;
  }
  .now {
    display: flex;
    align-items: center;
    gap: 20px;
  }
  .now-text {
    min-width: 0;
  }
  .ssid {
    font: var(--md-sys-typescale-headline-medium);
    font-weight: 750;
    font-stretch: 112%;
    overflow-wrap: anywhere;
  }
  .facts {
    margin-top: 20px;
    max-width: 560px;
  }
  .facts dt {
    color: inherit;
    opacity: 0.8;
  }
  .facts a {
    color: inherit;
    text-underline-offset: 3px;
  }
  .steps {
    display: grid;
    gap: 8px;
    margin: 0;
    padding-left: 22px;
    font: var(--md-sys-typescale-body-large);
  }
  .steps + p {
    margin: 12px 0 0;
  }
  .form {
    display: grid;
    gap: 12px;
  }
  .form p {
    margin: 0;
  }
  li.list-item :global(.chip) {
    flex: none;
  }
</style>
