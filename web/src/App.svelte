<script lang="ts">
  // Rahmen der Oberfläche: Navigation (Leiste unten, in der breiten Ansicht Schiene links), obere Leiste mit
  // Zurück-Pfeil auf Unterseiten, Seiten per Hash-Route (#/…). Seit 0.5.4: vier Ziele Welt, Übersicht, System,
  // Einstellungen; System und Einstellungen sind Listen mit Kategorien, ihre Seiten tragen „parent“.
  import type { Component } from 'svelte';
  import { Chip, Dialog, IconButton, Navigation, theme, type NavItem } from './lib/m3';
  import Logo from './lib/Logo.svelte';
  import Story from './lib/Story.svelte';
  import { device } from './lib/status.svelte';
  import { prefs } from './lib/prefs.svelte';
  import { themeSync } from './lib/theme-sync.svelte';
  import World from './pages/World.svelte';
  import Lager from './pages/Lager.svelte';
  import Overview from './pages/Overview.svelte';
  import SystemHub from './pages/SystemHub.svelte';
  import SettingsHub from './pages/SettingsHub.svelte';
  import Update from './pages/Update.svelte';
  import Versions from './pages/Versions.svelte';
  import Backup from './pages/Backup.svelte';
  import Restart from './pages/Restart.svelte';
  import Live from './pages/Live.svelte';
  import Log from './pages/Log.svelte';
  import Diagnostics from './pages/Diagnostics.svelte';
  import Hardware from './pages/Hardware.svelte';
  import Tests from './pages/Tests.svelte';
  import Experimental from './pages/Experimental.svelte';
  import Wifi from './pages/Wifi.svelte';
  import Devices from './pages/Devices.svelte';
  import Display from './pages/Display.svelte';
  import Protection from './pages/Protection.svelte';
  import Appearance from './pages/Appearance.svelte';

  // „System“ trägt einen Punkt, wenn die Prüfung (auch im Hintergrund) eine neuere Version gefunden hat
  const items: NavItem[] = $derived([
    { id: 'welt', label: 'Welt', icon: 'landscape', iconActive: 'landscape-fill', href: '#/welt' },
    { id: 'uebersicht', label: 'Übersicht', icon: 'dashboard', iconActive: 'dashboard-fill', href: '#/uebersicht' },
    { id: 'system', label: 'System', icon: 'settings', iconActive: 'settings-fill', href: '#/system', badge: !!device.data?.ota.avail },
    { id: 'einstellungen', label: 'Einstellungen', icon: 'tune', href: '#/einstellungen' },
  ]);

  type Page = { title: string; view: Component<any>; parent?: string; props?: Record<string, unknown> };
  const pages: Record<string, Page> = {
    welt: { title: 'Welt', view: World },
    lager: { title: 'Lager', view: Lager, parent: 'welt' },
    uebersicht: { title: 'Übersicht', view: Overview },
    system: { title: 'System', view: SystemHub },
    einstellungen: { title: 'Einstellungen', view: SettingsHub },
    // System → Wartung, Technik, Labor
    update: { title: 'Update', view: Update, parent: 'system' },
    versionen: { title: 'Versionsverlauf', view: Versions, parent: 'system' },
    sicherung: { title: 'Sicherung', view: Backup, parent: 'system' },
    neustart: { title: 'Neustart und Werksreset', view: Restart, parent: 'system' },
    live: { title: 'Leistung und Live-Bild', view: Live, parent: 'system' },
    log: { title: 'Live-Log', view: Log, parent: 'system' },
    diagnose: { title: 'Diagnose', view: Diagnostics, parent: 'system' },
    geraet: { title: 'Gerät und Speicher', view: Hardware, parent: 'system' },
    tests: { title: 'Gerätetests', view: Tests, parent: 'system' },
    experimentell: { title: 'Experimentell', view: Experimental, parent: 'system' },
    'experimentell/orte': { title: 'Testorte', view: Experimental, parent: 'experimentell', props: { part: 'orte' } },
    'experimentell/panel': { title: 'Panel und Stromsparen', view: Experimental, parent: 'experimentell', props: { part: 'panel' } },
    'experimentell/version': { title: 'Ältere Version', view: Experimental, parent: 'experimentell', props: { part: 'version' } },
    'experimentell/geschichte': { title: 'So entstand Pixelia', view: Story, parent: 'experimentell', props: { inline: true } },
    // Einstellungen → Verbindung, Gerät, Oberfläche
    wlan: { title: 'WLAN', view: Wifi, parent: 'einstellungen' },
    geraete: { title: 'Verbundene Geräte', view: Devices, parent: 'einstellungen' },
    display: { title: 'Display', view: Display, parent: 'einstellungen' },
    schutz: { title: 'Zugangsschutz', view: Protection, parent: 'einstellungen' },
    darstellung: { title: 'Darstellung', view: Appearance, parent: 'einstellungen' },
  };
  // Adressen bis 0.5.3 (Lesezeichen, Links in Doku und Release-Notes)
  const alias: Record<string, string> = { info: 'geraet', hardware: 'geraet', mehr: 'einstellungen' };

  const routeOf = () => {
    const path = location.hash.replace(/^#\/?/, '').replace(/\/$/, '');
    if (!path) return prefs.start;
    if (path in pages) return path;
    const first = path.split('/')[0];
    return alias[first] ?? first;
  };
  let route = $state(routeOf());
  const page = $derived(route in pages ? route : 'uebersicht');
  const cur = $derived(pages[page]);
  const View = $derived(cur.view);
  // oberstes Ziel der Navigation (Unterseiten von Unterseiten, z. B. Experimentell → Testorte)
  const top = $derived.by(() => {
    let p = page;
    while (pages[p].parent) p = pages[p].parent!;
    return p;
  });

  // Setup-Modus (Captive Portal): ohne eigene Route gleich auf die WLAN-Seite
  let setupChecked = false;
  $effect(() => {
    if (setupChecked || !device.data) return;
    setupChecked = true;
    if (device.data.wifi.ap && !location.hash.replace(/^#\/?/, '')) location.hash = '#/wlan';
  });

  $effect(() => {
    theme.apply();
    themeSync.changed(theme.json());
  });
  $effect(() => themeSync.fromStatus(device.data?.set.ui));
  $effect(() => {
    const exp = device.data?.set.exp;
    if (exp !== undefined) theme.allowed = exp;  // eigene Designs nur mit Experimentell
  });
  $effect(() => prefs.apply());
  $effect(() => {
    document.title = `${cur.title} · Pixelia`;
    device.setPage(page);
  });

  function cycleMode() {
    theme.mode = theme.dark ? 'light' : 'dark';
  }

  // Pixelia-Zeichen oben links: führt zur Übersicht; 7× schnell hintereinander öffnet „So entstand Pixelia“
  let story = $state(false);
  let taps: number[] = [];
  function logoTap() {
    const now = Date.now();
    taps = [...taps.filter((t) => now - t < 4000), now];
    if (taps.length >= 7) {
      taps = [];
      story = true;
    }
  }

  // Neue Firmware (Update aus einem anderen Browser oder Neustart mit anderer Version): Diese Seite stammt noch von der
  // alten und kennt neue Einstellungen nicht. Neu laden, auf der Seite Update erst beim nächsten Seitenwechsel.
  let bootVersion = '';
  let stale = false;
  $effect(() => {
    const v = device.data?.v;
    if (!v) return;
    if (!bootVersion) bootVersion = v;
    else if (v !== bootVersion && !stale) {
      stale = true;
      if (page !== 'update') location.reload();
    }
  });

  const linkText = { verbinde: 'Verbinde …', live: 'Live', abfrage: 'Abfrage', getrennt: 'Getrennt' } as const;
</script>

<svelte:window
  onhashchange={() => {
    if (stale) return location.reload();
    route = routeOf();
    scrollTo(0, 0);
  }}
/>

<Navigation {items} active={top}>
  {#snippet header()}
    <a href="#/uebersicht" aria-label="Pixelia, zur Übersicht" onclick={logoTap}><Logo size={48} /></a>
  {/snippet}
</Navigation>

<div class="frame">
  <header class="bar">
    {#if cur.parent}
      <IconButton icon="arrow_back" label="Zurück zu {pages[cur.parent].title}" onclick={() => (location.hash = `#/${cur.parent}`)} />
    {:else}
      <a class="mobile-logo" href="#/uebersicht" aria-label="Pixelia, zur Übersicht" onclick={logoTap}><Logo size={36} /></a>
    {/if}
    <h1>{cur.title}</h1>
    <Chip
      tone={device.link === 'live' ? 'primary' : device.link === 'getrennt' ? 'error' : 'neutral'}
      pulse={device.link === 'live'}
      icon={device.link === 'getrennt' ? 'wifi_off' : 'wifi'}
    >
      <span class="link-text">{linkText[device.link]}</span>
    </Chip>
    <IconButton
      icon={theme.dark ? 'light_mode' : 'dark_mode'}
      label={theme.dark ? 'Hell' : 'Dunkel'}
      onclick={cycleMode}
    />
  </header>

  <main>
    {#key page}
      <div class="page">
        <View {...cur.props} />
      </div>
    {/key}
  </main>
</div>

<Dialog />
<Story bind:open={story} />

<style>
  .frame {
    min-height: 100dvh;
    max-width: 640px;
    margin: 0 auto;
    padding-bottom: calc(88px + env(safe-area-inset-bottom));
  }
  .bar {
    position: sticky;
    top: 0;
    z-index: 5;
    display: flex;
    align-items: center;
    gap: 12px;
    height: 64px;
    padding: 0 8px 0 16px;
    background: var(--md-sys-color-surface);
  }
  h1 {
    flex: 1;
    min-width: 0;
    margin: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    font: var(--md-sys-typescale-title-large);
    font-weight: 600;
    font-stretch: 105%;
  }
  .mobile-logo {
    display: flex;
  }
  main {
    padding: 8px 16px 24px;
  }
  .page {
    animation: enter var(--m3-spatial) both;
  }
  @keyframes enter {
    from {
      opacity: 0;
      transform: translateY(24px);
    }
  }
  /* Schmale Ansicht am großen Bildschirm: Spalte wie am Handy, mittig; Seiten-Raster immer einspaltig */
  :global(html:not(.wide) .page-grid) {
    grid-template-columns: 1fr;
  }
  @media (min-width: 600px) {
    h1 {
      font: var(--md-sys-typescale-headline-small);
      font-weight: 650;
      font-stretch: 110%;
    }
    :global(html.wide) .frame {
      max-width: none;
      margin: 0 0 0 96px;
      padding-bottom: 24px;
    }
    :global(html.wide) .mobile-logo {
      display: none;
    }
    :global(html.wide) .bar {
      padding-left: 24px;
    }
    :global(html.wide) h1 {
      font: var(--md-sys-typescale-headline-medium);
      font-weight: 600;
    }
    :global(html.wide) main {
      max-width: 1280px;
      padding: 8px 24px 24px;
    }
  }
</style>
