import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'services/app_state.dart';
import 'screens/home_screen.dart';

// ============================================================
// HACKSTORM 2.0: SMART ALARM CLOCK
//
// Entry point. Wraps the app in Provider for state management.
//
// QUICK START FOR TEAMMATES:
//   1. Fill in config/api_config.dart with your backend URL
//   2. Fill in config/api_config.dart TuyaConfig with your board credentials
//   3. Run: flutter pub get && flutter run
//   4. All API calls are stubbed — app works offline for UI dev
//   5. Search "TODO [API]" for backend integration points
//   6. Search "TODO [TUYA]" for Tuya Board integration points
//   7. Search "TODO [AUDIO]" for microphone recording integration
// ============================================================

void main() {
  runApp(const SmartAlarmApp());
}

class SmartAlarmApp extends StatelessWidget {
  const SmartAlarmApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (_) => AppState(),
      child: MaterialApp(
        title: 'Smart Alarm',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(
          brightness: Brightness.dark,
          scaffoldBackgroundColor: const Color(0xFF0F0F1A),
          appBarTheme: const AppBarTheme(
            backgroundColor: Colors.transparent,
            elevation: 0,
            centerTitle: true,
          ),
          colorScheme: const ColorScheme.dark(
            primary: Color(0xFF7C4DFF),
            secondary: Color(0xFF448AFF),
          ),
        ),
        home: const HomeScreen(),
      ),
    );
  }
}
