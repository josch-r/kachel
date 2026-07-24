---
name: gotcha-ha-fan-percentage-mapping
description: state/air fan field must map HA fan percentage to contract 0..3 without emitting 4 — the /33 ceil trap
metadata:
  type: project
---

HA-side `state/air` publishes a `fan` field the SPEC §7 contract bounds to `{fan: 0..3}`.
The Core 300S (fan.core_300s_series) reports `percentage` as 33 / 67 / 100 for low / medium / high
(3 speed levels via HA's ranged-value mapping).

**Trap:** deriving level as `(percentage / 33) | round(0, "ceil")` overflows the contract:
67/33=2.03→ceil 3, 100/33=3.03→**ceil 4** (out of range, and desyncs the cmd/air round trip —
device sends fan=3, state reports fan=4). Seen in ha/setup_m2.py (M2).

**Why:** contract exactness (§7) is a graded dimension; an out-of-range fan feeds M3 ambient
air-encoding and the M4 Air layer with wrong data, and breaks the send-3-read-3 loop.

**How to apply:** when reviewing any `state/air` fan derivation, plug in percentage=100 and 67.
A correct map is `(percentage / 100 * 3) | round(0)` (or ceil on /33.34) → 33→1, 67→2, 100→3.
Note the *cmd* side (`[33,66,100][fan-1]` + VeSync's own ceil) maps correctly; the bug is
state-side only.
