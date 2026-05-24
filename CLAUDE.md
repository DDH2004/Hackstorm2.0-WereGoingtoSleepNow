# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

HackStorm 2.0 is a Smart Alarm Clock sleep intelligence platform consisting of two components:

1. **`app/`** — Flutter mobile app (iOS/Android) for alarm setup, sleep summaries, and dream journaling
2. **`board_firmware/`** — C firmware for the Tuya T5AI hardware board (320×480 LCD, WiFi, HTTP server)

The app currently runs fully offline with stubbed API responses. The board firmware currently displays a clock and exposes an HTTP server on port 8080.

## Flutter App Commands

```bash
cd app

# Install dependencies
flutter pub get

# Run (iOS simulator by default)
flutter run

# Run on specific target
flutter run -d ios          # physical iPhone
flutter run -d android

# Clean build artifacts
flutter clean && flutter pub get && flutter run

# Run tests
flutter test

# Run a single test file
flutter test test/widget_test.dart
```

## Board Firmware Commands

**One-time setup** (run from repo root after cloning AIoTHackStorm):
```bash
./setup.sh
```
Requires Python 3.12 exactly. Creates a venv at `AIoTHackStorm/.venv`, installs deps, and copies `board_firmware/` into `AIoTHackStorm/apps/hackstorm_clock/`.

**Every new terminal session** before building/flashing:
```bash
source export.sh
```

**Build:**
```bash
cd AIoTHackStorm/apps/hackstorm_clock
python ../../tos.py build
```

**Flash:**
```bash
# Find USB port first
ls /dev/cu.*                          # macOS
ls /dev/ttyUSB* /dev/ttyACM*         # Linux

python ../../tos.py flash -p /dev/cu.YOURPORT
```

**Monitor serial output:**
```bash
python ../../tos.py monitor -p /dev/cu.YOURPORT --baud 460800
```

**After code changes in `board_firmware/`:** re-run `./setup.sh` (copies files), then rebuild and flash.

## Architecture

### Flutter App (`app/lib/`)

State flows through a single `AppState` Provider → UI screens consume it:

```
HomeScreen (nav hub + alarm dismiss)
  ├── AlarmSetupScreen → AppState.saveAlarm() → ApiService.postAlarm()
  ├── SleepSummaryScreen → AppState.loadSleepSummary() → ApiService.getSleepSummary()
  ├── SleepHistoryScreen → ApiService.getSleepHistory()
  └── DreamJournalScreen → ApiService.getDreamJournal()

AppState (Provider, lib/services/app_state.dart)
  ├── ApiService  — all HTTP calls (lib/services/api_service.dart)
  └── TuyaService — board SDK integration (lib/services/tuya_service.dart)
```

Key files:
- `lib/config/api_config.dart` — **fill this first**: `ApiConfig.baseUrl`, `TuyaConfig` credentials
- `lib/services/api_service.dart` — all HTTP calls currently stubbed; search `TODO [API]` to activate
- `lib/services/tuya_service.dart` — Tuya SDK calls stubbed; search `TODO [TUYA]` to activate
- `lib/screens/sleep_summary_screen.dart` — audio recording stubbed; search `TODO [AUDIO]`

Integration markers: `TODO [API]` (10 locations), `TODO [TUYA]` (8 locations), `TODO [AUDIO]` (1 location).

### Board Firmware (`board_firmware/src/main.c`)

Single-file C application using Tuya Abstraction Layer (TAL) APIs:

1. Initializes system services (KV store, timers, work queue, NTP, CLI)
2. Opens the LCD display and allocates two PSRAM frame buffers for double-buffering
3. Connects to WiFi (hardcoded SSID/password at lines 27–28)
4. Syncs time via NTP
5. Spawns two threads: `display_update_thread` (updates LCD every second) and `http_server_thread` (placeholder on port 8080)

Display rendering uses `tdl_disp_draw_fill()` to paint white rectangles representing digit positions — actual digit rendering is not yet implemented (`(void)tm;` comment at line 149).

**WiFi credentials** are in `board_firmware/src/main.c` lines 27–28. The T5AI only supports 2.4 GHz networks.

### Communication Protocol

MQTT topics (defined but not yet implemented in firmware):

| Topic | Direction | Purpose |
|---|---|---|
| `tuya/alarm/config` | Cloud → Board | Set alarm time + ringtone |
| `tuya/alarm/dismiss` | Cloud → Board | User dismissed alarm |
| `tuya/alarm/triggered` | Board → Cloud | Alarm went off |
| `tuya/dream/audio` | Board → Cloud | Dream recording upload |

Sleep score formula: `100 - (2 × wakeUps) - (1 × snoringEvents)`

### SDK Structure

`AIoTHackStorm/` is the Tuya TuyaOpen SDK (cloned separately). The firmware app lives at `AIoTHackStorm/apps/hackstorm_clock/` (populated by `setup.sh`). Build configuration is in `board_firmware/app_default.config`; the key flag is `CONFIG_TUYA_T5AI_BOARD_EX_MODULE_35565LCD=y` for the LCD module.
