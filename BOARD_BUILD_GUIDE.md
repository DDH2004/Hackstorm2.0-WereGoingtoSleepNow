# HackStorm Clock - T5-E1 Board Build Guide

Build and flash the HackStorm clock app to your Tuya T5-E1-IPEX board.

## Prerequisites

✅ Board connected via USB to Mac  
✅ WiFi credentials updated in `AIoTHackStorm/apps/hackstorm_clock/src/main.c`  
✅ TuyaOpen environment set up

## Quick Build & Flash

### Step 1: Set Up TuyaOpen Environment

```bash
cd ~/Desktop/HackStorm\ Clock\ App
source export.sh
```

Expected output:
```
✓ Using python3
✓ Virtual environment activated
✓ Dependencies installed
✓ TuyaOpen environment ready!
```

### Step 2: Navigate to HackStorm Clock App

```bash
cd AIoTHackStorm
```

### Step 3: Build for T5-E1

```bash
python tos.py build -b hackstorm_clock -t t5 -p build_output
```

Parameters:
- `-b hackstorm_clock` - App name (located in apps/hackstorm_clock/)
- `-t t5` - Target board (T5-E1 module)
- `-p build_output` - Output directory

### Step 4: Flash to Board

Find your USB port:
```bash
ls /dev/tty.*
```

Flash the firmware:
```bash
python tos.py flash -p /dev/tty.YOUR_PORT -i build_output/hackstorm_clock.bin
```

Example:
```bash
python tos.py flash -p /dev/tty.usbserial-1410 -i build_output/hackstorm_clock.bin
```

### Step 5: Monitor Serial Output

```bash
python tos.py monitor -p /dev/tty.YOUR_PORT --baud 460800
```

**Note**: T5-E1 uses 460800 baud (not 115200 like ESP32)

Expected output:
```
========================================
HackStorm Smart Alarm Clock v1.0
========================================
[1/5] Initializing system services...
[2/5] Initializing display...
[3/5] Initializing WiFi...
[WiFi] Connected to JJ Lake!
[4/5] Synchronizing time via NTP...
[5/5] Starting application threads...
========================================
HackStorm Smart Alarm Clock - Ready!
========================================
```

---

## Troubleshooting

### "Build failed"

Check if the app directory exists:
```bash
ls AIoTHackStorm/apps/hackstorm_clock/
```

If not, the files may not have been created properly. Check the paths.

### "Port not found"

Make sure your board is connected and recognized:
```bash
system_profiler SPUSBDataType | grep -i "Tuya\|T5\|Serial"
```

### "Permission denied" on flash

```bash
sudo chmod 666 /dev/tty.YOUR_PORT
```

### "Time not synchronizing"

This is normal on first boot. NTP sync happens automatically once WiFi connects.
Check the serial monitor for the current time once WiFi is connected.

### WiFi won't connect

1. Verify SSID and password in `apps/hackstorm_clock/src/main.c`
2. Make sure network is 2.4 GHz (not 5 GHz)
3. Check WiFi is broadcasting the SSID

---

## Development Workflow

To rebuild after making changes:

```bash
# Make changes to src/main.c
cd AIoTHackStorm

# Rebuild
python tos.py build -b hackstorm_clock -t t5 -p build_output

# Flash
python tos.py flash -p /dev/tty.YOUR_PORT -i build_output/hackstorm_clock.bin

# Monitor
python tos.py monitor -p /dev/tty.YOUR_PORT --baud 460800
```

---

## What's Running

Once flashed and booted:

- **Display**: Shows time, updates every 1 second
- **WiFi**: Auto-connects to configured network
- **Time Sync**: Syncs via NTP once WiFi connects  
- **HTTP Server**: Runs on port 8080 (placeholder for now)

---

## Next Steps

Once this is working:

1. ✅ Board displays time
2. ✅ Board connected to WiFi  
3. ✅ Board responds to `curl` requests (add HTTP handlers)
4. → Build Flutter app to discover and control board

See: [ARCHITECTURE_COMMUNICATION.md](ARCHITECTURE_COMMUNICATION.md)

---

## References

- [Tuya T5-E1 Datasheet](https://developer.tuya.com/en/docs/iot/T5-E1-Module-Datasheet?id=Kdar6hf0kzmfi)
- [TuyaOpen Documentation](https://tuyaopen.ai/docs)
- [VR Avatar Project](AIoTHackStorm/apps/vr_avatar/) - Reference implementation
