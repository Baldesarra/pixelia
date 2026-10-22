<script lang="ts">
  // Material Symbol als Inline-SVG (nur die Pfade aus icon-names.ts sind im Bundle, Raster 240 statt 960: vite.config.ts)
  import icons from 'virtual:pixelia-icons';
  import type { IconName } from './icon-names';

  let { name, size = 24 }: { name: IconName; size?: number } = $props();
</script>

<svg class="icon" width={size} height={size} viewBox="0 -240 240 240" aria-hidden="true">
  <path d={icons[name]} />
</svg>

<style>
  .icon {
    flex: none;
    fill: currentColor;
    display: block;
  }
</style>
