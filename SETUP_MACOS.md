# Flutter Setup & Run Guide — macOS M4

## Prerequisites (5 min)

You'll need:
- **Xcode** (for iOS simulator)
- **Flutter SDK** (native ARM64 support on M4)
- **CocoaPods** (manages iOS dependencies)

---

## Step 1: Install Homebrew (if you don't have it)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Then add Homebrew to your PATH (Homebrew installer will print exact command — follow it).

---

## Step 2: Install Flutter SDK

The easiest way is via Homebrew:

```bash
brew install flutter
```

Verify installation:
```bash
flutter --version
flutter doctor
```

**Important:** `flutter doctor` will list what you're missing. Follow its recommendations.

---

## Step 3: Install Xcode & iOS toolchain

```bash
xcode-select --install
```

Then install CocoaPods (for iOS dependencies) via Homebrew:
```bash
brew install cocoapods
```

*(If Homebrew doesn't work, you can also use `sudo gem install cocoapods`, but Homebrew is preferred)*

---

## Step 4: Enable Simulators & Set Up

Check available iOS simulators:
```bash
xcrun simctl list devices
```

If none exist, create one via Xcode:
```bash
open -a Xcode
# Xcode → Window → Devices and Simulators → Simulators tab → + (add new)
# Choose iPhone 15 Pro (or latest), click Create
```

---

## Step 5: Generate Platform-Specific Code

The project needs iOS, macOS, web, and Android platform files to run:

```bash
cd /Users/shyamgupta/Desktop/HackStorm\ Clock\ App/app
flutter create . --platforms ios,android,macos,web
```

This generates the necessary native code directories (`ios/`, `android/`, `macos/`, `web/`).

---

## Step 6: Navigate to the project & Run

```bash
cd /Users/shyamgupta/Desktop/HackStorm\ Clock\ App/app

# Install dependencies
flutter pub get

# Run on iOS simulator
flutter run

# Or run on macOS (if you want a desktop build)
flutter run -d macos
```

---

## Common Issues on M4

### Issue: "flutter: command not found"
**Fix:** Homebrew didn't add Flutter to PATH. Run:
```bash
echo 'export PATH="$HOME/.local/share/flutter/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### Issue: "No devices found" or "iPhone simulator not supported"
**Fix:** You need to generate platform code first:
```bash
cd /Users/shyamgupta/Desktop/HackStorm\ Clock\ App/app
flutter create . --platforms ios,android,macos,web
```

Then start the simulator and run:
```bash
open -a Simulator
flutter run
```

If you still don't see devices, check what's available:
```bash
flutter devices
```

---

### Issue: "No iOS devices" (simulator exists but not detected)
**Fix:** Start the iOS simulator first:
```bash
open -a Simulator
```

Wait 10-15 seconds for it to fully boot, then run:
```bash
flutter run -d all    # Detects all available devices
```

### Issue: "CocoaPods dependency error"
**Fix:** 
```bash
cd /Users/shyamgupta/Desktop/HackStorm\ Clock\ App/app/ios
rm Podfile.lock
cd ..
flutter clean
flutter pub get
flutter run
```

### Issue: "Xcode build failed" or "Missing implementations" (record_linux)
**Fix:** This happens when package versions are incompatible. Update to newer versions:
```bash
cd /Users/shyamgupta/Desktop/HackStorm\ Clock\ App/app
# Update pubspec.yaml: change record: ^5.0.4 to record: ^6.0.0
# and audioplayers: ^5.2.1 to audioplayers: ^6.0.0
flutter clean
flutter pub get
flutter run
```

Or manually upgrade dependencies:
```bash
flutter pub upgrade record audioplayers
flutter clean
flutter pub get
flutter run
```

---

### Issue: "Xcode build failed" (general)
**Fix:** Upgrade Xcode and reset Flutter:
```bash
sudo xcode-select --reset
flutter clean
flutter pub get
flutter run
```

---

## Quick Reference: Common Commands

| Command | What it does |
|---------|-------------|
| `flutter run` | Run on default device (simulator or phone) |
| `flutter run -d macos` | Run on macOS desktop |
| `flutter run -d all` | Run on all available devices |
| `flutter clean` | Clear build cache (use if things break) |
| `flutter pub get` | Install dependencies from pubspec.yaml |
| `flutter doctor` | Check your environment setup |
| `flutter devices` | List available devices |

---

## Running the App

### Option A: iOS Simulator (Recommended for hackathon)
```bash
cd /Users/shyamgupta/Desktop/HackStorm\ Clock\ App/app
flutter run
```

The app will:
1. Compile (takes ~2 min on first run)
2. Launch in the iOS simulator
3. Hot-reload enabled (edit code → save → auto-refresh in app)

### Option B: macOS Desktop
```bash
flutter run -d macos
```

Same as iOS but runs as a native macOS app.

### Option C: Physical iPhone (if you want to test on device)
1. Plug in iPhone via USB
2. Run `flutter devices` to confirm it's detected
3. Run `flutter run -d ios`

---

## Development Workflow

1. **Make code changes** (e.g., edit `lib/screens/home_screen.dart`)
2. **Save the file**
3. **App auto-reloads** in the simulator (thanks to hot-reload)
4. **No need to rebuild** unless you change native code or pubspec.yaml

If hot-reload fails:
```bash
flutter run --no-fast-start
```

---

## Next Steps

1. **Run the app** to see the UI (it's fully functional offline)
2. **Fill in API endpoints** in `lib/config/api_config.dart` once backend is ready
3. **Fill in Tuya credentials** in the same file
4. **Uncomment API stubs** in `lib/services/api_service.dart` to connect to real backend

---

## Need Help?

- **Flutter docs:** https://flutter.dev/docs
- **iOS setup:** https://flutter.dev/docs/get-started/install/macos
- **Troubleshooting:** https://flutter.dev/docs/testing/troubleshooting

Good luck with the hackathon! 🚀
