#!/usr/bin/env python3
"""
serial_bridge.py — Bridges Flutter HTTP calls to the Tuya board over USB serial.

Usage:
    python serial_bridge.py --port /dev/cu.usbmodem5AAE1677231

The bridge is the source of truth for alarm state. It stores the alarm time
and checks the local clock every 30 s. When time matches it marks the alarm
as ringing so Flutter's next poll will see it. Serial commands to the board
are sent best-effort (sound plays independently on the board anyway).

Flutter must point to http://localhost:8080 (default in api_config.dart).
Stop the tos.py monitor before running this script.
"""

import argparse
import datetime
import json
import re
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

import serial

LISTEN_PORT = 8080
SERIAL_BAUD = 460800
RESPONSE_TIMEOUT = 2.0

# ── Test alarm ────────────────────────────────────────────────────────────────
# Set to "HH:MM" (24-hour) to pre-load an alarm for quick testing, e.g. "14:32"
# Set to "" to disable.
TEST_ALARM = "13:33"

# ── Serial (optional – best effort) ──────────────────────────────────────────
ser: serial.Serial | None = None
ser_lock = threading.Lock()


def send_command(cmd: str) -> str:
    """Send a CLI command to the board; return first non-log response line."""
    if ser is None:
        return ""
    try:
        with ser_lock:
            ser.reset_input_buffer()
            ser.write((cmd + "\r\n").encode())
            ser.flush()
            deadline = time.time() + RESPONSE_TIMEOUT
            while time.time() < deadline:
                line = ser.readline().decode(errors="replace").strip()
                if line and not line.startswith("["):
                    return line
    except Exception as e:
        print(f"[bridge] serial error: {e}")
    return ""


# ── Alarm state (bridge owns this) ───────────────────────────────────────────
state_lock = threading.Lock()
alarm_hour: int = -1
alarm_minute: int = -1
alarm_ringing: bool = False


def _check_alarm_loop():
    """Background thread: every 30 s check if it's alarm time."""
    global alarm_ringing
    while True:
        time.sleep(30)
        with state_lock:
            if alarm_hour < 0 or alarm_ringing:
                continue
            now = datetime.datetime.now()
            if now.hour == alarm_hour and now.minute == alarm_minute:
                alarm_ringing = True
                print(f"[bridge] ALARM TRIGGERED {alarm_hour:02d}:{alarm_minute:02d}")
                # Best-effort: tell board to ring too
                threading.Thread(
                    target=lambda: send_command("dismiss_alarm") or None,
                    daemon=True,
                ).start()


# ── HTTP handler ──────────────────────────────────────────────────────────────
class BridgeHandler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print(f"[bridge] {self.command} {self.path} → {fmt % args}")

    def _send_json(self, code: int, body: dict):
        data = json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def _read_body(self) -> dict:
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length) if length else b"{}"
        try:
            return json.loads(raw)
        except Exception:
            return {}

    # POST /api/alarms  {"alarmTime": "HH:MM" or ISO8601}
    def _handle_post_alarm(self):
        global alarm_hour, alarm_minute, alarm_ringing  # noqa: PLW0603
        body = self._read_body()
        alarm_time = body.get("alarmTime", "")
        m = re.search(r"(\d{1,2}):(\d{2})", alarm_time)
        if not m:
            self._send_json(400, {"error": "alarmTime not parseable"})
            return
        hh = m.group(1).zfill(2)
        mm = m.group(2)
        with state_lock:
            alarm_hour = int(hh)
            alarm_minute = int(mm)
            alarm_ringing = False
        print(f"[bridge] Alarm set to {hh}:{mm}")
        # Best-effort serial
        threading.Thread(
            target=lambda: send_command(f"set_alarm {hh} {mm}"),
            daemon=True,
        ).start()
        self._send_json(200, {"ok": True, "alarm": f"{hh}:{mm}"})

    # POST /api/alarm/dismiss
    def _handle_dismiss(self):
        global alarm_ringing  # noqa: PLW0603
        with state_lock:
            alarm_ringing = False
        print("[bridge] Alarm dismissed")
        threading.Thread(
            target=lambda: send_command("dismiss_alarm"),
            daemon=True,
        ).start()
        self._send_json(200, {"ok": True})

    # GET /api/alarm/status  → {ringing, alarmHour, alarmMinute}
    def _handle_alarm_status(self):
        with state_lock:
            self._send_json(200, {
                "ringing": alarm_ringing,
                "alarmHour": alarm_hour,
                "alarmMinute": alarm_minute,
            })

    # GET /api/sleep/summary/<userId>
    def _handle_sleep_summary(self):
        resp = send_command("get_status")
        parts = resp.split(":") if resp.startswith("STATUS") else []
        with state_lock:
            ah = int(parts[1]) if len(parts) > 2 else alarm_hour
            am = int(parts[2]) if len(parts) > 3 else alarm_minute
            aa = (parts[3] == "1") if len(parts) > 4 else (alarm_hour >= 0)
        self._send_json(200, {
            "sleepScore": 82,
            "totalTime": 443,
            "wakeUps": 2,
            "snoringMeter": 42.5,
            "dreamTranscription": "I was flying over a mountain...",
            "boardStatus": resp,
            "alarmHour": ah,
            "alarmMinute": am,
            "alarmActive": aa,
            "ringing": alarm_ringing,
        })

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_POST(self):
        if self.path == "/api/alarms":
            self._handle_post_alarm()
        elif self.path == "/api/alarm/dismiss":
            self._handle_dismiss()
        else:
            self._send_json(404, {"error": "not found"})

    def do_GET(self):
        if self.path == "/api/alarm/status":
            self._handle_alarm_status()
        elif self.path.startswith("/api/sleep/summary/"):
            self._handle_sleep_summary()
        else:
            self._send_json(404, {"error": "not found"})


# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="HackStorm serial bridge")
    parser.add_argument("--port", default="/dev/cu.usbmodem5AAE1677231",
                        help="Serial port for the Tuya board (optional)")
    args = parser.parse_args()

    global ser
    print(f"[bridge] Opening serial port {args.port} @ {SERIAL_BAUD}")
    try:
        ser = serial.Serial(args.port, SERIAL_BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"[bridge] WARNING: could not open serial port: {e}")
        print("[bridge] Running in bridge-only mode (alarm state managed here, no board sound).")
        ser = None

    if ser:
        print("[bridge] Waiting for board CLI to be ready...")
        for attempt in range(3):
            time.sleep(3)
            unix_ts = int(time.time())
            print(f"[bridge] set_time attempt {attempt+1}: {unix_ts}")
            resp = send_command(f"set_time {unix_ts}")
            print(f"[bridge] Board: {repr(resp)}")
            if resp == "TIME_SET":
                print("[bridge] Time synced.")
                break

    # Pre-load test alarm if set
    if TEST_ALARM:
        m = re.match(r"(\d{1,2}):(\d{2})", TEST_ALARM)
        if m:
            alarm_hour   = int(m.group(1))
            alarm_minute = int(m.group(2))
            print(f"[bridge] TEST_ALARM loaded: {alarm_hour:02d}:{alarm_minute:02d}")

    # Start background alarm checker
    t = threading.Thread(target=_check_alarm_loop, daemon=True)
    t.start()

    print(f"[bridge] Listening on http://localhost:{LISTEN_PORT}")
    HTTPServer(("", LISTEN_PORT), BridgeHandler).serve_forever()


if __name__ == "__main__":
    main()
