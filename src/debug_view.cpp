#include "debug_view.h"

#include <Arduino.h>
#include <lvgl.h>

#include "carousel.h"
#include "mqtt_client.h"
#include "palette.h"

// tile -> topics shown there (Lights has no state topic; shows link status)
struct tile_topics
{
    kachel_layer layer;
    kachel_topic topics[4];
    int count;
};
// ambient face tile belongs to the face from M3 on; its raw topics moved here
static const tile_topics assignments[] = {
    {KACHEL_LAYER_AIR, {KACHEL_TOPIC_AIR}, 1},
    {KACHEL_LAYER_HOUSEHOLD,
     {KACHEL_TOPIC_CALENDAR, KACHEL_TOPIC_BRING, KACHEL_TOPIC_WEATHER, KACHEL_TOPIC_TIMER},
     4},
};
static const char *topic_names[KACHEL_TOPIC_COUNT] = {
    "air", "weather", "calendar", "bring", "timer"};

static lv_obj_t *debug_labels[4]; // by layer index; ambient face unused

static void render(lv_timer_t *)
{
    char buf[1200];
    for (auto &a : assignments)
    {
        int off = 0;
        for (int i = 0; i < a.count; i++)
        {
            auto t = a.topics[i];
            auto s = mqtt_state(t);
            auto age = mqtt_state_age_s(t);
            char agestr[16];
            if (age < 0)
                snprintf(agestr, sizeof(agestr), "never");
            else
                snprintf(agestr, sizeof(agestr), "%lds", (long)age);
            off += snprintf(buf + off, sizeof(buf) - off, "%s (%s)%s\n%s\n\n",
                            topic_names[t], agestr,
                            mqtt_state_stale(t) ? " [STALE]" : "",
                            s->ever_received ? s->payload : "-");
        }
        lv_label_set_text(debug_labels[a.layer], buf);
    }
    lv_label_set_text_fmt(debug_labels[KACHEL_LAYER_LIGHTS],
                          "mqtt %s", mqtt_connected() ? "connected" : "down");
}

void debug_view_init()
{
    for (auto &a : assignments)
    {
        auto tile = carousel_tile(a.layer);
        auto label = lv_label_create(tile);
        lv_obj_set_width(label, 460);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(label, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);
        lv_label_set_text(label, "");
        debug_labels[a.layer] = label;
    }
    auto lights_tile = carousel_tile(KACHEL_LAYER_LIGHTS);
    auto lights_label = lv_label_create(lights_tile);
    lv_obj_set_style_text_color(lights_label, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_align(lights_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lights_label, "");
    debug_labels[KACHEL_LAYER_LIGHTS] = lights_label;

    // M2 round-trip triggers (M4 replaces with real controls):
    // lights tile: 2x2 scene buttons -> cmd/scene on release (§4: completion only)
    for (int i = 0; i < 4; i++)
    {
        auto btn = lv_button_create(lights_tile);
        lv_obj_set_size(btn, 200, 160);
        lv_obj_align(btn, LV_ALIGN_CENTER, (i % 2) ? 110 : -110, (i / 2) ? 130 : -30);
        lv_obj_set_style_bg_color(btn, KACHEL_SURFACE_LIGHTS, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        auto l = lv_label_create(btn);
        lv_label_set_text_fmt(l, "scene %d", i + 1);
        lv_obj_set_style_text_color(l, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
        lv_obj_center(l);
        lv_obj_add_event_cb(btn, [](lv_event_t *e)
                            { mqtt_cmd_scene(1 + (uint8_t)(uintptr_t)lv_event_get_user_data(e)); },
                            LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    // air tile: one button cycling fan 0..3
    auto air_btn = lv_button_create(carousel_tile(KACHEL_LAYER_AIR));
    lv_obj_set_size(air_btn, 200, 90);
    lv_obj_align(air_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(air_btn, KACHEL_SURFACE_AIR, LV_PART_MAIN);
    lv_obj_set_style_border_color(air_btn, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(air_btn, 1, LV_PART_MAIN);
    auto air_l = lv_label_create(air_btn);
    lv_label_set_text(air_l, "fan cycle");
    lv_obj_set_style_text_color(air_l, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_center(air_l);
    lv_obj_add_event_cb(air_btn, [](lv_event_t *)
                        {
        static uint8_t level = 0;
        level = (level + 1) % 4;
        mqtt_cmd_air(level); },
                        LV_EVENT_CLICKED, nullptr);

    lv_timer_create(render, 1000, nullptr);
}
