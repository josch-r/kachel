#pragma once

#include <lvgl.h>

// Link mark for the control layers (§10 2026-09-29). While the broker link
// is up: one dim ring. Down longer than KACHEL_T_OFFLINE_GRACE_MS: the ring
// grows into an "Offline · N min" pill and the registered controls dim and
// stop taking taps. Tapping the mark opens a diagnostics card (WiFi, broker,
// topic ages); tapping the card closes it. Call last in a layer's init so the
// mark and card sit above the controls.
void conn_status_attach(lv_obj_t *tile, lv_align_t align, int32_t x, int32_t y,
                        lv_obj_t *const *controls, int n_controls);
