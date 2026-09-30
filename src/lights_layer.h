#pragma once

#include <lvgl.h>

// Lights layer (SPEC §3 job 2): 2x2 scene grid, ~200 px targets (§4),
// cmd/scene fires on gesture completion only. Scene semantics live in HA
// (scenes kachel_s1..s4); the labels here must match them.
void lights_layer_init(lv_obj_t *tile);
