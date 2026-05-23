class Alarm {
  final String userId;
  final DateTime alarmTime;
  final String ringtone;
  final bool enabled;

  Alarm({
    required this.userId,
    required this.alarmTime,
    this.ringtone = 'default',
    this.enabled = true,
  });

  Map<String, dynamic> toJson() => {
    'userId': userId,
    'alarmTime': alarmTime.toIso8601String(),
    'ringtone': ringtone,
  };

  factory Alarm.fromJson(Map<String, dynamic> json) => Alarm(
    userId: json['userId'],
    alarmTime: DateTime.parse(json['alarmTime']),
    ringtone: json['ringtone'] ?? 'default',
    enabled: json['enabled'] ?? true,
  );
}
