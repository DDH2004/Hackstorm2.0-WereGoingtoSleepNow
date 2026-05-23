import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/app_state.dart';

// ============================================================
// SCREEN 1: ALARM SETUP
//
// User flow (Evening):
//   1. Pick alarm time (HH:MM)
//   2. Select ringtone
//   3. Toggle on/off
//   4. Save → POST /api/alarms → MQTT → Tuya Board
// ============================================================

class AlarmSetupScreen extends StatefulWidget {
  const AlarmSetupScreen({super.key});

  @override
  State<AlarmSetupScreen> createState() => _AlarmSetupScreenState();
}

class _AlarmSetupScreenState extends State<AlarmSetupScreen> {
  TimeOfDay _selectedTime = const TimeOfDay(hour: 7, minute: 0);
  String _selectedRingtone = 'Gentle Sunrise';
  bool _alarmEnabled = true;

  // TODO [TUYA]: If board supports custom ringtone upload, add file picker here
  final List<String> _ringtones = [
    'Gentle Sunrise',
    'Ocean Waves',
    'Forest Birds',
    'Custom Upload',  // TODO: Implement file picker for custom ringtone
  ];

  Future<void> _pickTime() async {
    final picked = await showTimePicker(
      context: context,
      initialTime: _selectedTime,
      builder: (context, child) {
        return Theme(
          data: Theme.of(context).copyWith(
            colorScheme: const ColorScheme.dark(
              primary: Color(0xFF7C4DFF),
              surface: Color(0xFF1E1E2E),
            ),
          ),
          child: child!,
        );
      },
    );
    if (picked != null) {
      setState(() => _selectedTime = picked);
    }
  }

  Future<void> _saveAlarm() async {
    final now = DateTime.now();
    var alarmDateTime = DateTime(
      now.year, now.month, now.day,
      _selectedTime.hour, _selectedTime.minute,
    );
    // If time already passed today, schedule for tomorrow
    if (alarmDateTime.isBefore(now)) {
      alarmDateTime = alarmDateTime.add(const Duration(days: 1));
    }

    final state = context.read<AppState>();
    await state.saveAlarm(alarmDateTime, _selectedRingtone);

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Alarm set for ${_selectedTime.format(context)}'),
          backgroundColor: const Color(0xFF7C4DFF),
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();

    return Scaffold(
      backgroundColor: const Color(0xFF0F0F1A),
      appBar: AppBar(
        title: const Text('Set Alarm'),
        backgroundColor: Colors.transparent,
        elevation: 0,
      ),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // --- Time Picker ---
            GestureDetector(
              onTap: _pickTime,
              child: Container(
                padding: const EdgeInsets.symmetric(vertical: 40),
                decoration: BoxDecoration(
                  color: const Color(0xFF1E1E2E),
                  borderRadius: BorderRadius.circular(20),
                  border: Border.all(color: const Color(0xFF7C4DFF), width: 1),
                ),
                child: Center(
                  child: Text(
                    _selectedTime.format(context),
                    style: const TextStyle(
                      fontSize: 64,
                      fontWeight: FontWeight.w300,
                      color: Colors.white,
                      letterSpacing: 4,
                    ),
                  ),
                ),
              ),
            ),
            const SizedBox(height: 12),
            const Center(
              child: Text('Tap to change time', style: TextStyle(color: Colors.white38, fontSize: 13)),
            ),

            const SizedBox(height: 32),

            // --- Ringtone Selector ---
            const Text('Ringtone', style: TextStyle(color: Colors.white70, fontSize: 14)),
            const SizedBox(height: 8),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              decoration: BoxDecoration(
                color: const Color(0xFF1E1E2E),
                borderRadius: BorderRadius.circular(12),
              ),
              child: DropdownButton<String>(
                value: _selectedRingtone,
                isExpanded: true,
                dropdownColor: const Color(0xFF1E1E2E),
                style: const TextStyle(color: Colors.white, fontSize: 16),
                underline: const SizedBox(),
                items: _ringtones.map((r) => DropdownMenuItem(
                  value: r,
                  child: Text(r),
                )).toList(),
                onChanged: (val) {
                  if (val == 'Custom Upload') {
                    // TODO: Implement custom ringtone file picker
                    ScaffoldMessenger.of(context).showSnackBar(
                      const SnackBar(content: Text('Custom upload not yet implemented')),
                    );
                    return;
                  }
                  setState(() => _selectedRingtone = val!);
                },
              ),
            ),

            const SizedBox(height: 24),

            // --- Enable/Disable Toggle ---
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
              decoration: BoxDecoration(
                color: const Color(0xFF1E1E2E),
                borderRadius: BorderRadius.circular(12),
              ),
              child: SwitchListTile(
                title: const Text('Alarm Enabled', style: TextStyle(color: Colors.white)),
                value: _alarmEnabled,
                activeThumbColor: const Color(0xFF7C4DFF),
                onChanged: (val) => setState(() => _alarmEnabled = val),
                contentPadding: EdgeInsets.zero,
              ),
            ),

            const Spacer(),

            // --- Save Button ---
            ElevatedButton(
              onPressed: state.isLoading ? null : _saveAlarm,
              style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xFF7C4DFF),
                padding: const EdgeInsets.symmetric(vertical: 18),
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
              ),
              child: state.isLoading
                ? const SizedBox(height: 20, width: 20, child: CircularProgressIndicator(color: Colors.white, strokeWidth: 2))
                : const Text('Save Alarm', style: TextStyle(fontSize: 18, color: Colors.white)),
            ),
          ],
        ),
      ),
    );
  }
}
