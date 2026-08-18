<#
.SYNOPSIS
    Executable size breakdown for Game.exe (MSVC / Windows). PowerShell port of
    BuildScripts/sizereport.sh — kept in sync with it.

.DESCRIPTION
    Produces two tables:
      1. PE section totals (.text/.rdata/.data/...) via dumpbin — the ground truth.
      2. Per-library weight inside the exe, parsed from the linker .map.

    The per-library number is an approximation: symbols (public + static) are sorted
    by address and the gap to the next symbol *within the same PE section* is charged
    to the owning library, so the mapped total lands within ~1% of the real exe size.
    Treat the ranking as authoritative and the absolute per-lib KB as good enough to
    decide what to cut. See ExeSize.md for the full analysis and reduction playbook.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File BuildScripts\sizereport.ps1 Release x64
    # or just: BuildScripts\sizereport.bat Release x64
#>
[CmdletBinding()]
param(
    [string]$Config = 'Release',
    [string]$Arch   = 'x64'
)

$ErrorActionPreference = 'Stop'

switch ($Config.ToLower()) { 'debug' { $Config = 'Debug' } default { $Config = 'Release' } }
switch ($Arch.ToLower())   { 'x86'   { $Arch   = 'x86'   } default { $Arch   = 'x64'    } }

$Root  = Split-Path -Parent $PSScriptRoot            # BuildScripts/ -> repo root
$Build = Join-Path $Root 'Build'
$Exe   = Join-Path $Build "$Arch\$Config\Game.exe"
$Map   = Join-Path $Build "$Arch\$Config\Game.map"

# --- locate dumpbin via vswhere (no hard-coded MSVC version) -----------------
function Find-DumpBin {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vsRoot = & $vswhere -latest -property installationPath 2>$null
        if ($vsRoot) {
            $hit = Get-ChildItem -Path (Join-Path $vsRoot 'VC\Tools\MSVC') -Recurse -Filter 'dumpbin.exe' `
                        -ErrorAction SilentlyContinue |
                    Where-Object { $_.FullName -match "Host$Arch\\$Arch\\dumpbin\.exe$" } |
                    Select-Object -First 1
            if ($hit) { return $hit.FullName }
        }
    }
    $cmd = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}
$DumpBin = Find-DumpBin

# --- ensure the map exists --------------------------------------------------
if (-not (Test-Path $Map)) {
    Write-Host ">> No $Map found - building Game with SFMX_SIZE_MAP=ON ..."
    cmake -B $Build -DCMAKE_BUILD_TYPE=$Config -DSFMX_SIZE_MAP=ON | Out-Null
    cmake --build $Build --config $Config --target Game | Out-Null
}
if (-not (Test-Path $Exe)) { Write-Error "Missing $Exe (build first: build.bat $Config)"; exit 1 }

$bytes = (Get-Item $Exe).Length
Write-Host ""
Write-Host ("=== Game.exe : {0:N2} MB ({1} / {2}) ===" -f ($bytes / 1MB), $Arch, $Config)

# --- 1) section totals (ground truth) ---------------------------------------
if ($DumpBin) {
    Write-Host ""
    Write-Host "--- PE sections (raw data, ground truth) ---"
    $name = '(unnamed)'; $tot = 0L
    & $DumpBin -HEADERS $Exe 2>$null | ForEach-Object {
        if     ($_ -match 'SECTION HEADER')                       { $name = '(unnamed)' }
        elseif ($_ -match '^\s+(\S+)\s+name$')                    { $name = $Matches[1] }
        elseif ($_ -match '^\s+([0-9A-Fa-f]+)\s+size of raw data') {
            $sz = [Convert]::ToInt64($Matches[1], 16); $tot += $sz
            Write-Host ("  {0,-12} {1,10:N1} KB" -f $name, ($sz / 1KB))
        }
    }
    Write-Host ("  {0,-12} {1,10:N1} KB" -f 'TOTAL', ($tot / 1KB))
} else {
    Write-Host "  (dumpbin not found - skipping section table)"
}

# --- 2) per-library breakdown from the map ----------------------------------
Write-Host ""
Write-Host "--- Per-library weight inside the exe (from Game.map) ---"

$rows = New-Object System.Collections.Generic.List[object]
$seen = $false
foreach ($line in [IO.File]::ReadLines($Map)) {
    if (-not $seen) { if ($line -match 'Publics by Value') { $seen = $true }; continue }
    $t = ($line.Trim()) -split '\s+'
    if ($t.Count -lt 2) { continue }
    $last = $t[-1]
    if ($last -notmatch ':') { continue }
    $sec = ($t[0] -split ':')[0]
    if ($sec -notmatch '^[0-9A-Fa-f]{4}$') { continue }
    $rva = $null
    foreach ($f in $t) { if ($f -match '^[0-9A-Fa-f]{16}$') { $rva = [Convert]::ToUInt64($f, 16); break } }
    if ($null -eq $rva) { continue }
    $rows.Add([pscustomobject]@{ rva = $rva; sec = $sec; lib = ($last -split ':')[0] })
}

$rows = $rows | Sort-Object rva
$size = @{}; $cnt = @{}
for ($i = 0; $i -lt $rows.Count; $i++) {
    $r = $rows[$i]
    if (-not $cnt.ContainsKey($r.lib)) { $cnt[$r.lib] = 0 }
    $cnt[$r.lib]++
    if ($i + 1 -lt $rows.Count) {
        $n = $rows[$i + 1]
        if ($n.sec -eq $r.sec) {
            $d = [int64]($n.rva - $r.rva)
            if ($d -gt 0 -and $d -lt 2000000) {
                if (-not $size.ContainsKey($r.lib)) { $size[$r.lib] = 0L }
                $size[$r.lib] += $d
            }
        }
    }
}

$total = 0L; foreach ($v in $size.Values) { $total += $v }
Write-Host ("  {0,10} {1,7} {2,8}  {3}" -f 'SIZE_KB', '%', '#syms', 'LIBRARY')
Write-Host "  ------------------------------------------------------------"
$size.GetEnumerator() | Sort-Object Value -Descending | ForEach-Object {
    Write-Host ("  {0,10:N1} {1,6:N1}% {2,8}  {3}" -f ($_.Value / 1KB), (100.0 * $_.Value / $total), $cnt[$_.Key], $_.Key)
}
Write-Host ("  {0,10:N1}                    == TOTAL (mapped)" -f ($total / 1KB))
Write-Host ""
