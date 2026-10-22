<script lang="ts">
  // Seite „Darstellung“: hell/dunkel, Farbstil, Akzentfarbe (dynamische Farben, auf dem Gerät gespeichert), seit 0.5.4
  // Ansicht, Schriftgröße, Startseite und Bewegung (nur dieser Browser, lib/prefs.svelte.ts) und eine kleine Vorschau
  import {
    Switch,
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
  import { themeSync } from '../lib/theme-sync.svelte';
  import { prefs, type FontSize, type StartPage } from '../lib/prefs.svelte';

  /** Hauptfarbe des Designs aus Experimentell; leer bei Material 3 */
  const own = $derived(theme.ownColor);

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
  const views = [
    { value: 'schmal', label: 'Schmal', icon: 'width_normal' as const },
    { value: 'breit', label: 'Breit', icon: 'width_wide' as const },
  ];
  const fonts: { value: FontSize; label: string }[] = [
    { value: 's', label: 'Klein' },
    { value: 'm', label: 'Normal' },
    { value: 'l', label: 'Groß' },
  ];
  const starts: { value: StartPage; label: string }[] = [
    { value: 'uebersicht', label: 'Übersicht' },
    { value: 'welt', label: 'Welt' },
  ];
  const round = shapePath('circle');
  const cookie = shapePath('cookie9');
  let demo = $state(0.62);
  let toggled = $state(true);
</script>

<div class="cols">
  <Card>
    <h2>Hell oder dunkel</h2>
    <ButtonGroup options={modes} bind:value={theme.mode} label="Hell oder dunkel" />
    {#if own}
      <p class="muted">
        Im Design aus Experimentell gilt eine eigene Akzentfarbe, im Stil des Designs: „Design“ nimmt seine eigenen Farben.
        Der Farbstil gilt nur für Material 3.
      </p>
    {:else}
      <h2>Farbstil</h2>
      <ButtonGroup options={styles} bind:value={theme.style} label="Farbstil" />
    {/if}
    <h2>Akzentfarbe</h2>
    <div class="swatches" role="radiogroup" aria-label="Akzentfarbe">
      {#each own ? [{ hex: '', name: 'Design', fill: own }, ...SEEDS] : SEEDS as c (c.hex)}
        {@const on = (own ? theme.dseed : theme.seed) === c.hex}
        <button
          class="swatch"
          class:on
          role="radio"
          aria-checked={on}
          aria-label={c.name}
          title={c.name}
          onclick={() => (own ? (theme.dseed = c.hex) : (theme.seed = c.hex))}
        >
          <svg viewBox="0 0 100 100" width="48" height="48"><path d={on ? cookie : round} fill={'fill' in c ? c.fill : c.hex} /></svg>
        </button>
      {/each}
      <label class="custom" title="Eigene Farbe">
        {#if own}
          <input type="color" bind:value={() => theme.dseed || own, (v) => (theme.dseed = v)} aria-label="Eigene Akzentfarbe" />
        {:else}
          <input type="color" bind:value={theme.seed} aria-label="Eigene Akzentfarbe" />
        {/if}
      </label>
    </div>
    <p class="hint">
      Alle Farbtöne werden aus der Akzentfarbe berechnet (Material Color Utilities, Spezifikation 2025). Pixelia
      speichert die Wahl für alle Browser, sie steckt auch in der Sicherung der Einstellungen.
    </p>
    {#if themeSync.unsaved}
      <div class="unsaved">
        <span>Nur in diesem Browser: Zum Speichern auf dem Gerät ist der PIN nötig.</span>
        <Button variant="tonal" icon="lock_open" onclick={() => themeSync.save(true)}>Speichern</Button>
      </div>
    {/if}
  </Card>

  <Card>
    <h2>Ansicht</h2>
    <ButtonGroup options={views} bind:value={() => (prefs.wide ? 'breit' : 'schmal'), (v: string) => (prefs.wide = v === 'breit')} label="Ansicht" />
    <p class="hint">Schmal: Karten untereinander und die Leiste unten, wie am Handy. Breit: Leiste links und Karten nebeneinander, für große Bildschirme.</p>
    <h2>Schriftgröße</h2>
    <ButtonGroup options={fonts} bind:value={prefs.font} label="Schriftgröße" />
    <h2>Startseite</h2>
    <ButtonGroup options={starts} bind:value={prefs.start} label="Startseite" />
    <div class="switch-row">
      <span>
        Bewegung reduzieren
        <span class="hint">Ohne Animationen beim Seitenwechsel, ohne Wellen und Puls</span>
      </span>
      <Switch label="Bewegung reduzieren" checked={prefs.reduce} onchange={(v) => (prefs.reduce = v)} />
    </div>
    <p class="hint">Ansicht, Schrift, Startseite und Bewegung gelten nur für diesen Browser, die Farben für alle.</p>
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
  .switch-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    margin-top: 20px;
  }
  .switch-row > span {
    display: grid;
  }
  .switch-row .hint {
    margin: 2px 0 0;
    font: var(--md-sys-typescale-body-small);
  }
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
  .unsaved {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 12px;
    margin-top: 16px;
    padding: 12px 16px;
    border-radius: var(--md-sys-shape-corner-large);
    background: var(--md-sys-color-secondary-container);
    color: var(--md-sys-color-on-secondary-container);
  }
  .unsaved span {
    flex: 1 1 200px;
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
