#!/usr/bin/env python3
"""48 h soak logger (SPEC §8 M5): append every kachel/sys/status heartbeat to
a JSONL file with a wall-clock stamp. Reboots show as uptime resets; leaks as
a negative heap slope. Run detached:

  nohup python3 tools/soak_log.py > /dev/null 2>&1 &

Analyze with: python3 tools/soak_log.py --report
"""
import json
import re
import sys
import time
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOG = ROOT / ".kachel" / "soak.jsonl"


def secret(name: str) -> str:
    txt = (ROOT / "include" / "secrets.h").read_text()
    return re.search(rf'#define {name}\s+"?([^"\n]+)"?', txt).group(1).strip()


def report() -> None:
    rows = [json.loads(l) for l in LOG.read_text().splitlines() if l.strip()]
    if not rows:
        sys.exit("no data")
    resets = sum(1 for a, b in zip(rows, rows[1:]) if b["uptime"] < a["uptime"])
    hours = (rows[-1]["ts_epoch"] - rows[0]["ts_epoch"]) / 3600
    heaps = [r["heap"] for r in rows if "heap" in r]
    print(f"span {hours:.1f} h, {len(rows)} beats, uptime resets: {resets}")
    if heaps:
        print(f"heap first/min/last: {heaps[0]}/{min(heaps)}/{heaps[-1]}")
    gaps = [(b["ts_epoch"] - a["ts_epoch"]) for a, b in zip(rows, rows[1:])]
    long_gaps = [g for g in gaps if g > 180]
    print(f"gaps >3 min: {len(long_gaps)}" + (f" (max {max(long_gaps):.0f} s)" if long_gaps else ""))


def main() -> None:
    if "--report" in sys.argv:
        report()
        return
    import paho.mqtt.subscribe as subscribe

    LOG.parent.mkdir(exist_ok=True)
    while True:
        try:
            m = subscribe.simple(
                "kachel/sys/status",
                hostname=secret("MQTT_HOST"),
                port=int(secret("MQTT_PORT")),
                auth={"username": secret("MQTT_USER"), "password": secret("MQTT_PASSWORD")},
            )
            row = json.loads(m.payload.decode())
            row["ts"] = datetime.now().isoformat(timespec="seconds")
            row["ts_epoch"] = time.time()
            with LOG.open("a") as f:
                f.write(json.dumps(row) + "\n")
        except Exception:
            time.sleep(30)  # broker outage (e.g. router-off test) — keep trying


if __name__ == "__main__":
    main()
