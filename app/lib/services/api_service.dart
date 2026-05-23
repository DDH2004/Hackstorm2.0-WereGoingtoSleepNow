import 'dart:convert';
import 'package:http/http.dart' as http;
import '../config/api_config.dart';
import '../models/alarm.dart';
import '../models/sleep_summary.dart';
import '../models/dream_entry.dart';

// ============================================================
// API SERVICE — All cloud backend calls go through here.
//
// WORKFLOW OVERVIEW:
//   1. User sets alarm   → postAlarm()      → backend stores + MQTT to board
//   2. User dismisses    → dismissAlarm()    → backend tells board to prompt dream
//   3. After sleep        → getSleepSummary() → backend aggregates overnight data
//   4. Dream recording   → uploadDreamAudio()→ backend runs speech-to-text
//
// TODO [API]: Each method has a stub response for offline dev.
//             Replace with real HTTP calls once backend is running.
// ============================================================

class ApiService {
  final String _baseUrl = ApiConfig.baseUrl;

  // ----------------------------------------------------------
  // POST /api/alarms
  // Sends alarm config to backend. Backend stores it and pushes
  // to Tuya Board via MQTT topic: tuya/alarm/config
  // ----------------------------------------------------------
  Future<bool> postAlarm(Alarm alarm) async {
    // TODO [API]: Uncomment when backend is live
    // final response = await http.post(
    //   Uri.parse('$_baseUrl${ApiConfig.postAlarm}'),
    //   headers: ApiConfig.headers,
    //   body: jsonEncode(alarm.toJson()),
    // );
    // return response.statusCode == 200;

    // STUB: Simulate success
    await Future.delayed(const Duration(milliseconds: 500));
    print('[STUB] POST /api/alarms → ${alarm.toJson()}');
    return true;
  }

  // ----------------------------------------------------------
  // POST /api/alarm/dismiss
  // Tells backend user dismissed the alarm. Backend publishes
  // MQTT 'dismiss' to board → board prompts "What did you dream?"
  // ----------------------------------------------------------
  Future<bool> dismissAlarm(String userId) async {
    // TODO [API]: Uncomment when backend is live
    // final response = await http.post(
    //   Uri.parse('$_baseUrl${ApiConfig.dismissAlarm}'),
    //   headers: ApiConfig.headers,
    //   body: jsonEncode({
    //     'userId': userId,
    //     'timestamp': DateTime.now().toIso8601String(),
    //   }),
    // );
    // return response.statusCode == 200;

    await Future.delayed(const Duration(milliseconds: 500));
    print('[STUB] POST /api/alarm/dismiss → userId: $userId');
    return true;
  }

  // ----------------------------------------------------------
  // GET /api/sleep/summary/{userId}
  // Returns last night's aggregated sleep data:
  //   sleepScore, totalTime, wakeUps, snoringMeter, dreamTranscription
  //
  // Backend computes this from:
  //   - Camera frames  → movement/wake detection (Cloud ML)
  //   - Audio segments → snoring classification (Cloud ML)
  //   - Dream audio    → speech-to-text (Google Cloud Speech)
  // ----------------------------------------------------------
  Future<SleepSummary> getSleepSummary(String userId) async {
    // TODO [API]: Uncomment when backend is live
    // final response = await http.get(
    //   Uri.parse('$_baseUrl${ApiConfig.sleepSummary(userId)}'),
    //   headers: ApiConfig.headers,
    // );
    // if (response.statusCode == 200) {
    //   return SleepSummary.fromJson(jsonDecode(response.body));
    // }
    // throw Exception('Failed to load sleep summary');

    // STUB: Return fake data for UI development
    await Future.delayed(const Duration(milliseconds: 800));
    return SleepSummary(
      sleepScore: 82,
      totalTime: const Duration(hours: 7, minutes: 23),
      wakeUps: 2,
      snoringMeter: 42.5,
      dreamTranscription: 'I was flying over a mountain and then landed in a field of sunflowers...',
      date: DateTime.now(),
    );
  }

  // ----------------------------------------------------------
  // GET /api/sleep/history/{userId}
  // Returns list of past sleep summaries for the calendar view.
  //
  // TODO [API]: This endpoint isn't in the spec yet — add to backend.
  //             For now returns stub data.
  // ----------------------------------------------------------
  Future<List<SleepSummary>> getSleepHistory(String userId) async {
    // TODO [API]: Implement once backend adds this endpoint
    await Future.delayed(const Duration(milliseconds: 800));

    // STUB: Generate 7 days of fake history
    return List.generate(7, (i) => SleepSummary(
      sleepScore: 70 + (i * 4) % 30,
      totalTime: Duration(hours: 6 + i % 3, minutes: 10 + i * 7),
      wakeUps: i % 3,
      snoringMeter: 30.0 + i * 5,
      dreamTranscription: i % 2 == 0 ? 'A dream about cats...' : null,
      date: DateTime.now().subtract(Duration(days: i)),
    ));
  }

  // ----------------------------------------------------------
  // POST /api/dream/audio
  // Uploads 30-second voice recording for speech-to-text.
  //
  // NOTE: The workflow doc says the Tuya Board captures this,
  //       but the app can also record directly from the phone mic.
  //       Pick whichever path your team prefers.
  // ----------------------------------------------------------
  Future<String?> uploadDreamAudio(String userId, String audioFilePath) async {
    // TODO [API]: Uncomment when backend is live
    // final request = http.MultipartRequest(
    //   'POST',
    //   Uri.parse('$_baseUrl${ApiConfig.postDreamAudio}'),
    // );
    // request.fields['userId'] = userId;
    // request.files.add(await http.MultipartFile.fromPath('audio', audioFilePath));
    // final response = await request.send();
    // if (response.statusCode == 200) {
    //   final body = await response.stream.bytesToString();
    //   return jsonDecode(body)['transcription'];
    // }
    // return null;

    await Future.delayed(const Duration(seconds: 2));
    print('[STUB] POST /api/dream/audio → file: $audioFilePath');
    return 'I dreamed about walking through a forest with talking animals...';
  }

  // ----------------------------------------------------------
  // GET dream journal entries
  // TODO [API]: Add endpoint to backend: GET /api/dreams/{userId}
  // ----------------------------------------------------------
  Future<List<DreamEntry>> getDreamJournal(String userId) async {
    // TODO [API]: Implement once backend adds this endpoint
    await Future.delayed(const Duration(milliseconds: 500));

    return [
      DreamEntry(date: DateTime.now().subtract(const Duration(days: 1)), transcription: 'Flying over mountains...'),
      DreamEntry(date: DateTime.now().subtract(const Duration(days: 3)), transcription: 'Walking through a city made of glass...'),
      DreamEntry(date: DateTime.now().subtract(const Duration(days: 5)), transcription: 'Swimming with dolphins in a purple ocean...'),
    ];
  }
}
