<script lang="ts" module>
  import type { IconName } from './icon-names';

  export type NavItem = { id: string; label: string; icon: IconName; iconActive?: IconName; href: string };
</script>

<script lang="ts">
  // M3-Expressive-Navigation: unten als Navigationsleiste (Handy), ab 600 px als Navigationsschiene links.
  // Aktiver Eintrag: Pille in secondary-container, wächst federnd auf; Icon wechselt auf die gefüllte Variante.
  import type { Snippet } from 'svelte';
  import Icon from './Icon.svelte';

  let { items, active, header }: { items: NavItem[]; active: string; header?: Snippet } = $props();
</script>

<nav class="nav" aria-label="Hauptnavigation">
  {#if header}<div class="header">{@render header()}</div>{/if}
  <ul>
    {#each items as it (it.id)}
      {@const on = it.id === active}
      <li>
        <a href={it.href} class:on aria-current={on ? 'page' : undefined}>
          <span class="pill"><Icon name={on && it.iconActive ? it.iconActive : it.icon} /></span>
          <span class="label">{it.label}</span>
        </a>
      </li>
    {/each}
  </ul>
</nav>

<style>
  .nav {
    position: fixed;
    z-index: 10;
    left: 0;
    right: 0;
    bottom: 0;
    background: var(--md-sys-color-surface-container);
    padding-bottom: env(safe-area-inset-bottom);
  }
  .header {
    display: none;
  }
  ul {
    display: flex;
    margin: 0;
    padding: 0;
    list-style: none;
    height: 64px;
  }
  li {
    flex: 1;
  }
  a {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 4px;
    height: 100%;
    color: var(--md-sys-color-on-surface-variant);
    text-decoration: none;
    font: var(--md-sys-typescale-label-medium);
  }
  .pill {
    position: relative;
    display: grid;
    place-items: center;
    width: 56px;
    height: 32px;
    border-radius: var(--md-sys-shape-corner-full);
    isolation: isolate;
  }
  .pill::before {
    content: '';
    position: absolute;
    inset: 0;
    z-index: -1;
    border-radius: inherit;
    background: var(--md-sys-color-secondary-container);
    transform: scaleX(0.3);
    opacity: 0;
    transition:
      transform var(--m3-spatial-fast),
      opacity var(--m3-effects-fast);
  }
  a:hover .pill::before {
    opacity: 0.5;
    transform: scaleX(1);
  }
  a.on {
    color: var(--md-sys-color-secondary);
    font-weight: 700;
  }
  a.on .pill {
    color: var(--md-sys-color-on-secondary-container);
  }
  a.on .pill::before {
    opacity: 1;
    transform: scaleX(1);
  }

  /* Ab mittlerer Breite: Navigationsschiene links */
  @media (min-width: 600px) {
    .nav {
      top: 0;
      right: auto;
      width: 96px;
      padding: 12px 0 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 24px;
      background: var(--md-sys-color-surface);
    }
    .header {
      display: block;
    }
    ul {
      flex-direction: column;
      height: auto;
      gap: 12px;
      width: 100%;
    }
    a {
      height: 64px;
    }
  }
</style>
