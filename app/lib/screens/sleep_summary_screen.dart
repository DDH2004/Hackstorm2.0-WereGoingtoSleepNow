import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../services/app_state.dart';
import '../widgets/sleep_score_card.dart';

// ============================================================
// SCREEN 2: SLEEP SUMMARY
//
// Shown after user dismisses alarm. Displays:
//   - Sleep Score (0-100, big number)
//   - Total sleep time
//   - Number of wake-ups
//   - Snore meter (dB)
//   - Dream recording prompt → transcription display
//
// Data source: GET /api/sleep/summary/{userId}
// Cloud computes score from overnight camera + audio analysis.
//
// Score formula: 100 - (2 * wakeUps) - (1 * snoringEvents), clamped 0-100
// ============================================================

class SleepSummaryScreen extends StatefulWidget {
  const SleepSummaryScreen({super.key});

  @override
  State<SleepSummaryScreen> createState() => _SleepSummaryScreenState();
}

class _SleepSummaryScreenState extends State<SleepSummaryScreen> {
  bool _isRecording = false;
  String? _dreamText;

  @override
  void initState() {
    super.initState();
    Future.microtask(() => context.read<AppState>().loadSleepSummary());
  }

  // ----------------------------------------------------------
  // Dream Recording Flow:
  //   1. User taps "Record Dream"
  //   2. App records 30 seconds of audio from phone mic
  //   3. Audio uploaded → POST /api/dream/audio
  //   4. Cloud runs Google Speech-to-Text
  //   5. Transcription returned and displayed
  //
  // NOTE: The workflow doc says the Tuya Board captures dream audio,
  //       but recording from the phone is more practical for demo.
  //       Choose whichever path works for your team.
  // ----------------------------------------------------------
  Future<void> _recordDream() async {
    setState(() => _isRecording = true);

    // TODO [AUDIO]: Implement actual recording with the `record` package
    //
    // Example:
    // final recorder = AudioRecorder();
    // if (await recorder.hasPermission()) {
    //   await recorder.start(const RecordConfig(), path: '/tmp/dream.m4a');
    //   await Future.delayed(const Duration(seconds: 30));
    //   final path = await recorder.stop();
    //   final transcription = await context.read<AppState>().submitDreamRecording(path!);
    //   setState(() => _dreamText = transcription);
    // }

    // STUB: Simulate recording
    await Future.delayed(const Duration(seconds: 3));
    final transcription = await context.read<AppState>().submitDreamRecording('/tmp/dream_stub.m4a');

    setState(() {
      _isRecording = false;
      _dreamText = transcription;
    });
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();
    final summary = state.lastSleepSummary;

    return Scaffold(
      backgroundColor: const Color(0xFF0F0F1A),
      appBar: AppBar(
        title: const Text('Sleep Summary'),
        backgroundColor: Colors.transparent,
        elevation: 0,
      ),
      body: state.isLoading || summary == null
        ? const Center(child: CircularProgressIndicator(color: Color(0xFF7C4DFF)))
        : SingleChildScrollView(
            padding: const EdgeInsets.all(24),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                // --- Sleep Score ---
                SleepScoreCard(score: summary.sleepScore),
                const SizedBox(height: 24),

                // --- Stats Grid ---
                Row(
                  children: [
                    _StatTile(
                      label: 'Sleep Time',
                      value: '${summary.totalTime.inHours}h ${summary.totalTime.inMinutes % 60}m',
                      icon: Icons.bedtime_outlined,
                    ),
                    const SizedBox(width: 12),
                    _StatTile(
                      label: 'Wake-ups',
                      value: '${summary.wakeUps}',
                      icon: Icons.visibility_outlined,
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                Row(
                  children: [
                    _StatTile(
                      label: 'Snore Meter',
                      value: '${summary.snoringMeter.toStringAsFixed(1)} dB',
                      icon: Icons.mic_outlined,
                    ),
                    const SizedBox(width: 12),
                    _StatTile(
                      label: 'Quality',
                      value: summary.sleepScore >= 80 ? 'Good' : summary.sleepScore >= 60 ? 'Fair' : 'Poor',
                      icon: Icons.star_outline,
                    ),
                  ],
                ),

                const SizedBox(height: 32),

                // --- Dream Journal Recording ---
                Container(
                  padding: const EdgeInsets.all(20),
                  decoration: BoxDecoration(
                    color: const Color(0xFF1E1E2E),
                    borderRadius: BorderRadius.circular(16),
                    border: Border.all(color: const Color(0xFF7C4DFF).withOpacity(0.3)),
                  ),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Row(
                        children: [
                          Icon(Icons.auto_stories, color: Color(0xFF7C4DFF), size: 22),
                          SizedBox(width: 8),
                          Text('Dream Journal', style: TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.w600)),
                        ],
                      ),
                      const SizedBox(height: 8),
                      const Text(
                        'Tell us about your dream! Tap record and speak for 30 seconds.',
                        style: TextStyle(color: Colors.white54, fontSize: 14),
                      ),
                      const SizedBox(height: 16),

                      if (_dreamText != null) ...[
                        Container(
                          width: double.infinity,
                          padding: const EdgeInsets.all(12),
                          decoration: BoxDecoration(
                            color: const Color(0xFF0F0F1A),
                            borderRadius: BorderRadius.circular(10),
                          ),
                          child: Text(
                            _dreamText!,
                            style: const TextStyle(color: Colors.white70, fontSize: 14, fontStyle: FontStyle.italic),
                          ),
                        ),
                      ] else ...[
                        Center(
                          child: ElevatedButton.icon(
                            onPressed: _isRecording ? null : _recordDream,
                            icon: Icon(_isRecording ? Icons.hearing : Icons.mic, size: 20),
                            label: Text(_isRecording ? 'Listening...' : 'Record Dream'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: const Color(0xFF7C4DFF),
                              foregroundColor: Colors.white,
                              padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 14),
                              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                            ),
                          ),
                        ),
                      ],
                    ],
                  ),
                ),
              ],
            ),
          ),
    );
  }
}

class _StatTile extends StatelessWidget {
  final String label;
  final String value;
  final IconData icon;

  const _StatTile({required this.label, required this.value, required this.icon});

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: const Color(0xFF1E1E2E),
          borderRadius: BorderRadius.circular(14),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Icon(icon, color: const Color(0xFF7C4DFF), size: 20),
            const SizedBox(height: 10),
            Text(value, style: const TextStyle(color: Colors.white, fontSize: 22, fontWeight: FontWeight.bold)),
            const SizedBox(height: 4),
            Text(label, style: const TextStyle(color: Colors.white38, fontSize: 12)),
          ],
        ),
      ),
    );
  }
}
