#include "lights_layer.h"

#include "conn_status.h"
#include "mqtt_client.h"
#include "palette.h"
#include "timing.h"

LV_FONT_DECLARE(font_guest_22);

// Slots 1+2 fixed (Josch 2026-07-25); 3+4 named 2026-09-30 (SPEC §10).
static const char *scene_names[4] = {"Alles an", "Alles aus", "TV-Chill", "Aufräumen"};

static void scene_pressed(lv_event_t *e)
{
    mqtt_cmd_scene(1 + (uint8_t)(uintptr_t)lv_event_get_user_data(e));
}

void lights_layer_init(lv_obj_t *tile)
{
    // release-fade transition carries the §5.8 feedback timing
    static lv_style_transition_dsc_t trans;
    static const lv_style_prop_t props[] = {LV_STYLE_BG_OPA, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_out,
                                 KACHEL_T_FEEDBACK_MS, 0, nullptr);

    lv_obj_t *btns[4];
    for (int i = 0; i < 4; i++)
    {
        auto btn = lv_button_create(tile);
        btns[i] = btn;
        lv_obj_set_size(btn, 208, 208);
        lv_obj_align(btn, LV_ALIGN_CENTER, (i % 2) ? 112 : -112, (i / 2) ? 112 : -112);
        lv_obj_set_ext_click_area(btn, 12); // §4 invisible hit-area expansion
        lv_obj_set_style_radius(btn, 24, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        lv_obj_set_style_border_opa(btn, LV_OPA_60, LV_PART_MAIN);
        lv_obj_set_style_transition(btn, &trans, LV_PART_MAIN);

        auto label = lv_label_create(btn);
        lv_label_set_text(label, scene_names[i]);
        lv_obj_set_style_text_font(label, &font_guest_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
        lv_obj_center(label);

        // CLICKED = press released inside target — §4 completion-only trigger
        lv_obj_add_event_cb(btn, scene_pressed, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    // link mark sits in the grid's center gap
    conn_status_attach(tile, LV_ALIGN_CENTER, 0, 0, btns, 4);
}
