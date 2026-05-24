#!/usr/bin/env bash
# Usage: source export.sh   (must be sourced, not executed)
# Activates the Python venv created by setup.sh and puts it on PATH.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK="$SCRIPT_DIR/AIoTHackStorm"
VENV="$SDK/.venv"

if [ ! -d "$SDK" ]; then
  echo "ERROR: AIoTHackStorm not found. Run ./setup.sh first."
  return 1
fi

if [ ! -f "$VENV/bin/activate" ]; then
  echo "ERROR: venv not found at $VENV"
  echo "Run ./setup.sh first."
  return 1
fi

source "$VENV/bin/activate"

export OPEN_SDK_ROOT="$SDK"
export PATH="$SDK:$PATH"

echo "TuyaOpen environment ready."
echo "  SDK   : $SDK"
echo "  Python: $(python --version 2>&1)"
