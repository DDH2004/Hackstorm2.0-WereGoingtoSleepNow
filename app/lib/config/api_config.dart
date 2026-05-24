// ============================================================
// API & TUYA CONFIGURATION
// All endpoints and keys live here. Fill these in first.
// ============================================================

class ApiConfig {
  // TODO [API]: Set your cloud backend base URL
  //   e.g. "https://your-firebase-project.cloudfunctions.net"
  //   or   "http://localhost:3000" for local dev
  // Use your Mac's LAN IP (run: ipconfig getifaddr en0) when on a physical device.
  // Use 'http://localhost:8080' for the iOS simulator.
  static const String baseUrl = 'http://192.168.34.207:8080';

  // --- REST Endpoints ---
  static const String postAlarm       = '/api/alarms';          // POST { userId, alarmTime, ringtone }
  static const String dismissAlarm    = '/api/alarm/dismiss';   // POST { userId, timestamp }
  static String sleepSummary(String userId) => '/api/sleep/summary/$userId'; // GET
  static const String postDreamAudio  = '/api/dream/audio';     // POST (multipart, audio file)

  // TODO [API]: Add auth token / Firebase ID token header if needed
  static Map<String, String> get headers => {
    'Content-Type': 'application/json',
    // 'Authorization': 'Bearer <TOKEN>',
  };
}

class TuyaConfig {
  // TODO [TUYA]: Fill in from Tuya IoT Platform → Cloud → your project
  static const String accessId     = '';  // Tuya Access ID
  static const String accessSecret = '';  // Tuya Access Secret
  static const String deviceId     = '';  // Your Tuya Board device ID
  static const String region       = 'us'; // us, eu, cn, in

  // MQTT topics (must match firmware)
  static const String topicAlarmConfig  = 'tuya/alarm/config';
  static const String topicAlarmDismiss = 'tuya/alarm/dismiss';
  static const String topicAlarmTriggered = 'tuya/alarm/triggered';
  static const String topicDreamAudio   = 'tuya/dream/audio';
}
