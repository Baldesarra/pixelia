<#
.SYNOPSIS
  Absturzbericht per WLAN holen und dekodieren, ganz ohne USB: Zusammenfassung (/api/coredump/summary), Rohdaten
  (/api/coredump), passendes ELF finden, Backtrace mit Quelltextzeilen, letzte Log-Zeilen vor dem Absturz.

.DESCRIPTION
  - Ziel: -Target (Standard http://pixelia/), sonst die IP. Statt per WLAN geht auch ein Diagnosepaket vom Handy:
    -File pixelia-diag-….json (enthält Zusammenfassung, Rohdaten, Reset-Historie und Log).
  - ELF: -Elf <Datei>, sonst automatisch das ELF, dessen SHA256 zum Bericht passt: firmware\build\pixelia.elf,
    Testvarianten firmware\build-test-*\pixelia.elf, zwischengespeicherte in .logs\elf\, sonst pixelia-vX.Y.Z.elf
    aus dem GitHub-Release der abgestürzten Version (wird nach .logs\elf\ geladen).
  - Dekodiert PC und Backtrace mit addr2line (Funktion, Datei, Zeile). Mit -Full zusätzlich esp-coredump
    (alle Tasks, Register, GDB aus der ESP-IDF-Installation), volle Ausgabe in .logs\crash-last.txt.
  - Zeigt die letzten -LogLines (Standard 30) Zeilen des abgestürzten Starts aus dem Log-Puffer und den Eintrag
    der Reset-Historie dazu.
  - -Erase löscht den Bericht danach auf dem Gerät (POST /api/coredump/erase).
  - Exit 0 = dekodiert, 1 = Fehler, 3 = kein Absturzbericht vorhanden.

.EXAMPLE
  .\tools\crash.ps1
  .\tools\crash.ps1 -Full
  .\tools\crash.ps1 -File $HOME\Downloads\pixelia-diag-20261019-1430-start12.json
  .\tools\crash.ps1 -Target 192.168.178.74 -Erase
#>
param(
    [string]$Target = 'http://pixelia/',
    [string]$File,
    [string]$Elf,
    [switch]$Full,
    [int]$LogLines = 30,
    [switch]$Erase,
    [string]$Pin
)

$ErrorActionPreference = 'Stop'
# Achtung: PowerShell behandelt „ und “ als Anführungszeichen, solche Texte nur in einfachen Anführungszeichen
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$env:PYTHONIOENCODING = 'utf-8'
$repoRoot = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $repoRoot '.logs'
$elfDir = Join-Path $logDir 'elf'
New-Item -ItemType Directory -Force $logDir, $elfDir | Out-Null
$log = Join-Path $logDir 'crash-last.txt'
Set-Content -Path $log -Value "crash.ps1 $(Get-Date -Format s)" -Encoding utf8

function Say([string]$text) {
    $text
    Add-Content -Path $log -Value $text -Encoding utf8
}
function Fail([string]$text, [int]$code = 1) {
    Say "FEHLER: $text"
    Say "--- crash.ps1 | Exit $code | Bericht: $log"
    exit $code
}

$cfg = Get-Content 'C:\Espressif\tools\eim_idf.json' -Raw | ConvertFrom-Json
$idf = $cfg.idfInstalled | Where-Object { $_.id -eq $cfg.idfSelectedId } | Select-Object -First 1
$toolsDir = $idf.idfToolsPath
function Find-Tool([string]$sub, [string]$name) {
    $t = Get-ChildItem -Path (Join-Path $toolsDir $sub) -Recurse -Filter $name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($t) { return $t.FullName }
    return $null
}

# Xtensa-Ausnahmen (EXCCAUSE)
$causes = @{ 0 = 'IllegalInstruction'; 2 = 'InstrFetchError'; 3 = 'LoadStoreError'; 6 = 'IntegerDivideByZero';
    9 = 'LoadStoreAlignment'; 12 = 'InstrPIFDataError'; 13 = 'LoadStorePIFDataError'; 14 = 'InstrPIFAddrError';
    15 = 'LoadStorePIFAddrError'; 20 = 'InstFetchProhibited'; 28 = 'LoadProhibited'; 29 = 'StoreProhibited' }

# --- Quelle: Gerät oder Diagnosepaket --------------------------------------------------------------------------
$summary = $null; $resets = $null; $rawPath = $null; $lines = @()
if ($File) {
    if (-not (Test-Path $File)) { Fail "Datei $File fehlt" 2 }
    $diag = [IO.File]::ReadAllText((Resolve-Path $File), [Text.Encoding]::UTF8) | ConvertFrom-Json
    if (-not $diag.pixeliaDiag) { Fail 'Das ist kein Pixelia-Diagnosepaket' 2 }
    Say ('Quelle: Diagnosepaket {0} (erstellt {1}, Version {2})' -f (Split-Path -Leaf $File), $diag.created, $diag.status.v)
    $summary = $diag.coredump.summary
    $resets = $diag.resets
    if ($diag.coredump.raw) {
        $rawPath = Join-Path $logDir 'coredump-last.bin'
        [IO.File]::WriteAllBytes($rawPath, [Convert]::FromBase64String($diag.coredump.raw))
    }
    $lines = @($diag.log.lines)
} else {
    if ($Target -notmatch '^https?://') { $Target = "http://$Target" }
    $uri = [Uri]$Target
    $ipBase = "$($uri.Scheme)://$($uri.Authority)"
    if ($uri.HostNameType -eq 'Dns') {
        try {
            $addr = [System.Net.Dns]::GetHostAddresses($uri.DnsSafeHost) |
                Where-Object { $_.AddressFamily -eq 'InterNetwork' -and $_.ToString() -ne '0.0.0.0' } | Select-Object -First 1
            if ($addr) { $ipBase = "$($uri.Scheme)://$($addr.ToString())" + $(if ($uri.IsDefaultPort) { '' } else { ":$($uri.Port)" }) }
        } catch { }
    }
    function Get-Api([string]$path, [int]$timeout = 15) {
        return Invoke-RestMethod -Uri "$ipBase$path" -TimeoutSec $timeout -UseBasicParsing
    }
    try { $st = Get-Api '/api/status' 6 } catch { Fail ('Pixelia unter {0} nicht erreichbar: {1}' -f $ipBase, $_.Exception.Message) }
    Say ('Gerät:  {0}, Version {1}, Start Nr. {2} ({3}), läuft seit {4} s' -f $ipBase, $st.v, $st.diag.boot, $st.diag.reset, $st.uptime)
    $summary = Get-Api '/api/coredump/summary'
    $resets = Get-Api '/api/resets'
    if ($summary.present) {
        $rawPath = Join-Path $logDir 'coredump-last.bin'
        Invoke-WebRequest -Uri "$ipBase/api/coredump" -OutFile $rawPath -TimeoutSec 30 -UseBasicParsing
    }
}
if (-not $summary.present) {
    Say 'Kein Absturzbericht vorhanden.'
    Say "--- crash.ps1 | Exit 3 | Bericht: $log"
    exit 3
}
$rawSize = if ($rawPath) { (Get-Item $rawPath).Length } else { 0 }
$cause = [int]$summary.excCause
$reason = ([string]$summary.reason -replace '\s*\r?\n\s*', ' ').Trim()
# Bei Watchdog oder abort() gibt es keine CPU-Ausnahme, das Feld enthält dann keinen gültigen Wert.
# Bei CPU-Ausnahmen steht umgekehrt kein Panic-Text im Bericht, dann gilt der Name der Ausnahme.
if ($cause -lt 64) {
    $name = if ($causes.ContainsKey($cause)) { $causes[$cause] } else { 'unbekannt' }
    $exc = 'Ausnahme {0} ({1}), Adresse {2}' -f $cause, $name, $summary.excVaddr
    if (-not $reason) { $reason = $name }
} else {
    $exc = 'keine CPU-Ausnahme (Watchdog oder abort)'
}
Say ''
Say ('Absturz in Task „{0}“ bei {1}: {2}' -f $summary.task, $summary.pc, $reason)
Say ('  {0}; Bericht {1} Byte, Prüfsumme {2}' -f $exc, $summary.size, $(if ($summary.valid) { 'ok' } else { 'FALSCH' }))
$ver = if ($summary.version) { '{0} ({1})' -f $summary.version, $summary.part } else { 'nicht mehr auf dem Gerät' }
Say ('  Firmware: ELF-SHA256 {0}…, Version {1}' -f $summary.elfSha, $ver)

# --- Passendes ELF -------------------------------------------------------------------------------------------
$want = ([string]$summary.elfSha).ToLower()
function Test-Elf([string]$path) {
    if (-not $want -or -not (Test-Path $path)) { return $false }
    $h = (Get-FileHash -Algorithm SHA256 $path).Hash.ToLower()
    return $h.StartsWith($want)
}
$elfPath = $null
if ($Elf) {
    $elfPath = (Resolve-Path $Elf).Path
    if (-not (Test-Elf $elfPath)) { Say 'WARNUNG: -Elf passt laut SHA256 nicht zum Bericht, Zeilenangaben können falsch sein' }
} else {
    $cands = @(Get-ChildItem -Path (Join-Path $repoRoot 'firmware') -Directory -Filter 'build*' -ErrorAction SilentlyContinue |
            ForEach-Object { Join-Path $_.FullName 'pixelia.elf' } | Where-Object { Test-Path $_ })
    $cands += @(Get-ChildItem -Path $elfDir -Filter '*.elf' -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
    foreach ($c in $cands) { if (Test-Elf $c) { $elfPath = $c; break } }
    if (-not $elfPath -and $summary.version -match '^\d+\.\d+\.\d+(-beta\.\d+)?$') {
        $v = $summary.version
        $dest = Join-Path $elfDir "pixelia-v$v.elf"
        $url = "https://github.com/Baldesarra/pixelia/releases/download/v$v/pixelia-v$v.elf"
        Say "ELF aus dem GitHub-Release laden: $url"
        try { Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing -TimeoutSec 120 } catch { Say "  Laden fehlgeschlagen: $($_.Exception.Message)" }
        if (Test-Elf $dest) { $elfPath = $dest } elseif (Test-Path $dest) { Say '  geladenes ELF passt nicht (SHA256)'; Remove-Item $dest }
    }
}
if (-not $elfPath) {
    Say 'Kein passendes ELF gefunden (lokaler Build mit gleichem Stand oder Release-Version nötig). Rohe Adressen:'
    Say ('  PC {0}, Backtrace {1}' -f $summary.pc, (($summary.backtrace) -join ' '))
    Say "--- crash.ps1 | Exit 1 | Bericht: $log"
    exit 1
}
Say "  ELF: $elfPath"

# --- Backtrace mit Quelltextzeilen ---------------------------------------------------------------------------
$a2l = Find-Tool 'xtensa-esp-elf' 'xtensa-esp32s3-elf-addr2line.exe'
if (-not $a2l) { Fail 'addr2line nicht gefunden (ESP-IDF-Werkzeuge)' }
$addrs = @($summary.pc) + @($summary.backtrace | Where-Object { $_ -ne $summary.pc })
Say ''
Say ('Backtrace{0}:' -f $(if ($summary.btCorrupted) { ' (laut Gerät beschädigt)' } else { '' }))
$decoded = & $a2l -pfiaC -e $elfPath @addrs 2>&1
# Pfade kürzen: alles ab firmware/, engine/, sim/ bzw. esp-idf/
function Short-Path([string]$s) {
    $s = $s -replace '(?i)[A-Z]:[/\\].*?[/\\](firmware|engine|sim)[/\\]', '$1/'
    return ($s -replace '(?i)[A-Z]:[/\\].*?[/\\]esp-idf[/\\]', 'esp-idf/')
}
foreach ($d in $decoded) { Say ('  ' + (Short-Path ([string]$d))) }

# --- Reset-Historie und Log vor dem Absturz ------------------------------------------------------------------
$entry = $resets.entries | Where-Object { $_.crash } | Select-Object -First 1
if ($entry) {
    $crashedBoot = [int]$entry.boot - 1
    $up = if ($null -ne $entry.prevUptime) { '{0} s' -f $entry.prevUptime } else { 'unbekannt' }
    Say ''
    Say ('Reset-Historie: Start Nr. {0} um {1} nach „{2}“, davor lief Start Nr. {3} {4} (letztes Lebenszeichen {5}){6}' -f $entry.boot,
        $entry.time, $entry.reasonText, $crashedBoot, $up, $entry.prevTime, $(if ($entry.note) { "; $($entry.note)" } else { '' }))
} else {
    $crashedBoot = [int]$resets.boot - 1
    Say ''
    Say "Reset-Historie: kein Eintrag mit Absturz (Bericht älter als 16 Starts?), nehme Start Nr. $crashedBoot"
}
if (-not $File) {
    $since = 0; $first = $true
    do {
        $q = "/api/log?max=65536&boot=$crashedBoot"
        if (-not $first) { $q += "&since=$since" }
        $r = Get-Api $q 20
        $lines += @($r.lines)
        $since = $r.last; $first = $false
    } while ($r.more)
} else {
    $lines = @($lines | Where-Object { $_.b -eq $crashedBoot })
}
Say ''
if ($lines.Count -eq 0) {
    Say "Log von Start Nr. ${crashedBoot}: keine Zeilen im Puffer (Stromausfall dazwischen oder Puffer neu angelegt)"
} else {
    $show = [Math]::Min($LogLines, $lines.Count)
    Say ('Letzte {0} von {1} Log-Zeilen aus Start Nr. {2}:' -f $show, $lines.Count, $crashedBoot)
    foreach ($l in ($lines | Select-Object -Last $show)) {
        $t = if ($l.t) { [DateTimeOffset]::FromUnixTimeSeconds([int64]$l.t).ToLocalTime().ToString('HH:mm:ss') } else { '--:--:--' }
        Say ('  [{0}] {1}' -f $t, $l.m)
    }
}

# --- Volle Auswertung mit esp-coredump -----------------------------------------------------------------------
if ($Full) {
    $gdb = Find-Tool 'xtensa-esp-elf-gdb' 'xtensa-esp32s3-elf-gdb.exe'
    if (-not $rawPath -or -not $gdb) { Fail 'Für -Full fehlen Rohdaten oder GDB' }
    Say ''
    Say '--- esp-coredump info_corefile (volle Ausgabe im Bericht)'
    $env:COLUMNS = '160'
    $out = & $idf.python -m esp_coredump --chip esp32s3 info_corefile --gdb $gdb --core $rawPath --core-format raw $elfPath 2>&1
    $out | ForEach-Object { Add-Content -Path $log -Value ([string]$_) -Encoding utf8 }
    # Auf dem Bildschirm: abgestürzte Task, Panic-Grund, Aufrufkette (GDB-Frames) und die Task-Liste
    $shown = 0
    foreach ($o in $out) {
        $s = Short-Path ([string]$o)
        if ($s -match 'Crashed task|Panic reason|^\s+- |^#\d+ |THREADS INFO|^\s*\*?\s*\d+\s+(process|Thread) ') {
            if ($shown -lt 60) { "  $s"; $shown++ }
        }
    }
}

# --- Löschen -------------------------------------------------------------------------------------------------
if ($Erase -and -not $File) {
    $headers = @{}
    if ($Pin) { $headers['X-Pixelia-Pin'] = $Pin }
    $r = Invoke-RestMethod -Method Post -Uri "$ipBase/api/coredump/erase" -Headers $headers -TimeoutSec 15 -UseBasicParsing
    Say ''
    Say "Gerät: $($r.msg)"
}
Say "--- crash.ps1 | Exit 0 | Bericht: $log"
exit 0
