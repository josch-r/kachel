---
name: gotcha-carousel-gesture-rules
description: Review lens for carousel/swipe tasks — check against SPEC §4 gesture qualities and §5.12 clock-only night state, not just "it swipes"
metadata:
  type: project
---

When reviewing the swipe carousel (M1 shell and beyond): "LVGL tileview with snap" is not evidence that SPEC §4's gesture requirements are met.

**Why:** SPEC §4 mandates spring physics, interruptible mid-gesture, and "deltas apply immediately"; SPEC §5.9 "Interruptible always"; SPEC §5.8 "Never bounce" (watch elastic overscroll at carousel ends — Lights leftmost, Household rightmost). Stock tileview default behavior does not obviously satisfy these and they go untested if the acceptance criterion is just "swipe with snap."

**How to apply:** For any carousel/gesture task, require acceptance criteria that verify spring feel, mid-gesture interruptibility, immediate delta rendering, and defined end-of-carousel behavior. Also check the night interaction: SPEC §5.12 is "ultra-dim **clock only**" 22:00–00:00 — a plan that invokes §5.12 but only implements brightness levels has dropped the content-suppression behavior and left "what happens if you swipe at 23:00" undefined. Carousel order per §4 is Lights ← Home → Air → Household with Home as rest/boot tile. See [[gotcha-flicker-wifi-nvs]].

**Two recurring hard-rule edge cases in this repo (M1 review, verified non-blocking but always surface them):**
- **AGENTS.md rule 6 (durations only via timing.h):** the carousel snap/scroll animation runs on LVGL's *stock tileview scroll time*, not a `timing.h` constant. Not an ad-hoc literal (rule 6 not literally violated) and the M1 plan's risk section explicitly permits stock scroll if the on-device feel test passes (it did). SPEC §8 defers the full timing-scale audit to M5. So flag it as an M5 foreshadow, do not block M1 on it.
- **AGENTS.md rule 5 (colors only via palette.h):** `display.cpp` uses `lv_color_black()` for the 00:00–06:00 blackout overlay instead of a palette token. It is functional darkness (invisible — backlight slews to 0) and not a hex literal, so judged non-blocking; the clean fix is a `KACHEL_BG_BLACK` token. Surface it as the top non-blocking note since Josch treats the ratchet rules strictly.

**Compress-test-flag hygiene:** `KACHEL_SCHEDULE_COMPRESS_TEST` must be **committed as 0**. During evidence filming the working tree carries an uncommitted flip to 1 — that is correct/expected. Check `git diff` vs HEAD: HEAD=0 is clean; a *committed* 1 is a defect.
