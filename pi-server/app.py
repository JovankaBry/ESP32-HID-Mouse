"""
Tiny Flask receiver for the ESP32's periodic temperature reports.

Keeps a rolling in-memory buffer of the last MAX_READINGS reports (no
persistence — a restart clears history, which is fine since tracklly's own
backend polls this endpoint every few minutes and persists what it reads
into its own database; this server is just the ESP32's landing point).

Endpoints:
  POST /api/temp   body: {"temp": <float>}      -> 201, appends a reading
  GET  /api/temp    -> [{"temp": <float>, "timestamp": <iso8601>}, ...]
                       oldest-first, same convention tracklly's own
                       temperature history endpoints already use.
  GET  /            -> plain-text status page, just for a quick sanity
                       check when hitting this port directly; the real
                       chart lives in tracklly's Pi Tracker page.
"""

from datetime import datetime, timezone
from threading import Lock

from flask import Flask, jsonify, request

app = Flask(__name__)

MAX_READINGS = 300
readings = []
readings_lock = Lock()


@app.post("/api/temp")
def post_temp():
    body = request.get_json(silent=True)
    if not body or "temp" not in body:
        return jsonify({"error": "expected JSON body with a 'temp' field"}), 400

    try:
        temp = float(body["temp"])
    except (TypeError, ValueError):
        return jsonify({"error": "'temp' must be a number"}), 400

    reading = {
        "temp": temp,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds"),
    }

    with readings_lock:
        readings.append(reading)
        del readings[:-MAX_READINGS]

    return jsonify(reading), 201


@app.get("/api/temp")
def get_temp():
    with readings_lock:
        return jsonify(list(readings))


@app.get("/")
def index():
    with readings_lock:
        count = len(readings)
        latest = readings[-1] if readings else None
    if latest:
        return f"ESP32 temp receiver — {count} readings buffered, latest: {latest['temp']}°C @ {latest['timestamp']}"
    return f"ESP32 temp receiver — {count} readings buffered, none yet"


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
