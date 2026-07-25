#pragma once

#include <lvgl.h>

// Air layer (SPEC §3 job 3): fan speed control (0..3, state-fed highlight),
// PM2.5 numeral + 24 h history chart, filter life. cmd/air fires on
// gesture completion only (§4); displayed level follows kachel/state/air.
void air_layer_init(lv_obj_t *tile);
