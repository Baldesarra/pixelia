<script lang="ts">
  // Seite „Welt“ (seit 0.5.4, erster Punkt der Navigation): bis die echten Welt-Seiten kommen ein Ausblick auf Pixelia.
  // Vorgeschichte, die Kapitel (ROADMAP ab 0.6.x) und die geplanten Seiten. Kommt eine Seite dazu, wird ihre Karte
  // zum Link (href) und wandert nach oben; „soon“ fällt dann weg.
  import { Card, Icon, WavyProgress, type IconName } from '../lib/m3';
  import CameraCard from '../lib/CameraCard.svelte';
  import { device } from '../lib/status.svelte';

  const chapters = [
    { n: 1, title: 'Der erste Frühling', text: 'Der Bahnhof mit dem gestrandeten Zug, sechzehn Bewohner, Notunterkünfte und Abende in der Taverne.' },
    { n: 2, title: 'Das Dorf wächst', text: 'Häuser, Bauernhof, Wunschliste, Zuzug, die Händlerin, Briefe und ein Museum.' },
    { n: 3, title: 'Die alte Grube', text: 'Grube und Steinbruch am Berghang, Funde und das Rätsel der Grabstelle.' },
    { n: 4, title: 'Die Lok', text: 'Die Lok wird Stück für Stück repariert, bis zur ersten Fahrt in die Nachbarstadt.' },
  ];
  const pages: { icon: IconName; title: string; text: string; chapter: number }[] = [
    { icon: 'group', title: 'Bewohner', text: 'Wer da lebt, was sie brauchen und können', chapter: 1 },
    { icon: 'psychology', title: 'Gedanken', text: 'Was gerade jemand denkt und warum', chapter: 1 },
    { icon: 'inventory_2', title: 'Lager', text: 'Vorräte, Füllstand und wie lange sie reichen', chapter: 1 },
    { icon: 'menu_book', title: 'Tagebuch', text: 'Was im Dorf passiert ist, auch während du weg warst', chapter: 1 },
    { icon: 'checklist', title: 'Wunschliste', text: 'Was das Dorf als Nächstes bauen möchte', chapter: 2 },
    { icon: 'mail', title: 'Briefe', text: 'Post aus Nachbardörfern und von Bewohnern', chapter: 2 },
    { icon: 'museum', title: 'Museum', text: 'Alle gefundenen Stücke mit ihrer Geschichte', chapter: 2 },
  ];

  // Fortschritt der Engine-Phase (0.5.0 bis 0.5.12), solange sie läuft
  const engine = $derived.by(() => {
    const m = /^0\.5\.(\d+)/.exec(device.data?.v ?? '');
    return m ? Math.min(+m[1], 12) : -1;
  });
</script>

<div class="world">
  <Card variant="primary" shape="extra-large">
    <div class="intro">
      <Icon name="landscape" size={40} />
      <div>
        <h2>Pixelia, um 1850</h2>
        <p>
          Ein Winter, der nicht enden wollte, hielt die Reisenden eines Zuges im kleinen Bahnhof Pixelia fest. Als der
          Frühling kam, waren die Gleise zugewachsen. Sie blieben, und aus Gestrandeten wird nach und nach ein Dorf.
        </p>
      </div>
    </div>
    {#if engine >= 0}
      <p class="small">Gerade entsteht die Bühne dafür: Baustein {engine} von 12 der Engine.</p>
      <WavyProgress value={engine / 12} label="Fortschritt der Engine" />
    {/if}
  </Card>

  <CameraCard />

  <h3>Die Kapitel</h3>
  <ol class="chapters">
    {#each chapters as c (c.n)}
      <li>
        <span class="num">{c.n}</span>
        <span><b>{c.title}</b><span class="muted">{c.text}</span></span>
      </li>
    {/each}
  </ol>

  <h3>Bald hier</h3>
  <div class="soon">
    {#each pages as p (p.title)}
      <div class="tile">
        <Icon name={p.icon} size={28} />
        <b>{p.title}</b>
        <span class="muted">{p.text}</span>
        <span class="chip">Kapitel {p.chapter}</span>
      </div>
    {/each}
  </div>
</div>

<style>
  .world {
    display: grid;
    gap: 8px;
  }
  .intro {
    display: flex;
    gap: 16px;
    align-items: flex-start;
  }
  h2 {
    margin: 0 0 6px;
    font: var(--md-sys-typescale-headline-small);
    font-weight: 700;
    font-stretch: 110%;
  }
  .intro p {
    margin: 0;
    font: var(--md-sys-typescale-body-large);
  }
  .small {
    margin: 16px 0 8px;
    font: var(--md-sys-typescale-body-medium);
  }
  h3 {
    margin: 16px 0 4px 4px;
    font: var(--md-sys-typescale-label-large);
    font-weight: 700;
    color: var(--md-sys-color-primary);
  }
  .chapters {
    display: grid;
    gap: 3px;
    margin: 0;
    padding: 0;
    list-style: none;
  }
  .chapters li {
    display: flex;
    align-items: center;
    gap: 16px;
    padding: 12px 16px;
    border-radius: var(--md-sys-shape-corner-extra-small);
    background: var(--md-sys-color-surface-container);
  }
  .chapters li:first-child {
    border-radius: var(--md-sys-shape-corner-extra-large) var(--md-sys-shape-corner-extra-large) var(--md-sys-shape-corner-extra-small)
      var(--md-sys-shape-corner-extra-small);
  }
  .chapters li:last-child {
    border-radius: var(--md-sys-shape-corner-extra-small) var(--md-sys-shape-corner-extra-small) var(--md-sys-shape-corner-extra-large)
      var(--md-sys-shape-corner-extra-large);
  }
  .chapters li > span:last-child {
    display: grid;
  }
  .num {
    display: grid;
    place-items: center;
    flex: none;
    width: 40px;
    height: 40px;
    border-radius: var(--md-sys-shape-corner-medium);
    background: var(--md-sys-color-tertiary-container);
    color: var(--md-sys-color-on-tertiary-container);
    font: var(--md-sys-typescale-title-large);
    font-weight: 750;
  }
  .soon {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
    gap: 8px;
  }
  .tile {
    display: grid;
    align-content: start;
    gap: 4px;
    padding: 16px;
    border-radius: var(--md-sys-shape-corner-large);
    border: 1px dashed var(--md-sys-color-outline-variant);
    color: var(--md-sys-color-on-surface-variant);
  }
  .tile b {
    margin-top: 4px;
    color: var(--md-sys-color-on-surface);
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
  .chip {
    justify-self: start;
    margin-top: 6px;
    padding: 2px 10px;
    border-radius: var(--md-sys-shape-corner-full);
    background: var(--md-sys-color-surface-container-highest);
    font: var(--md-sys-typescale-label-medium);
  }
</style>
