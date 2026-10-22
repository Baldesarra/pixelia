<script lang="ts">
  // „So entstand Pixelia“: Zeitleiste aus lib/story.ts. Als Easter Egg ein Blatt über der Seite (App.svelte nach 7×
  // Tippen aufs Pixelia-Zeichen; Escape oder Schließen beendet es), mit inline als Seite unter Experimentell.
  import { IconButton } from './m3';
  import Icon from './m3/Icon.svelte';
  import Logo from './Logo.svelte';
  import { STORY } from './story';
  import { dateDe } from './changelog';

  let { open = $bindable(false), inline = false }: { open?: boolean; inline?: boolean } = $props();

  // Wer aufs Zeichen tippt, tippt meist öfter als 7×: Die weiteren Tipper landen auf dem Hintergrund. Der schließt
  // das Blatt daher erst nach einer kurzen Sperre (LOCK_MS), das X und Escape gehen sofort.
  const LOCK_MS = 2000;
  let openedAt = 0;
  $effect(() => {
    if (open) openedAt = Date.now();
  });
  const scrimClose = () => {
    if (Date.now() - openedAt > LOCK_MS) open = false;
  };
</script>

{#snippet content()}
    <p class="lead">Wie aus einer Idee nach und nach ein Dorf wird.</p>
    <ol>
      {#each STORY as st, k (k)}
        <li>
          <span class="dot" aria-hidden="true"></span>
          <span class="d tnum">{dateDe(st.d)}</span>
          <b>{st.title}</b>
          <span>{st.text}</span>
          {#if st.links?.length}
            <span class="links">
              {#each st.links as l (l.href)}
                <a href={l.href} target="_blank" rel="noopener">{l.label}<Icon name="open_in_new" size={14} /></a>
              {/each}
            </span>
          {/if}
        </li>
      {/each}
    </ol>
    <p class="muted small">Die Skizzen öffnen sich im Internet auf GitHub. Fortsetzung folgt.</p>
{/snippet}

<svelte:window onkeydown={(e) => open && e.key === 'Escape' && (open = false)} />

{#if inline}
  <div class="inline">{@render content()}</div>
{:else if open}
  <div class="scrim" role="presentation" onclick={scrimClose}></div>
  <div class="sheet" role="dialog" aria-modal="true" aria-label="So entstand Pixelia">
    <header>
      <Logo size={40} />
      <h2>So entstand Pixelia</h2>
      <IconButton icon="close" label="Schließen" onclick={() => (open = false)} />
    </header>
    {@render content()}
  </div>
{/if}

<style>
  .scrim {
    position: fixed;
    inset: 0;
    z-index: 40;
    background: rgb(0 0 0 / 0.4);
    animation: fade var(--m3-effects) both;
  }
  .sheet {
    position: fixed;
    z-index: 41;
    left: 50%;
    bottom: 0;
    width: min(640px, 100%);
    max-height: 88dvh;
    overflow-y: auto;
    padding: 16px 20px calc(24px + env(safe-area-inset-bottom));
    border-radius: var(--md-sys-shape-corner-extra-large) var(--md-sys-shape-corner-extra-large) 0 0;
    background: var(--md-sys-color-surface-container-low);
    transform: translateX(-50%);
    animation: rise var(--m3-spatial) both;
  }
  .inline .lead {
    margin-top: 0;
  }
  header {
    display: flex;
    align-items: center;
    gap: 12px;
  }
  h2 {
    flex: 1;
    margin: 0;
    font: var(--md-sys-typescale-headline-small);
    font-weight: 700;
    font-stretch: 110%;
  }
  .lead {
    margin: 12px 0 20px;
    font: var(--md-sys-typescale-body-large);
    color: var(--md-sys-color-on-surface-variant);
  }
  ol {
    margin: 0;
    padding: 0;
    list-style: none;
  }
  li {
    position: relative;
    display: grid;
    gap: 2px;
    padding: 0 0 20px 28px;
    border-left: 2px solid var(--md-sys-color-outline-variant);
    margin-left: 7px;
  }
  li:last-child {
    border-left-color: transparent;
  }
  .dot {
    position: absolute;
    left: -8px;
    top: 2px;
    width: 14px;
    height: 14px;
    border-radius: 4px;
    background: var(--md-sys-color-primary);
    transform: rotate(45deg);
  }
  .d {
    color: var(--md-sys-color-primary);
    font: var(--md-sys-typescale-label-medium);
    font-weight: 650;
  }
  b {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
  .links {
    display: flex;
    flex-wrap: wrap;
    gap: 6px 16px;
    margin-top: 4px;
  }
  .links a {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    font-weight: 600;
    text-decoration: none;
  }
  .small {
    margin: 0;
    font: var(--md-sys-typescale-body-small);
  }
  @keyframes fade {
    from {
      opacity: 0;
    }
  }
  @keyframes rise {
    from {
      transform: translate(-50%, 40%);
      opacity: 0;
    }
  }
</style>
