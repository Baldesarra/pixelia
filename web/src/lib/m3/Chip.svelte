<script lang="ts">
  // Kleiner Status-Chip (nur Anzeige), optional mit Icon oder pulsierendem Punkt
  import type { Snippet } from 'svelte';
  import Icon from './Icon.svelte';
  import type { IconName } from './icon-names';

  let {
    tone = 'neutral',
    icon,
    pulse = false,
    children,
  }: {
    tone?: 'neutral' | 'primary' | 'tertiary' | 'error';
    icon?: IconName;
    pulse?: boolean;
    children: Snippet;
  } = $props();
</script>

<span class="chip {tone}">
  {#if pulse}<span class="dot"></span>{:else if icon}<Icon name={icon} size={18} />{/if}
  {@render children()}
</span>

<style>
  .chip {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    height: 32px;
    padding: 0 12px;
    border-radius: var(--md-sys-shape-corner-small);
    font: var(--md-sys-typescale-label-large);
    white-space: nowrap;
    background: var(--md-sys-color-surface-container-high);
    color: var(--md-sys-color-on-surface-variant);
    transition:
      background-color var(--m3-effects),
      color var(--m3-effects);
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
  .dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: currentColor;
    animation: pulse 2s var(--md-sys-motion-spring-default-effects) infinite;
  }
  @keyframes pulse {
    0%,
    100% {
      transform: scale(1);
      opacity: 1;
    }
    50% {
      transform: scale(0.6);
      opacity: 0.5;
    }
  }
</style>
