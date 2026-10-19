<script lang="ts">
  // Pixelia-Zeichen: Pixel-Häuschen in einer Keks-Form (M3 Expressive), Form dreht beim Überfahren federnd
  import { shapePath, type ShapeName } from './m3';

  let {
    size = 48,
    shape = 'cookie9',
    strong = false,
  }: { size?: number; shape?: ShapeName; strong?: boolean } = $props();
  const d = $derived(shapePath(shape));
</script>

<svg class="logo" class:strong width={size} height={size} viewBox="0 0 100 100" aria-hidden="true">
  <path class="bg" {d} />
  <g class="px">
    <rect x="46" y="26" width="8" height="8" />
    <rect x="38" y="34" width="24" height="8" />
    <rect x="30" y="42" width="40" height="8" />
    <rect x="34" y="50" width="32" height="24" />
  </g>
  <rect class="door" x="46" y="60" width="8" height="14" />
  <rect class="win" x="38" y="54" width="6" height="6" />
</svg>

<style>
  .logo {
    display: block;
    flex: none;
  }
  .bg {
    fill: var(--md-sys-color-primary-container);
    transform-origin: 50% 50%;
    transition: transform var(--m3-spatial-slow);
  }
  .logo:hover .bg {
    transform: rotate(40deg) scale(1.04);
  }
  .px {
    fill: var(--md-sys-color-on-primary-container);
  }
  .door {
    fill: var(--md-sys-color-primary-container);
  }
  .win {
    fill: var(--md-sys-color-tertiary);
  }
  /* Auf farbigem Grund (z. B. Hauptkarte): kräftige Form, helle Pixel */
  .strong .bg,
  .strong .door {
    fill: var(--md-sys-color-primary);
  }
  .strong .px {
    fill: var(--md-sys-color-on-primary);
  }
  .strong .win {
    fill: var(--md-sys-color-tertiary-container);
  }
</style>
