import 'dart:async';
import 'package:flutter/foundation.dart';
import '../models/alarm.dart';
import '../models/sleep_summary.dart';
import '../models/dream_entry.dart';
import 'api_service.dart';
import 'tuya_service.dart';

// ============================================================
// APP STATE (Provider)
// Central state for the app. Screens read from here.
// ============================================================

class AppState extends ChangeNotifier {
  final ApiService _api = ApiService();
  final TuyaService _tuya = TuyaService();

  final String userId = 'demo-user-001';

  Alarm? currentAlarm;
  SleepSummary? lastSleepSummary;
  List<SleepSummary> sleepHistory = [];
  List<DreamEntry> dreamJournal = [];

  bool isLoading = false;
  bool alarmIsRinging = false;

  Timer? _pollTimer;

  AppState() {
    // Poll the bridge every 30 s to detect when the alarm fires.
    _pollTimer = Timer.periodic(const Duration(seconds: 30), (_) => _pollAlarmStatus());
    // Also check immediately on startup.
    _pollAlarmStatus();
  }

  Future<void> _pollAlarmStatus() async {
    final ringing = await _api.getAlarmRinging();
    if (ringing != alarmIsRinging) {
      alarmIsRinging = ringing;
      notifyListeners();
    }
  }

  @override
  void dispose() {
    _pollTimer?.cancel();
    super.dispose();
  }

  // ----------------------------------------------------------
  // EVENING FLOW: User sets alarm
  // App → Cloud API → MQTT → Tuya Board
  // ----------------------------------------------------------
  Future<void> saveAlarm(DateTime time, String ringtone) async {
    currentAlarm = Alarm(
      userId: userId,
      alarmTime: time,
      ringtone: ringtone,
    );

    isLoading = true;
    notifyListeners();

    // Send to cloud (which relays to board via MQTT)
    await _api.postAlarm(currentAlarm!);

    // TODO [TUYA]: Optionally also send directly to board for redundancy
    // await _tuya.sendAlarmToBoard(currentAlarm!);

    isLoading = false;
    notifyListeners();
  }

  // ----------------------------------------------------------
  // MORNING FLOW: User dismisses alarm
  // ----------------------------------------------------------
  Future<void> dismissAlarm() async {
    alarmIsRinging = false;
    notifyListeners();

    await _api.dismissAlarm(userId);
    await loadSleepSummary();
  }

  // ----------------------------------------------------------
  // Load last night's sleep data from cloud
  // Cloud aggregates: camera frames + audio analysis + dream STT
  // ----------------------------------------------------------
  Future<void> loadSleepSummary() async {
    isLoading = true;
    notifyListeners();

    lastSleepSummary = await _api.getSleepSummary(userId);

    isLoading = false;
    notifyListeners();
  }

  // ----------------------------------------------------------
  // Load sleep history for calendar view
  // ----------------------------------------------------------
  Future<void> loadSleepHistory() async {
    sleepHistory = await _api.getSleepHistory(userId);
    notifyListeners();
  }

  // ----------------------------------------------------------
  // Record and upload dream audio
  // Audio → Cloud → Google Speech-to-Text → stored in DB
  // ----------------------------------------------------------
  Future<String?> submitDreamRecording(String audioPath) async {
    isLoading = true;
    notifyListeners();

    final transcription = await _api.uploadDreamAudio(userId, audioPath);

    isLoading = false;
    notifyListeners();
    return transcription;
  }

  // ----------------------------------------------------------
  // Load dream journal entries
  // ----------------------------------------------------------
  Future<void> loadDreamJournal() async {
    dreamJournal = await _api.getDreamJournal(userId);
    notifyListeners();
  }

  // ----------------------------------------------------------
  // Add a dream entry locally (after simulated recording)
  // ----------------------------------------------------------
  void addDreamEntry(String transcription) {
    dreamJournal = [
      DreamEntry(date: DateTime.now(), transcription: transcription),
      ...dreamJournal,
    ];
    notifyListeners();
  }
}
