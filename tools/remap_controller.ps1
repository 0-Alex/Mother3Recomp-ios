# MOTHER 3 Recomp - Controller Remapping Utility
# Native zero-latency remapping for Xbox, Nintendo Switch, PlayStation & PC gamepads.
param([string]$Preset = "")

$ErrorActionPreference = "Stop"
$iniPath = Join-Path $PSScriptRoot "keybinds.ini"

if (-not (Test-Path $iniPath)) {
    Write-Host "Error: keybinds.ini not found in $PSScriptRoot" -ForegroundColor Red
    Pause
    Exit 1
}

function Get-CurrentLayout {
    if (-not (Test-Path $iniPath)) { return "Unknown" }
    $layout = "xbox"
    $trigR = "true"
    Get-Content $iniPath | ForEach-Object {
        if ($_ -match '^\s*layout\s*=\s*(.+)$') { $layout = $matches[1].Trim() }
        if ($_ -match '^\s*trigger_r\s*=\s*(.+)$') { $trigR = $matches[1].Trim() }
    }
    if ($trigR -eq "dash") { return "Modern Run (RT = Dash)" }
    if ($layout -eq "nintendo" -or $layout -eq "switch") { return "Nintendo / Switch (B=Confirm, A=Dash)" }
    if ($layout -eq "classic") { return "Classic 1:1 Hardware" }
    return "Xbox / PC (A=Confirm, B=Dash, X=Dash)"
}

function Save-GamepadConfig($layout, $a, $b, $x, $y, $lb, $rb, $trigL, $trigR) {
    $lines = Get-Content $iniPath
    $out = [System.Collections.Generic.List[string]]::new()
    $inGamepad = $false
    $written = $false

    foreach ($line in $lines) {
        if ($line -match '^\s*\[gamepad1\]') {
            $inGamepad = $true
            $written = $true
            $out.Add("[gamepad1]")
            $out.Add("layout     = $layout")
            $out.Add("a          = $a")
            $out.Add("b          = $b")
            $out.Add("x          = $x")
            $out.Add("y          = $y")
            $out.Add("lb         = $lb")
            $out.Add("rb         = $rb")
            $out.Add("select     = back")
            $out.Add("start      = start")
            $out.Add("trigger_l  = $trigL")
            $out.Add("trigger_r  = $trigR")
            $out.Add("deadzone   = 10000")
            $out.Add("analog     = true")
            continue
        }
        if ($inGamepad) {
            if ($line -match '^\s*\[' -or $line -match '^#') {
                $inGamepad = $false
                $out.Add($line)
            }
            continue
        }
        $out.Add($line)
    }

    if (-not $written) {
        $out.Add("[gamepad1]")
        $out.Add("layout     = $layout")
        $out.Add("a          = $a")
        $out.Add("b          = $b")
        $out.Add("x          = $x")
        $out.Add("y          = $y")
        $out.Add("lb         = $lb")
        $out.Add("rb         = $rb")
        $out.Add("select     = back")
        $out.Add("start      = start")
        $out.Add("trigger_l  = $trigL")
        $out.Add("trigger_r  = $trigR")
        $out.Add("deadzone   = 10000")
        $out.Add("analog     = true")
    }

    Set-Content -Path $iniPath -Value $out -Encoding UTF8
    Write-Host "`n[OK] Controller layout saved to keybinds.ini!" -ForegroundColor Green
    Write-Host "[OK] Changes take effect IMMEDIATELY (even if game is running).`n" -ForegroundColor Cyan
}

if ($Preset -ne "") {
    switch ($Preset.ToLower()) {
        "xbox"     { Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "true" }
        "nintendo" { Save-GamepadConfig "nintendo" "b" "a" "a" "b" "l" "r" "true" "true" }
        "dash"     { Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "dash" }
        "classic"  { Save-GamepadConfig "classic" "a" "b" "x" "y" "l" "r" "true" "true" }
    }
    Exit 0
}

while ($true) {
    Clear-Host
    $curr = Get-CurrentLayout
    Write-Host "==================================================================" -ForegroundColor Cyan
    Write-Host "             MOTHER 3 RECOMP - CONTROLLER REMAPPER               " -ForegroundColor Yellow
    Write-Host "==================================================================" -ForegroundColor Cyan
    Write-Host "Current Configuration: $curr`n" -ForegroundColor White

    Write-Host "Choose a layout preset:" -ForegroundColor Green
    Write-Host "  [1] Xbox / PC Standard    (A = Confirm, B = Cancel/Dash, X = Dash, Y = Confirm)"
    Write-Host "  [2] Nintendo / Switch     (B = Confirm, A = Cancel/Dash, Y = Dash, X = Confirm)"
    Write-Host "  [3] Modern Trigger Dash   (A = Confirm, B = Cancel, Right Trigger RT = Dash!)"
    Write-Host "  [4] Classic 1:1 Hardware  (A = A, B = B, X = X, Y = Y)"
    Write-Host "  [5] Custom Button Setup   (Interactively pick buttons)"
    Write-Host "  [6] Open keybinds.ini in Notepad"
    Write-Host "  [0] Exit"
    Write-Host "==================================================================" -ForegroundColor Cyan
    $choice = Read-Host "Enter option [1-6, 0]"

    switch ($choice) {
        "1" {
            Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "true"
            Write-Host "Preset set: Xbox Standard." -ForegroundColor Green
            Start-Sleep -Seconds 1
        }
        "2" {
            Save-GamepadConfig "nintendo" "b" "a" "a" "b" "l" "r" "true" "true"
            Write-Host "Preset set: Nintendo / Switch Pro Controller." -ForegroundColor Green
            Start-Sleep -Seconds 1
        }
        "3" {
            Save-GamepadConfig "xbox" "a" "b" "b" "a" "l" "r" "true" "dash"
            Write-Host "Preset set: Modern Trigger Dash (RT dashes!)." -ForegroundColor Green
            Start-Sleep -Seconds 1
        }
        "4" {
            Save-GamepadConfig "classic" "a" "b" "x" "y" "l" "r" "true" "true"
            Write-Host "Preset set: Classic 1:1 Hardware." -ForegroundColor Green
            Start-Sleep -Seconds 1
        }
        "5" {
            Write-Host "`nCustom Button Setup (type button: a, b, x, y, l, r, start, select, none):" -ForegroundColor Yellow
            $a = Read-Host "Button for Physical A (Default: a)"
            if ([string]::IsNullOrWhiteSpace($a)) { $a = "a" }
            $b = Read-Host "Button for Physical B (Default: b)"
            if ([string]::IsNullOrWhiteSpace($b)) { $b = "b" }
            $x = Read-Host "Button for Physical X (Default: b for Dash)"
            if ([string]::IsNullOrWhiteSpace($x)) { $x = "b" }
            $y = Read-Host "Button for Physical Y (Default: a for Confirm)"
            if ([string]::IsNullOrWhiteSpace($y)) { $y = "a" }
            $trigR = Read-Host "Right Trigger RT function [r / dash / false] (Default: r)"
            if ([string]::IsNullOrWhiteSpace($trigR)) { $trigR = "true" }
            Save-GamepadConfig "xbox" $a $b $x $y "l" "r" "true" $trigR
            Start-Sleep -Seconds 2
        }
        "6" {
            Start-Process notepad.exe -ArgumentList "`"$iniPath`""
            Write-Host "Opened keybinds.ini in Notepad. Changes are hot-reloaded upon saving!" -ForegroundColor Cyan
            Start-Sleep -Seconds 2
        }
        "0" {
            Write-Host "Goodbye! Enjoy Mother 3 Recomp!" -ForegroundColor Yellow
            Exit 0
        }
        default {
            Write-Host "Invalid option. Please choose 1-6 or 0." -ForegroundColor Red
            Start-Sleep -Seconds 1
        }
    }
}
