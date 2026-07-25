#include "air_layer.h"

#include <cstdio>

#include "config.h"
#include "mqtt_client.h"
#include "palette.h"
#include "state_model.h"
#include "timing.h"

LV_FONT_DECLARE(font_timer_72);
LV_FONT_DECLARE(font_guest_22);

static lv_obj_t *pm25_label;
static lv_obj_t *filter_label;
static lv_obj_t *chart;
static lv_chart_series_t *series;
// 5 segments of one fan control: Aus, Auto, manual 1..3 (§10 2026-07-25)
static constexpr int FAN_SEGMENTS = 5;
static lv_obj_t *fan_btns[FAN_SEGMENTS];
static lv_obj_t *stale_dot;

static int32_t chart_vals[KACHEL_PM25_HISTORY_N];
static int shown_pm25 = -2;
static int shown_filter = -2;
static int shown_fan = -1;

static void fan_pressed(lv_event_t *e)
{
    int seg = (int)(uintptr_t)lv_event_get_user_data(e);
    if (seg == 1)
        mqtt_cmd_air_auto();
    else
        mqtt_cmd_air(seg == 0 ? 0 : seg - 1);
}

// segment index the state feed points at: 0=Aus, 1=Auto, 2..4=manual 1..3
static int fan_segment(const kachel_air &a)
{
    if (!a.valid)
        return -1;
    if (a.auto_mode)
        return 1;
    if (a.fan <= 0)
        return 0;
    return a.fan >= 3 ? 4 : a.fan + 1;
}

static void refresh(lv_timer_t *)
{
    bool sampled = state_history_tick();
    kachel_air a = state_air();

    int pm25 = a.valid ? a.pm25 : -1;
    if (pm25 != shown_pm25)
    {
        shown_pm25 = pm25;
        if (pm25 < 0)
            lv_label_set_text(pm25_label, "--");
        else
            lv_label_set_text_fmt(pm25_label, "%d", pm25);
    }

    int filter = a.valid ? a.filter_pct : -1;
    if (filter != shown_filter)
    {
        shown_filter = filter;
        if (filter < 0)
            lv_label_set_text(filter_label, "Filter --");
        else
            lv_label_set_text_fmt(filter_label, "Filter %d %%", filter);
    }

    // fan highlight follows the state feed, not the tap (M2 deferral closed)
    int seg = fan_segment(a);
    if (seg != shown_fan)
    {
        shown_fan = seg;
        for (int i = 0; i < FAN_SEGMENTS; i++)
        {
            bool active = (i == seg);
            lv_obj_set_style_border_color(fan_btns[i],
                                          active ? KACHEL_TEXT_PRIMARY : KACHEL_TEXT_DIM,
                                          LV_PART_MAIN);
            lv_obj_set_style_border_opa(fan_btns[i], active ? LV_OPA_COVER : LV_OPA_60,
                                        LV_PART_MAIN);
            lv_obj_set_style_text_color(lv_obj_get_child(fan_btns[i], 0),
                                        active ? KACHEL_TEXT_PRIMARY : KACHEL_TEXT_DIM,
                                        LV_PART_MAIN);
        }
    }

    // rebuild only when a sample landed — all appends run through this tick,
    // so equal consecutive values still shift the window (reviewer blocker)
    if (sampled)
    {
        static int16_t samples[KACHEL_PM25_HISTORY_N];
        int n = state_history(samples, KACHEL_PM25_HISTORY_N);
        int32_t peak = 40; // floor keeps the good-air trace low and quiet
        for (int i = 0; i < n; i++)
            if (samples[i] > peak)
                peak = samples[i];
        // right-aligned: newest sample at the right edge, gaps stay blank
        for (int i = 0; i < KACHEL_PM25_HISTORY_N; i++)
        {
            int src = i - (KACHEL_PM25_HISTORY_N - n);
            chart_vals[i] = src >= 0 ? samples[src] : LV_CHART_POINT_NONE;
        }
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, peak + 5);
        lv_chart_refresh(chart);
    }

    bool stale = !a.valid; // air is on-change (§7): only "never received" dims it
    if (lv_obj_has_flag(stale_dot, LV_OBJ_FLAG_HIDDEN) != !stale)
    {
        if (stale)
            lv_obj_remove_flag(stale_dot, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(stale_dot, LV_OBJ_FLAG_HIDDEN);
    }
}

void air_layer_init(lv_obj_t *tile)
{
    // PM2.5 now — the layer's primary datum, fixed anchor top-left (§5.2)
    pm25_label = lv_label_create(tile);
    lv_obj_set_style_text_font(pm25_label, &font_timer_72, LV_PART_MAIN);
    lv_obj_set_style_text_color(pm25_label, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_align(pm25_label, LV_ALIGN_TOP_LEFT, 32, 36);
    lv_label_set_text(pm25_label, "--");

    auto pm25_caption = lv_label_create(tile);
    lv_label_set_text(pm25_caption, "PM2.5");
    lv_obj_set_style_text_font(pm25_caption, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(pm25_caption, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_align(pm25_caption, LV_ALIGN_TOP_LEFT, 34, 124);

    filter_label = lv_label_create(tile);
    lv_obj_set_style_text_font(filter_label, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(filter_label, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_align(filter_label, LV_ALIGN_TOP_RIGHT, -32, 44);
    lv_label_set_text(filter_label, "Filter --");

    // §5.6 staleness mark: one dim dot next to the primary datum
    stale_dot = lv_obj_create(tile);
    lv_obj_set_size(stale_dot, 8, 8);
    lv_obj_set_style_radius(stale_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(stale_dot, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_border_width(stale_dot, 0, LV_PART_MAIN);
    lv_obj_align(stale_dot, LV_ALIGN_TOP_LEFT, 34, 108);
    lv_obj_add_flag(stale_dot, LV_OBJ_FLAG_HIDDEN);

    // 24 h history — quiet single trace, no grid, no points
    chart = lv_chart_create(tile);
    lv_obj_set_size(chart, 416, 140);
    lv_obj_align(chart, LV_ALIGN_TOP_MID, 0, 168);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_chart_set_point_count(chart, KACHEL_PM25_HISTORY_N);
    series = lv_chart_add_series(chart, KACHEL_TEXT_DIM, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(chart, series, chart_vals);
    for (int i = 0; i < KACHEL_PM25_HISTORY_N; i++)
        chart_vals[i] = LV_CHART_POINT_NONE;

    auto chart_caption = lv_label_create(tile);
    lv_label_set_text(chart_caption, "24 h");
    lv_obj_set_style_text_font(chart_caption, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(chart_caption, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_style_text_opa(chart_caption, LV_OPA_60, LV_PART_MAIN);
    lv_obj_align(chart_caption, LV_ALIGN_TOP_RIGHT, -32, 312);

    // one fan control, five segments (Aus/Auto/1/2/3) — 80 px floor (§4),
    // ext click area recovers the tightened gaps
    static lv_style_transition_dsc_t trans;
    static const lv_style_prop_t props[] = {LV_STYLE_BG_OPA, LV_STYLE_PROP_INV};
    lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_out,
                                 KACHEL_T_FEEDBACK_MS, 0, nullptr);
    static const char *fan_names[FAN_SEGMENTS] = {"Aus", "Auto", "1", "2", "3"};
    for (int i = 0; i < FAN_SEGMENTS; i++)
    {
        auto btn = lv_button_create(tile);
        fan_btns[i] = btn;
        lv_obj_set_size(btn, 80, 96);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 24 + i * 88, -32);
        lv_obj_set_ext_click_area(btn, 4);
        lv_obj_set_style_radius(btn, 20, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        lv_obj_set_style_border_opa(btn, LV_OPA_60, LV_PART_MAIN);
        lv_obj_set_style_transition(btn, &trans, LV_PART_MAIN);

        auto label = lv_label_create(btn);
        lv_label_set_text(label, fan_names[i]);
        lv_obj_set_style_text_font(label, &font_guest_22, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, KACHEL_TEXT_DIM, LV_PART_MAIN);
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, fan_pressed, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    lv_timer_create(refresh, 1000, nullptr);
}
