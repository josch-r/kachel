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
    kachel_topic topics[2];
    int count;
};
static const tile_topics assignments[] = {
    {KACHEL_LAYER_AMBIENT_FACE, {KACHEL_TOPIC_WEATHER, KACHEL_TOPIC_TIMER}, 2},
    {KACHEL_LAYER_AIR, {KACHEL_TOPIC_AIR}, 1},
    {KACHEL_LAYER_HOUSEHOLD, {KACHEL_TOPIC_CALENDAR, KACHEL_TOPIC_BRING}, 2},
};
static const char *topic_names[KACHEL_TOPIC_COUNT] = {
    "air", "weather", "calendar", "bring", "timer"};

static lv_obj_t *debug_labels[4]; // by layer index

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
    auto lights_label = lv_label_create(carousel_tile(KACHEL_LAYER_LIGHTS));
    lv_obj_set_style_text_color(lights_label, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_align(lights_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_label_set_text(lights_label, "");
    debug_labels[KACHEL_LAYER_LIGHTS] = lights_label;

    lv_timer_create(render, 1000, nullptr);
}
