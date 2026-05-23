# Architecture & Workflow Overview

> **📡 For detailed communication protocol between app, board, and backend, see [ARCHITECTURE_COMMUNICATION.md](ARCHITECTURE_COMMUNICATION.md)**

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         MOBILE APP (Flutter)                    │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ UI Screens                                               │   │
│  │  • Home (nav hub + alarm dismiss)                        │   │
│  │  • Alarm Setup (time picker + ringtone)                 │   │
│  │  • Sleep Summary (score + dream recording)              │   │
│  │  • Sleep History (calendar view)                        │   │
│  │  • Dream Journal (past entries)                         │   │
│  └──────────────────────────────────────────────────────────┘   │
│           ↓                                                      │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ AppState (Provider)                                      │   │
│  │  • Manages alarm, sleep data, dream journal            │   │
│  │  • Coordinates between screens                         │   │
│  └──────────────────────────────────────────────────────────┘   │
│           ↓                              ↓                       │
│  ┌─────────────────────┐    ┌──────────────────────────────┐    │
│  │  ApiService         │    │  TuyaService                 │    │
│  │  (Cloud API calls)  │    │  (Board SDK integration)     │    │
│  │ [TODO] HTTP stubs   │    │ [TODO] MQTT/SDK setup       │    │
│  └─────────────────────┘    └──────────────────────────────┘    │
│           ↓                              ↓                       │
└───────────┼──────────────────────────────┼───────────────────────┘
            │                              │
            ↓                              ↓
     ┌────────────────────┐       ┌──────────────────┐
     │  CLOUD BACKEND     │       │  TUYA BOARD      │
     │  (Node.js/Python)  │←─────→│  (Hardware Hub)  │
     │  [TODO] Implement  │       │  • Camera driver │
     │                    │       │  • Audio record  │
     │  REST API          │       │  • MQTT client   │
     │  Firebase/MongoDB  │       │  • Display time  │
     │  MQTT broker       │       └──────────────────┘
     └────────────────────┘
            ↓
     ┌────────────────────────────────────┐
     │  CLOUD ML SERVICES                 │
     │  • Google Speech-to-Text           │
     │  • Google Vision API               │
     │  • TensorFlow Lite (snoring)       │
     └────────────────────────────────────┘
```

---

## User Journey Timeline

### EVENING (Setup)

```
7:00 PM
├─ User opens app
├─ Taps "Set Alarm"
├─ Picks time: 7:00 AM
├─ Selects ringtone: "Gentle Sunrise"
├─ Taps "Save Alarm"
│
├─ [FLOW]
│  app → ApiService.postAlarm()
│       → [TODO] HTTP POST /api/alarms
│       → Cloud Backend
│       → Publishes to MQTT: tuya/alarm/config
│       → Tuya Board receives
│       → Board displays: "7:00 AM - Gentle Sunrise"
│
└─ ✅ Alarm is set & displayed on board
```

### NIGHT (Sleep Tracking)

```
7:00 PM - 7:00 AM
├─ Tuya Board:
│  ├─ Captures camera frames every 10 seconds
│  ├─ Records audio continuously (circular 8-hour buffer)
│  ├─ Stores in local buffer
│
└─ Cloud Backend (async):
   ├─ Polls for camera frames
   ├─ Analyzes for: bed occupied, movement patterns
   ├─ Polls for audio segments
   ├─ Analyzes for: snoring events
```

### MORNING (7:00 AM)

```
7:00 AM
├─ Tuya Board alarm triggers
├─ Plays ringtone: "Gentle Sunrise"
├─ Publishes MQTT: tuya/alarm/triggered
│
├─ App receives (if listening) [TODO: implement listener]
├─ Shows DISMISS button on screen
│
├─ User taps DISMISS
│
├─ [FLOW]
│  app → AppState.dismissAlarm()
│       → ApiService.dismissAlarm()
│       → [TODO] HTTP POST /api/alarm/dismiss
│       → Cloud Backend
│       → Publishes MQTT: tuya/alarm/dismiss
│       → Tuya Board receives
│       → Board says: "What did you dream?"
│       → Board records 30-second audio
│
└─ ✅ User is dismissed, board waiting for dream
```

### MORNING (Dream Recording)

```
7:01 AM
├─ User speaks their dream for 30 seconds
├─ Board records audio
├─ (Or app records from phone mic) [TODO: uncomment audio recording]
│
├─ [FLOW]
│  app → SleepSummaryScreen.recordDream()
│       → Records 30s audio from phone mic
│       → ApiService.uploadDreamAudio()
│       → [TODO] HTTP POST /api/dream/audio (multipart)
│       → Cloud Backend
│       → Calls Google Cloud Speech-to-Text API
│       → Transcription: "I was flying over a mountain..."
│       → Stores in database
│       → Returns transcription to app
│
├─ SleepSummaryScreen displays dream text
│
└─ ✅ Dream is recorded & transcribed
```

### MORNING-AFTERNOON (Sleep Analytics - Async)

```
7:00 AM - 2:00 PM (Cloud Backend, background job)
├─ Retrieve last night's data from board:
│  ├─ All camera frames (420 frames at 10s intervals)
│  ├─ Full audio buffer (8 hours)
│
├─ Cloud Vision API:
│  ├─ Analyze camera frames
│  ├─ Detect: bed occupancy, movement intensity
│  ├─ Count: wake-up events (movement bursts)
│  └─ Result: 2 wake-ups detected
│
├─ TensorFlow Lite (snoring classifier):
│  ├─ Segment audio into 10-second chunks
│  ├─ Classify each chunk: "snoring" or "silent"
│  ├─ Count snoring events
│  └─ Result: 8 snoring events detected
│
├─ Calculate Sleep Score:
│  Base:              100
│  - (2 × wakeUps):   - 4  (2 wake-ups × 2 points each)
│  - (1 × snoring):   - 8  (8 snoring events × 1 point each)
│  ────────────────────────
│  Final Score:        82  ✅
│
├─ Store in database:
│  {
│    userId: "demo-user",
│    date: "2024-05-22",
│    sleepScore: 82,
│    totalTime: 450,           // minutes (7h30m)
│    wakeUps: 2,
│    snoringMeter: 42.5,       // dB average
│    dreamTranscription: "I was flying over a mountain...",
│  }
│
└─ ✅ Analytics complete
```

### MORNING (Sleep Summary View)

```
8:00 AM (Next day)
├─ User opens app
├─ Taps "Last Night"
├─ SleepSummaryScreen loads:
│  ├─ [FLOW]
│  │  app → ApiService.getSleepSummary(userId)
│  │       → [TODO] HTTP GET /api/sleep/summary/userId
│  │       → Cloud Backend returns aggregated data
│  │       → Populates SleepSummary object
│  │
│  └─ [STUB DATA for offline dev]
│     Returns:
│     ├─ Sleep Score: 82 (great!)
│     ├─ Total Sleep: 7h 23m
│     ├─ Wake-ups: 2
│     ├─ Snore Meter: 42.5 dB
│     └─ Dream: "I was flying over a mountain..."
│
└─ ✅ User sees full sleep breakdown + dream
```

### ANY TIME (History & Calendar)

```
├─ User taps "History"
│  ├─ [FLOW]
│  │  app → ApiService.getSleepHistory(userId)
│  │       → [TODO] HTTP GET /api/sleep/history/userId (past 30 days)
│  │       → Cloud Backend returns array of SleepSummary
│  │
│  └─ Calendar displays with color-coded days
│     ├─ 🟢 Green: Score ≥ 80 (great sleep)
│     ├─ 🟡 Orange: Score 60-79 (decent sleep)
│     └─ 🔴 Red: Score < 60 (poor sleep)
│
│  User taps a day to see full breakdown
│
└─ ✅ See sleep trends over time

├─ User taps "Dream Journal"
│  ├─ [FLOW]
│  │  app → ApiService.getDreamJournal(userId)
│  │       → [TODO] HTTP GET /api/dreams/userId
│  │       → Cloud Backend returns list of DreamEntry
│  │
│  └─ Displays all past dream transcriptions
│
└─ ✅ Browse dream history
```

---

## File-by-File Responsibility

| File | Handles | Key Methods |
|------|---------|-------------|
| `main.dart` | App entry, theme setup | `main()` |
| `app_state.dart` | Central state (Provider) | `saveAlarm()`, `dismissAlarm()`, `loadSleepSummary()` |
| `api_service.dart` | All HTTP calls | `postAlarm()`, `getSleepSummary()`, `uploadDreamAudio()` |
| `tuya_service.dart` | Board SDK integration | `sendAlarmToBoard()`, `listenForAlarmTrigger()` |
| `home_screen.dart` | Navigation hub | Displays alarm, nav grid, dismiss button |
| `alarm_setup_screen.dart` | Time picker, ringtone | Saves alarm config |
| `sleep_summary_screen.dart` | Sleep data display, dream recording | Shows score, stats, records voice |
| `sleep_history_screen.dart` | Calendar view | Displays past sleep data |
| `dream_journal_screen.dart` | Dream history list | Shows transcriptions |

---

## Integration Checklist (in order)

1. **Make sure app runs** → `flutter run` ✅
2. **Set API base URL** → `config/api_config.dart` → `ApiConfig.baseUrl`
3. **Once backend is live:**
   - Uncomment HTTP calls in `api_service.dart`
   - Test each endpoint one by one
4. **Once Tuya board firmware is ready:**
   - Fill in `TuyaConfig` credentials in `api_config.dart`
   - Uncomment SDK init in `tuya_service.dart`
   - Test board communication
5. **End-to-end test:**
   - Set alarm on app → board displays it
   - Dismiss at alarm time → board prompts for dream
   - Record dream → app displays transcription
   - Check calendar next morning → shows sleep score

---

## Key Data Models

### Alarm
```dart
Alarm {
  userId: String,
  alarmTime: DateTime,      // e.g. 7:00 AM tomorrow
  ringtone: String,         // e.g. "Gentle Sunrise"
  enabled: bool,            // whether alarm is active
}
```

### SleepSummary
```dart
SleepSummary {
  sleepScore: int,          // 0-100 (calculated by backend)
  totalTime: Duration,      // e.g. 7h 23m
  wakeUps: int,             // number of wake events
  snoringMeter: double,     // dB level
  dreamTranscription: String?,  // speech-to-text result
  date: DateTime,           // which night
}
```

### DreamEntry
```dart
DreamEntry {
  date: DateTime,
  transcription: String,    // speech-to-text of dream audio
}
```

---

## MQTT Topics (Board ↔ Backend)

| Topic | Direction | Payload | Meaning |
|-------|-----------|---------|---------|
| `tuya/alarm/config` | Cloud → Board | `{alarmTime, ringtone}` | Configure alarm on board |
| `tuya/alarm/dismiss` | Cloud → Board | (empty) | Tell board user dismissed |
| `tuya/alarm/triggered` | Board → Cloud | (empty) | Board says alarm went off |
| `tuya/dream/audio` | Board → Cloud | (audio file or path) | Dream recording from board |

---

## Common Patterns in the Code

### Pattern 1: API Call with Stub
```dart
// Real call (commented out for offline dev)
// final response = await http.post(...);

// Stub (active for demo)
await Future.delayed(const Duration(milliseconds: 500));
print('[STUB] POST /api/alarms');
return true;
```

### Pattern 2: State Update
```dart
Future<void> loadSleepSummary() async {
  isLoading = true;
  notifyListeners();  // UI rebuilds with loading spinner
  
  lastSleepSummary = await _api.getSleepSummary(userId);
  
  isLoading = false;
  notifyListeners();  // UI rebuilds with data
}
```

### Pattern 3: Navigation
```dart
Navigator.push(context, MaterialPageRoute(
  builder: (_) => const SleepSummaryScreen(),
));
```

---

## Glossary

| Term | Definition |
|------|-----------|
| **AppState** | Provider-based central state for entire app |
| **ApiService** | Singleton that handles all HTTP calls to backend |
| **TuyaService** | Singleton that handles all board communication (MQTT/SDK) |
| **Hot Reload** | Edit code → save → app auto-refreshes (no rebuild) |
| **Stub** | Fake data returned during development (offline) |
| **MQTT** | Lightweight pub/sub protocol for board ↔ cloud messages |
| **Speech-to-Text** | Google Cloud API that converts audio to text |
| **Sleep Score** | 0-100 number calculated by: `100 - (2 × wakeUps) - (1 × snoringEvents)` |

---

## Notes

- **All APIs return stub data** until you uncomment real calls
- **App is fully functional offline** for UI development
- **No backend required** to test UI, navigation, forms
- **MQTT topics must match** between board firmware and cloud backend
- **Sleep score formula** is provided in the docs but can be adjusted

---

Good luck! 🎯
