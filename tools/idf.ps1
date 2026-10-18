<#
.SYNOPSIS
  Führt idf.py mit aktivierter ESP-IDF-Umgebung aus und zeigt nur das Wichtige.

.DESCRIPTION
  - Findet die ausgewählte ESP-IDF-Installation über C:\Espressif\tools\eim_idf.json (EIM).
  - Schreibt die komplette Ausgabe nach .logs\idf-last.log.
  - Gibt nur Fehler, Warnungen und Zusammenfassungszeilen aus (spart Token). Mit -Full alles.

.EXAMPLE
  .\tools\idf.ps1 -C firmware build
  .\tools\idf.ps1 -C firmware -p COM5 flash
  .\tools\idf.ps1 -Full -C firmware size-components
#>
# Bewusst ohne param()-Block: so landen idf.py-Optionen wie -C oder -p unverändert in $args.
$Full = $false
$IdfArgs = @()
foreach ($a in $args) { if ($a -eq '-Full') { $Full = $true } else { $IdfArgs += "$a" } }

$ErrorActionPreference = 'Continue'
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$env:PYTHONIOENCODING = 'utf-8'

# PATH aus der Registry nachladen (falls Werkzeuge nach Start der Shell installiert wurden)
$env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')

$eimJson = 'C:\Espressif\tools\eim_idf.json'
if (-not (Test-Path $eimJson)) { Write-Error "ESP-IDF nicht gefunden ($eimJson fehlt)."; exit 2 }
$cfg = Get-Content $eimJson -Raw | ConvertFrom-Json
$idf = $cfg.idfInstalled | Where-Object { $_.id -eq $cfg.idfSelectedId } | Select-Object -First 1
if (-not $idf) { Write-Error 'Keine ESP-IDF-Version ausgewählt.'; exit 2 }

# Umgebung aktivieren, Begrüßungstext unterdrücken
. $idf.activationScript *> $null

$repoRoot = Split-Path -Parent $PSScriptRoot
$logDir = Join-Path $repoRoot '.logs'
New-Item -ItemType Directory -Force $logDir | Out-Null
$log = Join-Path $logDir 'idf-last.log'

$start = Get-Date
& $idf.python (Join-Path $idf.path 'tools\idf.py') @IdfArgs 2>&1 |
    ForEach-Object { "$_" } |
    Set-Content -Path $log -Encoding utf8
$code = $LASTEXITCODE
$dauer = [int]((Get-Date) - $start).TotalSeconds

$lines = Get-Content $log -Encoding utf8
if ($Full) {
    $lines
} else {
    $pattern = 'error:|Error:|CMake Error|warning:|FAILED|fatal|Project build complete|Binary size|bytes \(|smallest app partition|Total sizes|Used stat D/IRAM|Flash Code|Flash Data|Hard resetting|Wrote \d+ bytes|Leaving|Serial port|Chip is|Detecting chip|Connected to|Features:|MAC:'
    $wichtig = $lines | Where-Object { $_ -match $pattern } | Select-Object -Unique
    if ($wichtig.Count -gt 60) { $wichtig = $wichtig | Select-Object -First 60; $wichtig += '… (gekürzt, siehe Log)' }
    $wichtig
    '--- letzte Zeilen ---'
    $lines | Select-Object -Last 6
}
"--- idf.py $($IdfArgs -join ' ') | Exit $code | ${dauer}s | Log: $log"
exit $code
