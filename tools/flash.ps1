<#
.SYNOPSIS
  Firmware bauen, per USB flashen und danach das serielle Log mitlesen.

.DESCRIPTION
  - Erkennt den COM-Port automatisch (wie monitor.ps1).
  - -Erase löscht vorher den kompletten Flash (alte Fremd-Firmware, NVS, Spielstände!).
  - Danach Reset und Log-Mitschnitt über monitor.ps1 (gefiltert, volles Log in .logs\).

.EXAMPLE
  .\tools\flash.ps1
  .\tools\flash.ps1 -Erase -Seconds 15
  .\tools\flash.ps1 -Seconds 20 -Send '+5','pat engine'
#>
param(
    [string]$Port,
    [double]$Seconds = 12,
    [switch]$Erase,
    [string[]]$Send = @(),
    [switch]$Full,
    [string]$Until
)

$ErrorActionPreference = 'Stop'
$tools = $PSScriptRoot

if (-not $Port) {
    $devs = Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '\(COM\d+\)' -and $_.PNPDeviceID -match 'VID_1A86|VID_10C4|VID_303A|VID_0403' }
    foreach ($d in $devs) { if ($d.Name -match '\((COM\d+)\)') { $Port = $Matches[1]; break } }
    if (-not $Port) { Write-Error 'Kein Board gefunden (USB-Datenkabel und Treiber prüfen).'; exit 2 }
}

if ($Erase) {
    & (Join-Path $tools 'idf.ps1') -C firmware -p $Port erase-flash
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

& (Join-Path $tools 'idf.ps1') -C firmware -p $Port flash
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$monArgs = @{ Port = $Port; Seconds = $Seconds; Reset = $true; Send = $Send; Full = $Full }
if ($Until) { $monArgs.Until = $Until }
& (Join-Path $tools 'monitor.ps1') @monArgs
exit $LASTEXITCODE
