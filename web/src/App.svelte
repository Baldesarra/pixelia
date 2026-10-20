<script lang="ts">
  // Rahmen der Oberfläche: Navigation (Leiste/Schiene), obere Leiste, Seiten per Hash-Route (#/…)
  import type { Component } from 'svelte';
  import { Chip, Dialog, IconButton, Navigation, theme, type NavItem } from './lib/m3';
  import Logo from './lib/Logo.svelte';
  import { device } from './lib/status.svelte';
  import { themeSync } from './lib/theme-sync.svelte';
  import Overview from './pages/Overview.svelte';
  import Appearance from './pages/Appearance.svelte';
  import Info from './pages/Info.svelte';
  import Hardware from './pages/Hardware.svelte';
  import Wifi from './pages/Wifi.svelte';
  import Devices from './pages/Devices.svelte';
  import System from './pages/System.svelte';
  import Log from './pages/Log.svelte';
  import More from './pages/More.svelte';
  import Versions from './pages/Versions.svelte';
  import Diagnostics from './pages/Diagnostics.svelte';
  import Live from './pages/Live.svelte';
  import Experimental from './pages/Experimental.svelte';

  // Handy-Leiste: höchstens fünf Ziele, der Rest steckt unter „Mehr“; die Schiene (ab 600 px) zeigt alles
  // „System“ trägt einen Punkt, wenn die Prüfung (auch im Hintergrund) eine neuere Version gefunden hat
  const items: NavItem[] = $derived([
    { id: 'uebersicht', label: 'Übersicht', icon: 'dashboard', iconActive: 'dashboard-fill', href: '#/' },
    { id: 'wlan', label: 'WLAN', icon: 'wifi', href: '#/wlan' },
    { id: 'system', label: 'System', icon: 'settings', iconActive: 'settings-fill', href: '#/system', badge: !!device.data?.ota.avail },
    { id: 'log', label: 'Log', icon: 'terminal', iconActive: 'terminal-fill', href: '#/log' },
    { id: 'info', label: 'Info', icon: 'info', iconActive: 'info-fill', href: '#/info', bar: false },
    { id: 'hardware', label: 'Hardware', icon: 'developer_board', iconActive: 'developer_board-fill', href: '#/hardware', bar: false },
    { id: 'geraete', label: 'Geräte', icon: 'devices', iconActive: 'devices-fill', href: '#/geraete', bar: false },
    { id: 'darstellung', label: 'Darstellung', icon: 'palette', iconActive: 'palette-fill', href: '#/darstellung', bar: false },
    { id: 'versionen', label: 'Versionen', icon: 'history', href: '#/versionen', bar: false },
    { id: 'diagnose', label: 'Diagnose', icon: 'monitor_heart', iconActive: 'monitor_heart-fill', href: '#/diagnose', bar: false },
    { id: 'live', label: 'Live', icon: 'monitoring', iconActive: 'monitoring-fill', href: '#/live', bar: false },
    { id: 'experimentell', label: 'Experimentell', icon: 'science', iconActive: 'science-fill', href: '#/experimentell', bar: false },
    { id: 'mehr', label: 'Mehr', icon: 'apps', iconActive: 'apps-fill', href: '#/mehr', rail: false },
  ]);
  const pages: Record<string, { title: string; view: Component }> = {
    uebersicht: { title: 'Übersicht', view: Overview },
    wlan: { title: 'WLAN', view: Wifi },
    system: { title: 'System', view: System },
    log: { title: 'Live-Log', view: Log },
    info: { title: 'Info', view: Info },
    hardware: { title: 'Hardware', view: Hardware },
    geraete: { title: 'Geräte', view: Devices },
    darstellung: { title: 'Darstellung', view: Appearance },
    versionen: { title: 'Versionsverlauf', view: Versions },
    diagnose: { title: 'Diagnose', view: Diagnostics },
    live: { title: 'Live-Vorschau und Leistung', view: Live },
    experimentell: { title: 'Experimentell', view: Experimental },
    mehr: { title: 'Mehr', view: More },
  };
  const inMore = ['info', 'hardware', 'geraete', 'darstellung', 'versionen', 'diagnose', 'live', 'experimentell'];

  const routeOf = () => (location.hash.replace(/^#\/?/, '') || 'uebersicht').split('/')[0];
  let route = $state(routeOf());
  const page = $derived(route in pages ? route : 'uebersicht');
  const View = $derived(pages[page].view);

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
    document.title = `${pages[page].title} · Pixelia`;
    device.setPage(page);
  });

  function cycleMode() {
    theme.mode = theme.dark ? 'light' : 'dark';
  }

  const linkText = { verbinde: 'Verbinde …', live: 'Live', abfrage: 'Abfrage', getrennt: 'Getrennt' } as const;
</script>

<svelte:window
  onhashchange={() => {
    route = routeOf();
    scrollTo(0, 0);
  }}
/>

<Navigation {items} active={page} activeBar={inMore.includes(page) ? 'mehr' : page}>
  {#snippet header()}
    <a href="#/" aria-label="Pixelia"><Logo size={48} /></a>
  {/snippet}
</Navigation>

<div class="frame">
  <header class="bar">
    <span class="mobile-logo"><Logo size={36} /></span>
    <h1>{pages[page].title}</h1>
    <Chip
      tone={device.link === 'live' ? 'primary' : device.link === 'getrennt' ? 'error' : 'neutral'}
      pulse={device.link === 'live'}
      icon={device.link === 'getrennt' ? 'wifi_off' : 'wifi'}
    >
      {linkText[device.link]}
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
        <View />
      </div>
    {/key}
  </main>
</div>

<Dialog />

<style>
  .frame {
    min-height: 100dvh;
    padding-bottom: calc(80px + env(safe-area-inset-bottom));
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
    display: contents;
  }
  main {
    padding: 8px 16px 24px;
    max-width: 1280px;
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
  @media (min-width: 600px) {
    .frame {
      margin-left: 96px;
      padding-bottom: 24px;
    }
    .mobile-logo {
      display: none;
    }
    .bar {
      padding-left: 24px;
    }
    h1 {
      font: var(--md-sys-typescale-headline-medium);
      font-weight: 600;
      font-stretch: 110%;
    }
    main {
      padding: 8px 24px 24px;
    }
  }
</style>
