class SleepSummary {
  final int sleepScore;        // 0-100
  final Duration totalTime;
  final int wakeUps;
  final double snoringMeter;   // dB
  final String? dreamTranscription;
  final DateTime date;

  SleepSummary({
    required this.sleepScore,
    required this.totalTime,
    required this.wakeUps,
    required this.snoringMeter,
    this.dreamTranscription,
    required this.date,
  });

  factory SleepSummary.fromJson(Map<String, dynamic> json) => SleepSummary(
    sleepScore: json['sleepScore'] ?? 0,
    totalTime: Duration(minutes: json['totalTimeMinutes'] ?? 0),
    wakeUps: json['wakeUps'] ?? 0,
    snoringMeter: (json['snoringMeter'] ?? 0).toDouble(),
    dreamTranscription: json['dreamTranscription'],
    date: DateTime.parse(json['date'] ?? DateTime.now().toIso8601String()),
  );
}
