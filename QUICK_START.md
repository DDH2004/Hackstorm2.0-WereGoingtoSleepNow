# ⚡ Quick Start Cheat Sheet

## 📚 First Time? Read These:

1. **[ARCHITECTURE.md](ARCHITECTURE.md)** - System overview
2. **[ARCHITECTURE_COMMUNICATION.md](ARCHITECTURE_COMMUNICATION.md)** - How app, board, and cloud talk
3. **[API_REFERENCE.md](API_REFERENCE.md)** - API endpoints quick lookup
4. **[SETUP_MACOS.md](SETUP_MACOS.md)** - Detailed board setup
5. **[DISPLAY_TIME_GUIDE.md](DISPLAY_TIME_GUIDE.md)** - How to display time on T5 board

---

## 30-Second Setup (M4 Mac)

### If Flutter is NOT installed:
```bash
brew install flutter
flutter doctor
# Follow any recommendations from doctor
```

### Run the app:
```bash
cd ~/Desktop/HackStorm\ Clock\ App/app
flutter pub get
flutter run
```

✨ App launches in iOS simulator. Done!

---

## Key Files to Know

| File | What to do |
|------|-----------|
| `lib/config/api_config.dart` | ⚠️ **FILL THIS FIRST** → API URL + Tuya credentials |
| `lib/services/api_service.dart` | **Uncomment HTTP calls** when backend is live |
| `lib/services/tuya_service.dart` | **Uncomment SDK setup** when using Tuya board |
| `lib/main.dart` | Entry point (read the comments for quick start) |

---

## Development Workflow

```
┌─────────────────────────────────────────────┐
│ 1. Edit code in any .dart file              │
│ 2. Save (⌘S)                                │
│ 3. App auto-reloads in simulator            │ ← Hot reload!
│ 4. See changes instantly                    │
└─────────────────────────────────────────────┘
```

If hot reload fails: `flutter run --no-fast-start`

---

## Common Commands

```bash
# Run on iOS simulator (default)
flutter run

# Run on macOS desktop
flutter run -d macos

# Run on physical iPhone
flutter run -d ios

# List available devices
flutter devices

# Clear all build cache (fixes most issues)
flutter clean

# Install dependencies
flutter pub get

# Check environment setup
flutter doctor
```

---

## Find Integration Points Quickly

Search for these in the code:

```
TODO [API]     ← Cloud backend HTTP calls (10 places)
TODO [TUYA]    ← Tuya Board setup (8 places)
TODO [AUDIO]   ← Microphone recording (1 place)
```

Each one is ready to uncomment once your team finishes that component.

---

## Project Status

✅ **Complete & Working:**
- All 5 screens fully built
- State management set up
- Navigation working
- UI/UX production-ready
- Offline data loaded

⏳ **Waiting for Your Team:**
- [ ] Cloud backend REST API
- [ ] Tuya board firmware
- [ ] MQTT broker
- [ ] Database (Firebase/MongoDB)

🔗 **Integration (1-2 Hours):**
- Uncomment API calls
- Fill in credentials
- Test end-to-end

---

## Screens at a Glance

### Home Screen
- Navigation grid (4 options)
- Current alarm display
- Dismiss button (when alarm ringing)

### Alarm Setup
- Time picker
- Ringtone dropdown
- Save button

### Sleep Summary
- Big sleep score (0-100)
- Stats: time, wake-ups, snoring
- Dream recording + transcription

### Sleep History
- Calendar (month view)
- Color-coded days (green = good, red = bad)
- Tap day for full breakdown

### Dream Journal
- List of past dream transcriptions
- Shows date & transcribed text

---

## Troubleshooting

**Q: `flutter: command not found`**  
A: Add Flutter to PATH:
```bash
echo 'export PATH="$HOME/.local/share/flutter/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

**Q: No iOS devices available**  
A: Start the simulator first:
```bash
open -a Simulator
```

**Q: Build fails with CocoaPods error**  
A: Clean and rebuild:
```bash
cd app
flutter clean
flutter pub get
flutter run
```

**Q: Hot reload not working**  
A: Try without fast start:
```bash
flutter run --no-fast-start
```

**Q: Xcode build error**  
A: Reset Xcode:
```bash
sudo xcode-select --reset
flutter clean
flutter run
```

---

## What's Stubbed (Offline Data)

All API calls return fake data for development:

- **Sleep summary:** Score 82, 7h23m sleep, 2 wake-ups, 42.5dB snoring
- **Dream history:** 3 sample dreams
- **Calendar:** 7 days of mock sleep data

Replace with real HTTP calls once backend is live (just uncomment lines in `api_service.dart`).

---

## Next Steps

### Day 1 (Today)
1. Run the app (`flutter run`)
2. Explore all 5 screens
3. Show judges the UI
4. Share codebase with team

### Day 2 (Backend Integration)
1. Backend team completes REST API
2. You uncomment HTTP calls in `api_service.dart`
3. Fill in `ApiConfig.baseUrl`
4. Test: set alarm → see on backend

### Day 2 (Tuya Board Integration)
1. Firmware team completes board code
2. You uncomment Tuya SDK code
3. Fill in `TuyaConfig` credentials
4. Test: alarm → board display

### Final Demo
- Set alarm on app
- Board displays it
- Dismiss at alarm time
- Show sleep summary
- Show calendar & dreams

---

## Resources

- **Flutter docs:** flutter.dev/docs
- **Provider docs:** pub.dev/packages/provider
- **Tuya SDK:** developer.tuya.com
- **This project:** All files are heavily commented

---

## Pro Tips

💡 **Tip 1:** Use `print()` for debugging — output shows in Flutter console

💡 **Tip 2:** Simulator is slow on first run (2-3 min compile). Be patient.

💡 **Tip 3:** "Flutter clean" fixes ~90% of weird build errors

💡 **Tip 4:** Keep `pubspec.yaml` open when adding packages

💡 **Tip 5:** Test on physical iPhone before demo (simulator ≠ real device)

---

You've got this! 🚀 Good luck with the hackathon!

Need help? Read one of the docs:
- **Setup issues?** → SETUP_MACOS_M4.md
- **How does it work?** → ARCHITECTURE.md
- **What now?** → README.md
