<script lang="ts">
  // Seite „Update“ (System → Wartung): von GitHub (Kanal, Suche, Abbrechen, Hintergrundprüfung) oder aus Datei,
  // je mit Kurzfassung der Änderungen; Fortschritt, Neustart und Probelauf.
  import { Button, ButtonGroup, Card, Chip, confirmDialog, Icon, LoadingIndicator, Switch, WavyProgress } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { auth } from '../lib/auth.svelte';
  import { onMount } from 'svelte';
  import { get, post, type GhState } from '../lib/api';
  import { HEADER_BYTES, parseHeader, upload } from '../lib/firmware';
  import { cmpVersion, dateDe, entryFromNotes, installedEntry, logFromBin, parseFw, type ChangeEntry as Entry } from '../lib/changelog';
  import ChangeEntry from '../lib/ChangeEntry.svelte';
  import { duration, num } from '../lib/format';

  const s = $derived(device.data);

  // --- Update -----------------------------------------------------------------------------------------------
  let fileInput = $state<HTMLInputElement>();
  let file = $state<File | null>(null);
  let fileInfo = $state<{ ok: boolean; text: string } | null>(null);
  let sending = $state(false);
  let sent = $state(0);
  let total = $state(0);
  let upMsg = $state<{ ok: boolean | null; text: string } | null>(null);
  let waitingFor = $state('');
  let cancel = $state<(() => void) | null>(null);
  let via = $state<'file' | 'github'>('file'); // woher das laufende Update kommt (Fortschritt im passenden Abschnitt)

  async function pickFile() {
    const f = fileInput?.files?.[0] ?? null;
    file = null;
    upMsg = null;
    preview = null;
    if (!f) return (fileInfo = null);
    const h = parseHeader(await f.slice(0, HEADER_BYTES).arrayBuffer());
    if ('err' in h) return (fileInfo = { ok: false, text: h.err });
    file = f;
    fileInfo = { ok: true, text: `Pixelia ${h.v} · gebaut ${h.date} ${h.time} · ${num(f.size / 1024)} KB` };
    preview = await makePreview(f, h.v);
  }

  // Änderungsvorschau: Verlauf aus der Datei lesen (Block mit Marker, lib/changelog.ts) und mit der installierten
  // Version vergleichen. Neuer → die Einträge dazwischen; älter → welche Versionen danach fehlen
  let preview = $state<{ tone: 'ok' | 'warn' | 'muted'; text: string; entries: Entry[] } | null>(null);
  async function makePreview(f: File, fileV: string): Promise<typeof preview> {
    const r = await logFromBin(f).catch(() => ({ ok: false, why: 'broken' }) as const);
    if (!r.ok) {
      const why = {
        none: 'Diese Datei enthält noch keinen Versionsverlauf (Versionen vor 0.4.0).',
        browser: 'Dieser Browser kann den Versionsverlauf der Datei nicht lesen.',
        broken: 'Der Versionsverlauf in der Datei ist beschädigt.',
      };
      return { tone: 'muted', text: why[r.why], entries: [] };
    }
    const from = s?.v ?? '';
    const inst = installedEntry(r.log, from) || parseFw(from).base;
    const target = installedEntry(r.log, fileV) || parseFw(fileV).base;
    const c = cmpVersion(target, inst);
    if (c === 0) return { tone: 'muted', text: `Gleiche Version wie installiert (${from}).`, entries: [] };
    if (c > 0) {
      const entries = r.log.filter((e) => cmpVersion(e.v, inst) > 0 && cmpVersion(e.v, target) <= 0);
      const n = entries.length;
      return { tone: 'ok', text: `Neu gegenüber ${from}: ${n} Version${n === 1 ? '' : 'en'}.`, entries };
    }
    const lost = r.log.filter((e) => cmpVersion(e.v, target) > 0 && cmpVersion(e.v, inst) <= 0).map((e) => e.v);
    return {
      tone: 'warn',
      text: `Ältere Version als installiert (${from}). Nach dem Update fehlen die Änderungen aus ${lost.join(', ') || inst}.`,
      entries: [],
    };
  }

  async function start() {
    if (!file) return;
    if (!(await auth.ensure('Updates sind mit einem PIN geschützt.'))) return;
    via = 'file';
    sending = true;
    sent = 0;
    total = file.size;
    upMsg = { ok: null, text: 'Übertrage …' };
    cancel = upload(file, {
      progress: (a, b) => {
        sent = a;
        total = b;
        upMsg = { ok: null, text: `Übertragen: ${Math.round((a * 100) / b)} % (${num(a / 1024)} von ${num(b / 1024)} KB)` };
      },
      done: (ok, msg, v) => {
        cancel = null;
        if (ok) {
          waitingFor = v ?? '';
          upMsg = { ok: true, text: 'Installiert und geprüft. Pixelia startet neu, danach folgt der Probelauf mit Selbsttest (etwa eine Minute) …' };
        } else {
          sending = false;
          upMsg = { ok: false, text: msg };
        }
      },
    });
  }

  // Nach dem Neustart: neue Version im Probelauf, bestanden oder zurückgenommen
  $effect(() => {
    if (!waitingFor || !s) return;
    if (s.v === waitingFor) {
      if (s.ota.trial) upMsg = { ok: null, text: `Neue Version ${s.v} läuft. Selbsttest: ${s.ota.trialS} von ${s.ota.trialNeed} s …` };
      else {
        upMsg = { ok: true, text: `Update auf ${s.v} erfolgreich, Selbsttest bestanden.` };
        waitingFor = '';
        void loadGh(); // nach dem Neustart: Ergebnis der alten Prüfung ist weg
        sending = false;
        file = null;
        fileInfo = null;
        preview = null;
        if (fileInput) fileInput.value = '';
      }
    } else if (s.rollback?.v === waitingFor) {
      upMsg = { ok: false, text: `Update auf ${waitingFor} wurde zurückgenommen: ${s.rollback.why}` };
      waitingFor = '';
      sending = false;
    }
  });
  const offline = $derived(device.link === 'getrennt');

  // --- Update von GitHub (seit 0.4.1) ------------------------------------------------------------------------
  // Das Gerät fragt die Releases ab (nur auf Knopfdruck: ohne Token 60 Abfragen pro Stunde) und lädt die .bin selbst.
  // Fortschritt kommt über den Status (ota.src = github), danach dieselben Schritte wie beim Upload (waitingFor).
  let gh = $state<GhState | null>(null);
  let ghChecking = $state(false);
  let ghMsg = $state<string | null>(null);
  let ghNote = $state(''); // Hinweis ohne Fehler (z. B. Download abgebrochen)
  let chLocal = $state<GhState['ch'] | null>(null);
  const ghCh = $derived(chLocal ?? gh?.ch ?? 'stable');
  const ghFresh = $derived(!!gh && gh.checkedS >= 0 && gh.checkedCh === ghCh);
  const ghTarget = $derived(ghFresh ? (gh?.rel[0] ?? null) : null);
  const ghEntries = $derived(ghFresh && gh ? gh.rel.map((r) => entryFromNotes(r.v, r.notes, r.date)) : []);
  const noNet = $derived(!s ? '' : s.wifi.ap ? 'Im Setup-Modus hat Pixelia kein Internet. Erst das WLAN einrichten, dann nach Updates suchen.' : s.wifi.state !== 'verbunden' ? 'Pixelia ist gerade nicht mit dem WLAN verbunden.' : '');
  const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));
  let ghAt = 0; // Zeitpunkt der letzten Antwort, damit „zuletzt gesucht vor …“ weiterläuft
  let tick = $state(0);
  const ghAgo = $derived(gh && gh.checkedS >= 0 ? (void tick, gh.checkedS + Math.round((Date.now() - ghAt) / 1000)) : -1);
  async function loadGh() {
    const st = await get<GhState>('/api/update').catch(() => null);
    if (st) [gh, ghAt] = [st, Date.now()];
  }
  // Neu laden, wenn die Seite wieder sichtbar wird oder sich der Stand auf dem Gerät geändert hat (Suche von einem
  // anderen Browser, Prüfung im Hintergrund, Kanal): der Status trägt dafür den Zähler ota.ghSeq
  onMount(() => {
    void loadGh();
    const id = setInterval(() => tick++, 15000);
    const vis = () => !document.hidden && void loadGh();
    document.addEventListener('visibilitychange', vis);
    return () => {
      clearInterval(id);
      document.removeEventListener('visibilitychange', vis);
    };
  });
  let seenSeq = -1;
  $effect(() => {
    const q = s?.ota.ghSeq;
    if (q == null || q === seenSeq) return;
    if (seenSeq >= 0 && !ghChecking) void loadGh();
    seenSeq = q;
  });

  async function setAuto(on: boolean) {
    const r = await post('/api/update/channel', { auto: on ? 1 : 0 }).catch(() => null);
    if (!r?.ok) ghMsg = r?.msg ?? 'Pixelia antwortet nicht';
    await loadGh();
  }

  // Laufenden Download von GitHub abbrechen: die neue Partition bleibt ungültig, kein Neustart
  let cancelling = $state(false);
  async function cancelGh() {
    cancelling = true;
    const r = await post('/api/update/cancel').catch(() => null);
    if (!r?.ok) {
      cancelling = false;
      upMsg = { ok: false, text: r?.msg ?? 'Pixelia antwortet nicht' };
    }
  }

  async function setChannel(ch: GhState['ch']) {
    chLocal = ch;
    const r = await post('/api/update/channel', { ch }).catch(() => null);
    if (r?.ok) await loadGh();
    chLocal = null;
  }

  async function checkGh() {
    ghMsg = null;
    ghNote = '';
    const r = await post('/api/update/check').catch(() => null);
    if (!r?.ok) return (ghMsg = r?.msg ?? 'Pixelia antwortet nicht');
    ghChecking = true;
    for (let i = 0; i < 60; i++) {
      await sleep(i < 5 ? 600 : 1000);
      await loadGh();
      if (gh && gh.job !== 'check') break;
    }
    ghChecking = false;
    if (gh?.err) ghMsg = gh.err;
  }

  async function installGh() {
    const t = ghTarget;
    if (!t) return;
    const ok = await confirmDialog({
      title: `Pixelia ${t.v} installieren?`,
      text: `Pixelia lädt die Datei (${num(t.size / 1024)} KB) direkt von GitHub, prüft sie und startet neu. Danach folgt der Probelauf mit Selbsttest; scheitert er, kehrt Pixelia zu ${s?.v} zurück.`,
      ok: 'Installieren',
      icon: 'system_update_alt',
    });
    if (!ok) return;
    ghMsg = null;
    ghNote = '';
    const r = await post('/api/update/install', { v: t.v }).catch(() => null);
    if (!r?.ok) return (ghMsg = r?.msg ?? 'Pixelia antwortet nicht');
    via = 'github';
    sending = true;
    waitingFor = t.v;
    upMsg = { ok: null, text: 'Verbinde mit GitHub …' };
    // Fehler vor dem Neustart (Download, Prüfung) meldet /api/update; startet Pixelia neu, antwortet es kurz nicht
    for (let i = 0; i < 600; i++) {
      await sleep(1000);
      const st = await get<GhState>('/api/update').catch(() => null);
      if (!st || st.job === 'install') continue;
      if (st.cancelled) {
        [gh, ghAt] = [st, Date.now()];
        upMsg = null;
        ghNote = `Download abgebrochen. Es läuft weiter ${s?.v}, nichts wurde geändert.`;
        waitingFor = '';
        sending = false;
        cancelling = false;
        break;
      }
      if (st.err) {
        [gh, ghAt] = [st, Date.now()];
        upMsg = { ok: false, text: st.err };
        waitingFor = '';
        sending = false;
      }
      if (st.err || s?.v === t.v || !waitingFor) break;
    }
  }

  // Download-Fortschritt (Status jede Sekunde)
  $effect(() => {
    if (!s || !waitingFor || !(s.ota.busy && s.ota.src === 'github')) return;
    const pct = s.ota.total ? Math.round((s.ota.bytes * 100) / s.ota.total) : 0;
    upMsg = { ok: null, text: `Lade von GitHub: ${pct} % (${num(s.ota.bytes / 1024)} von ${num(s.ota.total / 1024)} KB)` };
  });

  const ackRollback = () => post('/api/ota/ack').catch(() => null);
</script>

{#snippet progress()}
  {#if sending && !waitingFor}
    <WavyProgress value={total ? sent / total : undefined} label="Übertragung" />
  {:else if waitingFor}
    {#if s?.ota.busy && s.ota.src === 'github'}
      <WavyProgress value={s.ota.total ? s.ota.bytes / s.ota.total : undefined} label="Download von GitHub" />
      <div class="actions"><Button variant="text" icon="close" disabled={cancelling} onclick={cancelGh}>Abbrechen</Button></div>
    {:else}
      <WavyProgress value={s?.v === waitingFor && s.ota.trial ? s.ota.trialS / s.ota.trialNeed : undefined} tone="tertiary" label="Neustart und Probelauf" />
    {/if}
  {/if}
  {#if upMsg}
    <p class={upMsg.ok === false ? 'err-text' : upMsg.ok ? 'ok-text' : ''}>
      {offline && waitingFor ? 'Pixelia startet neu …' : upMsg.text}
    </p>
  {/if}
{/snippet}

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
    <!-- Versionskarte (seit 0.5.0): die installierte Version auf einen Blick -->
    <Card class="wide">
      <div class="brand">
        <span class="logo" aria-hidden="true"></span>
        <div>
          <div class="brand-name">Pixelia</div>
          <div class="brand-ver tnum">Version {s.v}</div>
        </div>
      </div>
    </Card>

    {#if s.rollback}
      <Card variant="error" class="wide">
        <div class="alert">
          <Icon name="error" />
          <div class="grow">
            <strong>Update auf {s.rollback.v} zurückgenommen</strong>
            <p>{s.rollback.why}. Es läuft wieder {s.v}.</p>
          </div>
          <Button variant="tonal" onclick={ackRollback}>OK</Button>
        </div>
      </Card>
    {/if}

    <Card class="wide">
      <h2 class="card-title">
        <Icon name="system_update_alt" size={20} />Update
        <span class="end"><Chip><span class="tnum">installiert: {s.v}</span></Chip></span>
      </h2>
      {#if s.ota.trial && !waitingFor}
        <p>Probelauf von {s.v}: Selbsttest läuft ({s.ota.trialS} von {s.ota.trialNeed} s{s.ota.trialNote ? `, ${s.ota.trialNote}` : ''}).</p>
        <WavyProgress value={s.ota.trialS / s.ota.trialNeed} tone="tertiary" label="Probelauf" />
      {:else if s.ota.busy && !sending}
        <p>Ein Update auf {s.ota.newV} wird gerade {s.ota.src === 'github' ? 'von GitHub geladen' : 'von einem anderen Browser installiert'} …</p>
        <WavyProgress value={s.ota.total ? s.ota.bytes / s.ota.total : undefined} label="Update" />
        {#if s.ota.src === 'github'}
          <div class="actions"><Button variant="text" icon="close" disabled={cancelling} onclick={cancelGh}>Abbrechen</Button></div>
        {/if}
      {:else}
        <h3 class="sub"><Icon name="cloud_download" size={18} />Von GitHub</h3>
        <div class="actions">
          <ButtonGroup
            label="Kanal"
            options={[
              { value: 'stable', label: 'Stabil' },
              { value: 'preview', label: 'Vorschau', icon: 'science' },
            ]}
            bind:value={() => ghCh, setChannel}
          />
          <Button variant="tonal" icon="refresh" disabled={!!noNet || ghChecking || sending} onclick={checkGh}>
            {ghChecking ? 'Suche …' : 'Nach Updates suchen'}
          </Button>
        </div>
        {#if via === 'github' && (sending || waitingFor)}
          {@render progress()}
        {:else if noNet}
          <p class="muted">{noNet}</p>
        {:else if ghChecking}
          <WavyProgress label="Suche auf GitHub" />
        {:else if ghMsg}
          <p class="err-text">{ghMsg}</p>
        {:else if ghTarget}
          <p class="ok-text">
            Neu auf GitHub: <b>Pixelia {ghTarget.v}</b>{ghTarget.pre ? ' (Vorschau)' : ''} vom {dateDe(ghTarget.date)} · {num(ghTarget.size / 1024)} KB{ghEntries.length > 1 ? ` · ${ghEntries.length} Versionen neuer als ${s.v}` : ''}
          </p>
          <div class="actions">
            <Button icon="system_update_alt" disabled={sending} onclick={installGh}>{ghTarget.v} installieren</Button>
          </div>
          <div class="preview">
            {#each ghEntries as e, k (e.v)}
              <ChangeEntry entry={e} toggle open={k === 0} />
            {/each}
          </div>
        {:else if ghFresh}
          <p>
            Pixelia ist aktuell. {!gh?.latest ? 'In diesem Kanal gibt es auf GitHub noch keine Version.' : cmpVersion(parseFw(s.v).base, gh.latest) > 0 ? `Installiert ist ${s.v}, neuer als die neueste ${ghCh === 'stable' ? 'stabile ' : ''}Version auf GitHub (${gh.latest}).` : `Neueste ${ghCh === 'stable' ? 'stabile ' : ''}Version auf GitHub: ${gh.latest}.`}
          </p>
        {:else}
          <p class="muted">
            <b>Stabil</b>: nur fertige Versionen. <b>Vorschau</b>: auch Vorabversionen (beta) zum Ausprobieren. Installiert
            wird nur auf Knopfdruck.
          </p>
        {/if}
        {#if ghNote && !(via === 'github' && (sending || waitingFor))}<p>{ghNote}</p>{/if}
        {#if gh && ghAgo >= 0 && !ghChecking && !noNet}
          <p class="muted small">
            Zuletzt gesucht vor {duration(ghAgo)}{gh.lastAuto ? ' (im Hintergrund)' : ''}{gh.rate.limit > 0 ? ` · freie GitHub-Abfragen: ${gh.rate.left} von ${gh.rate.limit} pro Stunde` : ''}
          </p>
        {/if}
        {#if gh}
          <div class="auto">
            <Switch label="Täglich im Hintergrund suchen" checked={gh.auto !== false} onchange={setAuto} />
            <span>
              Einmal täglich im Hintergrund nach neuen Versionen suchen
              <span class="muted small">Nur ein Hinweis (Punkt an „System“), installiert wird nie von selbst.</span>
            </span>
          </div>
        {/if}

        {#if !upMsg && s.ota.passed}
          <p class="ok-text">Letztes Update auf {s.v} erfolgreich, Selbsttest bestanden.</p>
        {/if}
      {/if}
    </Card>

    <!-- Aus Datei (seit 0.5.4 eigene Karte unten, Wunsch des Users): selten gebraucht -->
    {#if !(s.ota.trial && !waitingFor) && !(s.ota.busy && !sending)}
      <Card class="wide">
        <h2 class="card-title"><Icon name="upload_file" size={20} />Aus Datei</h2>
        <p class="muted">
          Neue Version als <b>.bin</b> wählen, z. B. <i>pixelia-v0.4.0.bin</i> von GitHub (nicht das Factory-Image). Die Datei
          wird vor dem Hochladen geprüft, nach dem Neustart folgt ein Probelauf mit Selbsttest; scheitert er, kehrt Pixelia
          zur alten Version zurück.
        </p>
        <input class="hidden" type="file" accept=".bin,application/octet-stream" bind:this={fileInput} onchange={pickFile} />
        <div class="actions">
          <Button variant="tonal" icon="upload_file" disabled={sending} onclick={() => fileInput?.click()}>Datei wählen</Button>
          <Button icon="system_update_alt" disabled={!file || sending} onclick={start}>Update installieren</Button>
          {#if cancel}<Button variant="text" onclick={() => cancel?.()}>Abbrechen</Button>{/if}
          {#if !file && !sending}<Button variant="text" icon="history" href="#/versionen">Versionsverlauf</Button>{/if}
        </div>
        {#if fileInfo}<p class={fileInfo.ok ? '' : 'err-text'}>{fileInfo.text}</p>{/if}
        {#if preview && !waitingFor}
          <p class={preview.tone === 'ok' ? 'ok-text' : preview.tone === 'warn' ? 'err-text' : 'muted'}>{preview.text}</p>
          {#if preview.entries.length}
            <div class="preview">
              {#each preview.entries as e, k (e.v)}
                <ChangeEntry entry={e} toggle open={k === 0} />
              {/each}
            </div>
          {/if}
        {/if}
        {#if via === 'file'}
          {@render progress()}
        {/if}
      </Card>
    {/if}
  </div>
{/if}

<style>
  .auto {
    display: flex;
    align-items: center;
    gap: 16px;
    margin-top: 12px;
  }
  .auto > span {
    display: flex;
    flex-direction: column;
  }
  .waiting {
    display: grid;
    place-items: center;
    padding: 48px 0;
  }
  .hidden {
    display: none;
  }
  /* Text unter Knöpfen und Fortschritt braucht Abstand nach oben, direkt unter dem Kartentitel nicht */
  p {
    margin: 12px 0 0;
  }
  .card-title + p {
    margin-top: 0;
  }
  p + :global(.wp),
  .actions + :global(.wp) {
    margin-top: 12px;
  }
  /* Abschnitte der Update-Karte: von GitHub, aus Datei */
  .sub {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 24px 0 12px;
    font: var(--md-sys-typescale-title-small);
    color: var(--md-sys-color-on-surface-variant);
  }
  .card-title + .sub {
    margin-top: 0;
  }
  .small {
    font: var(--md-sys-typescale-body-small);
  }
  .alert {
    display: flex;
    align-items: center;
    gap: 16px;
  }
  .alert p {
    margin: 4px 0 0;
  }
  .alert strong {
    font: var(--md-sys-typescale-title-medium);
    font-weight: 650;
  }
  .grow {
    flex: 1;
  }
  /* Versionskarte: Pixel-Zeichen wie das Symbol im Browser-Tab */
  .brand {
    display: flex;
    align-items: center;
    gap: 16px;
  }
  .logo {
    position: relative;
    flex: none;
    width: 48px;
    height: 48px;
    border-radius: 12px;
    background: var(--md-sys-color-primary);
  }
  .logo::before,
  .logo::after {
    content: '';
    position: absolute;
    width: 12px;
    height: 12px;
    background: var(--md-sys-color-on-primary);
  }
  .logo::before {
    left: 12px;
    top: 12px;
  }
  .logo::after {
    left: 24px;
    top: 24px;
  }
  .brand-name {
    font: var(--md-sys-typescale-headline-small);
    color: var(--md-sys-color-on-surface);
  }
  .brand-ver {
    font: var(--md-sys-typescale-body-large);
    color: var(--md-sys-color-on-surface-variant);
  }
  /* Änderungsvorschau: Einträge der neueren Versionen, scrollt bei vielen */
  .preview {
    display: grid;
    gap: 16px;
    max-height: 440px;
    margin-top: 12px;
    padding: 16px;
    overflow-y: auto;
    border-radius: var(--md-sys-shape-corner-large);
    background: var(--md-sys-color-surface-container-low);
  }
</style>
