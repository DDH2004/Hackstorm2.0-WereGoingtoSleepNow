# API Reference - HackStorm Smart Alarm Clock

Quick reference for all API endpoints across board, app, and backend.

---

## T5 Board - Local HTTP API

**Base URL**: `http://<board_ip>:8080`  
**Authentication**: None (local network only)  
**Content-Type**: `application/json`

### Alarms

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/alarms/create` | Create new alarm |
| `GET` | `/api/alarms` | List all alarms |
| `GET` | `/api/alarms/{id}` | Get alarm details |
| `PUT` | `/api/alarms/{id}` | Update alarm |
| `DELETE` | `/api/alarms/{id}` | Delete alarm |
| `POST` | `/api/alarms/{id}/trigger` | Manually trigger alarm |

#### POST /api/alarms/create
```json
Request:
{
  "time": "06:30",
  "label": "Morning Alarm",
  "enabled": true,
  "sound": "gentle_bells.wav",
  "repeat": ["MON", "TUE", "WED"],
  "snooze_duration": 5,
  "gradient_enabled": true,
  "light_duration": 30
}

Response (201):
{
  "alarm_id": "550e8400-e29b-41d4-a716-446655440000",
  "created_at": 1726920000,
  "status": "scheduled"
}
```

#### GET /api/alarms
```json
Response (200):
{
  "alarms": [
    {
      "alarm_id": "550e8400-e29b-41d4-a716-446655440000",
      "time": "06:30",
      "label": "Morning Alarm",
      "enabled": true,
      "repeat": ["MON", "TUE", "WED"]
    }
  ]
}
```

#### PUT /api/alarms/{id}
```json
Request:
{
  "time": "07:00",
  "label": "Updated Alarm",
  "enabled": false
}

Response (200):
{
  "alarm_id": "550e8400-e29b-41d4-a716-446655440000",
  "updated_at": 1726920600,
  "status": "updated"
}
```

---

### Time & System

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/status` | Board health & configuration |
| `GET` | `/api/time` | Get current board time |
| `POST` | `/api/time/sync` | Force NTP time synchronization |
| `PUT` | `/api/settings` | Update display/sound settings |

#### GET /api/status
```json
Response (200):
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
  "last_cloud_sync": 1726920000,
  "firmware_version": "1.0.0"
}
```

#### GET /api/time
```json
Response (200):
{
  "timestamp": 1726923645,
  "iso_8601": "2024-09-21T14:34:05Z",
  "epoch_seconds": 1726923645,
  "ntp_synced": true,
  "timezone": "UTC"
}
```

---

### Sleep & Health Data

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/api/sleep/current` | Current sleep session data |
| `POST` | `/api/sleep/end` | Mark sleep session ended |
| `GET` | `/api/health/snore` | Snore detection events |
| `GET` | `/api/health/voice` | Voice memo entries |

#### GET /api/sleep/current
```json
Response (200):
{
  "session_id": "sleep_20240921",
  "start_time": 1726884600,
  "duration_minutes": 120,
  "snore_events": [
    {
      "timestamp": 1726885200,
      "intensity": 2.3,
      "duration_seconds": 5
    },
    {
      "timestamp": 1726885500,
      "intensity": 1.8,
      "duration_seconds": 3
    }
  ],
  "voice_memos": [
    {
      "memo_id": "voice_001",
      "timestamp": 1726888200,
      "duration_seconds": 15,
      "audio_url": "file:///data/voice/memo_001.wav"
    }
  ]
}
```

#### GET /api/health/snore
```json
Response (200):
{
  "today": {
    "total_events": 12,
    "avg_intensity": 2.2,
    "max_intensity": 3.5
  },
  "events": [
    {
      "timestamp": 1726885200,
      "intensity": 2.3,
      "duration_seconds": 5
    }
  ]
}
```

---

## Cloud Backend - REST API

**Base URL**: `https://api.hackstorm.local`  
**Authentication**: Bearer JWT token  
**Content-Type**: `application/json`

### Authentication

| Method | Endpoint | Auth Required |
|--------|----------|---------------|
| `POST` | `/v1/auth/register` | No |
| `POST` | `/v1/auth/login` | No |
| `POST` | `/v1/auth/refresh` | No (uses refresh token) |
| `POST` | `/v1/auth/logout` | Yes |

#### POST /v1/auth/register
```json
Request:
{
  "email": "user@example.com",
  "password": "securepassword123",
  "device_id": "t5_board_001"
}

Response (201):
{
  "user_id": "user_001",
  "email": "user@example.com",
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refresh_token": "refresh_token_here",
  "expires_in": 2592000
}
```

#### POST /v1/auth/login
```json
Request:
{
  "email": "user@example.com",
  "password": "securepassword123"
}

Response (200):
{
  "user_id": "user_001",
  "access_token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refresh_token": "refresh_token_here",
  "expires_in": 2592000
}
```

---

### User Profile & Devices

| Method | Endpoint | Auth Required |
|--------|----------|---------------|
| `GET` | `/v1/user/profile` | Yes |
| `PUT` | `/v1/user/profile` | Yes |
| `GET` | `/v1/user/devices` | Yes |
| `POST` | `/v1/user/devices/pair` | Yes |
| `DELETE` | `/v1/user/devices/{id}` | Yes |

#### GET /v1/user/profile
```
Header: Authorization: Bearer {access_token}

Response (200):
{
  "user_id": "user_001",
  "email": "user@example.com",
  "name": "John Doe",
  "timezone": "America/New_York",
  "preference_notifications": true,
  "created_at": 1700000000
}
```

#### GET /v1/user/devices
```
Header: Authorization: Bearer {access_token}

Response (200):
{
  "devices": [
    {
      "device_id": "t5_board_001",
      "name": "Bedroom Clock",
      "type": "T5_DEVBOARD",
      "firmware_version": "1.0.0",
      "paired_at": 1700000000,
      "last_seen": 1726923600,
      "is_online": true
    }
  ]
}
```

#### POST /v1/user/devices/pair
```json
Request:
{
  "device_id": "t5_board_001",
  "device_secret": "secret_key_from_board",
  "device_name": "Bedroom Clock"
}

Response (201):
{
  "device_id": "t5_board_001",
  "status": "paired",
  "pairing_token": "pairing_token_here"
}
```

---

### Sleep History & Analytics

| Method | Endpoint | Auth Required |
|--------|----------|---------------|
| `GET` | `/v1/sleep/history` | Yes |
| `GET` | `/v1/sleep/{id}` | Yes |
| `GET` | `/v1/analytics/sleep` | Yes |
| `GET` | `/v1/analytics/snore` | Yes |

#### GET /v1/sleep/history
```
Query Parameters:
  ?days=7          (optional, default 7)
  ?device_id=t5_board_001 (optional)

Header: Authorization: Bearer {access_token}

Response (200):
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
    "total_sessions": 7,
    "avg_duration": 445,
    "total_snore_events": 89,
    "avg_quality": 75
  }
}
```

#### GET /v1/analytics/sleep
```
Query Parameters:
  ?period=week     (week, month, year)
  ?device_id=t5_board_001

Response (200):
{
  "period": "week",
  "data": [
    {
      "date": "2024-09-21",
      "duration_minutes": 480,
      "quality_score": 78,
      "snore_events": 12
    }
  ],
  "trends": {
    "duration_trend": 1.2,
    "quality_trend": 0.8,
    "snore_trend": -0.5
  }
}
```

---

### Alarms (Cloud Sync)

| Method | Endpoint | Auth Required |
|--------|----------|---------------|
| `GET` | `/v1/alarms` | Yes |
| `POST` | `/v1/alarms` | Yes |
| `PUT` | `/v1/alarms/{id}` | Yes |
| `DELETE` | `/v1/alarms/{id}` | Yes |
| `POST` | `/v1/alarms/{id}/sync` | Yes |

#### GET /v1/alarms
```
Header: Authorization: Bearer {access_token}

Response (200):
{
  "alarms": [
    {
      "alarm_id": "550e8400-e29b-41d4-a716-446655440000",
      "device_id": "t5_board_001",
      "time": "06:30",
      "label": "Morning Alarm",
      "enabled": true,
      "repeat": ["MON", "TUE", "WED"],
      "synced_at": 1726920000,
      "created_at": 1726900000
    }
  ]
}
```

#### POST /v1/alarms/{id}/sync
```
Force sync alarm to board immediately

Header: Authorization: Bearer {access_token}

Response (200):
{
  "alarm_id": "550e8400-e29b-41d4-a716-446655440000",
  "synced_at": 1726923645,
  "status": "synced_to_board"
}
```

---

### Voice & Transcription

| Method | Endpoint | Auth Required |
|--------|----------|---------------|
| `POST` | `/v1/voice/transcribe` | Yes |
| `GET` | `/v1/voice/memos` | Yes |
| `GET` | `/v1/voice/{id}` | Yes |

#### POST /v1/voice/transcribe
```json
Request:
{
  "audio_base64": "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR...",
  "audio_format": "wav",
  "language": "en"
}

Response (200):
{
  "transcription": "I had a dream about flying over mountains",
  "confidence": 0.92,
  "emotion": "peaceful",
  "duration_seconds": 15
}
```

---

## Device Sync Endpoint

| Method | Endpoint | Auth Required |
|--------|----------|---------------|
| `POST` | `/v1/device/sync` | Yes (Device JWT) |

#### POST /v1/device/sync
```json
Request:
{
  "device_id": "t5_board_001",
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
      "data": {
        "duration_minutes": 290,
        "snore_events": 12
      }
    }
  ]
}

Response (200):
{
  "status": "synced",
  "conflicts": [],
  "last_sync": 1726923600,
  "next_sync_after": 1726937000
}
```

---

## Error Response Format

All endpoints use consistent error format:

```json
{
  "error": {
    "code": "INVALID_REQUEST",
    "message": "Request validation failed",
    "details": [
      {
        "field": "time",
        "message": "Invalid time format"
      }
    ]
  }
}
```

### Common Error Codes

| Code | HTTP | Description |
|------|------|-------------|
| `INVALID_REQUEST` | 400 | Request validation failed |
| `UNAUTHORIZED` | 401 | Missing or invalid auth token |
| `FORBIDDEN` | 403 | User doesn't have permission |
| `NOT_FOUND` | 404 | Resource not found |
| `CONFLICT` | 409 | Data conflict (e.g., duplicate alarm) |
| `INTERNAL_ERROR` | 500 | Server error |
| `SERVICE_UNAVAILABLE` | 503 | Service temporarily unavailable |

---

## Rate Limiting

All cloud endpoints are rate-limited:

- **App API**: 100 requests/minute per user
- **Device API**: 10 requests/minute per device
- **Public endpoints**: 30 requests/minute per IP

Rate limit headers:
```
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 87
X-RateLimit-Reset: 1726923645
```

---

## Pagination

List endpoints support pagination:

```
GET /v1/sleep/history?page=1&limit=20

Response:
{
  "data": [...],
  "pagination": {
    "page": 1,
    "limit": 20,
    "total": 156,
    "pages": 8
  }
}
```

---

## Testing with cURL

### Board - Create Alarm
```bash
curl -X POST http://192.168.1.100:8080/api/alarms/create \
  -H "Content-Type: application/json" \
  -d '{
    "time": "06:30",
    "label": "Morning Alarm",
    "enabled": true
  }'
```

### Cloud - Login
```bash
curl -X POST https://api.hackstorm.local/v1/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "user@example.com",
    "password": "securepassword123"
  }'
```

### Cloud - Get Sleep History
```bash
curl -X GET https://api.hackstorm.local/v1/sleep/history?days=7 \
  -H "Authorization: Bearer {access_token}"
```

---

## OpenAPI/Swagger

Full OpenAPI 3.0 specification available at:
- **Board API**: `http://<board_ip>:8080/api/openapi.json`
- **Cloud API**: `https://api.hackstorm.local/openapi.json`

View in Swagger UI:
```
https://swagger.io/tools/swagger-ui/?url=http://<board_ip>:8080/api/openapi.json
```
