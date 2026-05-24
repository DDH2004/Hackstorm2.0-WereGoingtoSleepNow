# Run once after cloning:
#   powershell -ExecutionPolicy Bypass -File .\setup.ps1
# Sets up the Python venv and copies firmware into AIoTHackStorm on Windows.

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$SDK = Join-Path $ScriptDir "AIoTHackStorm"
$Firmware = Join-Path $ScriptDir "board_firmware"

# ── 1. Check AIoTHackStorm exists ─────────────────────────────────────────────
if (-not (Test-Path $SDK -PathType Container)) {
    Write-Host "ERROR: AIoTHackStorm not found." -ForegroundColor Red
    Write-Host "Clone it first:"
    Write-Host "  git clone https://github.com/DDH2004/AIoTHackStorm.git AIoTHackStorm"
    exit 1
}

# ── 2. Find Python ────────────────────────────────────────────────────────────
$PythonCmd = $null

try {
    $ver = & py -3.12 -c "import sys; print(sys.version)" 2>$null
    if ($LASTEXITCODE -eq 0) { $PythonCmd = @("py", "-3.12") }
} catch {}

if (-not $PythonCmd) {
    try {
        $ver = & python -c "import sys; print(sys.version)" 2>$null
        if ($LASTEXITCODE -eq 0) { $PythonCmd = @("python") }
    } catch {}
}

if (-not $PythonCmd) {
    Write-Host "ERROR: Python 3.12 not found." -ForegroundColor Red
    Write-Host "Install Python 3.12 and ensure 'py' or 'python' is in PATH."
    exit 1
}

# ── 3. Recreate the venv ──────────────────────────────────────────────────────
Write-Host "Creating virtual environment..."

$VenvPath = Join-Path $SDK ".venv"
$T5Build = Join-Path $SDK "platform\T5AI\t5_os\build"
$AppBuild = Join-Path $SDK "apps\hackstorm_clock\.build"

if (Test-Path $VenvPath) { Remove-Item $VenvPath -Recurse -Force }
if (Test-Path $T5Build)   { Remove-Item $T5Build -Recurse -Force }
if (Test-Path $AppBuild)  { Remove-Item $AppBuild -Recurse -Force }

& $PythonCmd[0] $PythonCmd[1..($PythonCmd.Length-1)] -m venv $VenvPath

$VenvPython = Join-Path $VenvPath "Scripts\python.exe"
$VenvPip = Join-Path $VenvPath "Scripts\pip.exe"

if (-not (Test-Path $VenvPython)) {
    Write-Host "ERROR: venv python.exe not found at $VenvPython" -ForegroundColor Red
    exit 1
}

# Optional compatibility shim:
# Some build tools may call 'python' explicitly. On Windows, that should
# already resolve inside .venv\Scripts when PATH is updated, so no symlink
# is usually needed.
Write-Host "Installing dependencies..."
& $VenvPip install -q -r (Join-Path $SDK "requirements.txt")

$TyuReq = Join-Path $SDK "tools\tyutool\requirements.txt"
if (Test-Path $TyuReq) {
    & $VenvPip install -q -r $TyuReq
}

# ── 4. Copy firmware into AIoTHackStorm/apps/hackstorm_clock ─────────────────
Write-Host "Copying firmware into AIoTHackStorm..."

$AppDir = Join-Path $SDK "apps\hackstorm_clock"
$SrcDir = Join-Path $AppDir "src"
$IncludeDir = Join-Path $AppDir "include"

New-Item -ItemType Directory -Force -Path $AppDir | Out-Null

if (Test-Path $SrcDir)     { Remove-Item $SrcDir -Recurse -Force }
if (Test-Path $IncludeDir) { Remove-Item $IncludeDir -Recurse -Force }

Copy-Item (Join-Path $Firmware "src") -Destination $AppDir -Recurse -Force

$FirmwareInclude = Join-Path $Firmware "include"
if (Test-Path $FirmwareInclude) {
    Copy-Item $FirmwareInclude -Destination $AppDir -Recurse -Force
}

Copy-Item (Join-Path $Firmware "CMakeLists.txt")     -Destination $AppDir -Force
Copy-Item (Join-Path $Firmware "app_default.config") -Destination $AppDir -Force

Write-Host ""
Write-Host "Setup complete. To build and flash:"
Write-Host "  cd $AppDir"
Write-Host "  `$env:Path = ""$($VenvPath)\Scripts;$env:Path"""
Write-Host "  python $SDK\tos.py build"
Write-Host "  python $SDK\tos.py flash -p COM3"