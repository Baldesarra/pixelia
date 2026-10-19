<script lang="ts">
  // Seite „Verbundene Geräte“ (Projektplan 4.12): Browser mit Geräte-ID, IP, Browser/System, erste Verbindung,
  // letzte Aktivität, aktuelle Seite, online; eigener Name pro Gerät. Im Setup-Modus die Geräte am Access Point.
  import { onMount, tick } from 'svelte';
  import { Button, Card, Chip, Icon, IconButton, LoadingIndicator, TextField } from '../lib/m3';
  import { get, post, type ClientList } from '../lib/api';
  import { clientId, isPhone } from '../lib/client';
  import { device } from '../lib/status.svelte';

  const PAGES: Record<string, string> = {
    uebersicht: 'Übersicht', wlan: 'WLAN', system: 'System', log: 'Live-Log', info: 'Info', hardware: 'Hardware',
    geraete: 'Geräte', darstellung: 'Darstellung', mehr: 'Mehr',
  };

  let data = $state<ClientList | null>(null);
  let failed = $state(false);
  let editing = $state('');
  let draft = $state('');
  let saving = $state(false);

  async function load() {
    try {
      data = await get<ClientList>('/api/clients');
      failed = false;
    } catch {
      failed = true;
    }
  }
  onMount(() => {
    void load();
    const t = setInterval(load, 5000);
    return () => clearInterval(t);
  });

  // Online zuerst, dann nach letzter Aktivität
  const list = $derived(
    [...(data?.list ?? [])].sort((a, b) => Number(b.online) - Number(a.online) || b.last - a.last),
  );

  const date = (epoch: number) =>
    new Date(epoch * 1000).toLocaleString('de-DE', { day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit' });

  function lastSeen(c: ClientList['list'][number]): string {
    if (c.online) return 'jetzt';
    const ago = c.agoS ?? (c.last && data?.epoch ? data.epoch - c.last : null);
    if (ago === null) return c.last ? date(c.last) : 'unbekannt';
    if (ago < 60) return 'gerade eben';
    if (ago < 3600) return `vor ${Math.round(ago / 60)} min`;
    if (ago < 86400) return `vor ${Math.round(ago / 3600)} h`;
    return c.last ? date(c.last) : `vor ${Math.round(ago / 86400)} Tagen`;
  }

  let nameInput = $state<HTMLInputElement>();
  async function edit(id: string, name: string) {
    editing = id;
    draft = name;
    await tick();
    nameInput?.focus();
  }

  async function save(e: SubmitEvent) {
    e.preventDefault();
    saving = true;
    await post('/api/clients/name', { id: editing, name: draft.trim() }).catch(() => null);
    saving = false;
    editing = '';
    void load();
  }
</script>

{#if !data}
  <div class="waiting">
    {#if failed}<p class="muted">Geräteliste nicht abrufbar.</p>{:else}<LoadingIndicator size={64} label="Lade" />{/if}
  </div>
{:else}
  <div class="page-grid">
    <Card class="wide">
      <h2 class="card-title">
        <Icon name="devices" size={20} />Verbundene Browser
        <span class="end tnum muted">{list.filter((c) => c.online).length} online · {list.length} bekannt</span>
      </h2>
      {#if list.length}
        <ul class="list">
          {#each list as c (c.id)}
            {@const self = c.id === clientId}
            <li class="list-item device" class:offline={!c.online}>
              <span class="lead" class:on={c.online}><Icon name={isPhone(c.agent) ? 'mobile' : 'computer'} /></span>
              <span class="text">
                {#if editing === c.id}
                  <form class="rename" onsubmit={save}>
                    <TextField label="Eigener Name" bind:value={draft} bind:input={nameInput} maxlength={24} placeholder="z. B. Handy Wohnzimmer" />
                    <Button type="submit" size="s" disabled={saving}>Speichern</Button>
                    <Button type="button" variant="text" size="s" onclick={() => (editing = '')}>Abbrechen</Button>
                  </form>
                {:else}
                  <span class="headline">{c.name || c.agent || 'Unbekannter Browser'}</span>
                  {#if self || c.online}
                    <span class="tags">
                      {#if c.online}<Chip tone="primary" pulse>online</Chip>{/if}
                      {#if self}<Chip tone="tertiary">Dieses Gerät</Chip>{/if}
                    </span>
                  {/if}
                  <span class="supporting tnum">
                    {[c.name ? c.agent : '', c.ip].filter(Boolean).join(' · ')}
                  </span>
                  <span class="supporting tnum">
                    {#if c.online}
                      Seite: {PAGES[c.page] ?? (c.page || '–')}
                    {:else}
                      zuletzt aktiv: {lastSeen(c)}
                    {/if}
                    · erste Verbindung: {c.first ? date(c.first) : 'unbekannt'} · {c.visits}× verbunden
                  </span>
                {/if}
              </span>
              {#if editing !== c.id}
                <IconButton icon="edit" label="Namen ändern" onclick={() => edit(c.id, c.name)} />
              {/if}
            </li>
          {/each}
        </ul>
      {:else}
        <p class="muted">Noch keine Browser angemeldet.</p>
      {/if}
      <p class="muted small">
        Jeder Browser bekommt eine zufällige Kennung, die nur in diesem Browser gespeichert ist. Einen echten Gerätenamen
        verrät kein Browser, deshalb lässt sich hier ein eigener Name vergeben. Pixelia merkt sich bis zu 12 Geräte.
      </p>
    </Card>

    {#if device.data?.wifi.ap || data.ap.length}
      <Card class="wide">
        <h2 class="card-title"><Icon name="router" size={20} />Am Setup-WLAN angemeldet</h2>
        {#if data.ap.length}
          <ul class="list">
            {#each data.ap as a (a.mac)}
              <li class="list-item">
                <span class="lead"><Icon name="wifi" /></span>
                <span class="text">
                  <span class="headline tnum">{a.ip || 'noch keine Adresse'}</span>
                  <span class="supporting tnum">MAC {a.mac} · {a.rssi} dBm</span>
                </span>
              </li>
            {/each}
          </ul>
        {:else}
          <p class="muted">Kein Gerät angemeldet.</p>
        {/if}
      </Card>
    {/if}
  </div>
{/if}

<style>
  .waiting {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  .device {
    align-items: flex-start;
  }
  .device .text {
    display: grid;
    gap: 2px;
  }
  .device .headline {
    white-space: normal;
    overflow-wrap: anywhere;
    font-weight: 600;
  }
  .tags {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin: 4px 0 2px;
  }
  .lead {
    transition:
      background-color var(--m3-effects),
      border-radius var(--m3-spatial);
  }
  .lead.on {
    background: var(--md-sys-color-primary);
    color: var(--md-sys-color-on-primary);
    border-radius: var(--md-sys-shape-corner-medium);
  }
  .offline .lead {
    background: var(--md-sys-color-surface-container-highest);
    color: var(--md-sys-color-on-surface-variant);
  }
  .rename {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;
  }
  .rename :global(.field) {
    flex: 1 1 220px;
  }
</style>
