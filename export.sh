#!/usr/bin/env bash

# Usage: . ./export.sh
#
# This script sets up the TuyaOpen SDK environment for HackStorm development
# It must be sourced, not executed: . ./export.sh

# Get the directory where this script is located
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# Check if AIoTHackStorm exists
if [ ! -d "$SCRIPT_DIR/AIoTHackStorm" ]; then
    echo "Error: AIoTHackStorm directory not found at $SCRIPT_DIR/AIoTHackStorm"
    echo "Make sure AIoTHackStorm is cloned in the project root."
    return 1
fi

# Point to AIoTHackStorm's export script
IOTDASH_ROOT="$SCRIPT_DIR/AIoTHackStorm"

echo "=================================================="
echo "HackStorm Clock App - TuyaOpen SDK Setup"
echo "=================================================="
echo "Project root: $SCRIPT_DIR"
echo "SDK root: $IOTDASH_ROOT"
echo ""

# Check if virtual environment is already activated
if [ -n "$VIRTUAL_ENV" ] && [ "$VIRTUAL_ENV" = "$IOTDASH_ROOT/.venv" ]; then
    echo "✓ Virtual environment already activated"
    return 0
fi

# Function to check Python version
check_python_version() {
    local python_cmd="$1"
    if command -v "$python_cmd" >/dev/null 2>&1; then
        local version=$($python_cmd -c "import sys; print('.'.join(map(str, sys.version_info[:3])))" 2>/dev/null)
        if [ $? -eq 0 ]; then
            local major=$(echo "$version" | cut -d. -f1)
            local minor=$(echo "$version" | cut -d. -f2)
            # Check if version >= 3.6.0
            if [ "$major" -eq 3 ] && [ "$minor" -ge 6 ]; then
                echo "$python_cmd"
                return 0
            elif [ "$major" -gt 3 ]; then
                echo "$python_cmd"
                return 0
            fi
        fi
    fi
    return 1
}

# Determine which Python command to use
PYTHON_CMD=""
if check_python_version "python3" >/dev/null 2>&1; then
    PYTHON_CMD=$(check_python_version "python3")
    echo "✓ Using python3 ($(python3 --version 2>&1))"
elif check_python_version "python" >/dev/null 2>&1; then
    PYTHON_CMD=$(check_python_version "python")
    echo "✓ Using python ($(python --version 2>&1))"
else
    echo "✗ Error: No suitable Python version found!"
    echo "  Please install Python 3.6.0 or higher."
    return 1
fi

# Create virtual environment if it doesn't exist
if [ ! -d "$IOTDASH_ROOT/.venv" ]; then
    echo ""
    echo "Creating virtual environment..."
    $PYTHON_CMD -m venv "$IOTDASH_ROOT/.venv"
    if [ $? -ne 0 ]; then
        echo "✗ Error: Failed to create virtual environment!"
        return 1
    fi
    echo "✓ Virtual environment created"
else
    echo "✓ Virtual environment already exists"
fi

# Verify virtual environment
if [ ! -f "$IOTDASH_ROOT/.venv/bin/activate" ]; then
    echo "✗ Error: Virtual environment activation script not found"
    return 1
fi

# Change to SDK directory for setup
cd "$IOTDASH_ROOT"

# Activate virtual environment
echo ""
echo "Activating virtual environment..."
source "$IOTDASH_ROOT/.venv/bin/activate"

if [ -z "$VIRTUAL_ENV" ]; then
    echo "✗ Error: Failed to activate virtual environment"
    return 1
fi

echo "✓ Virtual environment activated: $VIRTUAL_ENV"

# Export environment variables
export OPEN_SDK_ROOT="$IOTDASH_ROOT"
export OPEN_SDK_PYTHON="${IOTDASH_ROOT}/.venv/bin/python"
export OPEN_SDK_PIP="${IOTDASH_ROOT}/.venv/bin/pip"
export PATH="$PATH:${IOTDASH_ROOT}"

# Install dependencies
echo ""
echo "Installing dependencies..."
pip install -q -r "${IOTDASH_ROOT}/requirements.txt" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ Dependencies installed"
else
    echo "⚠ Warning: Some dependencies may not have installed"
fi

# Clean cache files
CACHE_PATH="${IOTDASH_ROOT}/.cache"
mkdir -p "${CACHE_PATH}"
rm -f "${CACHE_PATH}/.env.json"
rm -f "${CACHE_PATH}/.dont_prompt_update_platform"

# Setup bash completion for tos.py
if [ -f "${IOTDASH_ROOT}/tos.py" ]; then
    eval "$(bash -c '_TOS_PY_COMPLETE=bash_source ${IOTDASH_ROOT}/tos.py' 2>/dev/null)" || true
fi

# Return to original directory
cd "$SCRIPT_DIR"

# Display summary
echo ""
echo "=================================================="
echo "✓ TuyaOpen environment ready!"
echo "=================================================="
echo ""
echo "Next steps:"
echo "  1. cd board_firmware"
echo "  2. idf.py set-target esp32s3"
echo "  3. idf.py build"
echo "  4. idf.py -p /dev/tty.YOUR_PORT flash"
echo ""
echo "To deactivate: deactivate"
echo "=================================================="
