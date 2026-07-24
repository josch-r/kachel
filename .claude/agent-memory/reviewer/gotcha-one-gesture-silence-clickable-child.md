---
name: gotcha-one-gesture-silence-clickable-child
description: SPEC §4 "single tap anywhere silences" breaks when a clickable child (e.g. timer card) swallows the tap before the tile's CLICKED handler
metadata:
  type: project
---

The §4 one-gesture-silence dismiss handler is attached to the *tile* via `LV_EVENT_CLICKED`. A tap only reaches the tile if the object under the finger is non-clickable.

**Why:** In LVGL 9.2.2, base `lv_obj_create` sets `LV_OBJ_FLAG_CLICKABLE` by default (lv_obj.c:496). `lv_canvas`/`lv_image` and `lv_label` constructors *remove* CLICKABLE (lv_image.c:594, lv_label.c:702), so taps on the field/clock/calendar pass through to the tile — good. But any element built with `lv_obj_create` (the timer guest card) stays clickable and, without `LV_OBJ_FLAG_EVENT_BUBBLE`, swallows the tap. Result: tapping the glowing done-timer card — the most likely dismiss target — does NOT silence it; only taps elsewhere or the 60 s self-decay work. `lv_obj_remove_style_all()` does NOT clear flags.

**How to apply:** For any alerting/dismissable overlay element on the face, verify it either has EVENT_BUBBLE, has CLICKABLE removed, or carries its own dismiss handler. When reviewing §4, enumerate every clickable child that overlaps the alerting region, not just the canvas.
