#include "household_layer.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "mqtt_client.h"
#include "palette.h"
#include "state_model.h"

LV_FONT_DECLARE(font_guest_22);

static lv_obj_t *event_labels[KACHEL_EVENTS_MAX];
static lv_obj_t *bring_header;
static lv_obj_t *bring_items[KACHEL_BRING_ITEMS_MAX];
static lv_obj_t *cal_stale_dot;
static lv_obj_t *bring_stale_dot;

static const char *weekdays[7] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

static void set_stale(lv_obj_t *dot, bool stale)
{
    if (stale)
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
}

// lv_label_set_text always reallocs + invalidates; skip unchanged text so the
// 1 Hz poll never redraws a static layer
static void set_text_if_changed(lv_obj_t *label, const char *text)
{
    if (strcmp(lv_label_get_text(label), text) != 0)
        lv_label_set_text(label, text);
}

static void refresh(lv_timer_t *)
{
    kachel_event events[KACHEL_EVENTS_MAX];
    int n = state_events(events);
    time_t now = time(nullptr);
    struct tm today;
    localtime_r(&now, &today);

    for (int i = 0; i < KACHEL_EVENTS_MAX; i++)
    {
        if (i >= n)
        {
            set_text_if_changed(event_labels[i], "");
            continue;
        }
        struct tm t;
        localtime_r(&events[i].start, &t);
        char line[96];
        if (t.tm_yday == today.tm_yday && t.tm_year == today.tm_year)
            snprintf(line, sizeof(line), "%02d:%02d · %s", t.tm_hour, t.tm_min,
                     events[i].title);
        else
            snprintf(line, sizeof(line), "%s %02d:%02d · %s", weekdays[t.tm_wday],
                     t.tm_hour, t.tm_min, events[i].title);
        set_text_if_changed(event_labels[i], line);
    }

    kachel_bring b = state_bring();
    char header[32];
    if (!b.valid)
        snprintf(header, sizeof(header), "Bring!");
    else if (b.count == 0)
        snprintf(header, sizeof(header), "Bring! · leer");
    else
        snprintf(header, sizeof(header), "Bring! · %d", b.count);
    set_text_if_changed(bring_header, header);
    for (int i = 0; i < KACHEL_BRING_ITEMS_MAX; i++)
        set_text_if_changed(bring_items[i], (b.valid && i < b.item_count) ? b.items[i] : "");

    set_stale(cal_stale_dot, mqtt_state_stale(KACHEL_TOPIC_CALENDAR));
    set_stale(bring_stale_dot, mqtt_state_stale(KACHEL_TOPIC_BRING));
}

static lv_obj_t *make_line(lv_obj_t *tile, lv_color_t color, int y)
{
    auto label = lv_label_create(tile);
    lv_obj_set_width(label, 416);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 32, y);
    lv_label_set_text(label, "");
    return label;
}

static lv_obj_t *make_stale_dot(lv_obj_t *tile, int y)
{
    auto dot = lv_obj_create(tile);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_align(dot, LV_ALIGN_TOP_RIGHT, -34, y);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
    return dot;
}

void household_layer_init(lv_obj_t *tile)
{
    // calendar block: fixed anchor top (§5.2); empty lines stay empty —
    // emptiness is the all-well signal (§5.1)
    auto cal_header = lv_label_create(tile);
    lv_label_set_text(cal_header, "Termine");
    lv_obj_set_style_text_font(cal_header, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(cal_header, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_opa(cal_header, LV_OPA_60, LV_PART_MAIN);
    lv_obj_align(cal_header, LV_ALIGN_TOP_LEFT, 32, 36);
    cal_stale_dot = make_stale_dot(tile, 44);

    for (int i = 0; i < KACHEL_EVENTS_MAX; i++)
        event_labels[i] = make_line(tile, KACHEL_TEXT_PRIMARY, 78 + i * 38);

    // Bring! block: fixed anchor bottom half
    bring_header = lv_label_create(tile);
    lv_label_set_text(bring_header, "Bring!");
    lv_obj_set_style_text_font(bring_header, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(bring_header, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_opa(bring_header, LV_OPA_60, LV_PART_MAIN);
    lv_obj_align(bring_header, LV_ALIGN_TOP_LEFT, 32, 232);
    bring_stale_dot = make_stale_dot(tile, 240);

    for (int i = 0; i < KACHEL_BRING_ITEMS_MAX; i++)
        bring_items[i] = make_line(tile, KACHEL_TEXT_PRIMARY, 274 + i * 38);

    lv_timer_create(refresh, 1000, nullptr);
}
