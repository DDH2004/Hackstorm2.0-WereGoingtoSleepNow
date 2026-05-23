# HackStorm 2.0: Smart Alarm Clock
## Sleep Intelligence Platform

A Flutter mobile app for a hackathon project that tracks sleep, detects snoring, captures dreams, and displays sleep analytics.

---

## 📁 Project Structure

```
HackStorm Clock App/
├── README.md                          ← You are here
├── SETUP_MACOS_M4.md                  ← Installation & run guide (start here!)
├── app/                               ← Flutter mobile app
│   ├── lib/
│   │   ├── main.dart                  ← Entry point
│   │   ├── config/api_config.dart     ← Fill this first! (URLs, API keys, Tuya creds)
│   │   ├── models/                    ← Data models (Alarm, SleepSummary, DreamEntry)
│   │   ├── services/
│   │   │   ├── api_service.dart       ← Cloud API calls (all stubbed)
│   │   │   ├── tuya_service.dart      ← Tuya Board SDK integration
│   │   │   └── app_state.dart         ← Central state (Provider)
│   │   ├── screens/
│   │   │   ├── home_screen.dart       ← Main navigation hub
│   │   │   ├── alarm_setup_screen.dart    ← Time picker + ringtone
│   │   │   ├── sleep_summary_screen.dart  ← Sleep score + dream recording
│   │   │   ├── sleep_history_screen.dart  ← Calendar view
│   │   │   └── dream_journal_screen.dart  ← Dream entries list
│   │   └── widgets/
│   │       └── sleep_score_card.dart
│   ├── assets/ringtones/              ← Drop .mp3 files here
│   └── pubspec.yaml                   ← Dependencies (flutter pub get)
└── backend/                           ← (To be created) Node.js/Python cloud API
```

---

## 🚀 Quick Start (5 minutes)

### 1. Install Flutter & Dependencies
Follow **[SETUP_MACOS_M4.md](SETUP_MACOS_M4.md)** — it's tailored for M4 Macs.

### 2. Run the App

```bash
cd HackStorm\ Clock\ App/app
flutter pub get
flutter run
```

The app will launch in the iOS simulator. ✨

### 3. Explore the UI

You can:
- Set an alarm ✅
- See a mock sleep summary ✅
- View a calendar of sleep history ✅
- Read dream journal entries ✅

**Everything works offline** with stubbed data.

---

## 🔗 Integration Checklist

The app is **scaffold-only** — designed for parallel development. Here's what needs filling in:

### Phase 1: Configuration (Do First)
- [ ] **Edit `lib/config/api_config.dart`**
  - [ ] Set `ApiConfig.baseUrl` to your cloud backend
  - [ ] Fill in `TuyaConfig` with board credentials

### Phase 2: Backend API (Once Cloud is Ready)
- [ ] **Uncomment real HTTP calls in `lib/services/api_service.dart`**
  - [ ] `postAlarm()` → sends alarm to backend
  - [ ] `dismissAlarm()` → tells backend user dismissed
  - [ ] `getSleepSummary()` → fetch last night's data
  - [ ] `uploadDreamAudio()` → upload voice recording
  - [ ] `getSleepHistory()` → calendar data

| API Endpoint | Method | Purpose | Status |
|---|---|---|---|
| `/api/alarms` | POST | Store alarm config, relay to board via MQTT | TODO [API] |
| `/api/alarm/dismiss` | POST | Mark dismissed, trigger dream prompt on board | TODO [API] |
| `/api/sleep/summary/{userId}` | GET | Return aggregated sleep data (score, stats) | TODO [API] |
| `/api/dream/audio` | POST | Upload dream audio, return speech-to-text | TODO [API] |
| `/api/sleep/history/{userId}` | GET | Return past 30 days of sleep data | TODO [API] |

### Phase 3: Tuya Board Integration
- [ ] **Uncomment Tuya SDK code in `lib/services/tuya_service.dart`**
  - [ ] Initialize Tuya SDK with credentials
  - [ ] Send alarm config to board
  - [ ] Listen for alarm-triggered signal
  - [ ] Send dismiss command

### Phase 4: Audio Recording
- [ ] **Uncomment audio recording in `lib/screens/sleep_summary_screen.dart`**
  - [ ] Record 30s of dream audio from phone mic
  - [ ] Upload to backend

---

## 📍 Find Integration Points Quickly

Search the codebase for these markers:

```
TODO [API]     ← Cloud backend integration (10 locations)
TODO [TUYA]    ← Tuya Board communication (8 locations)
TODO [AUDIO]   ← Microphone recording (1 location)
```

Example in `api_service.dart`:
```dart
Future<bool> postAlarm(Alarm alarm) async {
  // TODO [API]: Uncomment when backend is live
  // final response = await http.post(...);
  
  // STUB: Simulate success
  await Future.delayed(const Duration(milliseconds: 500));
  return true;
}
```

---

## 🔄 Data Flow (Evening → Morning)

### Evening: Setting Alarm
```
User picks time & ringtone in AlarmSetupScreen
  ↓
saveAlarm() in AppState
  ↓
apiService.postAlarm()  [TODO: POST to /api/alarms]
  ↓
Backend stores + publishes to MQTT topic: tuya/alarm/config
  ↓
Tuya Board receives config & displays alarm time
```

### Morning: Dismissing & Recording Dream
```
Tuya Board alarm triggers
  ↓
Board publishes to MQTT: tuya/alarm/triggered
  ↓
App shows dismiss button in HomeScreen  [TODO: Listen for MQTT]
  ↓
User taps "DISMISS"
  ↓
dismissAlarm() in AppState
  ↓
apiService.dismissAlarm()  [TODO: POST to /api/alarm/dismiss]
  ↓
Backend publishes to board: tuya/alarm/dismiss
  ↓
Board prompts: "What did you dream?"
  ↓
User records 30s audio → uploadDreamAudio()  [TODO: Uncomment audio recording]
  ↓
apiService.uploadDreamAudio()  [TODO: POST to /api/dream/audio]
  ↓
Backend calls Google Cloud Speech-to-Text
  ↓
Dream transcription stored & returned to app
  ↓
SleepSummaryScreen displays transcription
```

### Post-Sleep: Analytics
```
Backend analyzes overnight data:
  - Camera frames → movement/wake detection (Cloud Vision API)
  - Audio segments → snoring classification (TensorFlow Lite or custom model)
  - Dream audio → speech-to-text (Google Speech API)
  ↓
Computes sleep score: 100 - (2 × wakeUps) - (1 × snoringEvents)
  ↓
apiService.getSleepSummary()  [TODO: GET /api/sleep/summary]
  ↓
SleepSummaryScreen displays: score, sleep time, wake-ups, snoring meter
  ↓
Calendar & history updated via getSleepHistory()  [TODO: GET /api/sleep/history]
```

---

## 🛠 Development Tips

### Hot Reload (Your Friend!)
Edit any `.dart` file and save → app auto-refreshes. No rebuild needed.

```bash
flutter run
# ... app is running in simulator
# Edit lib/screens/home_screen.dart
# Save
# App instantly reloads!
```

### Debugging
- **View console logs:** Flutter console shows all `print()` statements
- **Debug breakpoints:** Use VS Code + Flutter extension
- **DevTools:** Run `flutter run -d all` then visit DevTools URL

### Stub Data
All API responses return fake data for offline development:
- Sleep summary: 82 score, 7h23m sleep, 2 wake-ups, 42.5dB snoring
- Dream history: 3 sample dreams
- Calendar: 7 days of mock data

Replace with real HTTP calls once backend is live.

---

## 📦 Dependencies

Key packages in `pubspec.yaml`:
- **flutter** — Core framework
- **provider** — State management
- **http** — REST API calls
- **table_calendar** — Calendar widget
- **record** — Audio recording from mic
- **audioplayers** — Play ringtones
- **shared_preferences** — Local alarm cache
- **intl** — Date formatting

Install with:
```bash
flutter pub get
```

---

## 🎨 UI/UX Notes

- **Color scheme:** Dark mode (purple `#7C4DFF` accent)
- **Typography:** Clean, readable
- **Responsive:** Works on all screen sizes
- **Offline-first:** App fully functional without internet

All screens are production-quality for a 48-hour hackathon demo.

---

## 🚨 Important Notes for Your Team

1. **The app is FULLY FUNCTIONAL right now** — use it to demo the UI to judges
2. **All API calls return stub data** — no backend needed for UI testing
3. **Once your backend is up**, uncomment the real HTTP calls (marked with `TODO [API]`)
4. **Tuya Board integration** is optional for the MVP — can be mocked
5. **Audio recording** is stubbed — uncomment when `record` package is tested

---

## 📋 Hackathon Deliverables Checklist

- [x] **Code**
  - [x] Fully scaffolded Flutter app
  - [x] Clean file structure
  - [x] Documented with TODOs
  - [x] Working offline

- [ ] **Backend** (Your team)
  - [ ] Node.js/Python REST API
  - [ ] Firebase or MongoDB
  - [ ] MQTT pub/sub for board

- [ ] **Tuya Board Firmware** (Your team)
  - [ ] Camera driver
  - [ ] Audio recording
  - [ ] MQTT client
  - [ ] Dream prompt

- [ ] **Integration** (Day 2)
  - [ ] Fill in API config
  - [ ] Uncomment real API calls
  - [ ] Test end-to-end

- [ ] **Demo** (Final)
  - [ ] Set alarm on app
  - [ ] Board displays time
  - [ ] Dismiss alarm
  - [ ] Show sleep summary

---

## 🎯 Success Criteria

✅ App runs on M4 Mac
✅ All 5 screens are navigable
✅ Alarm setup works
✅ Sleep summary shows mock data
✅ Calendar and dream journal display
✅ Ready for backend integration

---

## 📞 Quick Help

**Q: Where do I change the API URL?**  
A: `lib/config/api_config.dart` → `ApiConfig.baseUrl`

**Q: The app won't compile**  
A: Run `flutter clean && flutter pub get && flutter run`

**Q: How do I add a ringtone?**  
A: Drop a `.mp3` file in `assets/ringtones/` and add it to `pubspec.yaml`

**Q: Can I run this on a physical iPhone?**  
A: Yes! `flutter run -d ios` (requires provisioning profile)

**Q: How long until the backend API calls work?**  
A: Just uncomment lines in `api_service.dart` once your cloud backend is live. ~2 minutes.

---

## 📚 Resources

- **Flutter docs:** https://flutter.dev/docs
- **Provider docs:** https://pub.dev/packages/provider
- **Tuya SDK:** https://developer.tuya.com/en/docs/app-development/sdk-development
- **iOS setup guide:** https://flutter.dev/docs/get-started/install/macos

---

Good luck with the hackathon! 🚀 You've got this.
