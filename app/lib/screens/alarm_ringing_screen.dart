import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/app_state.dart';

class AlarmRingingScreen extends StatelessWidget {
  const AlarmRingingScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    final alarmLabel = state.currentAlarm != null
        ? TimeOfDay.fromDateTime(state.currentAlarm!.alarmTime).format(context)
        : 'Alarm';

    return Scaffold(
      backgroundColor: const Color(0xFF0F0F1A),
      body: SafeArea(
        child: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.alarm, color: Color(0xFF7C4DFF), size: 80),
              const SizedBox(height: 32),
              const Text(
                'Wake Up!',
                style: TextStyle(
                  color: Colors.white,
                  fontSize: 40,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 12),
              Text(
                alarmLabel,
                style: const TextStyle(
                  color: Colors.white54,
                  fontSize: 24,
                  letterSpacing: 2,
                ),
              ),
              const SizedBox(height: 64),
              SizedBox(
                width: 260,
                height: 64,
                child: ElevatedButton(
                  onPressed: () => state.dismissAlarm(),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.redAccent,
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(16),
                    ),
                  ),
                  child: const Text(
                    'DISMISS',
                    style: TextStyle(
                      fontSize: 22,
                      fontWeight: FontWeight.bold,
                      color: Colors.white,
                    ),
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
