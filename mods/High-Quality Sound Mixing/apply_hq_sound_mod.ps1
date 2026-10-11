param(
    [string]$RomPath
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$GameRoot = Split-Path (Split-Path $ScriptDir -Parent) -Parent

if (-not $RomPath) {
    $candidates = @(
        ".\MOTHER 3 (English v1.3).gba",
        ".\mother3_en_v13.gba",
        "..\..\roms\MOTHER 3 (English v1.3).gba",
        "..\..\roms\mother3_en_v13.gba",
        (Join-Path $GameRoot "roms\MOTHER 3 (English v1.3).gba"),
        (Join-Path $GameRoot "roms\mother3_en_v13.gba")
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path $c)) {
            $RomPath = (Resolve-Path $c).Path
            break
        }
    }
}

if (-not $RomPath -or -not (Test-Path $RomPath)) {
    Write-Host "Error: Could not find MOTHER 3 (English v1.3).gba" -ForegroundColor Red
    Write-Host "Please pass your ROM path or place it in the roms folder."
    exit 1
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$PatchPath = Join-Path $ScriptDir "mother3_hq_sound_mixing.ups"

if (-not (Test-Path $PatchPath)) {
    Write-Host "Error: Patch file not found at $PatchPath" -ForegroundColor Red
    exit 1
}

Write-Host "Reading ROM: $RomPath" -ForegroundColor Cyan
$src = [System.IO.File]::ReadAllBytes($RomPath)
$patch = [System.IO.File]::ReadAllBytes($PatchPath)

$bodyEnd = $patch.Length - 12
$cursor = 4

function DecNum {
    param([ref]$cur)
    [UInt64]$data = 0
    [UInt64]$shift = 1
    while ($cur.Value -lt $bodyEnd) {
        $x = $patch[$cur.Value]
        $cur.Value++
        $data += ($x -band 0x7F) * $shift
        if ($x -band 0x80) { return $data }
        $shift = $shift -shl 7
        $data += $shift
    }
    return $data
}

$curRef = [ref]$cursor
$inSz = DecNum $curRef
$outSz = DecNum $curRef
$cursor = $curRef.Value

[byte[]]$target = [byte[]]::new($outSz)
[Array]::Copy($src, $target, [Math]::Min($src.Length, $target.Length))

[Int64]$pos = 0
while ($cursor -lt $bodyEnd) {
    $curRef = [ref]$cursor
    $skip = DecNum $curRef
    $cursor = $curRef.Value
    $pos += $skip
    while ($cursor -lt $bodyEnd) {
        $x = $patch[$cursor]
        $cursor++
        if ($x -eq 0) { $pos++; break }
        $target[$pos] = $target[$pos] -bxor $x
        $pos++
    }
}

$OutDir = Split-Path -Parent $RomPath
$OutName = [System.IO.Path]::GetFileNameWithoutExtension($RomPath) + " (HQ Audio).gba"
$OutPath = Join-Path $OutDir $OutName

[System.IO.File]::WriteAllBytes($OutPath, $target)

$sha = [System.Security.Cryptography.SHA1]::Create()
$h = -join ($sha.ComputeHash($target) | ForEach-Object { '{0:x2}' -f $_ })

Write-Host ""
Write-Host "==========================================================" -ForegroundColor Green
Write-Host " SUCCESS: High-Quality Sound Mixing Mod Applied!" -ForegroundColor Green
Write-Host " Output ROM: $OutPath" -ForegroundColor Green
Write-Host " SHA-1:      $h" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# Automatically configure Mother3Recomp to use the new HQ Audio ROM
$GameRoot = Split-Path (Split-Path $ScriptDir -Parent) -Parent
$RomCfg = Join-Path $GameRoot "rom.cfg"
$ConfigIni = Join-Path $GameRoot "config.ini"

if (Test-Path $GameRoot) {
    Set-Content -Path $RomCfg -Value $OutPath -Encoding ASCII
    Write-Host ""
    Write-Host " [Auto-Config] Updated rom.cfg to point to HQ Audio ROM." -ForegroundColor Cyan

    if (Test-Path $ConfigIni) {
        $iniContent = Get-Content $ConfigIni -Raw
        if ($iniContent -match "skip_launcher\s*=\s*\d+") {
            $iniContent = $iniContent -replace "skip_launcher\s*=\s*\d+", "skip_launcher = 1"
        } else {
            $iniContent = $iniContent -replace "\[Launcher\]", "[Launcher]`r`nskip_launcher = 1"
        }
        Set-Content -Path $ConfigIni -Value $iniContent -Encoding ASCII
        Write-Host " [Auto-Config] Set skip_launcher = 1 in config.ini." -ForegroundColor Cyan
    }
    Write-Host ""
    Write-Host " Setup complete! Double-click 'Play Mother 3.cmd' to start playing with HQ Audio!" -ForegroundColor Yellow
    Write-Host ""
}
