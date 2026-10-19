<script lang="ts">
  // Seite „Darstellung“: hell/dunkel, Farbstil, Akzentfarbe (dynamische Farben) und eine kleine Vorschau
  import {
    Button,
    ButtonGroup,
    Card,
    Chip,
    CircularProgress,
    LoadingIndicator,
    SEEDS,
    shapePath,
    theme,
    WavyProgress,
    type ThemeMode,
    type ThemeStyle,
  } from '../lib/m3';

  const modes: { value: ThemeMode; label: string; icon: 'brightness_auto' | 'light_mode' | 'dark_mode' }[] = [
    { value: 'system', label: 'System', icon: 'brightness_auto' },
    { value: 'light', label: 'Hell', icon: 'light_mode' },
    { value: 'dark', label: 'Dunkel', icon: 'dark_mode' },
  ];
  const styles: { value: ThemeStyle; label: string }[] = [
    { value: 'tonal', label: 'Ruhig' },
    { value: 'vibrant', label: 'Lebhaft' },
    { value: 'expressive', label: 'Expressiv' },
  ];
  const round = shapePath('circle');
  const cookie = shapePath('cookie9');
  let demo = $state(0.62);
  let toggled = $state(true);
</script>

<div class="cols">
  <Card>
    <h2>Helligkeit</h2>
    <ButtonGroup options={modes} bind:value={theme.mode} label="Hell oder dunkel" />
    <h2>Farbstil</h2>
    <ButtonGroup options={styles} bind:value={theme.style} label="Farbstil" />
    <h2>Akzentfarbe</h2>
    <div class="swatches" role="radiogroup" aria-label="Akzentfarbe">
      {#each SEEDS as c (c.hex)}
        {@const on = theme.seed === c.hex}
        <button
          class="swatch"
          class:on
          role="radio"
          aria-checked={on}
          aria-label={c.name}
          title={c.name}
          onclick={() => (theme.seed = c.hex)}
        >
          <svg viewBox="0 0 100 100" width="48" height="48"><path d={on ? cookie : round} fill={c.hex} /></svg>
        </button>
      {/each}
      <label class="custom" title="Eigene Farbe">
        <input type="color" bind:value={theme.seed} aria-label="Eigene Akzentfarbe" />
      </label>
    </div>
    <p class="hint">
      Alle Farbtöne werden aus der Akzentfarbe berechnet (Material Color Utilities, Spezifikation 2025). Die Wahl
      speichert dieser Browser.
    </p>
  </Card>

  <Card variant="outlined">
    <h2>Vorschau</h2>
    <div class="row">
      <Button>Gefüllt</Button>
      <Button variant="tonal">Tonal</Button>
      <Button variant="outlined">Umrandet</Button>
      <Button variant="text">Text</Button>
    </div>
    <div class="row">
      <Button variant="elevated" icon="palette">Erhöht</Button>
      <Button variant="tonal" shape="square" selected={toggled} onclick={() => (toggled = !toggled)}>
        Umschalten
      </Button>
      <Button size="m" icon="check">Groß</Button>
    </div>
    <div class="row chips">
      <Chip tone="primary" pulse>Live</Chip>
      <Chip tone="tertiary" icon="warning">Hinweis</Chip>
      <Chip tone="error" icon="error">Fehler</Chip>
    </div>
    <div class="row progress">
      <CircularProgress value={demo} size={96} label="Beispiel">
        <strong class="tnum">{Math.round(demo * 100)} %</strong>
      </CircularProgress>
      <LoadingIndicator size={64} label="Beispiel-Ladeanzeige" />
      <LoadingIndicator size={64} contained label="Beispiel-Ladeanzeige mit Hintergrund" />
    </div>
    <WavyProgress value={demo} label="Beispiel wellig" />
    <div class="gap"></div>
    <WavyProgress label="Beispiel unbestimmt" tone="tertiary" />
    <div class="gap"></div>
    <input class="range" type="range" min="0" max="1" step="0.01" bind:value={demo} aria-label="Beispielwert" />
  </Card>
</div>

<style>
  .cols {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(min(100%, 380px), 1fr));
    gap: 16px;
    align-items: start;
  }
  h2 {
    margin: 20px 0 12px;
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
    color: var(--md-sys-color-on-surface-variant);
  }
  h2:first-child {
    margin-top: 0;
  }
  .swatches {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    align-items: center;
  }
  .swatch {
    padding: 0;
    border: none;
    background: none;
    cursor: pointer;
    border-radius: 50%;
  }
  .swatch svg {
    display: block;
    transition: transform var(--m3-spatial);
  }
  .swatch path {
    transition: d var(--m3-spatial);
  }
  .swatch.on svg {
    transform: rotate(20deg) scale(1.08);
  }
  .custom {
    display: grid;
    place-items: center;
    width: 48px;
    height: 48px;
    border-radius: var(--md-sys-shape-corner-medium);
    background: conic-gradient(#e53935, #fdd835, #43a047, #1e88e5, #8e24aa, #e53935);
    cursor: pointer;
    overflow: hidden;
  }
  .custom input {
    opacity: 0;
    width: 100%;
    height: 100%;
    cursor: pointer;
  }
  .hint {
    margin: 16px 0 0;
    color: var(--md-sys-color-on-surface-variant);
  }
  .row {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px;
    margin-bottom: 16px;
  }
  .progress {
    gap: 24px;
  }
  .gap {
    height: 16px;
  }
  .range {
    width: 100%;
    accent-color: var(--md-sys-color-primary);
  }
</style>
