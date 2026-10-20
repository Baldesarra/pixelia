<script lang="ts">
  // Seite „System“: Update hochladen (mit Änderungsvorschau aus der gewählten .bin), Anzeige (Drehung, Helligkeit,
  // langer Druck), Schutz (PIN), Sicherung (Export/Import), Diagnose (Kurzfassung, Details auf #/diagnose),
  // Neustart und Werksreset.
  import {
    Button,
    ButtonGroup,
    Card,
    Chip,
    confirmDialog,
    Icon,
    LoadingIndicator,
    promptDialog,
    Slider,
    WavyProgress,
  } from '../lib/m3';
  import { device } from '../lib/status.svelte';
  import { auth } from '../lib/auth.svelte';
  import { post, type Backup, type Reply } from '../lib/api';
  import { HEADER_BYTES, parseHeader, upload } from '../lib/firmware';
  import { cmpVersion, installedEntry, logFromBin, parseFw, type ChangeEntry as Entry } from '../lib/changelog';
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

  // --- Neustart --------------------------------------------------------------------------------------------
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

  // --- Schutz (PIN) -----------------------------------------------------------------------------------------
  const pinOn = $derived(!!s?.auth?.pin);
  let loggedIn = $state<boolean | null>(null);
  let pinMsg = $state<{ ok: boolean; text: string } | null>(null);
  async function refreshAuth() {
    loggedIn = (await auth.state().catch(() => null))?.ok ?? null;
  }
  // Nach PIN-Wechsel, Neustart oder Anmeldung in einem Dialog neu fragen
  $effect(() => {
    void pinOn;
    void auth.token;
    void refreshAuth();
  });

  async function setPin() {
    pinMsg = null;
    let first = '';
    const pin = await promptDialog({
      title: pinOn ? 'Neuer PIN' : 'PIN festlegen',
      text: 'Schützt Einstellungen, Updates, Neustart, Werksreset, Befehle und Gerätenamen. Ansehen bleibt ohne PIN möglich.',
      icon: 'password',
      ok: 'Weiter',
      field: {
        label: 'PIN oder Passwort',
        type: 'password',
        autocomplete: 'new-password',
        supporting: '4 bis 32 Zeichen, Ziffern oder Wörter',
        check: async (v) => (v.length < 4 || v.length > 32 ? '4 bis 32 Zeichen' : ((first = v), null)),
      },
    });
    if (pin === null) return;
    const again = await promptDialog({
      title: 'PIN wiederholen',
      icon: 'password',
      ok: 'Speichern',
      field: {
        label: 'PIN noch einmal',
        type: 'password',
        autocomplete: 'new-password',
        check: async (v) => (v === first ? null : 'Stimmt nicht mit dem ersten überein'),
      },
    });
    if (again === null) return;
    const r = await post<Reply & { token?: string }>('/api/pin', { new: pin }).catch(() => null);
    if (r?.ok && r.token) auth.save(r.token);
    pinMsg = { ok: !!r?.ok, text: r?.msg ?? 'Keine Antwort vom Gerät' };
  }

  async function pinOff() {
    const ok = await confirmDialog({
      title: 'PIN ausschalten?',
      text: 'Danach kann jeder im Heimnetz Einstellungen ändern, Updates installieren und Pixelia zurücksetzen.',
      ok: 'Ausschalten',
      icon: 'lock_open',
      danger: true,
    });
    if (!ok) return;
    const r = await post('/api/pin', { new: '' }).catch(() => null);
    if (r?.ok) auth.save('');
    pinMsg = { ok: !!r?.ok, text: r?.msg ?? 'Keine Antwort vom Gerät' };
  }

  async function logout() {
    await auth.logout();
    pinMsg = { ok: true, text: 'Abgemeldet. Dieser Browser fragt beim nächsten Ändern wieder nach dem PIN.' };
  }

  // --- Sicherung (Export/Import) ----------------------------------------------------------------------------
  let importInput = $state<HTMLInputElement>();
  let backupMsg = $state<{ ok: boolean; text: string } | null>(null);
  let importing = $state(false);

  function describe(b: Backup): string {
    const parts = [
      `Bilddrehung ${b.set.rot}°`,
      `Helligkeit ${b.set.bright} %`,
      `langer Druck ${num(b.set.lp / 1000, 1)} s`,
    ];
    if (b.set.ui) parts.push('Farbwahl der Oberfläche');
    if (b.names.length) parts.push(`${b.names.length} Gerätename${b.names.length === 1 ? '' : 'n'}`);
    return parts.join(', ');
  }

  async function importFile() {
    const f = importInput?.files?.[0];
    if (importInput) importInput.value = '';
    if (!f) return;
    backupMsg = null;
    let b: Backup;
    try {
      b = JSON.parse(await f.text()) as Backup;
      if (b.pixelia !== 'settings' || typeof b.set !== 'object') throw new Error();
      b.names = Array.isArray(b.names) ? b.names.filter((n) => typeof n?.id === 'string' && typeof n?.name === 'string') : [];
      b.wifi = Array.isArray(b.wifi) ? b.wifi : [];
    } catch {
      backupMsg = { ok: false, text: 'Das ist keine Pixelia-Sicherung (.json aus „Sicherung herunterladen“).' };
      return;
    }
    const extra = [
      b.wifi.length ? `WLAN-Netze (${b.wifi.join(', ')}) stehen nur zur Erinnerung drin, ohne Passwörter: bei Bedarf auf der Seite WLAN neu eintragen.` : '',
      b.pin ? 'Ein PIN war gesetzt; er steckt nie in der Sicherung und wird hier nicht geändert.' : '',
    ].filter(Boolean);
    const ok = await confirmDialog({
      title: 'Sicherung übernehmen?',
      text: `Von ${b.host || 'Pixelia'} ${b.v ? `(${b.v}${b.date ? `, ${b.date}` : ''})` : ''}: ${describe(b)}. ${extra.join(' ')}`,
      ok: 'Übernehmen',
      icon: 'settings_backup_restore',
    });
    if (!ok) return;
    importing = true;
    const data: Record<string, string | number> = { rot: b.set.rot === 180 ? 180 : 0, bright: b.set.bright, lp: b.set.lp };
    if (typeof b.set.ui === 'string' && b.set.ui) data.ui = b.set.ui;
    const r = await post('/api/settings', data).catch(() => null);
    let names = 0;
    if (r?.ok) {
      for (const n of b.names) {
        const nr = await post('/api/clients/name', { id: n.id, name: n.name, create: 1 }).catch(() => null);
        if (nr?.ok) names++;
      }
    }
    importing = false;
    backupMsg = r?.ok
      ? { ok: true, text: `Übernommen: ${describe(b)}${b.names.length ? ` (${names} von ${b.names.length} Namen)` : ''}.` }
      : { ok: false, text: r?.msg ?? 'Keine Antwort vom Gerät' };
  }

  // --- Werksreset -------------------------------------------------------------------------------------------
  let factoryDone = $state(false);
  async function factoryReset() {
    const ok = await confirmDialog({
      title: 'Auf Werkseinstellungen zurücksetzen?',
      text: 'Löscht WLAN-Netze, Einstellungen, Farbwahl, Gerätenamen und den PIN. Pixelia startet danach im Setup-Modus und muss neu eingerichtet werden. Vorher am besten die Sicherung herunterladen.',
      ok: 'Zurücksetzen',
      icon: 'delete_forever',
      danger: true,
    });
    if (!ok) return;
    const r = await post('/api/factory-reset', { confirm: 1 }).catch(() => null);
    if (r?.ok) {
      factoryDone = true;
      auth.save('');
    }
  }
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
      <h2 class="card-title">
        <Icon name="shield_lock" size={20} />Schutz
        <span class="end">
          <Chip tone={pinOn ? 'primary' : 'neutral'} icon={pinOn ? 'lock' : 'lock_open'}>
            {pinOn ? (loggedIn ? 'PIN an · angemeldet' : 'PIN an') : 'kein PIN'}
          </Chip>
        </span>
      </h2>
      <p class="muted">
        {#if pinOn}
          Einstellungen, Updates, Neustart, Werksreset, Befehle und Gerätenamen brauchen den PIN. Nach 5 Fehlversuchen
          sperrt Pixelia 30 Sekunden, danach jedes Mal doppelt so lange.
        {:else}
          Optional: Ein PIN oder Passwort schützt Einstellungen, Updates, Neustart, Werksreset, Befehle und Gerätenamen.
          Ansehen bleibt für alle im Heimnetz möglich.
        {/if}
      </p>
      {#if s.auth?.lockS}<p class="err-text">Gesperrt nach Fehlversuchen, noch {s.auth.lockS} s.</p>{/if}
      <div class="actions">
        <Button variant={pinOn ? 'tonal' : 'filled'} icon="password" onclick={setPin}>{pinOn ? 'PIN ändern' : 'PIN festlegen'}</Button>
        {#if pinOn}
          <Button variant="text" class="danger" icon="lock_open" onclick={pinOff}>Ausschalten</Button>
          {#if loggedIn && auth.token}<Button variant="text" icon="logout" onclick={logout}>Abmelden</Button>{/if}
        {/if}
      </div>
      {#if pinMsg}<p class={pinMsg.ok ? 'ok-text' : 'err-text'}>{pinMsg.text}</p>{/if}
      <p class="muted small">
        PIN vergessen? Am Gerät lange aufs Display drücken, auf der Info-Karte „Zurücksetzen …“ tippen und „PIN
        entfernen“ 3 Sekunden halten. Über das Netz geht das bewusst nicht.
      </p>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="settings_backup_restore" size={20} />Sicherung</h2>
      <p class="muted">
        Anzeige, Farbwahl und Gerätenamen als Datei sichern und wieder einspielen, z. B. nach einem Werksreset. Die Datei
        enthält nie WLAN-Passwörter oder den PIN.
      </p>
      <input class="hidden" type="file" accept=".json,application/json" bind:this={importInput} onchange={importFile} />
      <div class="actions">
        <Button icon="download" href="/api/backup" download>Sicherung herunterladen</Button>
        <Button variant="tonal" icon="upload" disabled={importing} onclick={() => importInput?.click()}>Einspielen</Button>
      </div>
      {#if backupMsg}<p class={backupMsg.ok ? 'ok-text' : 'err-text'}>{backupMsg.text}</p>{/if}
    </Card>

    <Card>
      <h2 class="card-title">
        <Icon name="monitor_heart" size={20} />Diagnose
        {#if s.diag.crash}<span class="end"><Chip tone="error" icon="bug_report">Absturzbericht</Chip></span>{/if}
      </h2>
      <dl class="kv">
        <dt>Start</dt>
        <dd class="tnum">Nr. {s.diag.boot} · {s.diag.reset}</dd>
        <dt>Seit dem Start</dt>
        <dd class="tnum">{s.diag.errors} Fehler · {s.diag.warnings} Warnungen</dd>
        <dt>Absturzbericht</dt>
        <dd class={s.diag.crash ? 'err-text' : ''}>
          {#if s.diag.crash}vorhanden: Task „{s.diag.crash.task}“{s.diag.crash.v ? ` · ${s.diag.crash.v}` : ''}{:else}keiner{/if}
        </dd>
      </dl>
      <div class="actions">
        <Button variant="tonal" icon="monitor_heart" href="#/diagnose">Abstürze und Neustarts</Button>
        <Button variant="text" icon="download" href="/api/diag" download>Diagnosepaket</Button>
      </div>
    </Card>

    <Card>
      <h2 class="card-title"><Icon name="restart_alt" size={20} />Neustart und Werksreset</h2>
      {#if factoryDone}
        <p class="err-text">
          Zurückgesetzt. Pixelia startet im Setup-Modus: Mit dem WLAN „Pixelia-Setup“ verbinden (Passwort steht auf dem
          Display), dann öffnet sich die Einrichtung.
        </p>
      {:else}
        <p class="muted">Läuft seit {duration(s.uptime)}. Beim Neustart bleiben Einstellungen und WLAN-Netze erhalten.</p>
        <div class="actions">
          <Button class="danger" icon="restart_alt" onclick={reboot}>Neu starten</Button>
          <Button variant="text" icon="open_in_new" href="/setup">Einfache Einrichtungsseite</Button>
        </div>
        <p class="muted">
          Der Werksreset löscht WLAN-Netze, Einstellungen, Farbwahl, Gerätenamen und PIN. Startzähler und Neustart-Historie
          bleiben für die Diagnose.
        </p>
        <div class="actions">
          <Button variant="outlined" class="danger" icon="delete_forever" onclick={factoryReset}>Werksreset …</Button>
        </div>
      {/if}
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
