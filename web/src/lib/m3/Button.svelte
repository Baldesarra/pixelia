<script lang="ts">
  // M3-Expressive-Button: Varianten, Größen, Form rund/eckig, Form-Morphing beim Drücken und bei Auswahl
  import type { Snippet } from 'svelte';
  import type { HTMLAnchorAttributes, HTMLButtonAttributes } from 'svelte/elements';
  import Icon from './Icon.svelte';
  import type { IconName } from './icon-names';

  type Props = {
    variant?: 'filled' | 'tonal' | 'outlined' | 'text' | 'elevated';
    size?: 'xs' | 's' | 'm' | 'l';
    shape?: 'round' | 'square';
    icon?: IconName;
    trailingIcon?: IconName;
    selected?: boolean;
    href?: string;
    children?: Snippet;
  } & HTMLButtonAttributes &
    HTMLAnchorAttributes;

  let {
    variant = 'filled',
    size = 's',
    shape = 'round',
    icon,
    trailingIcon,
    selected,
    href,
    children,
    class: cls = '',
    ...rest
  }: Props = $props();

  const iconSize = $derived({ xs: 20, s: 20, m: 24, l: 32 }[size]);
</script>

<svelte:element
  this={href ? 'a' : 'button'}
  {href}
  class="btn {variant} {size} {shape} {cls}"
  class:selected
  aria-pressed={selected === undefined || href ? undefined : selected}
  {...rest}
>
  {#if icon}<Icon name={icon} size={iconSize} />{/if}
  {#if children}<span class="label">{@render children()}</span>{/if}
  {#if trailingIcon}<Icon name={trailingIcon} size={iconSize} />{/if}
</svelte:element>

<style>
  .btn {
    --h: 40px;
    --pad: 16px;
    --r: calc(var(--h) / 2);
    --r-square: var(--md-sys-shape-corner-medium);
    --r-pressed: var(--md-sys-shape-corner-small);
    --bg: transparent;
    --fg: var(--md-sys-color-primary);
    position: relative;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    height: var(--h);
    padding: 0 var(--pad);
    border: none;
    border-radius: var(--r);
    background: var(--bg);
    color: var(--fg);
    font: var(--md-sys-typescale-label-large);
    text-decoration: none;
    cursor: pointer;
    user-select: none;
    overflow: hidden;
    isolation: isolate;
    white-space: nowrap;
    transition:
      border-radius var(--m3-spatial-fast),
      background-color var(--m3-effects),
      color var(--m3-effects),
      box-shadow var(--m3-effects);
  }
  .btn::before {
    content: '';
    position: absolute;
    inset: 0;
    background: currentColor;
    opacity: 0;
    z-index: -1;
    transition: opacity var(--m3-effects);
  }
  .btn:hover::before {
    opacity: var(--md-sys-state-hover);
  }
  .btn:active::before {
    opacity: var(--md-sys-state-pressed);
  }
  /* Form-Morphing: gedrückt wird die Ecke kleiner, ausgewählt wird rund zu eckig (bzw. eckig zu rund) */
  .btn:active {
    border-radius: var(--r-pressed);
  }
  .square {
    border-radius: var(--r-square);
  }
  .round.selected {
    border-radius: var(--r-square);
  }
  .square.selected {
    border-radius: var(--r);
  }
  .btn:disabled {
    cursor: default;
    pointer-events: none;
    --bg: color-mix(in srgb, var(--md-sys-color-on-surface) 10%, transparent);
    --fg: color-mix(in srgb, var(--md-sys-color-on-surface) 38%, transparent);
    box-shadow: none;
  }

  .xs {
    --h: 32px;
    --pad: 12px;
    --r-square: var(--md-sys-shape-corner-medium);
    --r-pressed: var(--md-sys-shape-corner-small);
  }
  .m {
    --h: 56px;
    --pad: 24px;
    --r-square: var(--md-sys-shape-corner-large);
    --r-pressed: var(--md-sys-shape-corner-medium);
    font: var(--md-sys-typescale-title-medium);
  }
  .l {
    --h: 96px;
    --pad: 48px;
    --r-square: var(--md-sys-shape-corner-extra-large);
    --r-pressed: var(--md-sys-shape-corner-large);
    font: var(--md-sys-typescale-headline-small);
  }

  .filled {
    --bg: var(--md-sys-color-primary);
    --fg: var(--md-sys-color-on-primary);
  }
  .filled.selected {
    --bg: var(--md-sys-color-primary);
  }
  .tonal {
    --bg: var(--md-sys-color-secondary-container);
    --fg: var(--md-sys-color-on-secondary-container);
  }
  .tonal.selected {
    --bg: var(--md-sys-color-secondary);
    --fg: var(--md-sys-color-on-secondary);
  }
  .elevated {
    --bg: var(--md-sys-color-surface-container-low);
    box-shadow: var(--md-sys-elevation-1);
  }
  .outlined {
    box-shadow: inset 0 0 0 1px var(--md-sys-color-outline-variant);
    --fg: var(--md-sys-color-on-surface-variant);
  }
  .outlined.selected {
    --bg: var(--md-sys-color-inverse-surface);
    --fg: var(--md-sys-color-inverse-on-surface);
    box-shadow: none;
  }
  .text {
    --pad: 12px;
  }
  /* Gefährliche Aktion (Neustart, Löschen): class="danger" */
  .btn.filled.danger {
    --bg: var(--md-sys-color-error);
    --fg: var(--md-sys-color-on-error);
  }
  .btn.danger:not(.filled) {
    --fg: var(--md-sys-color-error);
  }
  .label {
    display: inline-flex;
    align-items: center;
  }
</style>
