---
name: gotcha-ring-window-refresh-guard
description: Scrolling-window charts fed from the PM2.5 ring freeze when the redraw guard only compares the newest sample — check this on any history/ring UI change
metadata:
  type: project
---

Kachel UI refresh callbacks (1 Hz `lv_timer`) guard redraws with `shown_*` caches to stay
calm/cheap. For the **PM2.5 24 h chart** that pattern is a trap: a guard of the form
`if (n != shown_samples || chart_vals[N-1] != samples[n-1])` stops firing once the ring is
full (`n` pinned at 288 after 24 h uptime) and consecutive samples are equal — the window
has shifted but nothing redraws, so old features (e.g. a spike) stay frozen on screen even
after they have aged out of the ring entirely. Found and fixed during M4 review; the accepted
shape is `bool state_history_tick()` returning "a sample was appended", with the chart
rebuilding on that signal alone. Do not let a value-compare guard come back.

**Why:** PM2.5 is an integer µg/m³ and sits constant for hours in a clean room, so the
"newest value changed" proxy is false most ticks. Manifests only after 24 h uptime — i.e.
exactly in the §8 M5 48 h soak, not in a bench test.

**How to apply:** For any windowed/scrolling view fed by a ring buffer, require the redraw
guard to key on an *append/sequence counter* (or a bool return from the tick), never on the
newest value. Verify with a tiny host simulation: fill the ring, then feed a constant value
after a spike and check the spike scrolls off. Two related facts worth re-checking:
- Air staleness uses `a.valid` only (air is on-change, §7), so during a broker outage the
  tick keeps sampling the last-known pm25 and fabricates a flat trace with no staleness
  mark — deferred to M5 fail-calm, still open as of M4 accept.
- `LV_LABEL_LONG_DOT` mutates `label->text` in place (lvgl 9.2 lv_label.c:1386), so a
  `strcmp(lv_label_get_text(..), new)` redraw-skip guard never matches on truncated lines.
  Harmless, but not proof that redraws are suppressed.

See [[gotcha-ha-fan-percentage-mapping]].
