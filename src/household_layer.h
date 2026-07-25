#pragma once

#include <lvgl.h>

// Household layer (SPEC §3 job 4): next shared calendar events (max 3) on
// top, Bring! list summary (count + top 5) below. Read-only; fixed anchors.
void household_layer_init(lv_obj_t *tile);
