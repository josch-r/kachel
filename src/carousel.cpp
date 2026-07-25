#include "carousel.h"

#include "palette.h"

static lv_obj_t *tileview;
static lv_obj_t *tile_objs[4];

struct tile_spec
{
    lv_color_t surface;
};

static const tile_spec tiles[4] = {
    {KACHEL_SURFACE_LIGHTS},
    {KACHEL_BG_REST},
    {KACHEL_SURFACE_AIR},
    {KACHEL_SURFACE_HOUSE},
};

lv_obj_t *carousel_create()
{
    auto screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, KACHEL_BG_REST, LV_PART_MAIN);

    tileview = lv_tileview_create(screen);
    lv_obj_set_style_bg_opa(tileview, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    for (uint8_t i = 0; i < 4; i++)
    {
        auto tile = lv_tileview_add_tile(tileview, i, 0, LV_DIR_HOR);
        tile_objs[i] = tile;
        lv_obj_set_style_bg_color(tile, tiles[i].surface, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, LV_PART_MAIN);
        // content: ambient face (M3) and layer modules (M4) own their tiles
    }

    carousel_return_home(false);
    return tileview;
}

void carousel_return_home(bool animated)
{
    lv_tileview_set_tile_by_index(tileview, KACHEL_LAYER_AMBIENT_FACE, 0,
                                  animated ? LV_ANIM_ON : LV_ANIM_OFF);
}

bool carousel_is_home()
{
    auto active = lv_tileview_get_tile_active(tileview);
    return active != nullptr && lv_obj_get_index(active) == KACHEL_LAYER_AMBIENT_FACE;
}

lv_obj_t *carousel_tile(kachel_layer layer)
{
    return tile_objs[layer];
}
