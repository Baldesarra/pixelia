<#
.SYNOPSIS
  Log und Diagnose-Befehle per WLAN, ganz ohne USB: Live-Log über den WebSocket /ws/log, Befehle über
  POST /api/cmd, frühere Starts aus dem Log-Puffer über GET /api/log. Gegenstück zu tools\monitor.ps1.

.DESCRIPTION
  - Ziel: -Target (Standard http://pixelia/), sonst die IP (steht auf der Info-Karte: 3 s aufs Display drücken).
    Der Name wird einmal aufgelöst, danach wird die IP benutzt (wie tools\ota.ps1).
  - Live-Log: -Seconds n (Standard 10, mit -Send 2 s nach dem letzten Befehl, 0 = kein Live-Log), -Until <Regex>
    beendet vorzeitig. -Tail n zeigt beim Verbinden zuerst die letzten n Zeilen aus dem Puffer.
  - Befehle: -Send 'info','+2','tasks' wie bei monitor.ps1 ("+2" = 2 s warten, das Live-Log läuft weiter).
    Die Ausgabe des Befehls kommt direkt als Antwort. Gefährliche Befehle (reboot, wifi setup, log clear …)
    lehnt das Gerät ohne -Confirm ab (Exit 1 mit Grund).
  - Verlauf: -Boot prev (Start vor dem letzten Neustart, z. B. vor einem Absturz), -Boot <Nr> oder -All
    (ganzer Puffer). Ohne -Seconds gibt es dabei kein Live-Log.
  - Filter wie monitor.ps1: Pixelia-Meldungen, Warnungen, Fehler; -Full zeigt alles, -Filter <Regex> eigenen.
  - Ausgabe höchstens -MaxLines Zeilen; volles Log (ungefiltert, mit Datum) in .logs\netlog-last.log.
  - -Pin für den optionalen PIN ab 0.3.x (Kopf X-Pixelia-Pin).
  - Exit 0 = ok, 1 = Gerät nicht erreichbar oder Befehl abgelehnt/unbekannt, 2 = falscher Aufruf.

.EXAMPLE
  .\tools\netlog.ps1 -Seconds 20
  .\tools\netlog.ps1 -Send 'info','tasks','log'
  .\tools\netlog.ps1 -Send 'reboot' -Confirm -Seconds 15
  .\tools\netlog.ps1 -Boot prev -Full
  .\tools\netlog.ps1 -Target 192.168.178.74 -Tail 30 -Seconds 0
#>
param(
    [string]$Target = 'http://pixelia/',
    [double]$Seconds = -1,
    [string[]]$Send = @(),
    [switch]$Confirm,
    [int]$Tail = 0,
    [string]$Boot,
    [switch]$All,
    [switch]$Full,
    [string]$Filter,
    [string]$Until,
    [int]$MaxLines = 200,
    [string]$Pin
)

$ErrorActionPreference = 'Stop'
# Achtung: PowerShell behandelt „ und “ als Anführungszeichen, solche Texte nur in einfachen Anführungszeichen
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$repoRoot = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $repoRoot '.logs'
New-Item -ItemType Directory -Force $logDir | Out-Null
$log = Join-Path $logDir 'netlog-last.log'
Set-Content -Path $log -Value "netlog.ps1 $(Get-Date -Format s) $Target" -Encoding utf8

$ownTags = 'app|cmd|diag|dns|gfx|i2c|input|lcd|log|ota|perf|set|sys|web|wlan|zeit|flash|test|bl|tour|touch|ui'
$defaultFilter = "^[IWE] \(\d+\) ($ownTags):|^[WE] \(\d+\)|Guru Meditation|Backtrace|abort\(\)|assert|panic"
$filterRx = if ($Filter) { [regex]$Filter } else { [regex]$defaultFilter }
$untilRx = if ($Until) { [regex]$Until } else { $null }

$script:printed = 0
$script:total = 0
$script:stop = $false
$script:cmdLines = @{}  # Ausgaben von Befehlen: im Live-Log nicht doppelt zeigen

function Out-Line([string]$text, [bool]$force = $false) {
    if ($script:printed -lt $MaxLines) {
        $text
        $script:printed++
        if ($script:printed -eq $MaxLines) { '… (Ausgabe gekürzt, volles Log in der Datei)' }
    }
}

function Fail([string]$text, [int]$code = 1) {
    "FEHLER: $text"
    Add-Content -Path $log -Value "FEHLER: $text" -Encoding utf8
    "--- netlog.ps1 | Exit $code | Log: $log"
    exit $code
}

function Format-Time($t) {
    if (-not $t) { return '--:--:--' }
    return [DateTimeOffset]::FromUnixTimeSeconds([int64]$t).ToLocalTime().ToString('HH:mm:ss')
}

function Format-Date($t) {
    if (-not $t) { return '---------- --:--:--' }
    return [DateTimeOffset]::FromUnixTimeSeconds([int64]$t).ToLocalTime().ToString('yyyy-MM-dd HH:mm:ss')
}

# Eine Log-Zeile aus /api/log oder /ws/log verarbeiten
function Show-LogLine($l) {
    $script:total++
    $m = [string]$l.m
    Add-Content -Path $log -Value ('{0} #{1} {2}' -f (Format-Date $l.t), $l.b, $m) -Encoding utf8
    if ($script:cmdLines.ContainsKey($m)) {
        $script:cmdLines[$m]--
        if ($script:cmdLines[$m] -le 0) { $script:cmdLines.Remove($m) }
        return
    }
    if ($Full -or $filterRx.IsMatch($m)) { Out-Line ('[{0} #{1}] {2}' -f (Format-Time $l.t), $l.b, $m) }
    if ($untilRx -and $untilRx.IsMatch($m)) { $script:stop = $true }
}

# --- Ziel ----------------------------------------------------------------------------------------------------
if ($Target -notmatch '^https?://') { $Target = "http://$Target" }
$uri = [Uri]$Target
$base = "$($uri.Scheme)://$($uri.Authority)"
$ipBase = $base
$hostPort = $uri.Authority
if ($uri.HostNameType -eq 'Dns') {
    try {
        $addr = [System.Net.Dns]::GetHostAddresses($uri.DnsSafeHost) |
            Where-Object { $_.AddressFamily -eq 'InterNetwork' -and $_.ToString() -ne '0.0.0.0' } | Select-Object -First 1
        if ($addr) {
            $hostPort = $addr.ToString() + $(if ($uri.IsDefaultPort) { '' } else { ":$($uri.Port)" })
            $ipBase = "$($uri.Scheme)://$hostPort"
        }
    } catch { }
}

# $body bewusst ohne Typ: [string] machte aus $null einen leeren Text (dann versucht GET einen Inhalt zu senden)
function Invoke-Api([string]$method, [string]$path, $body = $null, [int]$timeoutMs = 8000) {
    $req = [System.Net.HttpWebRequest]::Create("$ipBase$path")
    $req.Method = $method
    $req.Timeout = $timeoutMs
    $req.ReadWriteTimeout = $timeoutMs
    $req.Proxy = $null
    $req.KeepAlive = $false
    if ($Pin) { $req.Headers.Add('X-Pixelia-Pin', $Pin) }
    if ($null -ne $body) {
        $bytes = [Text.Encoding]::UTF8.GetBytes($body)
        $req.ContentType = 'application/x-www-form-urlencoded'
        $req.ContentLength = $bytes.Length
        $s = $req.GetRequestStream()
        $s.Write($bytes, 0, $bytes.Length)
        $s.Close()
    }
    try {
        $resp = $req.GetResponse()
    } catch [System.Net.WebException] {
        $resp = $_.Exception.Response
        if (-not $resp) { throw }
    }
    try {
        $reader = New-Object System.IO.StreamReader($resp.GetResponseStream(), [Text.Encoding]::UTF8)
        $text = $reader.ReadToEnd()
        return [pscustomobject]@{ Status = [int]$resp.StatusCode; Json = ($text | ConvertFrom-Json) }
    } finally { $resp.Close() }
}

try {
    $st = (Invoke-Api 'GET' '/api/status' $null 6000).Json
} catch {
    Fail ('Pixelia unter {0} nicht erreichbar ({1}). Die IP steht auf der Info-Karte (3 s aufs Display drücken).' -f $base, $_.Exception.Message)
}
$shown = if ($ipBase -ne $base) { "$base → $ipBase" } else { $base }
$crash = if ($st.diag -and $st.diag.crash) { ('; Absturzbericht: Task {0}' -f $st.diag.crash.task) } else { '' }
$head = 'Gerät:  {0}, Version {1}, Start Nr. {2} ({3}), läuft seit {4} s, WLAN {5} dBm, {6} Fehler/{7} Warnungen{8}' -f $shown,
    $st.v, $st.diag.boot, $st.diag.reset, $st.uptime, $st.wifi.rssi, $st.diag.errors, $st.diag.warnings, $crash
$head
Add-Content -Path $log -Value $head -Encoding utf8

# --- Verlauf aus dem Puffer ----------------------------------------------------------------------------------
$history = $All -or $Boot
if ($history) {
    $bootNr = $null
    if ($Boot -eq 'prev') { $bootNr = [int]$st.diag.boot - 1 }
    elseif ($Boot) { $bootNr = [int]$Boot }
    $since = 0
    $first = $true
    do {
        $q = "/api/log?max=65536"
        if (-not $first) { $q += "&since=$since" }
        if ($null -ne $bootNr) { $q += "&boot=$bootNr" }
        $r = (Invoke-Api 'GET' $q $null 15000).Json
        foreach ($l in $r.lines) { Show-LogLine $l }
        $since = $r.last
        $first = $false
    } while ($r.more -and -not $script:stop)
    $what = if ($null -ne $bootNr) { "Start Nr. $bootNr" } else { 'ganzer Puffer' }
    "--- Verlauf ($what): $($script:total) Zeilen"
    if ($Seconds -lt 0 -and $Send.Count -eq 0) { $Seconds = 0 }
}

if ($Seconds -lt 0) { $Seconds = if ($Send.Count) { 2 } else { 10 } }

# --- Live-Log ------------------------------------------------------------------------------------------------
$ws = $null
$script:recv = $null
$buf = New-Object byte[] 65536
$seg = [System.ArraySegment[byte]]::new($buf)
$ms = New-Object System.IO.MemoryStream
$cts = New-Object System.Threading.CancellationTokenSource
if ($Seconds -gt 0) {
    $ws = New-Object System.Net.WebSockets.ClientWebSocket
    $ws.Options.KeepAliveInterval = [TimeSpan]::FromSeconds(20)
    $wsUri = [Uri]("ws://$hostPort/ws/log?tail=$Tail")
    try {
        if (-not $ws.ConnectAsync($wsUri, $cts.Token).Wait(6000)) { Fail 'WebSocket /ws/log: keine Verbindung nach 6 s' }
    } catch {
        $inner = $_.Exception
        while ($inner.InnerException) { $inner = $inner.InnerException }
        Fail "WebSocket /ws/log: $($inner.Message)"
    }
    $script:recv = $ws.ReceiveAsync($seg, $cts.Token)
}

# Fertig empfangene Rahmen verarbeiten, ohne zu warten
function Pump-WebSocket {
    if (-not $ws) { return }
    while ($script:recv.IsCompleted) {
        if ($script:recv.IsFaulted -or $script:recv.IsCanceled) { $script:wsClosed = $true; return }
        $r = $script:recv.Result
        if ($r.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close) { $script:wsClosed = $true; return }
        $ms.Write($buf, 0, $r.Count)
        if ($r.EndOfMessage) {
            $text = [Text.Encoding]::UTF8.GetString($ms.ToArray())
            $ms.SetLength(0)
            try { $frame = $text | ConvertFrom-Json } catch { $frame = $null }
            if ($frame) { foreach ($l in $frame.lines) { Show-LogLine $l } }
        }
        $script:recv = $ws.ReceiveAsync($seg, $cts.Token)
    }
}

# --- Befehle -------------------------------------------------------------------------------------------------
$exit = 0
$t0 = Get-Date
$queue = New-Object System.Collections.Queue
foreach ($s in $Send) { $queue.Enqueue($s) }
$nextSend = (Get-Date).AddMilliseconds(300)
$deadline = $null
$script:wsClosed = $false

while (-not $script:stop) {
    Pump-WebSocket
    if ($script:wsClosed) { Out-Line '--- WebSocket geschlossen (Neustart?)'; break }
    $now = Get-Date
    if ($queue.Count -gt 0 -and $now -ge $nextSend) {
        $item = [string]$queue.Dequeue()
        if ($item.StartsWith('+')) {
            $nextSend = $now.AddSeconds([double]$item.Substring(1))
            continue
        }
        Out-Line ">>> $item"
        Add-Content -Path $log -Value ">>> $item" -Encoding utf8
        $body = 'cmd=' + [Uri]::EscapeDataString($item)
        if ($Confirm) { $body += '&confirm=1' }
        if ($Pin) { $body += '&pin=' + [Uri]::EscapeDataString($Pin) }
        try {
            $r = Invoke-Api 'POST' '/api/cmd' $body 25000
        } catch {
            Out-Line "FEHLER: Befehl nicht zugestellt: $($_.Exception.Message)"
            $exit = 1
            $nextSend = (Get-Date).AddMilliseconds(300)
            continue
        }
        $j = $r.Json
        foreach ($line in ([string]$j.out -split "`n")) {
            if (-not $line) { continue }
            Add-Content -Path $log -Value "    $line" -Encoding utf8
            Out-Line "    $line"
            if ($script:cmdLines.ContainsKey($line)) { $script:cmdLines[$line]++ } else { $script:cmdLines[$line] = 1 }
        }
        if (-not $j.ok) {
            Out-Line ('    → abgelehnt ({0}): {1}' -f $r.Status, $j.msg)
            $exit = 1
        } else {
            Out-Line ('    → ok ({0} ms)' -f $j.ms)
        }
        $nextSend = (Get-Date).AddMilliseconds(300)
        continue
    }
    if ($queue.Count -eq 0) {
        if (-not $deadline) { $deadline = (Get-Date).AddSeconds($Seconds) }
        if ((Get-Date) -ge $deadline) { break }
    }
    Start-Sleep -Milliseconds 50
}

if ($ws) {
    try { $ws.CloseAsync([System.Net.WebSockets.WebSocketCloseStatus]::NormalClosure, 'Ende', $cts.Token).Wait(1000) | Out-Null } catch { }
    $ws.Dispose()
}
$secs = ((Get-Date) - $t0).TotalSeconds
$why = if ($script:stop -and $untilRx) { ' (Abbruch durch -Until)' } else { '' }
"--- netlog.ps1 | Exit $exit | $($script:total) Log-Zeilen in {0:N1} s$why, $($script:printed) angezeigt | Log: $log" -f $secs
exit $exit
