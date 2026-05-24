# Usage:
#   . .\export.ps1
#
# Dot-source this file so environment variables remain in the current shell.

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$IOTDASH_ROOT = Join-Path $ScriptDir "AIoTHackStorm"
$ExpectedVenv = Join-Path $IOTDASH_ROOT ".venv"

if (-not (Test-Path $IOTDASH_ROOT -PathType Container)) {
    Write-Host "Error: AIoTHackStorm directory not found at $IOTDASH_ROOT" -ForegroundColor Red
    Write-Host "Make sure AIoTHackStorm is cloned in the project root."
    return 1
}

Write-Host "=================================================="
Write-Host "HackStorm Clock App - TuyaOpen SDK Setup"
Write-Host "=================================================="
Write-Host "Project root: $ScriptDir"
Write-Host "SDK root: $IOTDASH_ROOT"
Write-Host ""

if ($env:VIRTUAL_ENV) {
    try {
        $currentVenv = (Resolve-Path $env:VIRTUAL_ENV).Path
        $targetVenv = (Resolve-Path $ExpectedVenv -ErrorAction SilentlyContinue).Path
        if ($targetVenv -and ($currentVenv -eq $targetVenv)) {
            Write-Host "[OK] Virtual environment already activated"
            return 0
        }
    } catch {
    }
}

function Get-UsablePython {
    $candidates = @(
        @{ Exe = "py"; Args = @("-3.12") },
        @{ Exe = "py"; Args = @("-3") },
        @{ Exe = "python"; Args = @() }
    )

    foreach ($candidate in $candidates) {
        try {
            $versionText = & $candidate.Exe @($candidate.Args) --version 2>&1
            if ($LASTEXITCODE -ne 0) { continue }

            if ($versionText -match 'Python\s+(\d+)\.(\d+)\.(\d+)') {
                $major = [int]$matches[1]
                $minor = [int]$matches[2]

                if (($major -gt 3) -or (($major -eq 3) -and ($minor -ge 6))) {
                    return $candidate
                }
            }
        } catch {
        }
    }

    return $null
}

$Python = Get-UsablePython
if (-not $Python) {
    Write-Host "Error: No suitable Python version found." -ForegroundColor Red
    Write-Host "Please install Python 3.6 or higher."
    return 1
}

$pyVersionText = & $Python.Exe @($Python.Args) --version 2>&1
Write-Host "[OK] Using $($Python.Exe) ($pyVersionText)"

$VenvPath = $ExpectedVenv
$ActivateScript = Join-Path $VenvPath "Scripts\Activate.ps1"
$VenvPython = Join-Path $VenvPath "Scripts\python.exe"
$VenvPip = Join-Path $VenvPath "Scripts\pip.exe"

if (-not (Test-Path $VenvPath -PathType Container)) {
    Write-Host ""
    Write-Host "Creating virtual environment..."
    & $Python.Exe @($Python.Args) -m venv $VenvPath
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error: Failed to create virtual environment." -ForegroundColor Red
        return 1
    }
    Write-Host "[OK] Virtual environment created"
}
else {
    Write-Host "[OK] Virtual environment already exists"
}

if (-not (Test-Path $ActivateScript -PathType Leaf)) {
    Write-Host "Error: Virtual environment activation script not found: $ActivateScript" -ForegroundColor Red
    return 1
}

Push-Location $IOTDASH_ROOT

Write-Host ""
Write-Host "Activating virtual environment..."

try {
    . $ActivateScript
}
catch {
    Write-Host "Error: Failed to activate virtual environment." -ForegroundColor Red
    Write-Host "PowerShell may be blocking scripts."
    Write-Host "Run this first, then try again:"
    Write-Host "  Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass"
    Pop-Location
    return 1
}

if (-not $env:VIRTUAL_ENV) {
    Write-Host "Error: Failed to activate virtual environment." -ForegroundColor Red
    Pop-Location
    return 1
}

Write-Host "[OK] Virtual environment activated: $env:VIRTUAL_ENV"

$env:OPEN_SDK_ROOT = $IOTDASH_ROOT
$env:OPEN_SDK_PYTHON = $VenvPython
$env:OPEN_SDK_PIP = $VenvPip

$pathEntries = $env:Path -split ';'
if ($pathEntries -notcontains $IOTDASH_ROOT) {
    $env:Path = $env:Path + ";" + $IOTDASH_ROOT
}

Write-Host ""
Write-Host "Installing dependencies..."
try {
    & $VenvPip install -q -r (Join-Path $IOTDASH_ROOT "requirements.txt") 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[OK] Dependencies installed"
    }
    else {
        Write-Host "Warning: Some dependencies may not have installed" -ForegroundColor Yellow
    }
}
catch {
    Write-Host "Warning: Some dependencies may not have installed" -ForegroundColor Yellow
}

$CachePath = Join-Path $IOTDASH_ROOT ".cache"
New-Item -ItemType Directory -Force -Path $CachePath | Out-Null

$EnvJson = Join-Path $CachePath ".env.json"
$DontPrompt = Join-Path $CachePath ".dont_prompt_update_platform"

if (Test-Path $EnvJson) {
    Remove-Item $EnvJson -Force
}
if (Test-Path $DontPrompt) {
    Remove-Item $DontPrompt -Force
}

Pop-Location

Write-Host ""
Write-Host "=================================================="
Write-Host "[OK] TuyaOpen environment ready"
Write-Host "=================================================="
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. cd board_firmware"
Write-Host "  2. idf.py set-target esp32s3"
Write-Host "  3. idf.py build"
Write-Host "  4. idf.py -p COM3 flash"
Write-Host ""
Write-Host "To deactivate: deactivate"
Write-Host "=================================================="