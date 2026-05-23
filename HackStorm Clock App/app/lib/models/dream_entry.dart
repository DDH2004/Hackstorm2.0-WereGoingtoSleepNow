class DreamEntry {
  final DateTime date;
  final String transcription;

  DreamEntry({
    required this.date,
    required this.transcription,
  });

  factory DreamEntry.fromJson(Map<String, dynamic> json) => DreamEntry(
    date: DateTime.parse(json['date']),
    transcription: json['transcription'] ?? '',
  );
}
