<script lang="ts">
  // Seite „System“: Update hochladen, Anzeige (Drehung, Helligkeit, langer Druck), Diagnose (Paket, Absturzbericht),
  // Neustart-Historie, Neustart. Werksreset und PIN folgen mit 0.3.2.
  import { onMount } from 'svelte';
  import {
    Button,
    ButtonGroup,
    Card,
    Chip,
    confirmDialog,
    Icon,
    LoadingIndicator,
    Slider,
    WavyProgress,
  } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { get, post, type CrashSummary, type ResetList } from '../lib/api';
  import { HEADER_BYTES, parseHeader, upload } from '../lib/firmware';
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

  async function pickFile() {
    const f = fileInput?.files?.[0] ?? null;
    file = null;
    upMsg = null;
    if (!f) return (fileInfo = null);
    const h = parseHeader(await f.slice(0, HEADER_BYTES).arrayBuffer());
    if ('err' in h) return (fileInfo = { ok: false, text: h.err });
    file = f;
    fileInfo = { ok: true, text: `Pixelia ${h.v} · gebaut ${h.date} ${h.time} · ${num(f.size / 1024)} KB` };
  }

  function start() {
    if (!file) return;
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
        sending = false;
        file = null;
        fileInfo = null;
        if (fileInput) fileInput.value = '';
      }
    } else if (s.rollback?.v === waitingFor) {
      upMsg = { ok: false, text: `Update auf ${waitingFor} wurde zurückgenommen: ${s.rollback.why}` };
      waitingFor = '';
      sending = false;
    }
  });
  const offline = $derived(device.link === 'getrennt');

  // --- Anzeige ----------------------------------------------------------------------------------------------
  let bright = $state(80);
  let lpS = $state(3);
  let touchedAt = 0;
  let brightTimer = 0;
  $effect(() => {
    if (!s || performance.now() - touchedAt < 3000) return;
    bright = s.set.bright;
    lpS = Math.round(s.set.lp / 100) / 10;
  });
  function setBright(v: number) {
    touchedAt = performance.now();
    clearTimeout(brightTimer);
    brightTimer = window.setTimeout(() => post('/api/settings', { bright: v }).catch(() => null), 150);
  }
  function setLp(v: number) {
    touchedAt = performance.now();
    void post('/api/settings', { lp: Math.round(v * 1000) }).catch(() => null);
  }
  let rotLocal = $state<'0' | '180' | null>(null);
  const rot = $derived(rotLocal ?? (String(s?.set.rot ?? 0) as '0' | '180'));
  function setRot(v: '0' | '180') {
    rotLocal = v;
    void post('/api/settings', { rot: v })
      .catch(() => null)
      .finally(() => setTimeout(() => (rotLocal = null), 2500));
  }

  // --- Diagnose und Neustarts -------------------------------------------------------------------------------
  let crash = $state<CrashSummary | null>(null);
  let resets = $state<ResetList | null>(null);
  async function loadDiag() {
    [crash, resets] = await Promise.all([
      get<CrashSummary>('/api/coredump/summary').catch(() => null),
      get<ResetList>('/api/resets').catch(() => null),
    ]);
  }
  onMount(() => void loadDiag());
  // Neuer Absturzbericht (z. B. nach einem Neustart): neu laden
  let crashSeen = '';
  $effect(() => {
    const k = s?.diag.crash ? `${s.diag.crash.task}/${s.diag.crash.v}` : '-';
    if (k !== crashSeen) {
      crashSeen = k;
      void loadDiag();
    }
  });

  async function eraseCrash() {
    const ok = await confirmDialog({
      title: 'Absturzbericht löschen?',
      text: 'Vorher am besten das Diagnosepaket herunterladen: Es enthält den Bericht zum Auswerten.',
      ok: 'Löschen',
      icon: 'delete',
      danger: true,
    });
    if (!ok) return;
    await post('/api/coredump/erase').catch(() => null);
    void loadDiag();
  }

  async function reboot() {
    const ok = await confirmDialog({
      title: 'Pixelia neu starten?',
      text: 'Die Anzeige ist ein paar Sekunden dunkel, die Oberfläche verbindet sich danach von selbst neu.',
      ok: 'Neu starten',
      icon: 'restart_alt',
      danger: true,
    });
    if (ok) await post('/api/reboot').catch(() => null);
  }

  const ackRollback = () => post('/api/ota/ack').catch(() => null);
</script>

{#if !s}
  <div class="waiting"><LoadingIndicator size={64} label="Lade" /></div>
{:else}
  <div class="page-grid">
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
        <p>Ein Update auf {s.ota.newV} wird gerade von einem anderen Browser installiert …</p>
        <WavyProgress value={s.ota.total ? s.ota.bytes / s.ota.total : undefined} label="Update" />
      {:else}
        <p class="muted">
          Neue Version als <b>.bin</b> wählen, z. B. <i>pixelia-v0.3.1.bin</i> von GitHub (nicht das Factory-Image). Die Datei
          wird vor dem Hochladen geprüft, nach dem Neustart folgt ein Probelauf mit Selbsttest; scheitert er, kehrt Pixelia
          zur alten Version zurück.
        </p>
        <input class="hidden" type="file" accept=".bin,application/octet-stream" bind:this={fileInput} onchange={pickFile} />
        <div class="actions">
          <Button variant="tonal" icon="upload_file" disabled={sending} onclick={() => fileInput?.click()}>Datei wählen</Button>
          <Button icon="system_update_alt" disabled={!file || sending} onclick={start}>Update installieren</Button>
          {#if cancel}<Button variant="text" onclick={() => cancel?.()}>Abbrechen</Button>{/if}
        </div>
        {#if fileInfo}<p class={fileInfo.ok ? '' : 'err-text'}>{fileInfo.text}</p>{/if}
        {#if sending && !waitingFor}
          <WavyProgress value={total ? sent / total : undefined} label="Übertragung" />
        {:else if waitingFor}
          <WavyProgress value={s.v === waitingFor && s.ota.trial ? s.ota.trialS / s.ota.trialNeed : undefined} tone="tertiary" label="Neustart und Probelauf" />
        {/if}
        {#if upMsg}
          <p class={upMsg.ok === false ? 'err-text' : upMsg.ok ? 'ok-text' : ''}>
            {offline && waitingFor ? 'Pixelia startet neu …' : upMsg.text}
          </p>
        {:else if s.ota.passed}
          <p class="ok-text">Letztes Update auf {s.v} erfolgreich, Selbsttest bestanden.</p>
        {/if}
      {/if}
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="light_mode" size={20} />Anzeige</h2>
      <div class="setting">
        <span>Bilddrehung</span>
        <ButtonGroup
          label="Bilddrehung"
          options={[
            { value: '0', label: '0° normal' },
            { value: '180', label: '180°', icon: 'mobile_rotate' },
          ]}
          bind:value={() => rot, setRot}
        />
      </div>
      <div class="setting">
        <span class="row"><span>Helligkeit</span><span class="tnum">{bright} %</span></span>
        <Slider label="Helligkeit" min={1} max={100} unit=" %" bind:value={bright} oninput={setBright} />
      </div>
      <div class="setting">
        <span class="row"><span>Langer Druck (Info-Karte)</span><span class="tnum">{num(lpS, 1)} s</span></span>
        <Slider label="Dauer langer Druck" min={1} max={10} step={0.5} unit=" s" bind:value={lpS} onchange={setLp} />
      </div>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="bug_report" size={20} />Diagnose</h2>
      <dl class="kv">
        <dt>Start</dt>
        <dd class="tnum">Nr. {s.diag.boot} · {s.diag.reset}</dd>
        <dt>Seit dem Start</dt>
        <dd class="tnum">{s.diag.errors} Fehler · {s.diag.warnings} Warnungen</dd>
        <dt>Log-Puffer</dt>
        <dd class="tnum">{num(s.diag.logLines)} Zeilen</dd>
        <dt>Absturzbericht</dt>
        <dd class={crash?.present ? 'err-text' : ''}>
          {#if crash?.present}vorhanden: Task „{crash.task}“{crash.version ? ` · ${crash.version}` : ''}{:else}keiner{/if}
        </dd>
        {#if crash?.present && crash.reason}
          <dt>Grund</dt>
          <dd>{crash.reason}</dd>
        {/if}
      </dl>
      <div class="actions">
        <Button icon="download" href="/api/diag" download>Diagnosepaket</Button>
        {#if crash?.present}<Button variant="text" class="danger" icon="delete" onclick={eraseCrash}>Bericht löschen</Button>{/if}
      </div>
      <p class="muted small">
        Eine Datei mit Status, Versionen, Einstellungen, Neustarts, Log und Absturzbericht, ohne Passwörter. Auf dem Handy
        landet sie unter „Downloads“.
        {#if s.wifi.ap}Im Setup-Fenster des Handys klappt das Herunterladen oft nicht; dann im normalen Browser
          <a href="http://192.168.4.1/">http://192.168.4.1</a> öffnen.{/if}
      </p>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="history" size={20} />Letzte Starts</h2>
      {#if resets?.entries.length}
        <ul class="list">
          {#each resets.entries.slice(0, 8) as r (r.boot)}
            <li class="list-item">
              <span class="lead" class:bad={!!r.crash || r.rollback}>
                <Icon name={r.crash ? 'bug_report' : r.rollback ? 'error' : 'restart_alt'} size={20} />
              </span>
              <span class="text">
                <span class="headline">Nr. {r.boot} · {r.reasonText}</span>
                <span class="supporting tnum">
                  {r.v}{r.time ? ` · ${r.time}` : ''}{r.prevUptime !== null ? ` · lief davor ${duration(r.prevUptime)}` : ''}{r.trial ? ' · Probelauf' : ''}{r.crash ? ` · Absturz in ${r.crash.task}` : ''}
                </span>
              </span>
            </li>
          {/each}
        </ul>
      {:else}
        <p class="muted">{resets ? 'Keine Einträge.' : 'Lade …'}</p>
      {/if}
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="restart_alt" size={20} />Neustart</h2>
      <p class="muted">Läuft seit {duration(s.uptime)}. Einstellungen und WLAN-Netze bleiben erhalten.</p>
      <div class="actions">
        <Button class="danger" icon="restart_alt" onclick={reboot}>Neu starten</Button>
        <Button variant="text" icon="open_in_new" href="/setup">Einfache Einrichtungsseite</Button>
      </div>
    </Card>
  </div>
{/if}

<style>
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
  .setting {
    display: grid;
    gap: 10px;
    margin-bottom: 20px;
    color: var(--md-sys-color-on-surface-variant);
  }
  .row {
    display: flex;
    justify-content: space-between;
  }
  .lead.bad {
    background: var(--md-sys-color-error-container);
    color: var(--md-sys-color-on-error-container);
  }
</style>
