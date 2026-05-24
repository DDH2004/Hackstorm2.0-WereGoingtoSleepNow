import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:intl/intl.dart';
import '../services/app_state.dart';

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

  Future<void> _startRecording() async {
    await showDialog(
      context: context,
      barrierDismissible: false,
      builder: (_) => const _ListeningDialog(),
    );

    if (!mounted) return;
    context.read<AppState>().addDreamEntry('I dreamt I was fighting with the Avengers to defeat SpongeBob...');
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
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _startRecording,
        backgroundColor: const Color(0xFF7C4DFF),
        icon: const Icon(Icons.mic, color: Colors.white),
        label: const Text('Record a Dream', style: TextStyle(color: Colors.white)),
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
                  Text('Tap the button below to record a dream!',
                      style: TextStyle(color: Colors.white24, fontSize: 12)),
                ],
              ),
            )
          : ListView.builder(
              padding: const EdgeInsets.fromLTRB(16, 16, 16, 96),
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
                        style: const TextStyle(
                            color: Colors.white70,
                            fontSize: 15,
                            height: 1.5,
                            fontStyle: FontStyle.italic),
                      ),
                    ],
                  ),
                );
              },
            ),
    );
  }
}

class _ListeningDialog extends StatefulWidget {
  const _ListeningDialog();

  @override
  State<_ListeningDialog> createState() => _ListeningDialogState();
}

class _ListeningDialogState extends State<_ListeningDialog>
    with SingleTickerProviderStateMixin {
  Timer? _timer;
  late AnimationController _pulseController;

  @override
  void initState() {
    super.initState();
    _pulseController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 900),
    )..repeat(reverse: true);

    _timer = Timer(const Duration(seconds: 20), () {
      Navigator.of(context).pop();
    });
  }

  @override
  void dispose() {
    _timer?.cancel();
    _pulseController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      backgroundColor: const Color(0xFF1E1E2E),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      child: Padding(
        padding: const EdgeInsets.symmetric(vertical: 48, horizontal: 32),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            AnimatedBuilder(
              animation: _pulseController,
              builder: (_, __) => Container(
                width: 80,
                height: 80,
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  color: const Color(0xFF7C4DFF)
                      .withValues(alpha: 0.15 + 0.2 * _pulseController.value),
                ),
                child: const Icon(Icons.mic, color: Color(0xFF7C4DFF), size: 40),
              ),
            ),
            const SizedBox(height: 24),
            const Text(
              'Listening…',
              style: TextStyle(
                  color: Colors.white, fontSize: 20, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 8),
            const Text(
              'Describe your dream',
              style: TextStyle(color: Colors.white54, fontSize: 14),
            ),
          ],
        ),
      ),
    );
  }
}
