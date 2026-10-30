<#
.SYNOPSIS
  Pixelia-Simulator bauen und starten (LLVM-MinGW, CMake, Ninja), Ausgabe gekürzt.

.DESCRIPTION
  - Baut sim/ nach build-sim\ (mit SDL3-Fenster) bzw. mit -Headless nach build-sim-headless\ (ohne SDL, schneller).
  - Volles Build-Log: .logs\sim-build.log. Angezeigt werden nur Fehler, Warnungen und die Zusammenfassung.
  - Alle übrigen Argumente gehen unverändert an pixelia-sim (Arbeitsverzeichnis = Repo-Hauptordner).
  - Ohne Simulator-Argumente wird nur gebaut.

.EXAMPLE
  .\tools\sim.ps1
  .\tools\sim.ps1 --time 20:30
  .\tools\sim.ps1 -Headless --headless --time 06:30 --screenshot build-sim-headless/out/morgen.png
  .\tools\sim.ps1 -Headless --headless --contact-sheet build-sim-headless/out/kontaktbogen.png --span-days 2
#>
# Bewusst ohne param()-Block: Simulator-Optionen wie --headless landen unverändert in $args.
$Headless = $false
$Clean = $false
$SimArgs = @()
foreach ($a in $args) {
    if ($a -eq '-Headless') { $Headless = $true }
    elseif ($a -eq '-Clean') { $Clean = $true }
    else { $SimArgs += "$a" }
}

$ErrorActionPreference = 'Continue'
[Console]::OutputEncoding = [Text.Encoding]::UTF8
$env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot $(if ($Headless) { 'build-sim-headless' } else { 'build-sim' })
$logDir = Join-Path $repoRoot '.logs'
New-Item -ItemType Directory -Force $logDir | Out-Null
$log = Join-Path $logDir 'sim-build.log'

if ($Clean -and (Test-Path $buildDir)) { Remove-Item -Recurse -Force $buildDir }

$start = Get-Date
$out = @()
if (-not (Test-Path (Join-Path $buildDir 'build.ninja'))) {
    $window = if ($Headless) { 'OFF' } else { 'ON' }
    $out += & cmake -S (Join-Path $repoRoot 'sim') -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "-DPIXELIA_SIM_WINDOW=$window" 2>&1 | ForEach-Object { "$_" }
    if ($LASTEXITCODE -ne 0) {
        $out | Set-Content -Path $log -Encoding utf8
        $out | Where-Object { $_ -match 'Error|error' } | Select-Object -First 30
        "--- CMake-Konfiguration fehlgeschlagen | Log: $log"
        exit 1
    }
}
$out += & cmake --build $buildDir 2>&1 | ForEach-Object { "$_" }
$code = $LASTEXITCODE
$out | Set-Content -Path $log -Encoding utf8
$dauer = [int]((Get-Date) - $start).TotalSeconds

# Warnungen aus fremdem Code (SDL unter _deps) und CMake-Feature-Tests ausblenden
$wichtig = $out | Where-Object {
    ($_ -match 'error|warning:|undefined reference' -or $_ -cmatch 'FAILED') -and $_ -notmatch '_deps[\\/]|Performing Test'
} | Select-Object -Unique
if ($wichtig.Count -gt 40) { $wichtig = $wichtig | Select-Object -First 40; $wichtig += '… (gekürzt, siehe Log)' }
$wichtig
"--- Simulator-Build | Exit $code | ${dauer}s | Log: $log"
if ($code -ne 0) { exit $code }

if ($SimArgs.Count -gt 0) {
    Push-Location $repoRoot
    try {
        $exe = Join-Path $buildDir 'pixelia-sim.exe'
        if (-not (Test-Path $exe)) { $exe = Join-Path $buildDir 'pixelia-sim' }
        # -Last statt -First: -First beendete den Simulator nach 60 Zeilen (Exit -1 mitten in --test all)
        & $exe @SimArgs 2>&1 | ForEach-Object { "$_" } | Tee-Object -FilePath (Join-Path $logDir 'sim-last.log') |
            Select-Object -Last 60
        $code = $LASTEXITCODE
    } finally {
        Pop-Location
    }
}
exit $code
