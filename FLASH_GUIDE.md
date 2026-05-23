# Flash Guide — HackStorm Clock (Tuya T5AI)

## Requirements

- macOS (Apple Silicon — M1/M2/M3/M4)
- Python 3.12 — download from [python.org](https://www.python.org/downloads/mac-osx/)
- T5AI board connected via USB

---

## Step 1 — Clone the repos

```bash
git clone https://github.com/YOUR_ORG/HackStormClockApp.git
cd HackStormClockApp
git clone https://github.com/DDH2004/AIoTHackStorm.git AIoTHackStorm
```

---

## Step 2 — Run setup (one time only)

```bash
./setup.sh
```

This creates the Python 3.12 venv, installs all dependencies, and copies the firmware into place.

---

## Step 3 — Activate the environment

```bash
source export.sh
```

Run this in every new terminal session before building or flashing.

---

## Step 4 — Build

```bash
cd AIoTHackStorm/apps/hackstorm_clock
python ../../tos.py build
```

A successful build ends with:

```
Target    : hackstorm_clock_QIO_1.0.0.bin
Chip      : T5AI
Board     : TUYA_T5AI_BOARD
```

---

## Step 5 — Find your USB port

```bash
ls /dev/cu.*
```

Look for an entry like `/dev/cu.usbmodem5AAE1670581`. Ignore `Bluetooth` and `debug-console`.

> If two `usbmodem` ports appear, use the lower-numbered one.

---

## Step 6 — Flash

```bash
python ../../tos.py flash -p /dev/cu.YOURPORT
```

Replace `YOURPORT` with the port from Step 5. A successful flash ends with:

```
[INFO]: Flash write success.
```

---

## Step 7 — Monitor (optional)

```bash
python ../../tos.py monitor -p /dev/cu.YOURPORT --baud 460800
```

Press `Ctrl+C` to exit.

---

## Rebuild after code changes

```bash
source export.sh
cd AIoTHackStorm/apps/hackstorm_clock
python ../../tos.py build
python ../../tos.py flash -p /dev/cu.YOURPORT
```

---

## WiFi credentials

Edit [board_firmware/src/main.c](board_firmware/src/main.c) lines 27–28, re-run `./setup.sh`, then rebuild:

```c
#define WIFI_SSID     "YourNetwork"
#define WIFI_PASSWORD "YourPassword"
```

> The T5AI only supports **2.4 GHz** networks.

---

## Troubleshooting

| Problem | Fix |
|---|---|
| `setup.sh: command not found` | Run `chmod +x setup.sh` first |
| `python3.12: command not found` in setup.sh | Install Python 3.12 from python.org |
| `Get bus error` during flash | Try the other `cu.usbmodem*` port |
| Black screen on board | Confirm `board_firmware/app_default.config` has `CONFIG_TUYA_T5AI_BOARD_EX_MODULE_35565LCD=y` |
| WiFi not connecting | Check SSID/password and confirm the network is 2.4 GHz |
