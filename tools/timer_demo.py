#!/usr/bin/env python3
"""Publish retained kachel/state/timer payloads for on-device timer-card demos.

Usage:
  python3 tools/timer_demo.py start 5 "Pasta"   # timer ending in 5 minutes
  python3 tools/timer_demo.py soon              # ends in 70 s (T-60 cue + done pulse)
  python3 tools/timer_demo.py clear             # remove card

Credentials are read from include/secrets.h and never printed.
"""
import json
import re
import sys
from datetime import datetime, timedelta, timezone
from pathlib import Path

import paho.mqtt.publish as publish

SECRETS = Path(__file__).resolve().parent.parent / "include" / "secrets.h"


def secret(name: str) -> str:
    m = re.search(rf'#define {name}\s+"?([^"\n]+)"?', SECRETS.read_text())
    if not m:
        sys.exit(f"{name} not found in {SECRETS}")
    return m.group(1).strip()


def main() -> None:
    if len(sys.argv) < 2 or sys.argv[1] not in ("start", "soon", "clear"):
        sys.exit(__doc__)

    cmd = sys.argv[1]
    if cmd == "clear":
        payload = "{}"
    else:
        minutes = float(sys.argv[2]) if cmd == "start" else 70 / 60
        label = sys.argv[3] if len(sys.argv) > 3 else "Timer"
        ends_at = datetime.now(timezone.utc) + timedelta(minutes=minutes)
        payload = json.dumps({"label": label, "ends_at": ends_at.strftime("%Y-%m-%dT%H:%M:%SZ")})

    publish.single(
        "kachel/state/timer",
        payload,
        retain=True,
        hostname=secret("MQTT_HOST"),
        port=int(secret("MQTT_PORT")),
        auth={"username": secret("MQTT_USER"), "password": secret("MQTT_PASSWORD")},
    )
    print(f"published retained: {payload}")


if __name__ == "__main__":
    main()
