<#
.SYNOPSIS
  Firmware per WLAN aufs Board spielen (POST /api/ota) und prüfen, ob die neue Version läuft. Ohne USB.

.DESCRIPTION
  - Ziel: -Target (Standard http://pixelia/). Die IP funktioniert immer, z. B. -Target 192.168.178.42.
    pixelia.local klappt an Windows mit FritzBox nicht (deren DNS antwortet 0.0.0.0).
  - Datei: -Bin (Standard firmware\build\pixelia.bin). Mit -Build vorher tools\idf.ps1 -C firmware build.
  - Vorab: Kopf der Datei prüfen (ESP-Image, Projekt „pixelia“, Version) und /api/status abfragen.
  - Hochladen mit Fortschritt in 10-%-Schritten (KB/s), Antwort des Geräts auswerten.
  - Danach: Neustart abwarten, /api/status prüfen: neue Version läuft, Probelauf mit Selbsttest bestanden (60 s).
    Erkennt einen Rückfall auf die alte Version und zeigt den Grund. Mit -NoWait nur hochladen.
  - Exit 0 = neue Version läuft und hat den Selbsttest bestanden, 1 = Fehler oder Rückfall, 2 = falscher Aufruf.
  - Mit PIN (seit 0.3.2): -Pin 1234 oder $env:PIXELIA_PIN (Kopf X-Pixelia-Pin).
  - Volles Protokoll: .logs\ota-last.log

.EXAMPLE
  .\tools\ota.ps1
  .\tools\ota.ps1 -Build
  .\tools\ota.ps1 -Target 192.168.178.42 -Bin firmware\build-test-crash\pixelia.bin
#>
param(
    [string]$Target = 'http://pixelia/',
    [string]$Bin,
    [switch]$Build,
    [switch]$NoWait,
    [int]$TimeoutS = 240,
    # Optionaler PIN des Geräts (seit 0.3.2), sonst aus der Umgebungsvariable PIXELIA_PIN
    [string]$Pin = $env:PIXELIA_PIN
)

$ErrorActionPreference = 'Stop'
# Achtung: PowerShell behandelt „ und “ als Anführungszeichen. Texte mit deutschen Anführungszeichen deshalb in
# einfachen Anführungszeichen mit -f schreiben.
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$repoRoot = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $repoRoot '.logs'
New-Item -ItemType Directory -Force $logDir | Out-Null
$log = Join-Path $logDir 'ota-last.log'
Set-Content -Path $log -Value "ota.ps1 $(Get-Date -Format s)" -Encoding utf8

function Say([string]$text) {
    $text
    Add-Content -Path $log -Value $text -Encoding utf8
}
function Fail([string]$text, [int]$code = 1) {
    Say "FEHLER: $text"
    Say "--- ota.ps1 | Exit $code | Log: $log"
    exit $code
}

# --- Ziel ----------------------------------------------------------------------------------------------------
if ($Target -notmatch '^https?://') { $Target = "http://$Target" }
$uri = [Uri]$Target
$base = "$($uri.Scheme)://$($uri.Authority)"
# Name einmal auflösen und danach die IP benutzen: schneller und unabhängig vom DNS-Zwischenspeicher
$ipBase = $base
if ($uri.HostNameType -eq 'Dns') {
    try {
        $addr = [System.Net.Dns]::GetHostAddresses($uri.DnsSafeHost) |
            Where-Object { $_.AddressFamily -eq 'InterNetwork' -and $_.ToString() -ne '0.0.0.0' } | Select-Object -First 1
        if ($addr) {
            $ipBase = "$($uri.Scheme)://$($addr.ToString())" + $(if ($uri.IsDefaultPort) { '' } else { ":$($uri.Port)" })
        }
    } catch { }
}

function Get-Json([string]$url, [int]$timeoutMs = 4000) {
    try {
        $req = [System.Net.HttpWebRequest]::Create($url)
        $req.Timeout = $timeoutMs
        $req.ReadWriteTimeout = $timeoutMs
        $req.Proxy = $null
        $req.KeepAlive = $false
        $resp = $req.GetResponse()
        try {
            $reader = New-Object System.IO.StreamReader($resp.GetResponseStream(), [Text.Encoding]::UTF8)
            return $reader.ReadToEnd() | ConvertFrom-Json
        } finally { $resp.Close() }
    } catch {
        return $null
    }
}

function Read-Error([System.Net.WebException]$ex) {
    if (-not $ex.Response) { return $ex.Message }
    try {
        $reader = New-Object System.IO.StreamReader($ex.Response.GetResponseStream(), [Text.Encoding]::UTF8)
        $body = $reader.ReadToEnd()
        $j = $body | ConvertFrom-Json
        if ($j.msg) { return $j.msg }
        return $body
    } catch {
        return $ex.Message
    }
}

# --- Datei ---------------------------------------------------------------------------------------------------
if ($Build) {
    Say '--- Firmware bauen'
    & (Join-Path $PSScriptRoot 'idf.ps1') -C firmware build
    if ($LASTEXITCODE -ne 0) { Fail 'Build fehlgeschlagen' }
}
if (-not $Bin) { $Bin = Join-Path $repoRoot 'firmware\build\pixelia.bin' }
if (-not (Test-Path $Bin)) { Fail "Datei $Bin fehlt (erst bauen, z. B. mit -Build)" 2 }
$Bin = (Resolve-Path $Bin).Path
$bytes = [System.IO.File]::ReadAllBytes($Bin)
$len = $bytes.Length
# ESP-App-Image: Kennung 0xE9, Beschreibung (esp_app_desc_t) ab Byte 32: Magic 0xABCD5432, Version ab 48, Projekt ab 80
if ($len -lt 256 -or $bytes[0] -ne 0xE9) { Fail 'Keine ESP-Firmware (Kennung 0xE9 fehlt). Factory-Images sind nur für den Web-Flasher.' 2 }
# L-Suffix: ohne liest PowerShell 5.1 den Wert als negative Int32
if ([BitConverter]::ToUInt32($bytes, 32) -ne 0xABCD5432L) { Fail 'Keine App-Beschreibung im Image gefunden' 2 }
function Read-CString([byte[]]$b, [int]$off, [int]$max) {
    $end = $off
    while ($end -lt $off + $max -and $b[$end] -ne 0) { $end++ }
    return [Text.Encoding]::UTF8.GetString($b, $off, $end - $off)
}
$newVersion = Read-CString $bytes 48 32
$project = Read-CString $bytes 80 32
if ($project -ne 'pixelia') { Fail ('Das ist keine Pixelia-Firmware (Projekt „{0}“)' -f $project) 2 }
Say ("Datei:  {0} ({1:N0} KB), Version {2}" -f (Split-Path -Leaf $Bin), ($len / 1KB), $newVersion)

# --- Gerät ---------------------------------------------------------------------------------------------------
$before = Get-Json "$ipBase/api/status" 6000
if (-not $before) {
    $hint = if ($ipBase -eq $base -and $uri.HostNameType -eq 'Dns') { ' (Name „{0}“ nicht auflösbar)' -f $uri.DnsSafeHost } else { '' }
    Fail "Pixelia unter $base nicht erreichbar$hint. Die IP steht auf der Info-Karte (3 s aufs Display drücken)."
}
$shown = if ($ipBase -ne $base) { "$base → $ipBase" } else { $base }
Say ('Gerät:  {0}, Version {1} ({2}), WLAN „{3}“ {4} dBm, läuft seit {5} s' -f $shown, $before.v, $before.part,
    $before.wifi.ssid, $before.wifi.rssi, $before.uptime)
if ($before.ota.busy) { Fail 'Auf dem Gerät läuft bereits ein Update' }
if ($before.auth -and $before.auth.pin -and -not $Pin) {
    Fail 'Das Gerät ist mit einem PIN geschützt: -Pin angeben oder $env:PIXELIA_PIN setzen'
}

# --- Hochladen -----------------------------------------------------------------------------------------------
[System.Net.ServicePointManager]::Expect100Continue = $false
$req = [System.Net.HttpWebRequest]::Create("$ipBase/api/ota")
$req.Method = 'POST'
$req.ContentType = 'application/octet-stream'
$req.ContentLength = $len
$req.AllowWriteStreamBuffering = $false
$req.SendChunked = $false
$req.KeepAlive = $false
$req.Proxy = $null
if ($Pin) { $req.Headers.Add('X-Pixelia-Pin', $Pin) }
# Das Gerät löscht vor dem Schreiben den Zielbereich (~3 s), in der Zeit stockt die Übertragung
$req.Timeout = 90000
$req.ReadWriteTimeout = 90000

$t0 = Get-Date
$chunk = 16KB
$nextPct = 10
try {
    $stream = $req.GetRequestStream()
    for ($off = 0; $off -lt $len; $off += $chunk) {
        $n = [Math]::Min($chunk, $len - $off)
        $stream.Write($bytes, $off, $n)
        $done = $off + $n
        $pct = [int][Math]::Floor($done * 100 / $len)
        if ($pct -ge $nextPct) {
            $secs = ((Get-Date) - $t0).TotalSeconds
            Say ("  {0,3} %  {1,5:N0} KB  {2,5:N0} KB/s" -f $pct, ($done / 1KB), ($done / 1KB / [Math]::Max($secs, 0.001)))
            while ($nextPct -le $pct) { $nextPct += 10 }
        }
    }
    $stream.Close()
    $resp = $req.GetResponse()
    $reader = New-Object System.IO.StreamReader($resp.GetResponseStream(), [Text.Encoding]::UTF8)
    $answer = $reader.ReadToEnd() | ConvertFrom-Json
    $resp.Close()
} catch [System.Net.WebException] {
    Fail "Update abgelehnt: $(Read-Error $_.Exception)"
} catch {
    $inner = $_.Exception
    while ($inner.InnerException) { $inner = $inner.InnerException }
    Fail "Übertragung abgebrochen: $($inner.Message)"
}
$secs = ((Get-Date) - $t0).TotalSeconds
if (-not $answer.ok) { Fail "Update abgelehnt: $($answer.msg)" }
Say ("Hochgeladen: {0:N0} KB in {1:N1} s ({2:N0} KB/s). Gerät: {3}" -f ($len / 1KB), $secs, ($len / 1KB / $secs), $answer.msg)
if ($NoWait) {
    Say '--- ota.ps1 | Exit 0 | ohne Prüfung nach dem Neustart (-NoWait)'
    exit 0
}

# --- Neustart und Probelauf prüfen -----------------------------------------------------------------------------
$deadline = (Get-Date).AddSeconds($TimeoutS)
$wentDown = $false
$lastTrial = -100
$status = $null
while ((Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 1500
    $s = Get-Json "$ipBase/api/status" 3000
    if (-not $s) {
        if (-not $wentDown) { Say 'Neustart …' }
        $wentDown = $true
        continue
    }
    # Neu gestartet, wenn das Gerät weg war oder die Laufzeit kleiner ist als vorher
    if (-not $wentDown -and $s.uptime -ge $before.uptime) { continue }
    $wentDown = $true
    $status = $s
    if ($s.v -ne $newVersion) {
        $why = if ($s.rollback) { "$($s.rollback.why)" } else { 'kein Grund gemeldet' }
        Fail "Nach dem Neustart läuft Version $($s.v) statt ${newVersion}: Rückfall ($why)"
    }
    if ($s.ota.passed -or -not $s.ota.trial) { break }
    # Probelauf alle 15 s melden
    if ($s.ota.trialS -ge $lastTrial + 15) {
        $note = "Probelauf {0}/{1} s" -f $s.ota.trialS, $s.ota.trialNeed
        if ($s.ota.trialNote) { $note += " ($($s.ota.trialNote))" }
        Say $note
        $lastTrial = $s.ota.trialS
    }
}
if (-not $status) { Fail "Pixelia meldet sich nach $TimeoutS s nicht zurück (Display ansehen; serielles Log mit tools\monitor.ps1)" }
if ($status.ota.trial -and -not $status.ota.passed) {
    Fail "Selbsttest nach $TimeoutS s noch nicht bestanden (zuletzt $($status.ota.trialS) s, $($status.ota.trialNote))"
}
$total = ((Get-Date) - $t0).TotalSeconds
Say ("OK: Version {0} läuft ({1}), Selbsttest bestanden. {2:N1} fps, RAM {3} KB frei (Tiefstwert {4} KB), WLAN {5} dBm, IP {6}" -f
    $status.v, $status.part, $status.perf.fps, $status.perf.ram, $status.perf.ramMin, $status.wifi.rssi, $status.wifi.ip)
Say ("--- ota.ps1 | Exit 0 | {0:N0} s gesamt | Log: {1}" -f $total, $log)
exit 0
