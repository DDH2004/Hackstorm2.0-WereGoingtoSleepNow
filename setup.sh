#!/usr/bin/env bash
# Run once after cloning: ./setup.sh
# Sets up the Python venv and copies firmware into AIoTHackStorm.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SDK="$SCRIPT_DIR/AIoTHackStorm"
FIRMWARE="$SCRIPT_DIR/board_firmware"

# ── 1. Check AIoTHackStorm exists ─────────────────────────────────────────────
if [ ! -d "$SDK" ]; then
  echo "ERROR: AIoTHackStorm not found."
  echo "Clone it first:"
  echo "  git clone https://github.com/DDH2004/AIoTHackStorm.git AIoTHackStorm"
  exit 1
fi

# ── 2. Recreate the venv ───────────────────────────────────────────────────────
# Wipe stale build caches too: they bake in absolute cmake/python paths from
# the old venv, so they must be regenerated alongside the new venv.
echo "Creating virtual environment..."
rm -rf "$SDK/.venv"
rm -rf "$SDK/platform/T5AI/t5_os/build"
rm -rf "$SDK/apps/hackstorm_clock/.build"
python3.12 -m venv "$SDK/.venv"

# CMake calls 'python' (not 'python3') to detect arm64 vs x86_64.
# Without this symlink the wrong toolchain is selected and the build fails.
ln -sf "$(python3.12 -c 'import sys; print(sys.executable)')" "$SDK/.venv/bin/python"

echo "Installing dependencies..."
"$SDK/.venv/bin/pip" install -q -r "$SDK/requirements.txt"

# Install tyutool (flash utility) dependencies if tyutool is already present.
# tos.py flash auto-clones tyutool on first run, but won't re-install its deps
# when the venv is recreated. Install them now to avoid a missing-module error.
if [ -f "$SDK/tools/tyutool/requirements.txt" ]; then
  "$SDK/.venv/bin/pip" install -q -r "$SDK/tools/tyutool/requirements.txt"
fi

# ── 3. Copy firmware into AIoTHackStorm/apps/hackstorm_clock ─────────────────
# app_default.config selects the board + LCD module. cmake reads it on first
# build and generates the full config (using.config) from scratch.
# We never run 'tos.py config choice' here — that command overwrites
# app_default.config with the bare board defaults, erasing our LCD selection.
echo "Copying firmware into AIoTHackStorm..."
mkdir -p "$SDK/apps/hackstorm_clock"
rm -rf "$SDK/apps/hackstorm_clock/src"
rm -rf "$SDK/apps/hackstorm_clock/include"
cp -r "$FIRMWARE/src"                "$SDK/apps/hackstorm_clock/"
cp -r "$FIRMWARE/include"            "$SDK/apps/hackstorm_clock/" 2>/dev/null || true
cp    "$FIRMWARE/CMakeLists.txt"     "$SDK/apps/hackstorm_clock/"
cp    "$FIRMWARE/app_default.config" "$SDK/apps/hackstorm_clock/"

echo ""
echo "Setup complete. To build and flash:"
echo "  cd $SDK/apps/hackstorm_clock"
echo "  export PATH=\"$SDK/.venv/bin:\$PATH\""
echo "  python $SDK/tos.py build"
echo "  python $SDK/tos.py flash -p /dev/cu.YOURPORT"
