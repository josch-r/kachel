// The ambient face (SPEC §6) — implementation of docs/DESIGN_FACE.md
// (design pass 2026-07-24). All numeric tokens trace to that document.
#include "ambient_face.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "oklch.h"
#include "palette.h"
#include "state_model.h"
#include "time_sync.h"
#include "timing.h"

LV_FONT_DECLARE(font_clock_176);
LV_FONT_DECLARE(font_timer_72);
LV_FONT_DECLARE(font_guest_22);

// ---------------------------------------------------------------- params

struct grad_params
{
    oklch stops[3]; // top / horizon(65%) / bottom
    oklch clock;
};

// Phase tables (DESIGN_FACE.md §A/§D). Pre-dawn == evening by design.
static const grad_params PH_EVENING = {
    {{0.11f, 0.015f, 75}, {0.15f, 0.035f, 62}, {0.08f, 0.010f, 75}},
    {0.68f, 0.045f, 70}};
static const grad_params PH_DAWN_PEAK = {
    {{0.17f, 0.025f, 250}, {0.26f, 0.090f, 45}, {0.10f, 0.015f, 75}},
    {0.82f, 0.028f, 80}};
static const grad_params PH_DAY = {
    {{0.28f, 0.030f, 240}, {0.33f, 0.020f, 85}, {0.15f, 0.015f, 75}},
    {0.90f, 0.020f, 85}};
static const grad_params PH_DUSK_PEAK = {
    {{0.14f, 0.022f, 255}, {0.24f, 0.095f, 58}, {0.09f, 0.012f, 70}},
    {0.78f, 0.035f, 72}};

static grad_params blend_params(const grad_params &a, const grad_params &b, float t)
{
    grad_params r;
    for (int i = 0; i < 3; i++)
        r.stops[i] = oklab_lerp(a.stops[i], b.stops[i], t);
    r.clock = oklab_lerp(a.clock, b.clock, t);
    return r;
}

// Weather modifiers (DESIGN_FACE.md §B): ΔL per stop, chroma multiplier,
// then condition-specific hue treatment.
static grad_params apply_condition(grad_params p, const kachel_weather &w,
                                   kachel_condition cond)
{
    struct mod
    {
        float dl[3];
        float cmul;
    };
    mod m = {{0, 0, 0}, 1.0f};
    switch (cond)
    {
    case KACHEL_COND_PARTLYCLOUDY: m = {{0, -0.01f, 0}, 0.75f}; break;
    case KACHEL_COND_CLOUDY:       m = {{0.02f, -0.04f, 0.01f}, 0.45f}; break;
    case KACHEL_COND_RAIN:         m = {{-0.02f, -0.05f, -0.02f}, 0.55f}; break;
    case KACHEL_COND_FOG:          m = {{0.04f, -0.02f, 0.05f}, 0.30f}; break;
    case KACHEL_COND_SNOW:         m = {{0.02f, 0.01f, 0.06f}, 0.40f}; break;
    default: break;
    }
    for (int i = 0; i < 3; i++)
    {
        p.stops[i].L += m.dl[i];
        p.stops[i].C *= m.cmul;
    }
    if (cond == KACHEL_COND_RAIN)
        for (int i = 0; i < 3; i++)
        {
            oklch anchor = {p.stops[i].L, 0.030f, 245};
            p.stops[i] = oklab_lerp(p.stops[i], anchor, 0.35f);
        }
    if (cond == KACHEL_COND_SNOW)
        p.stops[2].H = 85;
    return p;
}

// Air-quality shift (DESIGN_FACE.md §C): OKLab blend toward the dust-violet
// anchor, L untouched. Hysteresis so the field never flickers between bands.
static int air_band; // 0 good, 1 elevated, 2 poor
static void update_air_band(int pm25)
{
    if (pm25 < 0)
        return;
    switch (air_band)
    {
    case 0:
        if (pm25 > 35) air_band = 2;
        else if (pm25 > 12) air_band = 1;
        break;
    case 1:
        if (pm25 > 35) air_band = 2;
        else if (pm25 < 10) air_band = 0;
        break;
    case 2:
        if (pm25 < 31) air_band = pm25 > 12 ? 1 : 0;
        break;
    }
}

static grad_params apply_air(grad_params p)
{
    if (air_band == 0)
        return p;
    float k = air_band == 2 ? 0.85f : 0.40f;
    float cfloor_h = air_band == 2 ? 0.075f : 0.035f;
    float cfloor_tb = air_band == 2 ? 0.060f : 0.035f;
    for (int i = 0; i < 3; i++)
    {
        float keepL = p.stops[i].L;
        oklch anchor = {keepL, 0.080f, 330};
        p.stops[i] = oklab_lerp(p.stops[i], anchor, k);
        p.stops[i].L = keepL;
        float floor_c = i == 1 ? cfloor_h : cfloor_tb;
        if (p.stops[i].C < floor_c)
            p.stops[i].C = floor_c;
    }
    return p;
}

static grad_params params_for_now()
{
    auto w = state_weather();
    int minute = time_sync_minute_of_day();
    if (minute < 0)
        minute = 12 * 60; // fail calm: no clock -> neutral day field

    const int sr = w.sunrise_min, ss = w.sunset_min;
    grad_params p;
    if (minute < sr - 40)
        p = PH_EVENING; // pre-dawn shares evening stops (§5.13)
    else if (minute < sr)
        p = blend_params(PH_EVENING, PH_DAWN_PEAK, (float)(minute - (sr - 40)) / 40.0f);
    else if (minute < sr + 40)
        p = blend_params(PH_DAWN_PEAK, PH_DAY, (float)(minute - sr) / 40.0f);
    else if (minute < ss - 40)
        p = PH_DAY;
    else if (minute < ss)
        p = blend_params(PH_DAY, PH_DUSK_PEAK, (float)(minute - (ss - 40)) / 40.0f);
    else if (minute < ss + 40)
        p = blend_params(PH_DUSK_PEAK, PH_EVENING, (float)(minute - ss) / 40.0f);
    else
        p = PH_EVENING;

    grad_params modified = apply_condition(p, w, w.condition);
    // rain-later (§B): sky heavies preemptively
    if (w.condition != KACHEL_COND_RAIN && w.precip_12h_mm > 0.5f)
    {
        grad_params rainy = apply_condition(p, w, KACHEL_COND_RAIN);
        for (int i = 0; i < 3; i++)
            modified.stops[i] = oklab_lerp(modified.stops[i], rainy.stops[i], 0.60f);
    }
    update_air_band(state_air().pm25);
    modified = apply_air(modified);

    for (auto &s : modified.stops)
    {
        s.L = s.L < 0.05f ? 0.05f : (s.L > 0.35f ? 0.35f : s.L);
        s.C = s.C > 0.10f ? 0.10f : s.C;
    }
    return modified;
}

// ---------------------------------------------------------------- renderer

static constexpr int W = 480, H = 480;
static constexpr int HORIZON_Y = 312; // 65% (DESIGN_FACE.md §A)
static lv_obj_t *canvas;
static lv_color16_t *canvas_buf;
static grad_params current, target;

// 8x8 Bayer matrix — ±1 LSB ordered dither breaks RGB565 banding (§6)
static const uint8_t bayer8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42}, {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41}, {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21}};

static void render_gradient(const grad_params &p, float breath_dl)
{
    grad_params b = p;
    b.stops[1].L += breath_dl;
    for (int y = 0; y < H; y++)
    {
        oklch c;
        if (y < HORIZON_Y)
            c = oklab_lerp(b.stops[0], b.stops[1], (float)y / HORIZON_Y);
        else
            c = oklab_lerp(b.stops[1], b.stops[2], (float)(y - HORIZON_Y) / (H - HORIZON_Y));
        rgbf f = oklch_to_srgb(c);
        float r31 = f.r * 31.0f, g63 = f.g * 63.0f, b31 = f.b * 31.0f;
        lv_color16_t *row = canvas_buf + y * W;
        for (int x = 0; x < W; x++)
        {
            float d = bayer8[y & 7][x & 7] / 64.0f;
            uint8_t r = (uint8_t)(r31 + d), g = (uint8_t)(g63 + d), bl = (uint8_t)(b31 + d);
            row[x].red = r > 31 ? 31 : r;
            row[x].green = g > 63 ? 63 : g;
            row[x].blue = bl > 31 ? 31 : bl;
        }
    }
    lv_obj_invalidate(canvas);
}

// ---------------------------------------------------------------- elements

static lv_obj_t *clock_label;
static lv_obj_t *event_label;
static lv_obj_t *timer_card;
static lv_obj_t *timer_name_label;
static lv_obj_t *timer_count_label;

enum timer_ui_state
{
    TIMER_HIDDEN,
    TIMER_RUNNING,
    TIMER_NOTABLE, // T-60s color accent active
    TIMER_DONE,    // urgent fired; static until tap (60 s self-decay fallback)
};
static timer_ui_state timer_ui = TIMER_HIDDEN;
static uint32_t timer_done_ms;

static lv_color_t oklch_to_lv(const oklch &c)
{
    rgbf f = oklch_to_srgb(c);
    return lv_color_make((uint8_t)(f.r * 255), (uint8_t)(f.g * 255), (uint8_t)(f.b * 255));
}

static void fade_to(lv_obj_t *obj, int32_t from, int32_t to, uint32_t dur, bool hide_after)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, dur);
    lv_anim_set_exec_cb(&a, [](void *o, int32_t v)
                        { lv_obj_set_style_opa((lv_obj_t *)o, v, LV_PART_MAIN); });
    if (hide_after)
        lv_anim_set_completed_cb(&a, [](lv_anim_t *an)
                                 { lv_obj_add_flag((lv_obj_t *)an->var, LV_OBJ_FLAG_HIDDEN); });
    lv_anim_start(&a);
}

// The one urgent beat (DESIGN_FACE.md §E): fill rises to amber peak in
// 350 ms, decays to the static done-state over 1600 ms. Fires once.
static const oklch PULSE_PEAK = {0.45f, 0.110f, 70};
static const oklch PULSE_DONE = {0.17f, 0.040f, 70};
static const oklch CARD_FILL = {0.20f, 0.015f, 75};

static void pulse_exec(void *obj, int32_t v)
{
    // v 0..350 rise, 350..1950 decay
    oklch c;
    lv_opa_t opa;
    if (v <= (int32_t)KACHEL_T_PULSE_RISE_MS)
    {
        float t = (float)v / KACHEL_T_PULSE_RISE_MS;
        c = oklab_lerp(CARD_FILL, PULSE_PEAK, t);
        opa = 120 + (lv_opa_t)((216 - 120) * t);
    }
    else
    {
        float t = (float)(v - KACHEL_T_PULSE_RISE_MS) / KACHEL_T_PULSE_DECAY_MS;
        c = oklab_lerp(PULSE_PEAK, PULSE_DONE, t);
        opa = 216 - (lv_opa_t)((216 - 160) * t);
    }
    lv_obj_set_style_bg_color((lv_obj_t *)obj, oklch_to_lv(c), LV_PART_MAIN);
    lv_obj_set_style_bg_opa((lv_obj_t *)obj, opa, LV_PART_MAIN);
}

static void urgent_pulse()
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, timer_card);
    lv_anim_set_values(&a, 0, KACHEL_T_PULSE_RISE_MS + KACHEL_T_PULSE_DECAY_MS);
    lv_anim_set_duration(&a, KACHEL_T_PULSE_RISE_MS + KACHEL_T_PULSE_DECAY_MS);
    lv_anim_set_exec_cb(&a, pulse_exec);
    lv_anim_start(&a);
    lv_obj_set_style_border_color(timer_card, oklch_to_lv({0.45f, 0.080f, 70}), LV_PART_MAIN);
    lv_obj_set_style_border_opa(timer_card, 90, LV_PART_MAIN);
    lv_obj_set_style_text_color(timer_count_label, oklch_to_lv({0.72f, 0.085f, 70}), LV_PART_MAIN);
}

static void reset_card_style()
{
    lv_obj_set_style_bg_color(timer_card, oklch_to_lv(CARD_FILL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(timer_card, 120, LV_PART_MAIN);
    lv_obj_set_style_border_color(timer_card, oklch_to_lv({0.35f, 0.020f, 75}), LV_PART_MAIN);
    lv_obj_set_style_border_opa(timer_card, 60, LV_PART_MAIN);
    lv_obj_set_style_text_color(timer_count_label, KACHEL_TEXT_PRIMARY, LV_PART_MAIN);
}

static void tick(lv_timer_t *)
{
    auto const now_ms = millis();

    // --- gradient: slew toward target over the ambient beat, breathe at rest ---
    target = params_for_now();
    float t = (float)KACHEL_T_FACE_TICK_MS / KACHEL_T_AMBIENT_MS;
    for (int i = 0; i < 3; i++)
        current.stops[i] = oklab_lerp(current.stops[i], target.stops[i], t);
    current.clock = oklab_lerp(current.clock, target.clock, t);
    // breathing (§F): horizon L ±0.010, 6 cycles/min; §5.7 ceiling
    float breath = 0.010f * sinf((float)now_ms * 2.0f * (float)M_PI / KACHEL_T_BREATH_PERIOD_MS);
    if (timer_ui == TIMER_DONE)
        breath = 0; // suspended during the done beat per design
    render_gradient(current, breath);
    lv_obj_set_style_text_color(clock_label, oklch_to_lv(current.clock), LV_PART_MAIN);

    // --- clock ---
    char text[8];
    if (time_sync_clock_text(text, sizeof(text)))
        lv_label_set_text(clock_label, text);

    // --- calendar guest line (§6: within 2 h; leaves 5 min after start) ---
    auto ev = state_next_event();
    time_t now = time(nullptr);
    bool show_event = ev.valid && (ev.start - now) <= 2 * 3600 && now <= ev.start + 300;
    bool event_shown = !lv_obj_has_flag(event_label, LV_OBJ_FLAG_HIDDEN);
    if (show_event)
    {
        struct tm st;
        localtime_r(&ev.start, &st);
        lv_label_set_text_fmt(event_label, "%02d:%02d \xC2\xB7 %s", st.tm_hour, st.tm_min, ev.title);
        if (!event_shown)
            fade_to(event_label, LV_OPA_TRANSP, LV_OPA_COVER, KACHEL_T_INFO_FADE_MS, false);
    }
    else if (event_shown)
        fade_to(event_label, LV_OPA_COVER, LV_OPA_TRANSP, KACHEL_T_INFO_FADE_MS, true);

    // --- timer guest card + escalation ladder ---
    auto tmr = state_timer();
    if (tmr.active)
    {
        long remain = (long)(tmr.ends_at - now);
        if (remain > 0)
        {
            lv_label_set_text(timer_name_label, tmr.label);
            if (remain >= 3600)
                lv_label_set_text_fmt(timer_count_label, "%ld:%02ld:%02ld",
                                      remain / 3600, (remain % 3600) / 60, remain % 60);
            else
                lv_label_set_text_fmt(timer_count_label, "%ld:%02ld", remain / 60, remain % 60);
            if (timer_ui == TIMER_HIDDEN || timer_ui == TIMER_DONE)
            {
                reset_card_style();
                fade_to(timer_card, LV_OPA_TRANSP, LV_OPA_COVER, KACHEL_T_CARD_IN_MS, false);
                timer_ui = TIMER_RUNNING;
            }
            if (timer_ui == TIMER_RUNNING && remain <= 60)
            {
                // notable accent (§5.5): countdown warms, no motion
                lv_obj_set_style_text_color(timer_count_label,
                                            oklch_to_lv({0.72f, 0.070f, 70}), LV_PART_MAIN);
                timer_ui = TIMER_NOTABLE;
            }
        }
        else
        {
            if (timer_ui == TIMER_RUNNING || timer_ui == TIMER_NOTABLE)
            {
                lv_label_set_text(timer_count_label, "0:00");
                urgent_pulse();
                timer_ui = TIMER_DONE;
                timer_done_ms = now_ms;
            }
            // §5.5 self-decay fallback if nobody taps
            if (timer_ui == TIMER_DONE && now_ms - timer_done_ms > KACHEL_T_DONE_DECAY_MS)
            {
                fade_to(timer_card, LV_OPA_COVER, LV_OPA_TRANSP, KACHEL_T_CARD_OUT_MS, true);
                timer_ui = TIMER_HIDDEN;
            }
        }
    }
    else if (timer_ui != TIMER_HIDDEN)
    {
        fade_to(timer_card, LV_OPA_COVER, LV_OPA_TRANSP, KACHEL_T_CARD_OUT_MS, true);
        timer_ui = TIMER_HIDDEN;
    }
}

void ambient_face_init(lv_obj_t *tile)
{
    canvas_buf = (lv_color16_t *)heap_caps_malloc(W * H * sizeof(lv_color16_t),
                                                  MALLOC_CAP_SPIRAM);
    if (canvas_buf == nullptr)
    {
        log_e("ambient face: PSRAM canvas alloc failed");
        return;
    }
    canvas = lv_canvas_create(tile);
    lv_canvas_set_buffer(canvas, canvas_buf, W, H, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(canvas, 0, 0);

    current = target = params_for_now();
    render_gradient(current, 0);

    // clock: center (240, 200), never moves (DESIGN_FACE.md §D)
    clock_label = lv_label_create(tile);
    lv_obj_set_style_text_font(clock_label, &font_clock_176, LV_PART_MAIN);
    lv_label_set_text(clock_label, "--:--");
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_color(clock_label, oklch_to_lv(current.clock), LV_PART_MAIN);

    // calendar line: center (240, 448)
    event_label = lv_label_create(tile);
    lv_obj_set_style_text_font(event_label, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(event_label, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_set_width(event_label, 400);
    lv_label_set_long_mode(event_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(event_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(event_label, LV_ALIGN_CENTER, 0, 208);
    lv_obj_add_flag(event_label, LV_OBJ_FLAG_HIDDEN);

    // timer card: 280x96, center (240, 376)
    timer_card = lv_obj_create(tile);
    lv_obj_remove_style_all(timer_card);
    lv_obj_set_size(timer_card, 280, 96);
    lv_obj_align(timer_card, LV_ALIGN_CENTER, 0, 136);
    lv_obj_set_style_radius(timer_card, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(timer_card, 1, LV_PART_MAIN);
    // §4 one-gesture silence must work on the card itself: bubble the tap
    // up to the tile's dismiss handler (card keeps default CLICKABLE)
    lv_obj_add_flag(timer_card, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(timer_card, LV_OBJ_FLAG_HIDDEN);
    timer_name_label = lv_label_create(timer_card);
    lv_obj_set_style_text_font(timer_name_label, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(timer_name_label, KACHEL_TEXT_DIM, LV_PART_MAIN);
    lv_obj_align(timer_name_label, LV_ALIGN_TOP_MID, 0, 6);
    timer_count_label = lv_label_create(timer_card);
    lv_obj_set_style_text_font(timer_count_label, &font_timer_72, LV_PART_MAIN);
    lv_obj_align(timer_count_label, LV_ALIGN_BOTTOM_MID, 0, -2);
    reset_card_style();

    // one-gesture silence (§4): any tap on the face dismisses a done timer
    lv_obj_add_event_cb(tile, [](lv_event_t *)
                        {
        if (timer_ui == TIMER_DONE)
        {
            fade_to(timer_card, LV_OPA_COVER, LV_OPA_TRANSP, KACHEL_T_CARD_OUT_MS, true);
            timer_ui = TIMER_HIDDEN;
        } },
                        LV_EVENT_CLICKED, nullptr);

    lv_timer_create(tick, KACHEL_T_FACE_TICK_MS, nullptr);
}
