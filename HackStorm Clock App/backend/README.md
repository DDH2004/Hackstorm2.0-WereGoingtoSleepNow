# HackStorm Backend (FastAPI)

Minimal FastAPI server for the Flutter app MVP.

## Setup

```bash
cd "HackStorm Clock App/backend"
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Run

```bash
uvicorn main:app --reload --port 8000
```

## Endpoints (MVP)

- POST /api/alarms
- GET /api/alarms
- POST /api/alarm/dismiss
- GET /api/sleep/summary/{userId}
- GET /api/sleep/history/{userId}
- POST /api/dream/audio (multipart)
- GET /api/dreams/{userId}

Health check: GET /api/health
