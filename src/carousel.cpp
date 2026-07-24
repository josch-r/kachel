#include "carousel.h"

#include "palette.h"

static lv_obj_t *tileview;
static lv_obj_t *tile_objs[4];

struct tile_spec
{
    const char *name;
    lv_color_t surface;
};

// M1 placeholders: layer name + index on token surfaces. Real content M3/M4.
static const tile_spec tiles[4] = {
    {"lights", KACHEL_SURFACE_LIGHTS},
    {"ambient face", KACHEL_BG_REST},
    {"air", KACHEL_SURFACE_AIR},
    {"household", KACHEL_SURFACE_HOUSE},
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

        auto label = lv_label_create(tile);
        lv_label_set_text_fmt(label, "%s\n%u/4", tiles[i].name, i + 1);
        lv_obj_set_style_text_color(label, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_center(label);
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
