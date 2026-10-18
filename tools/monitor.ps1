<#
.SYNOPSIS
  Liest das serielle Log des Boards N Sekunden lang mit, ohne Interaktion, und zeigt nur das Wichtige.

.DESCRIPTION
  - Erkennt den COM-Port automatisch (CH340/CP210x/FTDI/ESP-USB), sonst -Port angeben.
  - Nutzt Python + pyserial aus der ESP-IDF-Umgebung (EIM).
  - Volles Log: .logs\serial-last.log (mit -Save name zusätzlich .logs\serial-<name>.log).
  - Ausgabe: nur Pixelia-Meldungen, Warnungen, Fehler und Abstürze (mit -Full alles).
  - Backtraces werden automatisch mit addr2line gegen firmware\build\pixelia.elf dekodiert.
  - -Send schickt Befehle an die Diagnose-Schnittstelle der Firmware ("+2" = 2 s warten).

.EXAMPLE
  .\tools\monitor.ps1 -Seconds 15 -Reset
  .\tools\monitor.ps1 -Seconds 8 -Send 'pat frame','+3','info'
  .\tools\monitor.ps1 -Seconds 60 -Until 'Alle vier Ecken'
#>
param(
    [string]$Port,
    [double]$Seconds = 10,
    [switch]$Reset,
    [string[]]$Send = @(),
    [double]$Wait = -1,
    [switch]$Full,
    [string]$Filter,
    [string]$Until,
    [int]$MaxLines = 200,
    [string]$Save
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$env:PYTHONIOENCODING = 'utf-8'

function Find-BoardPort {
    $devs = Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '\(COM\d+\)' -and $_.PNPDeviceID -match 'VID_1A86|VID_10C4|VID_303A|VID_0403' }
    foreach ($d in $devs) {
        if ($d.Name -match '\((COM\d+)\)') { return $Matches[1] }
    }
    return $null
}

if (-not $Port) {
    $Port = Find-BoardPort
    if (-not $Port) {
        Write-Error 'Kein Board gefunden. USB-Kabel (Datenkabel) prüfen; im Geräte-Manager muss z. B. "USB-SERIAL CH340 (COMx)" erscheinen, sonst fehlt der Treiber.'
        exit 2
    }
}

$eimJson = 'C:\Espressif\tools\eim_idf.json'
$cfg = Get-Content $eimJson -Raw | ConvertFrom-Json
$idf = $cfg.idfInstalled | Where-Object { $_.id -eq $cfg.idfSelectedId } | Select-Object -First 1
$python = $idf.python

$repoRoot = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $repoRoot '.logs'
New-Item -ItemType Directory -Force $logDir | Out-Null
$log = Join-Path $logDir 'serial-last.log'

$pyArgs = @((Join-Path $PSScriptRoot 'serial_log.py'), '--port', $Port, '--seconds', "$Seconds", '--out', $log, '--max-lines', "$MaxLines")
if ($Reset) { $pyArgs += '--reset' }
if ($Full) { $pyArgs += '--full' }
if ($Wait -ge 0) { $pyArgs += @('--wait', "$Wait") }
if ($Filter) { $pyArgs += @('--filter', $Filter) }
if ($Until) { $pyArgs += @('--until', $Until) }
foreach ($s in $Send) { $pyArgs += "--send=$s" }

$elf = Join-Path $repoRoot 'firmware\build\pixelia.elf'
if (Test-Path $elf) {
    $a2l = Get-ChildItem -Path (Join-Path $idf.idfToolsPath 'xtensa-esp-elf') -Recurse -Filter 'xtensa-esp32s3-elf-addr2line.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($a2l) { $pyArgs += @('--elf', $elf, '--addr2line', $a2l.FullName) }
}

& $python @pyArgs
$code = $LASTEXITCODE
if ($Save) { Copy-Item $log (Join-Path $logDir "serial-$Save.log") -Force }
exit $code
