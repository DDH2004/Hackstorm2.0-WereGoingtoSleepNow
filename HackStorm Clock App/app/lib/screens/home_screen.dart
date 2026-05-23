import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/app_state.dart';
import 'alarm_setup_screen.dart';
import 'sleep_summary_screen.dart';
import 'sleep_history_screen.dart';
import 'dream_journal_screen.dart';

// ============================================================
// SCREEN 3: HOME / MAIN APP
//
// Central hub with navigation to all features:
//   - Current alarm status
//   - Quick access to: Set Alarm, Sleep Summary, Calendar, Dream Journal
//   - Dismiss button when alarm is ringing
//
// This is also where the alarm-ringing state is handled:
//   Tuya Board triggers alarm → MQTT → Cloud → App shows dismiss UI
// ============================================================

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  @override
  void initState() {
    super.initState();
    // TODO [TUYA]: Listen for alarm trigger from board
    // context.read<AppState>().tuya.listenForAlarmTrigger().listen((triggered) {
    //   if (triggered) setState(() => /* show dismiss UI */);
    // });
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();

    return Scaffold(
      backgroundColor: const Color(0xFF0F0F1A),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // --- Header ---
              const Text(
                'Smart Alarm',
                style: TextStyle(color: Colors.white, fontSize: 28, fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 4),
              const Text(
                'Sleep Intelligence Platform',
                style: TextStyle(color: Colors.white38, fontSize: 14),
              ),
              const SizedBox(height: 8),

              // --- Board Connection Status ---
              // TODO [TUYA]: Show real connection status
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                decoration: BoxDecoration(
                  color: const Color(0xFF1E1E2E),
                  borderRadius: BorderRadius.circular(20),
                ),
                child: const Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Icon(Icons.circle, size: 8, color: Colors.greenAccent),
                    SizedBox(width: 6),
                    Text('Board Connected', style: TextStyle(color: Colors.white54, fontSize: 12)),
                    // TODO [TUYA]: Change to red/disconnected when board is offline
                  ],
                ),
              ),

              const SizedBox(height: 32),

              // --- Current Alarm Display ---
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(24),
                decoration: BoxDecoration(
                  gradient: const LinearGradient(
                    colors: [Color(0xFF7C4DFF), Color(0xFF448AFF)],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                  borderRadius: BorderRadius.circular(20),
                ),
                child: Column(
                  children: [
                    const Text('Next Alarm', style: TextStyle(color: Colors.white70, fontSize: 14)),
                    const SizedBox(height: 8),
                    Text(
                      state.currentAlarm != null
                        ? TimeOfDay.fromDateTime(state.currentAlarm!.alarmTime).format(context)
                        : '-- : --',
                      style: const TextStyle(color: Colors.white, fontSize: 48, fontWeight: FontWeight.w300, letterSpacing: 2),
                    ),
                    if (state.currentAlarm != null) ...[
                      const SizedBox(height: 4),
                      Text(
                        state.currentAlarm!.ringtone,
                        style: const TextStyle(color: Colors.white54, fontSize: 13),
                      ),
                    ],
                  ],
                ),
              ),

              // --- Alarm Ringing: Dismiss Button ---
              // TODO [TUYA]: This shows when board sends alarm-triggered signal
              if (state.alarmIsRinging) ...[
                const SizedBox(height: 16),
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton(
                    onPressed: () => state.dismissAlarm(),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.redAccent,
                      padding: const EdgeInsets.symmetric(vertical: 18),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
                    ),
                    child: const Text('DISMISS ALARM', style: TextStyle(fontSize: 20, color: Colors.white, fontWeight: FontWeight.bold)),
                  ),
                ),
              ],

              const SizedBox(height: 32),

              // --- Navigation Grid ---
              Expanded(
                child: GridView.count(
                  crossAxisCount: 2,
                  crossAxisSpacing: 14,
                  mainAxisSpacing: 14,
                  childAspectRatio: 1.1,
                  children: [
                    _NavCard(
                      icon: Icons.alarm_add,
                      label: 'Set Alarm',
                      subtitle: 'Configure time & ringtone',
                      onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const AlarmSetupScreen())),
                    ),
                    _NavCard(
                      icon: Icons.nightlight_round,
                      label: 'Last Night',
                      subtitle: 'Sleep score & stats',
                      onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const SleepSummaryScreen())),
                    ),
                    _NavCard(
                      icon: Icons.calendar_month,
                      label: 'History',
                      subtitle: 'Monthly sleep calendar',
                      onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const SleepHistoryScreen())),
                    ),
                    _NavCard(
                      icon: Icons.auto_stories,
                      label: 'Dream Journal',
                      subtitle: 'Past dream entries',
                      onTap: () => Navigator.push(context, MaterialPageRoute(builder: (_) => const DreamJournalScreen())),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _NavCard extends StatelessWidget {
  final IconData icon;
  final String label;
  final String subtitle;
  final VoidCallback onTap;

  const _NavCard({required this.icon, required this.label, required this.subtitle, required this.onTap});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        padding: const EdgeInsets.all(18),
        decoration: BoxDecoration(
          color: const Color(0xFF1E1E2E),
          borderRadius: BorderRadius.circular(16),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(icon, color: const Color(0xFF7C4DFF), size: 30),
            const SizedBox(height: 12),
            Text(label, style: const TextStyle(color: Colors.white, fontSize: 16, fontWeight: FontWeight.w600)),
            const SizedBox(height: 4),
            Text(subtitle, style: const TextStyle(color: Colors.white38, fontSize: 11)),
          ],
        ),
      ),
    );
  }
}
