<script lang="ts">
  // M3-Expressive-Ladeanzeige: Form wechselt federnd (Keks → Blüte → Fünfeck …) und dreht sich dabei
  import { shapePath, type ShapeName } from './shapes';

  let { size = 48, contained = false, label = 'Lädt' }: { size?: number; contained?: boolean; label?: string } =
    $props();

  const seq: ShapeName[] = ['cookie9', 'pentagon', 'pill', 'sunny', 'cookie6', 'oval', 'flower'];
  const values = [...seq, seq[0]].map((s) => shapePath(s, 50, 38)).join(';');
  const n = seq.length;
  const keyTimes = Array.from({ length: n + 1 }, (_, i) => (i / n).toFixed(3)).join(';');
  // Zügiges Überblenden je Abschnitt mit Ruhephase (SMIL erlaubt kein Überschwingen, Werte nur 0…1)
  const keySplines = Array(n).fill('0.5 0 0.1 1').join(';');
</script>

<div class="li" class:contained style:width="{size}px" style:height="{size}px" role="progressbar" aria-label={label}>
  <svg viewBox="0 0 100 100" width={size} height={size}>
    <path d={shapePath(seq[0], 50, 38)}>
      <animate
        attributeName="d"
        dur="{n * 0.65}s"
        repeatCount="indefinite"
        calcMode="spline"
        {values}
        {keyTimes}
        {keySplines}
      />
    </path>
  </svg>
</div>

<style>
  .li {
    display: inline-grid;
    place-items: center;
    color: var(--md-sys-color-primary);
  }
  .contained {
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-primary-container);
    color: var(--md-sys-color-on-primary-container);
  }
  .contained svg {
    transform: scale(0.7);
  }
  svg {
    animation: spin 4.55s linear infinite;
  }
  path {
    fill: currentColor;
  }
  @keyframes spin {
    to {
      rotate: 360deg;
    }
  }
</style>
