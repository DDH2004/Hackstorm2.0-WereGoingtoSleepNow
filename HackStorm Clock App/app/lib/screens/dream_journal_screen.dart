import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:intl/intl.dart';
import '../services/app_state.dart';

// ============================================================
// SCREEN: DREAM JOURNAL
//
// Lists all past dream transcriptions captured after alarm dismiss.
//
// Data flow:
//   User speaks after alarm dismiss
//   → Audio captured (30s) by app or Tuya Board
//   → POST /api/dream/audio → Google Cloud Speech-to-Text
//   → Transcription stored in DB
//   → This screen fetches and displays them
//
// TODO [API]: Backend needs GET /api/dreams/{userId} endpoint
// ============================================================

class DreamJournalScreen extends StatefulWidget {
  const DreamJournalScreen({super.key});

  @override
  State<DreamJournalScreen> createState() => _DreamJournalScreenState();
}

class _DreamJournalScreenState extends State<DreamJournalScreen> {
  @override
  void initState() {
    super.initState();
    Future.microtask(() => context.read<AppState>().loadDreamJournal());
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();

    return Scaffold(
      backgroundColor: const Color(0xFF0F0F1A),
      appBar: AppBar(
        title: const Text('Dream Journal'),
        backgroundColor: Colors.transparent,
        elevation: 0,
      ),
      body: state.dreamJournal.isEmpty
        ? const Center(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.auto_stories, color: Colors.white24, size: 48),
                SizedBox(height: 12),
                Text('No dreams recorded yet', style: TextStyle(color: Colors.white38)),
                SizedBox(height: 4),
                Text('Dismiss an alarm and record your dream!', style: TextStyle(color: Colors.white24, fontSize: 12)),
              ],
            ),
          )
        : ListView.builder(
            padding: const EdgeInsets.all(16),
            itemCount: state.dreamJournal.length,
            itemBuilder: (context, index) {
              final entry = state.dreamJournal[index];
              return Container(
                margin: const EdgeInsets.only(bottom: 12),
                padding: const EdgeInsets.all(18),
                decoration: BoxDecoration(
                  color: const Color(0xFF1E1E2E),
                  borderRadius: BorderRadius.circular(14),
                  border: Border.all(color: const Color(0xFF7C4DFF).withValues(alpha: 0.15)),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      children: [
                        const Icon(Icons.nights_stay, color: Color(0xFF7C4DFF), size: 16),
                        const SizedBox(width: 8),
                        Text(
                          DateFormat('EEEE, MMM d, yyyy').format(entry.date),
                          style: const TextStyle(color: Colors.white54, fontSize: 13),
                        ),
                      ],
                    ),
                    const SizedBox(height: 10),
                    Text(
                      entry.transcription,
                      style: const TextStyle(color: Colors.white70, fontSize: 15, height: 1.5, fontStyle: FontStyle.italic),
                    ),
                  ],
                ),
              );
            },
          ),
    );
  }
}
