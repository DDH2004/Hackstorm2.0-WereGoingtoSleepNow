import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:table_calendar/table_calendar.dart';
import 'package:intl/intl.dart';
import '../services/app_state.dart';
import '../models/sleep_summary.dart';

// ============================================================
// SCREEN: SLEEP HISTORY (Calendar View)
//
// Shows a month-at-a-time calendar. Days with sleep data are marked.
// Tapping a day shows that night's details:
//   sleep score, sleep time, wake-ups, snore meter
//
// Data source: GET /api/sleep/history/{userId}
// TODO [API]: Backend needs a history endpoint (not in original spec)
// ============================================================

class SleepHistoryScreen extends StatefulWidget {
  const SleepHistoryScreen({super.key});

  @override
  State<SleepHistoryScreen> createState() => _SleepHistoryScreenState();
}

class _SleepHistoryScreenState extends State<SleepHistoryScreen> {
  DateTime _focusedDay = DateTime.now();
  DateTime? _selectedDay;
  SleepSummary? _selectedSummary;

  @override
  void initState() {
    super.initState();
    Future.microtask(() => context.read<AppState>().loadSleepHistory());
  }

  SleepSummary? _getSummaryForDay(DateTime day, List<SleepSummary> history) {
    for (final s in history) {
      if (isSameDay(s.date, day)) return s;
    }
    return null;
  }

  Color _scoreColor(int score) {
    if (score >= 80) return Colors.greenAccent;
    if (score >= 60) return Colors.orangeAccent;
    return Colors.redAccent;
  }

  @override
  Widget build(BuildContext context) {
    final state = context.watch<AppState>();

    return Scaffold(
      backgroundColor: const Color(0xFF0F0F1A),
      appBar: AppBar(
        title: const Text('Sleep History'),
        backgroundColor: Colors.transparent,
        elevation: 0,
      ),
      body: Column(
        children: [
          // --- Calendar ---
          Container(
            margin: const EdgeInsets.symmetric(horizontal: 16),
            decoration: BoxDecoration(
              color: const Color(0xFF1E1E2E),
              borderRadius: BorderRadius.circular(16),
            ),
            child: TableCalendar(
              firstDay: DateTime.now().subtract(const Duration(days: 365)),
              lastDay: DateTime.now(),
              focusedDay: _focusedDay,
              selectedDayPredicate: (day) => isSameDay(_selectedDay, day),
              onDaySelected: (selectedDay, focusedDay) {
                final summary = _getSummaryForDay(selectedDay, state.sleepHistory);
                setState(() {
                  _selectedDay = selectedDay;
                  _focusedDay = focusedDay;
                  _selectedSummary = summary;
                });
              },
              calendarStyle: const CalendarStyle(
                defaultTextStyle: TextStyle(color: Colors.white70),
                weekendTextStyle: TextStyle(color: Colors.white70),
                outsideTextStyle: TextStyle(color: Colors.white24),
                todayDecoration: BoxDecoration(color: Color(0xFF7C4DFF), shape: BoxShape.circle),
                selectedDecoration: BoxDecoration(color: Color(0xFF448AFF), shape: BoxShape.circle),
              ),
              headerStyle: const HeaderStyle(
                titleTextStyle: TextStyle(color: Colors.white, fontSize: 16),
                formatButtonVisible: false,
                leftChevronIcon: Icon(Icons.chevron_left, color: Colors.white54),
                rightChevronIcon: Icon(Icons.chevron_right, color: Colors.white54),
              ),
              daysOfWeekStyle: const DaysOfWeekStyle(
                weekdayStyle: TextStyle(color: Colors.white38, fontSize: 12),
                weekendStyle: TextStyle(color: Colors.white38, fontSize: 12),
              ),
              calendarBuilders: CalendarBuilders(
                markerBuilder: (context, date, events) {
                  final summary = _getSummaryForDay(date, state.sleepHistory);
                  if (summary != null) {
                    return Positioned(
                      bottom: 1,
                      child: Container(
                        width: 6, height: 6,
                        decoration: BoxDecoration(
                          color: _scoreColor(summary.sleepScore),
                          shape: BoxShape.circle,
                        ),
                      ),
                    );
                  }
                  return null;
                },
              ),
            ),
          ),

          const SizedBox(height: 20),

          // --- Selected Day Detail ---
          if (_selectedSummary != null)
            Expanded(
              child: Container(
                margin: const EdgeInsets.symmetric(horizontal: 16),
                padding: const EdgeInsets.all(20),
                decoration: BoxDecoration(
                  color: const Color(0xFF1E1E2E),
                  borderRadius: const BorderRadius.vertical(top: Radius.circular(20)),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      DateFormat('EEEE, MMM d').format(_selectedDay!),
                      style: const TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.w600),
                    ),
                    const SizedBox(height: 16),
                    _DetailRow('Sleep Score', '${_selectedSummary!.sleepScore}/100', _scoreColor(_selectedSummary!.sleepScore)),
                    _DetailRow('Total Sleep', '${_selectedSummary!.totalTime.inHours}h ${_selectedSummary!.totalTime.inMinutes % 60}m', Colors.white),
                    _DetailRow('Wake-ups', '${_selectedSummary!.wakeUps}', Colors.white),
                    _DetailRow('Snoring', '${_selectedSummary!.snoringMeter.toStringAsFixed(1)} dB', Colors.white),
                    if (_selectedSummary!.dreamTranscription != null) ...[
                      const SizedBox(height: 12),
                      const Text('Dream:', style: TextStyle(color: Color(0xFF7C4DFF), fontSize: 13)),
                      const SizedBox(height: 4),
                      Text(
                        _selectedSummary!.dreamTranscription!,
                        style: const TextStyle(color: Colors.white54, fontSize: 13, fontStyle: FontStyle.italic),
                      ),
                    ],
                  ],
                ),
              ),
            )
          else
            const Expanded(
              child: Center(
                child: Text('Select a day to see details', style: TextStyle(color: Colors.white38)),
              ),
            ),
        ],
      ),
    );
  }
}

class _DetailRow extends StatelessWidget {
  final String label;
  final String value;
  final Color valueColor;

  const _DetailRow(this.label, this.value, this.valueColor);

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(color: Colors.white54, fontSize: 15)),
          Text(value, style: TextStyle(color: valueColor, fontSize: 15, fontWeight: FontWeight.w600)),
        ],
      ),
    );
  }
}
