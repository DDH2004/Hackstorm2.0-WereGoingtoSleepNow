#!/usr/bin/env python3
"""
serial_bridge.py — Bridges Flutter HTTP calls to the Tuya board over USB serial.

Usage:
    python serial_bridge.py --port /dev/cu.usbmodem5AAE1677233

Flutter app must point to http://localhost:8080 (already the default in api_config.dart).
Stop the tos.py monitor before running this script (they share the same serial port).
"""

import argparse
import json
import re
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

import serial

LISTEN_PORT = 8080
SERIAL_BAUD = 460800
RESPONSE_TIMEOUT = 3.0  # seconds to wait for board response

ser: serial.Serial = None
ser_lock = threading.Lock()


def send_command(cmd: str) -> str:
    """Send a CLI command to the board and return the first response line."""
    with ser_lock:
        ser.reset_input_buffer()
        ser.write((cmd + "\r\n").encode())
        ser.flush()
        deadline = time.time() + RESPONSE_TIMEOUT
        while time.time() < deadline:
            line = ser.readline().decode(errors="replace").strip()
            # Skip log lines (they start with '['); wait for our response
            if line and not line.startswith("["):
                return line
        return ""


class BridgeHandler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        print(f"[bridge] {fmt % args}")

    def _send_json(self, code: int, body: dict):
        data = json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _read_body(self) -> dict:
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length) if length else b"{}"
        try:
            return json.loads(raw)
        except Exception:
            return {}

    # POST /api/alarms  →  set_alarm <HH> <MM>
    def _handle_post_alarm(self):
        body = self._read_body()
        alarm_time = body.get("alarmTime", "")
        # Accept ISO8601 or "HH:MM" strings
        m = re.search(r"(\d{1,2}):(\d{2})", alarm_time)
        if not m:
            self._send_json(400, {"error": "alarmTime not parseable"})
            return
        hh, mm = m.group(1).zfill(2), m.group(2)
        resp = send_command(f"set_alarm {hh} {mm}")
        if resp.startswith("ALARM_SET"):
            self._send_json(200, {"ok": True, "alarm": f"{hh}:{mm}"})
        else:
            self._send_json(500, {"error": "board did not confirm", "raw": resp})

    # POST /api/alarm/dismiss  →  dismiss_alarm
    def _handle_dismiss(self):
        resp = send_command("dismiss_alarm")
        if resp == "ALARM_DISMISSED":
            self._send_json(200, {"ok": True})
        else:
            self._send_json(500, {"error": "board did not confirm", "raw": resp})

    # GET /api/sleep/summary/<userId>  →  get_status + stub sleep data
    def _handle_sleep_summary(self):
        resp = send_command("get_status")
        # STATUS:<alarm_h>:<alarm_m>:<active>:<cur_h>:<cur_m>
        parts = resp.split(":") if resp.startswith("STATUS") else []
        self._send_json(200, {
            "sleepScore": 82,
            "totalTime": 443,
            "wakeUps": 2,
            "snoringMeter": 42.5,
            "dreamTranscription": "I was flying over a mountain...",
            "boardStatus": resp,
            "alarmHour":   int(parts[1]) if len(parts) > 2 else -1,
            "alarmMinute": int(parts[2]) if len(parts) > 3 else -1,
            "alarmActive": parts[3] == "1" if len(parts) > 4 else False,
        })

    def do_POST(self):
        if self.path == "/api/alarms":
            self._handle_post_alarm()
        elif self.path == "/api/alarm/dismiss":
            self._handle_dismiss()
        else:
            self._send_json(404, {"error": "not found"})

    def do_GET(self):
        if self.path.startswith("/api/sleep/summary/"):
            self._handle_sleep_summary()
        else:
            self._send_json(404, {"error": "not found"})


def main():
    parser = argparse.ArgumentParser(description="HackStorm serial bridge")
    parser.add_argument("--port", default="/dev/cu.usbmodem5AAE1677233",
                        help="Serial port for the Tuya board")
    args = parser.parse_args()

    global ser
    print(f"[bridge] Opening serial port {args.port} @ {SERIAL_BAUD}")
    try:
        ser = serial.Serial(args.port, SERIAL_BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"[bridge] ERROR: {e}")
        print("[bridge] Make sure tos.py monitor is stopped first (Ctrl+C it).")
        sys.exit(1)

    print(f"[bridge] Listening on http://localhost:{LISTEN_PORT}")
    print("[bridge] Flutter app should point to http://localhost:8080")
    HTTPServer(("localhost", LISTEN_PORT), BridgeHandler).serve_forever()


if __name__ == "__main__":
    main()
