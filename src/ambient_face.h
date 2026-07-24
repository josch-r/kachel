#pragma once

#include <lvgl.h>

// The signature surface (SPEC §6): generative OKLCH gradient field encoding
// time-of-day (phase), weather (character), air quality (hue temperature);
// clock primary; timer + calendar guest cards; escalation ladder for the
// timer-complete urgent beat.
void ambient_face_init(lv_obj_t *tile);
