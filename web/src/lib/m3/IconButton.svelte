<script lang="ts">
  // M3-Expressive-Icon-Button (Standard, Tonal, Filled), Form-Morphing wie beim Button
  import type { HTMLButtonAttributes } from 'svelte/elements';
  import Icon from './Icon.svelte';
  import type { IconName } from './icon-names';

  let {
    icon,
    label,
    variant = 'standard',
    ...rest
  }: { icon: IconName; label: string; variant?: 'standard' | 'tonal' | 'filled' } & HTMLButtonAttributes = $props();
</script>

<button class="ib {variant}" aria-label={label} title={label} {...rest}>
  <Icon name={icon} />
</button>

<style>
  .ib {
    position: relative;
    display: inline-grid;
    place-items: center;
    width: 40px;
    height: 40px;
    padding: 0;
    border: none;
    border-radius: var(--md-sys-shape-corner-full);
    background: transparent;
    color: var(--md-sys-color-on-surface-variant);
    cursor: pointer;
    overflow: hidden;
    isolation: isolate;
    transition: border-radius var(--m3-spatial-fast);
  }
  .ib::before {
    content: '';
    position: absolute;
    inset: 0;
    background: currentColor;
    opacity: 0;
    z-index: -1;
    transition: opacity var(--m3-effects);
  }
  .ib:hover::before {
    opacity: var(--md-sys-state-hover);
  }
  .ib:active {
    border-radius: var(--md-sys-shape-corner-small);
  }
  .ib:active::before {
    opacity: var(--md-sys-state-pressed);
  }
  .tonal {
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
  }
  .filled {
    background: var(--md-sys-color-primary);
    color: var(--md-sys-color-on-primary);
  }
</style>
