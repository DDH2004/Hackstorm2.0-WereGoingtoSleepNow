# HackStorm Clock - Board Firmware

Minimal T5 board application that displays time and exposes a simple HTTP API.

## Features

✅ **Display Time** - Shows HH:MM:SS on 320x480 LCD  
✅ **WiFi Connectivity** - Connects to home WiFi  
✅ **NTP Time Sync** - Syncs time from internet  
✅ **HTTP API** - JSON API on port 8080  
✅ **mDNS Discovery** - Board discoverable as `hackstorm.local`

## Quick Start

### Prerequisites

- T5 Devboard with USB cable connected to Mac
- TuyaOpen SDK (AIoTHackStorm cloned locally)
- `esp-idf` tools (installed via TuyaOpen)

### Step 1: Set Your WiFi Credentials

Edit `src/main.c` and update:

```c
#define WIFI_SSID     "YOUR_SSID_HERE"
#define WIFI_PASSWORD "YOUR_PASSWORD_HERE"
```

### Step 2: Set Up TuyaOpen Environment

```bash
cd ~/Desktop/HackStorm\ Clock\ App/AIoTHackStorm
source export.sh
```

This sets up the SDK paths and tools.

### Step 3: Build the Firmware

```bash
cd ~/Desktop/HackStorm\ Clock\ App/board_firmware

# Configure for your T5 board
idf.py set-target esp32s3  # or appropriate chip

# Build
idf.py build
```

**Build output**: `build/hackstorm_clock.bin`

### Step 4: Flash to Board

```bash
# Find your USB port
ls /dev/tty.*  # Look for something like /dev/tty.usbserial-XXXXX

# Flash
idf.py -p /dev/tty.usbserial-XXXXX flash
```

### Step 5: Monitor Serial Output

```bash
idf.py -p /dev/tty.usbserial-XXXXX monitor
```

You should see:
```
[startup] Initializing display...
[startup] Initializing WiFi...
[WiFi] Connected to YOUR_SSID_HERE
[startup] HTTP Server listening on port 8080
```

---

## Testing the Board

### Test 1: Verify WiFi Connection

```bash
# Find the board's IP on your network
arp-scan -l | grep -i "ESP32\|Tuya"

# Or use mDNS
avahi-browse -a | grep hackstorm
```

### Test 2: Check Time Endpoint

```bash
curl http://<board_ip>:8080/api/time

# Example response:
{
  "timestamp": 1726923645,
  "iso_8601": "2024-09-21T14:34:05Z",
  "time": {
    "hour": 14,
    "minute": 34,
    "second": 5,
    "day": 21,
    "month": 9,
    "year": 2024
  },
  "ntp_synced": true,
  "timezone": "UTC"
}
```

### Test 3: Check Status Endpoint

```bash
curl http://<board_ip>:8080/api/status

# Example response:
{
  "device_id": "t5_clock_001",
  "device_name": "HackStorm Clock",
  "version": "1.0.0",
  "display": {
    "width": 320,
    "height": 480,
    "brightness": 100
  }
}
```

---

## Project Structure

```
board_firmware/
├── CMakeLists.txt              # Build configuration
├── include/
│   ├── display_manager.h       # Display API
│   ├── http_server.h           # HTTP server
│   └── time_manager.h          # Time sync
├── src/
│   ├── main.c                  # Application entry point
│   ├── display_manager.c       # Display implementation
│   ├── http_server.c           # HTTP server implementation
│   └── time_manager.c          # Time sync implementation
└── README.md                   # This file
```

---

## API Endpoints

### GET /api/status

Returns board information and configuration.

```
curl http://192.168.1.100:8080/api/status
```

**Response**: 200 OK
```json
{
  "device_id": "t5_clock_001",
  "device_name": "HackStorm Clock",
  "version": "1.0.0",
  "uptime_seconds": 3600,
  "timestamp": 1726923645,
  "display_enabled": true,
  "display": {
    "width": 320,
    "height": 480,
    "brightness": 100
  },
  "storage": {
    "total_kb": 1024,
    "available_kb": 512
  }
}
```

### GET /api/time

Returns current time in multiple formats.

```
curl http://192.168.1.100:8080/api/time
```

**Response**: 200 OK
```json
{
  "timestamp": 1726923645,
  "iso_8601": "2024-09-21T14:34:05Z",
  "time": {
    "hour": 14,
    "minute": 34,
    "second": 5,
    "day": 21,
    "month": 9,
    "year": 2024
  },
  "ntp_synced": true,
  "timezone": "UTC"
}
```

---

## Troubleshooting

### Board won't connect to WiFi

1. Double-check SSID and password in `src/main.c`
2. Ensure WiFi network is 2.4 GHz (not 5 GHz)
3. Check serial monitor for error messages
4. Try:
   ```bash
   idf.py erase-flash
   idf.py flash
   ```

### Can't find board on network

1. Check the serial monitor - you should see IP address:
   ```
   [WiFi] Connected! IP: 192.168.1.100
   ```

2. Try mDNS discovery:
   ```bash
   ping hackstorm.local
   ```

3. If that doesn't work, use arp-scan:
   ```bash
   sudo arp-scan -l | grep ESP32
   ```

### HTTP requests timeout

1. Make sure board IP is correct: `curl http://<ip>:8080/api/status`
2. Check firewall isn't blocking port 8080
3. Try from another device on same WiFi
4. Check serial monitor for errors

### Display not showing time

1. Check display is initialized in serial output
2. Verify `board_register_hardware()` is called
3. Check if your T5 board variant has the correct display name (might not be `lcd_disp`)

---

## Next Steps

Once this firmware is working:

1. ✅ Board displays time on LCD
2. ✅ Board responds to HTTP requests
3. ✅ Board is discoverable on local network

Then build the Flutter app to:
- Discover the board automatically
- Query time and status
- Later add alarm management

See: [ARCHITECTURE_COMMUNICATION.md](../ARCHITECTURE_COMMUNICATION.md)

---

## Development Notes

### Adding New API Endpoints

1. Add handler function in `http_server.c`:
   ```c
   static void handle_my_endpoint(int socket) {
       // Create JSON response
       // Send via send_http_response()
   }
   ```

2. Route in `handle_http_request()`:
   ```c
   else if (strcmp(path, "/api/my_endpoint") == 0) {
       handle_my_endpoint(socket);
   }
   ```

### Increasing Display Update Rate

In `main.c`, `display_update_thread()`:
```c
// Default: update every 1000ms
// Change to 500ms for smoother animation
tal_system_sleep(500);
```

### Testing with Mock Backend

While board is running, test Flutter app with board HTTP API:

```dart
// Flutter app code
final response = await http.get(
  Uri.parse('http://192.168.1.100:8080/api/time'),
);
```

---

## References

- [DISPLAY_TIME_GUIDE.md](../DISPLAY_TIME_GUIDE.md) - Detailed display API usage
- [ARCHITECTURE_COMMUNICATION.md](../ARCHITECTURE_COMMUNICATION.md) - Full system architecture
- [TuyaOpen Docs](https://tuyaopen.ai/docs) - SDK documentation
