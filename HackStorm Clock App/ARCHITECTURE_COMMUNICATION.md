# HackStorm Smart Alarm Clock - Communication Architecture

## System Overview

```
┌─────────────────┐         WiFi          ┌──────────────────┐
│  Flutter App    │ ◄────────────────────► │  T5 Devboard     │
│  (Mobile)       │                        │  (Smart Clock)   │
└────────┬────────┘                        └──────────────────┘
         │                                          │
         │                                          │
         │              HTTPS/REST                  │ NTP Time Sync
         │        ┌───────────────────┐             │
         └───────►│  Custom Backend   │◄────────────┘
                  │  (Node/Python)    │
                  │                   │
                  │ • User Auth       │
                  │ • Alarm Store     │
                  │ • Sleep Analytics │
                  │ • Voice Processing│
                  └───────────────────┘
```

---

## Communication Layers

### Layer 1: Direct WiFi (Local Network)
**Purpose**: Real-time communication, low latency, works offline  
**Protocol**: HTTP REST / WebSocket  
**When Active**: Board and app on same WiFi network

### Layer 2: Cloud Sync (Custom Backend)
**Purpose**: Persistent storage, analytics, voice processing  
**Protocol**: HTTPS REST  
**When Active**: Periodic sync (every X hours), when offline restoration needed

---

## Data Flow Scenarios

### Scenario 1: Real-Time Control (Board & App on Same WiFi)

```
User sets alarm on app
        ↓
App sends POST /api/alarms/create
        ↓
T5 Board receives → Updates display immediately
        ↓
Both devices show same alarm ✓
```

**Endpoint**: `POST http://<board_ip>:8080/api/alarms/create`

```json
{
  "alarm_id": "uuid",
  "time": "06:30",
  "label": "Morning Alarm",
  "enabled": true,
  "sound": "gentle_bells.wav",
  "snooze_interval": 5
}
```

---

### Scenario 2: Periodic Cloud Sync (Background)

```
Every X hours:
  ↓
Board collects data:
  • Sleep metrics
  • Snore detections
  • Last successful sync time
  ↓
POST http://<backend_url>/api/device/sync
  ↓
Backend stores data
  ↓
App queries backend for updates
  ↓
App displays analytics dashboard
```

**Endpoint**: `POST https://api.hackstorm.local/v1/device/sync`

```json
{
  "device_id": "t5_board_001",
  "session_timestamp": 1726920000,
  "sleep_data": {
    "duration_minutes": 480,
    "snore_events": 12,
    "snore_intensity": [2.3, 1.8, 3.1],
    "heart_rate_avg": 62,
    "movement_count": 3
  },
  "alarms_synced": [
    {
      "alarm_id": "uuid_123",
      "triggered_at": 1726920000,
      "dismissed_at": 1726920180
    }
  ],
  "timestamp": 1726923600
}
```

---

### Scenario 3: WiFi Failure Recovery

```
WiFi drops → Board goes offline
        ↓
Board continues:
  • Display time (RTC backup)
  • Record snore/voice locally
  • Store alarms in NVS flash
        ↓
WiFi restores → Board detects connection
        ↓
Board syncs all offline data to backend
        ↓
App pulls missed data from backend
        ↓
Data restored ✓
```

---

## T5 Board API Endpoints

### Local HTTP Server (runs on board)

**Base URL**: `http://<board_ip>:8080`

#### Alarms Management

```
POST   /api/alarms/create       # Create new alarm
GET    /api/alarms              # List all alarms
GET    /api/alarms/{id}         # Get alarm details
PUT    /api/alarms/{id}         # Update alarm
DELETE /api/alarms/{id}         # Delete alarm
POST   /api/alarms/{id}/trigger # Manually trigger alarm
```

**Create Alarm**:
```http
POST /api/alarms/create
Content-Type: application/json

{
  "time": "06:30",
  "label": "Morning Alarm",
  "enabled": true,
  "sound": "gentle_bells.wav",
  "repeat": ["MON", "TUE", "WED", "THU", "FRI"],
  "snooze_duration": 5,
  "gradient_enabled": true,
  "light_duration": 30
}

Response 201:
{
  "alarm_id": "550e8400-e29b-41d4-a716-446655440000",
  "created_at": 1726920000,
  "status": "scheduled"
}
```

#### Time & System Status

```
GET    /api/status              # Board health & config
PUT    /api/settings            # Update display/sound settings
GET    /api/time                # Get current board time
POST   /api/time/sync           # Force NTP sync
```

**Get Status**:
```http
GET /api/status

Response 200:
{
  "device_id": "t5_board_001",
  "ip_address": "192.168.1.100",
  "ssid": "HomeNetwork",
  "uptime_seconds": 86400,
  "wifi_signal_strength": -45,
  "time": "2024-09-21T14:30:45Z",
  "display": {
    "width": 320,
    "height": 480,
    "brightness": 100
  },
  "storage": {
    "free_flash_kb": 512,
    "used_nvs_kb": 128
  },
  "alarms_count": 3,
  "last_cloud_sync": 1726920000
}
```

#### Sleep & Health Data

```
GET    /api/sleep/current       # Current sleep session data
POST   /api/sleep/end           # Mark sleep session ended
GET    /api/health/snore        # Snore events
GET    /api/health/voice        # Dream journal entries
```

**Get Current Sleep**:
```http
GET /api/sleep/current

Response 200:
{
  "session_id": "sleep_20240921",
  "start_time": 1726884600,
  "duration_minutes": 120,
  "snore_events": [
    {
      "timestamp": 1726885200,
      "intensity": 2.3,
      "duration_seconds": 5
    }
  ],
  "voice_memos": [
    {
      "timestamp": 1726888200,
      "duration_seconds": 15,
      "audio_url": "file:///data/voice/memo_001.wav"
    }
  ]
}
```

---

## Flutter App → Backend Endpoints

**Base URL**: `https://api.hackstorm.local`

### Authentication

```
POST   /v1/auth/register        # User registration
POST   /v1/auth/login           # User login
POST   /v1/auth/refresh         # Refresh auth token
POST   /v1/auth/logout          # Logout
```

### User Profile & Devices

```
GET    /v1/user/profile         # Get user info
PUT    /v1/user/profile         # Update profile
GET    /v1/user/devices         # List paired devices
POST   /v1/user/devices/pair    # Pair new board
DELETE /v1/user/devices/{id}    # Unpair device
```

### Sleep History & Analytics

```
GET    /v1/sleep/history        # Get sleep sessions
GET    /v1/sleep/{id}           # Get session details
GET    /v1/analytics/sleep      # Sleep analytics (week/month)
GET    /v1/analytics/snore      # Snore patterns
POST   /v1/voice/transcribe     # Transcribe dream journal
```

**Get Sleep History**:
```http
GET /v1/sleep/history?days=7&device_id=t5_board_001

Response 200:
{
  "sessions": [
    {
      "session_id": "sleep_20240921",
      "device_id": "t5_board_001",
      "start_time": 1726884600,
      "end_time": 1726902000,
      "duration_minutes": 290,
      "snore_events": 12,
      "snore_score": 65,
      "quality_score": 78,
      "notes": ""
    }
  ],
  "summary": {
    "avg_duration": 445,
    "total_snore_events": 89,
    "avg_quality": 75
  }
}
```

### Alarms (Cloud-Synced)

```
GET    /v1/alarms               # Get all alarms
POST   /v1/alarms               # Create alarm
PUT    /v1/alarms/{id}          # Update alarm
DELETE /v1/alarms/{id}          # Delete alarm
POST   /v1/alarms/{id}/sync     # Force sync to board
```

---

## Board-to-Backend Communication

### Periodic Sync Interval

**Default**: Every 6 hours (or configurable)

```
Sync triggers:
1. Scheduled (every X hours)
2. On WiFi connection restored
3. When storage threshold reached
4. On user-initiated "sync now"
5. Before sleep session ends
```

### Sync Payload Structure

```json
{
  "device_id": "t5_board_001",
  "auth_token": "jwt_token_here",
  "sync_version": 1,
  "timestamp": 1726923600,
  "events": [
    {
      "type": "alarm_triggered",
      "alarm_id": "uuid_123",
      "triggered_at": 1726920000,
      "dismissed_at": 1726920180
    },
    {
      "type": "sleep_session",
      "session_id": "sleep_20240921",
      "start_time": 1726884600,
      "end_time": 1726902000,
      "data": { /* sleep metrics */ }
    },
    {
      "type": "snore_event",
      "timestamp": 1726885200,
      "intensity": 2.3,
      "duration_seconds": 5
    },
    {
      "type": "voice_memo",
      "timestamp": 1726888200,
      "audio_base64": "...",
      "duration_seconds": 15
    }
  ]
}
```

---

## Offline Storage (Board Side)

When WiFi fails, board stores locally using NVS (Non-Volatile Storage):

```
NVS Namespace: "alarms"
  └─ alarm_001 = {...}
  └─ alarm_002 = {...}

NVS Namespace: "sleep_data"
  └─ session_20240921_data.json (max 4KB per session)

NVS Namespace: "snore_events"
  └─ events_[timestamp].json

NVS Namespace: "voice_memos"
  └─ memo_001.wav (stored on PSRAM/SD card)
  └─ memo_manifest.json (index of memos)
```

**Implementation** (in C):
```c
#include "tal_kv.h"

// Save alarm locally
OPERATE_RET save_alarm_local(const char *alarm_json)
{
    return tal_kv_set("alarms", "alarm_001", 
                      (const uint8_t *)alarm_json, 
                      strlen(alarm_json));
}

// Retrieve alarm locally
uint8_t *get_alarm_local(const char *alarm_id)
{
    uint32_t len = 0;
    return tal_kv_get("alarms", alarm_id, &len);
}
```

---

## Authentication & Security

### Device Authentication

Each board has:
- **Device ID**: Unique identifier (burned at factory)
- **Device Secret**: Private key (stored securely in NVS)
- **JWT Token**: Generated on first cloud connection

**Flow**:
```
1. Board → Backend: POST /auth/device_register
   { device_id, device_secret }
   
2. Backend → Board: JWT token (valid 30 days)
   
3. Board uses JWT for all future requests
   
4. Token refresh: POST /auth/refresh before expiry
```

### App Authentication

```
1. User registers: POST /auth/register
   { email, password, device_id }
   
2. Backend creates user account, links to device
   
3. User login: POST /auth/login
   { email, password }
   
4. Backend returns JWT + refresh token
   
5. App stores JWT in secure storage (Keychain/Keystore)
   
6. All app requests include: Authorization: Bearer {jwt}
```

### HTTPS & Encryption

```
├─ All cloud endpoints: HTTPS/TLS 1.3
├─ Device auth tokens: Signed with RS256
├─ Sensitive data: AES-256-GCM encrypted at rest
├─ Voice memos: Server-side encryption
└─ Board-to-board: mDNS + TLS optional (for advanced setup)
```

---

## Network Discovery (Automatic Board Connection)

### Method 1: mDNS (Recommended)

Board advertises itself via mDNS:

```c
// Board announces itself
// Service name: _hackstorm._tcp.local.
// Instance: "HackStorm_Clock_[device_id]"
// Port: 8080

// App discovers board
// Look for services: _hackstorm._tcp.local.
// Connect to announced IP:port
```

**Flutter Implementation**:
```dart
import 'package:multicast_dns/multicast_dns.dart';

Future<String?> discoverBoard() async {
  final client = MDnsClient();
  final query = ResourceRecordQuery.serviceInstance('_hackstorm._tcp');
  
  await for (final result in client.lookup(query)) {
    if (result.name.contains('hackstorm')) {
      return result.targets.first.name.replaceAll('.local.', '');
    }
  }
  return null;
}
```

### Method 2: Manual IP Entry

User manually enters board IP (192.168.1.100:8080)

### Method 3: Cloud-Assisted Discovery

Board registers its IP with backend when connecting:

```
Board: POST https://api.hackstorm.local/device/register_ip
  { device_id, current_ip }

App: GET https://api.hackstorm.local/device/ip/{device_id}
  Response: { ip: "192.168.1.100", port: 8080 }
```

---

## Error Handling & Retry Strategy

### Network Failures

```
Board attempts to sync:
  ↓
Request fails (no internet)
  ↓
Store event in NVS queue
  ↓
Retry with exponential backoff:
  • 1st retry: 1 minute
  • 2nd retry: 5 minutes
  • 3rd retry: 30 minutes
  • 4th retry: 2 hours
  ↓
Once WiFi restored: flush queue
```

**Code** (C):
```c
typedef struct {
    uint32_t retry_count;
    uint32_t last_retry_time;
    uint32_t next_retry_time;
} sync_retry_t;

uint32_t calculate_next_retry(sync_retry_t *state)
{
    uint32_t backoff[] = {60, 300, 1800, 7200}; // seconds
    uint32_t idx = MIN(state->retry_count, 3);
    return state->last_retry_time + backoff[idx];
}
```

### Sync Conflicts

If board and app both modify same alarm:

```
Timestamp comparison:
  ↓
Later timestamp wins
  ↓
Loser's version stored as "conflicted_[id]"
  ↓
User notified to resolve
```

---

## Data Sync State Machine

```
┌─────────────┐
│   IDLE      │
└──────┬──────┘
       │
       │ WiFi connected + Sync interval
       ↓
┌──────────────┐      Failure      ┌──────────────┐
│ SYNCING      │─────────────────→ │ SYNC_FAILED  │
└──────┬───────┘                   └──────┬───────┘
       │                                   │
       │ Success                           │ Retry interval
       │                                   │
       ↓                                   ↓
┌──────────────┐                   ┌──────────────┐
│ SYNCED       │                   │ RETRY        │
└──────┬───────┘                   └──────┬───────┘
       │                                   │
       │ New data                          │
       └───────────────┬────────────────────┘
                       │
                       ↓
                  (back to SYNCING)
```

---

## Testing Communication

### Mock Backend (for development)

```python
# Python Flask server for testing
from flask import Flask, jsonify, request

app = Flask(__name__)

@app.route('/v1/sleep/history', methods=['GET'])
def get_sleep_history():
    return jsonify({
        "sessions": [
            {
                "session_id": "test_001",
                "duration_minutes": 480,
                "snore_score": 65
            }
        ]
    })

@app.route('/api/alarms/create', methods=['POST'])
def create_alarm():
    data = request.json
    return jsonify({"alarm_id": "uuid_123", "status": "scheduled"}), 201
```

### Test Cases

1. **Normal Sync**: WiFi up → sync succeeds → data persisted
2. **Offline Mode**: WiFi down → store locally → WiFi up → sync
3. **Concurrent Updates**: App and board both modify alarm → conflict resolution
4. **Large Payload**: 1000+ snore events → chunk transfers
5. **Connection Loss**: Mid-sync interruption → resume on reconnect

---

## Implementation Checklist

### T5 Board (C/ESP-IDF)

- [ ] Local HTTP server running on port 8080
- [ ] mDNS service advertisement
- [ ] JWT token management
- [ ] NVS offline storage
- [ ] Periodic cloud sync (configurable interval)
- [ ] Exponential backoff retry logic
- [ ] Voice memo recording to PSRAM/SD
- [ ] NTP time sync on boot
- [ ] Timestamp validation (detect clock skew)

### Flutter App (Dart)

- [ ] Board discovery (mDNS or manual IP)
- [ ] Direct WiFi API calls to board
- [ ] Cloud API client with JWT handling
- [ ] Secure token storage (Keychain/Keystore)
- [ ] Conflict resolution UI
- [ ] Offline queue & sync status indicator
- [ ] Voice transcription service integration
- [ ] Sleep analytics dashboard

### Backend (Node.js/Python)

- [ ] User authentication & JWT generation
- [ ] Device registration & pairing
- [ ] Sleep data aggregation & analytics
- [ ] Voice transcription (Whisper/similar)
- [ ] Data encryption at rest
- [ ] API rate limiting & monitoring
- [ ] Cloud storage (S3 for audio files)
- [ ] Scheduled cleanup (old sessions)

---

## Example: Complete Alarm Create Flow

### Step 1: User creates alarm in app
```dart
// Flutter App
final alarm = {
  'time': '06:30',
  'label': 'Morning Alarm',
  'enabled': true,
};

// Try direct board connection first
try {
  final boardIp = await discoverBoard();
  final response = await http.post(
    Uri.parse('http://$boardIp:8080/api/alarms/create'),
    headers: {'Content-Type': 'application/json'},
    body: jsonEncode(alarm),
  );
  
  if (response.statusCode == 201) {
    // Success! Display confirmation
    showSnackBar('Alarm set on clock');
  }
} on SocketException {
  // No WiFi connection, sync via cloud
  await createAlarmViaCloud(alarm);
}
```

### Step 2: Board receives and displays
```c
// T5 Board HTTP Handler
static cJSON *handle_alarm_create(cJSON *request_json)
{
    // Parse request
    char *time = cJSON_GetStringValue(cJSON_GetObjectItem(request_json, "time"));
    
    // Store alarm
    save_alarm_local(time);
    
    // Update display (draw new alarm on clock)
    display_add_alarm(time);
    
    // Queue for cloud sync
    queue_sync_event("alarm_created", request_json);
    
    // Return success
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "alarm_id", generate_uuid());
    cJSON_AddNumberToObject(response, "status", 201);
    
    return response;
}
```

### Step 3: Periodic sync to backend
```c
// T5 Board background task
void sync_to_backend(void)
{
    // Get queued events
    cJSON *events = get_sync_queue();
    
    // Create sync payload
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "device_id", DEVICE_ID);
    cJSON_AddItemToObject(payload, "events", events);
    
    // POST to backend
    char response[4096];
    http_client_post(BACKEND_URL "/device/sync", 
                     jwt_token, payload, response);
    
    // Clear queue on success
    if (response_status == 200) {
        clear_sync_queue();
    }
}
```

---

## Next Steps

1. **Design API contracts** (OpenAPI/Swagger)
2. **Implement board HTTP server** (port 8080)
3. **Build Flutter API client layer**
4. **Create mock backend** for testing
5. **Test offline scenarios** extensively
6. **Add monitoring & logging** for production
7. **Security audit** before launch
