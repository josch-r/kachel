---
name: gotcha-carousel-gesture-rules
description: Review lens for carousel/swipe tasks — check against SPEC §4 gesture qualities and §5.12 clock-only night state, not just "it swipes"
metadata:
  type: project
---

When reviewing the swipe carousel (M1 shell and beyond): "LVGL tileview with snap" is not evidence that SPEC §4's gesture requirements are met.

**Why:** SPEC §4 mandates spring physics, interruptible mid-gesture, and "deltas apply immediately"; SPEC §5.9 "Interruptible always"; SPEC §5.8 "Never bounce" (watch elastic overscroll at carousel ends — Lights leftmost, Household rightmost). Stock tileview default behavior does not obviously satisfy these and they go untested if the acceptance criterion is just "swipe with snap."

**How to apply:** For any carousel/gesture task, require acceptance criteria that verify spring feel, mid-gesture interruptibility, immediate delta rendering, and defined end-of-carousel behavior. Also check the night interaction: SPEC §5.12 is "ultra-dim **clock only**" 22:00–00:00 — a plan that invokes §5.12 but only implements brightness levels has dropped the content-suppression behavior and left "what happens if you swipe at 23:00" undefined. Carousel order per §4 is Lights ← Home → Air → Household with Home as rest/boot tile. See [[gotcha-flicker-wifi-nvs]].
