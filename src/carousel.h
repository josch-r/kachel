#pragma once

#include <lvgl.h>

// Layer order per SPEC §4: Lights <- Ambient face (home) -> Air -> Household
enum kachel_layer
{
    KACHEL_LAYER_LIGHTS = 0,
    KACHEL_LAYER_AMBIENT_FACE = 1,
    KACHEL_LAYER_AIR = 2,
    KACHEL_LAYER_HOUSEHOLD = 3,
};

// Creates the 4-tile swipe carousel on the active screen, resting on the
// ambient face. Returns the tileview.
lv_obj_t *carousel_create();

// Snap back to the ambient face (the §4 recentering path; used by the
// idle-return logic from task 4 onward).
void carousel_return_home(bool animated);

// True while the ambient face tile is the active one.
bool carousel_is_home();
