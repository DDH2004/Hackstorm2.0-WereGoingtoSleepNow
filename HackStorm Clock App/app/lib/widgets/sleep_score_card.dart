import 'package:flutter/material.dart';

class SleepScoreCard extends StatelessWidget {
  final int score;

  const SleepScoreCard({super.key, required this.score});

  Color get _color {
    if (score >= 80) return Colors.greenAccent;
    if (score >= 60) return Colors.orangeAccent;
    return Colors.redAccent;
  }

  String get _label {
    if (score >= 80) return 'Great sleep!';
    if (score >= 60) return 'Decent night';
    return 'Rough night';
  }

  @override
  Widget build(BuildContext context) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.symmetric(vertical: 32),
      decoration: BoxDecoration(
        color: const Color(0xFF1E1E2E),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: _color.withOpacity(0.3), width: 1),
      ),
      child: Column(
        children: [
          const Text('Sleep Score', style: TextStyle(color: Colors.white54, fontSize: 14)),
          const SizedBox(height: 8),
          Text(
            '$score',
            style: TextStyle(color: _color, fontSize: 72, fontWeight: FontWeight.bold),
          ),
          Text(_label, style: TextStyle(color: _color.withOpacity(0.7), fontSize: 16)),
        ],
      ),
    );
  }
}
