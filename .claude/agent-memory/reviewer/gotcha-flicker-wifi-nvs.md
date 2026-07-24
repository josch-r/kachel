---
name: gotcha-flicker-wifi-nvs
description: Review lens for the "no flicker at rest" criterion — WiFi/NVS writes and current spikes are the real triggers, and flicker tests must specify radio-on conditions
metadata:
  type: project
---

When reviewing any milestone that claims "no flicker at rest" (SPEC §8 M1, RESEARCH §1 quirks 1 & 4): the flicker trigger on the Guition 4848S040 is any flash/NVS write stalling the RGB refill, plus WiFi current spikes on weak supplies.

**Why:** RESEARCH §1 quirk 1 (flash+PSRAM shared bus) and quirk 4 (WiFi current spikes) are the documented flicker causes. A flicker test run with the radio idle/off gives a false pass, because the shipped rest state runs WiFi continuously and the WiFi stack writes RF-calibration/state to NVS.

**How to apply:** Reject a flicker test whose acceptance criterion doesn't state (a) WiFi connected + any background netstack active during the test, (b) the concrete evidence artifact (video/serial, not "observed"), and (c) that the documented build-flag mitigations — `CONFIG_SPIRAM_XIP_FROM_PSRAM`, bounce buffers, `DATA_CACHE_LINE_64B` — are actually committed, not hidden behind a generic "build flags" note. Watch for WiFi being pulled into an early milestone (M1) "out of necessity" for SNTP while networking is officially sequenced at M2. See [[gotcha-carousel-gesture-rules]].
