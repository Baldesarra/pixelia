<script lang="ts">
  // Rahmen der Oberfläche: Navigation (Leiste/Schiene), obere Leiste, Seiten per Hash-Route (#/…)
  import { Chip, IconButton, Navigation, theme, type NavItem } from './lib/m3';
  import Logo from './lib/Logo.svelte';
  import { device } from './lib/status.svelte';
  import Overview from './pages/Overview.svelte';
  import Appearance from './pages/Appearance.svelte';

  const items: NavItem[] = [
    { id: 'uebersicht', label: 'Übersicht', icon: 'dashboard', iconActive: 'dashboard-fill', href: '#/' },
    { id: 'darstellung', label: 'Darstellung', icon: 'palette', iconActive: 'palette-fill', href: '#/darstellung' },
    { id: 'einrichtung', label: 'Einrichtung', icon: 'settings', iconActive: 'settings-fill', href: '/setup' },
  ];
  const titles: Record<string, string> = { uebersicht: 'Übersicht', darstellung: 'Darstellung' };

  const routeOf = () => (location.hash.replace(/^#\/?/, '') || 'uebersicht').split('/')[0];
  let route = $state(routeOf());
  const page = $derived(route in titles ? route : 'uebersicht');

  $effect(() => theme.apply());
  $effect(() => {
    document.title = `${titles[page]} · Pixelia`;
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

<Navigation {items} active={page}>
  {#snippet header()}
    <a href="#/" aria-label="Pixelia"><Logo size={48} /></a>
  {/snippet}
</Navigation>

<div class="frame">
  <header class="bar">
    <span class="mobile-logo"><Logo size={36} /></span>
    <h1>{titles[page]}</h1>
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
        {#if page === 'darstellung'}
          <Appearance />
        {:else}
          <Overview />
        {/if}
      </div>
    {/key}
  </main>
</div>

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
    margin: 0;
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
