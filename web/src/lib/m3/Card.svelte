<script lang="ts">
  // M3-Karte: gefüllt, erhöht oder umrandet; Formgröße wählbar (Expressive nutzt gern große Ecken)
  import type { Snippet } from 'svelte';
  import type { HTMLAttributes } from 'svelte/elements';

  let {
    variant = 'filled',
    shape = 'large',
    children,
    class: cls = '',
    ...rest
  }: {
    variant?: 'filled' | 'elevated' | 'outlined' | 'primary' | 'tertiary' | 'error';
    shape?: 'medium' | 'large' | 'extra-large';
    children: Snippet;
  } & HTMLAttributes<HTMLElement> = $props();
</script>

<section class="card {variant} {cls}" style:--r="var(--md-sys-shape-corner-{shape})" {...rest}>
  {@render children()}
</section>

<style>
  .card {
    position: relative;
    padding: 20px;
    border-radius: var(--r);
    background: var(--md-sys-color-surface-container);
    color: var(--md-sys-color-on-surface);
    transition:
      background-color var(--m3-effects),
      border-radius var(--m3-spatial);
  }
  .elevated {
    background: var(--md-sys-color-surface-container-low);
    box-shadow: var(--md-sys-elevation-1);
  }
  .outlined {
    background: var(--md-sys-color-surface);
    box-shadow: inset 0 0 0 1px var(--md-sys-color-outline-variant);
  }
  .primary {
    background: var(--md-sys-color-primary-container);
    color: var(--md-sys-color-on-primary-container);
  }
  .tertiary {
    background: var(--md-sys-color-tertiary-container);
    color: var(--md-sys-color-on-tertiary-container);
  }
  .error {
    background: var(--md-sys-color-error-container);
    color: var(--md-sys-color-on-error-container);
  }
</style>
