from __future__ import annotations

from datetime import datetime, timedelta, timezone
from typing import Dict, List, Optional
import uuid

from fastapi import FastAPI, File, HTTPException, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

app = FastAPI(title="HackStorm API", version="0.1.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


class AlarmIn(BaseModel):
    userId: str
    alarmTime: str
    ringtone: str = "default"
    enabled: bool = True


class AlarmOut(AlarmIn):
    alarmId: str
    createdAt: str


class DismissIn(BaseModel):
    userId: str
    timestamp: str


class SleepSummaryOut(BaseModel):
    sleepScore: int
    totalTimeMinutes: int
    wakeUps: int
    snoringMeter: float
    dreamTranscription: Optional[str]
    date: str


class DreamEntryOut(BaseModel):
    date: str
    transcription: str


alarms_by_user: Dict[str, List[AlarmOut]] = {}
last_dismiss_by_user: Dict[str, str] = {}


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _sample_summary(date: datetime, has_dream: bool) -> SleepSummaryOut:
    dream_text = None
    if has_dream:
        dream_text = "I dreamed about walking through a forest with talking animals."

    return SleepSummaryOut(
        sleepScore=82,
        totalTimeMinutes=443,
        wakeUps=2,
        snoringMeter=42.5,
        dreamTranscription=dream_text,
        date=date.isoformat(),
    )


@app.get("/api/health")
def health() -> Dict[str, str]:
    return {"status": "ok", "time": _now_iso()}


@app.post("/api/alarms", response_model=AlarmOut)
def create_alarm(payload: AlarmIn) -> AlarmOut:
    alarm = AlarmOut(
        alarmId=str(uuid.uuid4()),
        createdAt=_now_iso(),
        userId=payload.userId,
        alarmTime=payload.alarmTime,
        ringtone=payload.ringtone,
        enabled=payload.enabled,
    )
    alarms_by_user.setdefault(payload.userId, []).append(alarm)
    return alarm


@app.get("/api/alarms")
def list_alarms(userId: Optional[str] = None) -> Dict[str, List[AlarmOut]]:
    if userId is None:
        all_alarms: List[AlarmOut] = []
        for user_alarms in alarms_by_user.values():
            all_alarms.extend(user_alarms)
        return {"alarms": all_alarms}

    return {"alarms": alarms_by_user.get(userId, [])}


@app.post("/api/alarm/dismiss")
def dismiss_alarm(payload: DismissIn) -> Dict[str, str]:
    last_dismiss_by_user[payload.userId] = payload.timestamp
    return {"status": "dismissed", "timestamp": payload.timestamp}


@app.get("/api/sleep/summary/{user_id}", response_model=SleepSummaryOut)
def sleep_summary(user_id: str) -> SleepSummaryOut:
    if user_id == "":
        raise HTTPException(status_code=400, detail="userId required")

    return _sample_summary(datetime.now(timezone.utc), has_dream=True)


@app.get("/api/sleep/history/{user_id}")
def sleep_history(user_id: str) -> Dict[str, List[SleepSummaryOut]]:
    if user_id == "":
        raise HTTPException(status_code=400, detail="userId required")

    today = datetime.now(timezone.utc)
    items: List[SleepSummaryOut] = []
    for i in range(7):
        items.append(_sample_summary(today - timedelta(days=i), has_dream=i % 2 == 0))

    return {"sessions": items}


@app.post("/api/dream/audio")
def upload_dream_audio(userId: str, audio: UploadFile = File(...)) -> Dict[str, str]:
    if userId == "":
        raise HTTPException(status_code=400, detail="userId required")

    if audio is None:
        raise HTTPException(status_code=400, detail="audio file required")

    return {
        "transcription": "I dreamed about walking through a forest with talking animals.",
        "fileName": audio.filename or "unknown",
    }


@app.get("/api/dreams/{user_id}")
def dream_journal(user_id: str) -> Dict[str, List[DreamEntryOut]]:
    if user_id == "":
        raise HTTPException(status_code=400, detail="userId required")

    now = datetime.now(timezone.utc)
    entries = [
        DreamEntryOut(
            date=(now - timedelta(days=1)).isoformat(),
            transcription="Flying over mountains...",
        ),
        DreamEntryOut(
            date=(now - timedelta(days=3)).isoformat(),
            transcription="Walking through a city made of glass...",
        ),
        DreamEntryOut(
            date=(now - timedelta(days=5)).isoformat(),
            transcription="Swimming with dolphins in a purple ocean...",
        ),
    ]

    return {"entries": entries}
