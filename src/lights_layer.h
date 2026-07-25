#pragma once

#include <lvgl.h>

// Lights layer (SPEC §3 job 2): 2x2 scene grid, ~200 px targets (§4),
// cmd/scene fires on gesture completion only. Scene names stubbed until
// the co-design session (§11 homework).
void lights_layer_init(lv_obj_t *tile);
