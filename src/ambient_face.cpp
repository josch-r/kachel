// The ambient face v2 "Three Strata" — implementation of docs/DESIGN_FACE.md
// and docs/DESIGN_PALETTE_V2.md (design pass 2026-07-25, signed off).
// Strata: FIELD (phase + weather band/veil + air), MARK (Doto clock, anchor
// eternal), SLOT (one amber next-thing band), plus LINE (temp + precip) and
// DROPLETS (precip <=12 h). All numeric tokens trace to those documents.
#include "ambient_face.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "oklch.h"
#include "palette.h"
#include "state_model.h"
#include "time_sync.h"
#include "timing.h"

LV_FONT_DECLARE(font_clock_100);
LV_FONT_DECLARE(font_timer_44);
LV_FONT_DECLARE(font_text_22);
LV_FONT_DECLARE(font_guest_22); // Inter Tight 22 — slot text (Josch 2026-07-25)

// ---------------------------------------------------------------- params

struct phase_params
{
    oklch stops[3]; // top y=0 / horizon y=312 / bottom y=480
    oklch clock;
    oklch line; // secondary text line color
};

// PALETTE_V2 §1/§4. Law: light lives at the horizon, top L <= 0.15 always.
static const phase_params PH_HUSH = { // pre-dawn: total slate
    {{0.10f, 0.020f, 250}, {0.16f, 0.032f, 248}, {0.09f, 0.012f, 250}},
    {0.72f, 0.030f, 85}, {0.58f, 0.022f, 78}};
static const phase_params PH_RIFT = { // sunrise: cool lid, rose seam
    {{0.14f, 0.030f, 250}, {0.36f, 0.105f, 45}, {0.10f, 0.018f, 75}},
    {0.84f, 0.026f, 80}, {0.64f, 0.020f, 80}};
static const phase_params PH_VAULT = { // day: blue vault, bone seam
    {{0.16f, 0.042f, 243}, {0.44f, 0.026f, 90}, {0.14f, 0.016f, 80}},
    {0.90f, 0.018f, 85}, {0.70f, 0.018f, 85}};
static const phase_params PH_EMBER = { // sunset: darker lid, warmer seam
    {{0.11f, 0.028f, 255}, {0.32f, 0.100f, 57}, {0.09f, 0.014f, 75}},
    {0.80f, 0.034f, 78}, {0.64f, 0.020f, 80}};
static const phase_params PH_HEARTH = { // evening: total bone, lamplight
    {{0.09f, 0.016f, 75}, {0.17f, 0.035f, 75}, {0.09f, 0.012f, 78}},
    {0.68f, 0.045f, 75}, {0.58f, 0.022f, 78}};

// Weather band + veil (PALETTE_V2 §2): absolute constants, never phase-tinted
static const oklch BAND_PARTLY = {0.22f, 0.020f, 244};
static const oklch BAND_OVERCAST = {0.29f, 0.012f, 250};
static const oklch VEIL = {0.19f, 0.026f, 247};

// Slot ambers (PALETTE_V2 §4)
static const oklch SLOT_TEXT = {0.74f, 0.085f, 70};
static const oklch SLOT_FILL = {0.17f, 0.030f, 70};
static const oklch SLOT_TEXT_URGENT = {0.79f, 0.110f, 70};
static const oklch SLOT_FILL_URGENT = {0.30f, 0.085f, 70};
static const oklch DROPLET = {0.80f, 0.020f, 80};

// The full continuously-slewed face state (everything the renderer needs)
struct face_params
{
    oklch stops[3];
    oklch clock;
    oklch line;
    float band_on;    // 0..1 sky band presence
    float band_flat;  // 0 = partly (graded), 1 = overcast (flat lid)
    oklch band_color; // post-air band value
    float veil_frac;  // 0..1 of the y160..y312 span
    oklch veil_color; // post-air veil value
};

static phase_params blend_phase(const phase_params &a, const phase_params &b, float t)
{
    phase_params r;
    for (int i = 0; i < 3; i++)
        r.stops[i] = oklab_lerp(a.stops[i], b.stops[i], t);
    r.clock = oklab_lerp(a.clock, b.clock, t);
    r.line = oklab_lerp(a.line, b.line, t);
    return r;
}

// --- air quality (PALETTE_V2 §3), hysteresis unchanged from v1 ---
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

// Apply the dust-violet blend to one color. Rose-guard (§C): stops whose
// native hue sits in the sun family [30,70] with C > 0.05 are skipped so the
// blend can never pass through forbidden red at dawn/dusk.
static oklch air_shift(oklch c, int band, float cfloor)
{
    if (band == 0)
        return c;
    if (c.H >= 30 && c.H <= 70 && c.C > 0.05f)
        return c; // rose-guard
    float keepL = c.L;
    if (band == 2)
        c.C *= 0.5f; // pre-drain: violet arrives pure, not mixed to mud
    float k = band == 2 ? 0.80f : 0.30f;
    oklch anchor = {keepL, band == 2 ? 0.090f : 0.070f, 330};
    c = oklab_lerp(c, anchor, k);
    c.L = keepL;
    if (c.C < cfloor)
        c.C = cfloor;
    return c;
}

// --- daypart engine (DESIGN_FACE §H) ---
enum daypart
{
    DP_RUSH,
    DP_DAY,
    DP_EVENING,
    DP_NIGHT, // display.cpp owns the screen; face keeps rendering beneath
};

static daypart daypart_now(int minute, int sunset_min)
{
    if (minute < 0)
        return DP_DAY; // fail calm
    if (minute >= KACHEL_NIGHT_START_MIN || minute < KACHEL_DAY_START_MIN)
        return DP_NIGHT;
    time_t now = time(nullptr);
    struct tm lt;
    localtime_r(&now, &lt);
    bool weekday = lt.tm_wday >= 1 && lt.tm_wday <= 5;
    if (weekday && minute >= KACHEL_RUSH_START_MIN && minute < KACHEL_RUSH_END_MIN)
        return DP_RUSH;
    if (minute >= sunset_min - 40)
        return DP_EVENING;
    return DP_DAY;
}

// --- cooking suppression (§C): while a timer runs (+30 min), air escalation
// is capped at elevated — frying spikes are expected, not alerts.
static time_t last_timer_active_at;

static face_params target_for_now()
{
    auto w = state_weather();
    int minute = time_sync_minute_of_day();
    if (minute < 0)
        minute = 12 * 60;
    const int sr = w.sunrise_min, ss = w.sunset_min;

    phase_params p;
    if (minute < sr - 40)
        p = PH_HUSH;
    else if (minute < sr)
        p = blend_phase(PH_HUSH, PH_RIFT, (float)(minute - (sr - 40)) / 40.0f);
    else if (minute < sr + 40)
        p = blend_phase(PH_RIFT, PH_VAULT, (float)(minute - sr) / 40.0f);
    else if (minute < ss - 40)
        p = PH_VAULT;
    else if (minute < ss)
        p = blend_phase(PH_VAULT, PH_EMBER, (float)(minute - (ss - 40)) / 40.0f);
    else if (minute < ss + 40)
        p = blend_phase(PH_EMBER, PH_HEARTH, (float)(minute - ss) / 40.0f);
    else
        p = PH_HEARTH;

    face_params f = {};
    for (int i = 0; i < 3; i++)
        f.stops[i] = p.stops[i];
    f.clock = p.clock;
    f.line = p.line;

    // --- weather band + veil (absolute encoding, §B) ---
    int band = 0; // 0 none, 1 partly, 2 overcast
    switch (w.condition)
    {
    case KACHEL_COND_PARTLYCLOUDY: band = 1; break;
    case KACHEL_COND_CLOUDY:
    case KACHEL_COND_FOG:
    case KACHEL_COND_RAIN:
    case KACHEL_COND_SNOW: band = 2; break;
    default: break;
    }
    float frac = 0;
    if (w.precip_12h_mm > 8.0f) frac = 1.0f;
    else if (w.precip_12h_mm > 2.0f) frac = 2.0f / 3.0f;
    else if (w.precip_12h_mm > 0.5f) frac = 1.0f / 3.0f;
    if ((w.condition == KACHEL_COND_RAIN || w.condition == KACHEL_COND_SNOW) && frac == 0)
        frac = 1.0f / 3.0f; // precipitating now trumps a dry forecast sum
    if (frac > 0 && band == 0)
        band = 1; // veil implies band >= partly

    f.band_on = band ? 1.0f : 0.0f;
    f.band_flat = band == 2 ? 1.0f : 0.0f;
    f.band_color = band == 2 ? BAND_OVERCAST : BAND_PARTLY;
    f.veil_frac = frac;
    f.veil_color = VEIL;

    // warmth bridge (§B): overcast mutes but never erases the seam identity
    if (band == 2)
    {
        f.stops[1].C *= 0.75f;
        float floor_c = 0;
        if (f.stops[1].H >= 40 && f.stops[1].H <= 60) floor_c = 0.055f; // Rift/Ember
        else if (f.stops[1].H >= 80) floor_c = 0.018f;                  // Vault
        if (f.stops[1].C < floor_c)
            f.stops[1].C = floor_c;
    }
    // snow ground cue (§B, kept from v1)
    if (w.condition == KACHEL_COND_SNOW)
    {
        f.stops[2].H = 85;
        f.stops[2].L += 0.06f;
    }

    // --- air shift, cooking-capped (§C) ---
    update_air_band(state_air().pm25);
    int eff_band = air_band;
    time_t now = time(nullptr);
    if (state_timer().active)
        last_timer_active_at = now;
    if (eff_band == 2 && last_timer_active_at != 0 &&
        now - last_timer_active_at < 30 * 60)
        eff_band = 1;
    for (int i = 0; i < 3; i++)
        f.stops[i] = air_shift(f.stops[i], eff_band,
                               eff_band == 2 ? (i == 1 ? 0.080f : 0.060f) : 0.030f);
    f.band_color = air_shift(f.band_color, eff_band, eff_band == 2 ? 0.060f : 0.030f);
    f.veil_color = air_shift(f.veil_color, eff_band, eff_band == 2 ? 0.060f : 0.030f);

    // clamps: field floor L 0.09 (below = true RGB565 black, PALETTE_V2), C cap
    for (auto &s : f.stops)
    {
        s.L = s.L < 0.09f ? 0.09f : (s.L > 0.45f ? 0.45f : s.L);
        s.C = s.C > 0.10f ? 0.10f : s.C;
    }
    return f;
}

// ---------------------------------------------------------------- renderer

static constexpr int W = 480, H = 480;
static constexpr int HORIZON_Y = 312;
static constexpr int BAND_Y = 160;
static lv_obj_t *canvas;
static lv_color16_t *canvas_buf;
static face_params current, target;
static oklch row_colors[H];

// 8x8 Bayer matrix — ±1 LSB ordered dither, screen-anchored (§A)
static const uint8_t bayer8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42}, {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41}, {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21}};

// linear feather: replace rows in [center-half, center+half] with the lerp
// between the window's edge colors (the §B feather rule)
static void feather_rows(int center, int half)
{
    int a = center - half, b = center + half;
    if (a < 0) a = 0;
    if (b > H - 1) b = H - 1;
    if (b <= a) return;
    oklch ca = row_colors[a], cb = row_colors[b];
    for (int y = a; y <= b; y++)
        row_colors[y] = oklab_lerp(ca, cb, (float)(y - a) / (b - a));
}

static void compute_rows(const face_params &f, float breath_dl, bool fog)
{
    oklch horizon = f.stops[1];
    horizon.L += breath_dl;

    // base phase gradient
    for (int y = 0; y < H; y++)
    {
        if (y < HORIZON_Y)
            row_colors[y] = oklab_lerp(f.stops[0], horizon, (float)y / HORIZON_Y);
        else
            row_colors[y] = oklab_lerp(horizon, f.stops[2],
                                       (float)(y - HORIZON_Y) / (H - HORIZON_Y));
    }

    if (f.band_on > 0.01f)
    {
        int veil_y = BAND_Y + (int)(f.veil_frac * (HORIZON_Y - BAND_Y));
        int edge_y = f.veil_frac > 0.01f ? veil_y : BAND_Y;
        oklch edge_color = f.veil_frac > 0.01f ? f.veil_color : f.band_color;
        for (int y = 0; y < HORIZON_Y; y++)
        {
            oklch sky;
            if (y < BAND_Y)
            {
                sky = f.band_color;
                // partly keeps a ±0.015 internal grade; overcast is dead flat
                sky.L += (1.0f - f.band_flat) * 0.015f * (1.0f - 2.0f * (float)y / BAND_Y);
            }
            else if (y < edge_y)
                sky = f.veil_color;
            else if (edge_y < HORIZON_Y)
                sky = oklab_lerp(edge_color, horizon,
                                 (float)(y - edge_y) / (HORIZON_Y - edge_y));
            else
                sky = edge_color; // full veil: the seam is erased
            row_colors[y] = oklab_lerp(row_colors[y], sky, f.band_on);
        }
        // band bottom edge: 24 px partly / 12 px overcast feather
        feather_rows(BAND_Y, (int)(12 - 6 * f.band_flat));
        if (f.veil_frac > 0.01f && veil_y < HORIZON_Y)
            feather_rows(veil_y, 14); // veil terminator, 28 px
    }
    if (fog)
        feather_rows(HORIZON_Y, 96); // fog diffuses the seam (§B)

    for (int y = 0; y < H; y++)
    {
        rgbf c = oklch_to_srgb(row_colors[y]);
        float r31 = c.r * 31.0f, g63 = c.g * 63.0f, b31 = c.b * 31.0f;
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
static lv_obj_t *line_label;
static lv_obj_t *droplet_lines[3];
static lv_obj_t *slot_band;
static lv_obj_t *slot_label;
static lv_obj_t *slot_count_label;
static lv_point_precise_t droplet_pts[3][2];

enum timer_ui_state
{
    TIMER_HIDDEN,
    TIMER_RUNNING,
    TIMER_NOTABLE,
    TIMER_DONE,
};
static timer_ui_state timer_ui = TIMER_HIDDEN;
static uint32_t timer_done_ms;
static bool slot_shown;

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

static void slot_show(bool with_count)
{
    if (!slot_shown)
    {
        fade_to(slot_band, LV_OPA_TRANSP, LV_OPA_COVER, KACHEL_T_CARD_IN_MS, false);
        slot_shown = true;
    }
    if (with_count)
    {
        lv_obj_remove_flag(slot_count_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(slot_label, LV_ALIGN_BOTTOM_MID, 0, -6); // countdown above
        lv_obj_set_style_bg_opa(slot_band, 160, LV_PART_MAIN); // timer earns a stage
    }
    else
    {
        lv_obj_add_flag(slot_count_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(slot_label, LV_ALIGN_CENTER, 0, 0); // wrapped text centers
        // calendar text sits bare on the field — the plinth over the bright
        // seam zone read as a gray stripe (photo verdict 2026-07-25)
        lv_obj_set_style_bg_opa(slot_band, LV_OPA_TRANSP, LV_PART_MAIN);
    }
}

static void slot_hide()
{
    if (slot_shown)
    {
        fade_to(slot_band, LV_OPA_COVER, LV_OPA_TRANSP, KACHEL_T_CARD_OUT_MS, true);
        slot_shown = false;
    }
}

static void reset_slot_style()
{
    lv_obj_set_style_bg_color(slot_band, oklch_to_lv(SLOT_FILL), LV_PART_MAIN);
    lv_obj_set_style_text_color(slot_label, oklch_to_lv(SLOT_TEXT), LV_PART_MAIN);
    lv_obj_set_style_text_color(slot_count_label, oklch_to_lv(SLOT_TEXT), LV_PART_MAIN);
}

// The one urgent beat (§E): fill rises to the urgent amber in 350 ms, decays
// back to the regular plinth over 1600 ms; text stays urgent until tap/decay.
static void pulse_exec(void *obj, int32_t v)
{
    oklch c;
    if (v <= (int32_t)KACHEL_T_PULSE_RISE_MS)
        c = oklab_lerp(SLOT_FILL, SLOT_FILL_URGENT, (float)v / KACHEL_T_PULSE_RISE_MS);
    else
        c = oklab_lerp(SLOT_FILL_URGENT, SLOT_FILL,
                       (float)(v - KACHEL_T_PULSE_RISE_MS) / KACHEL_T_PULSE_DECAY_MS);
    lv_obj_set_style_bg_color((lv_obj_t *)obj, oklch_to_lv(c), LV_PART_MAIN);
}

static void urgent_pulse()
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, slot_band);
    lv_anim_set_values(&a, 0, KACHEL_T_PULSE_RISE_MS + KACHEL_T_PULSE_DECAY_MS);
    lv_anim_set_duration(&a, KACHEL_T_PULSE_RISE_MS + KACHEL_T_PULSE_DECAY_MS);
    lv_anim_set_exec_cb(&a, pulse_exec);
    lv_anim_start(&a);
    lv_obj_set_style_text_color(slot_count_label, oklch_to_lv(SLOT_TEXT_URGENT), LV_PART_MAIN);
    lv_obj_set_style_text_color(slot_label, oklch_to_lv(SLOT_TEXT_URGENT), LV_PART_MAIN);
}

// --- slot content (§E ladder, "next event always" — §10 2026-07-25).
// Returns true when the slot is occupied. ---
static const char *WEEKDAYS_DE[7] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

static bool fill_slot_calendar(daypart dp)
{
    kachel_event events[KACHEL_EVENTS_MAX];
    int n = state_events(events);
    time_t now = time(nullptr);
    struct tm today, evt;
    localtime_r(&now, &today);

    for (int i = 0; i < n; i++)
    {
        if (!events[i].valid || events[i].start + 300 < now)
            continue; // leaves 5 min after start
        localtime_r(&events[i].start, &evt);
        bool is_today = evt.tm_yday == today.tm_yday && evt.tm_year == today.tm_year;

        if (dp == DP_RUSH && is_today)
        {
            time_t leave = events[i].start - KACHEL_LEAVE_LEAD_MIN * 60;
            struct tm lv_tm;
            localtime_r(&leave, &lv_tm);
            lv_label_set_text_fmt(slot_label, "%02d:%02d \xC2\xB7 %s \xE2\x80\x94 los um %02d:%02d",
                                  evt.tm_hour, evt.tm_min, events[i].title,
                                  lv_tm.tm_hour, lv_tm.tm_min);
            return true;
        }
        if (is_today)
            lv_label_set_text_fmt(slot_label, "%02d:%02d \xC2\xB7 %s",
                                  evt.tm_hour, evt.tm_min, events[i].title);
        else
        {
            time_t tomorrow = now + 24 * 3600;
            struct tm tm_tom;
            localtime_r(&tomorrow, &tm_tom);
            bool is_tomorrow = evt.tm_yday == tm_tom.tm_yday && evt.tm_year == tm_tom.tm_year;
            if (is_tomorrow)
                lv_label_set_text_fmt(slot_label, "Morgen %02d:%02d \xC2\xB7 %s",
                                      evt.tm_hour, evt.tm_min, events[i].title);
            else
                lv_label_set_text_fmt(slot_label, "%s %02d:%02d \xC2\xB7 %s",
                                      WEEKDAYS_DE[evt.tm_wday], evt.tm_hour, evt.tm_min,
                                      events[i].title);
        }
        return true;
    }
    return false;
}

static void tick(lv_timer_t *)
{
    auto const now_ms = millis();
    auto w = state_weather();
    int minute = time_sync_minute_of_day();
    daypart dp = daypart_now(minute, w.sunset_min);

    // --- field: slew toward target over the ambient beat, breathe at rest ---
    target = target_for_now();
    float t = (float)KACHEL_T_FACE_TICK_MS / KACHEL_T_AMBIENT_MS;
    for (int i = 0; i < 3; i++)
        current.stops[i] = oklab_lerp(current.stops[i], target.stops[i], t);
    current.clock = oklab_lerp(current.clock, target.clock, t);
    current.line = oklab_lerp(current.line, target.line, t);
    current.band_color = oklab_lerp(current.band_color, target.band_color, t);
    current.veil_color = oklab_lerp(current.veil_color, target.veil_color, t);
    current.band_on += (target.band_on - current.band_on) * t;
    current.band_flat += (target.band_flat - current.band_flat) * t;
    current.veil_frac += (target.veil_frac - current.veil_frac) * t;

    float breath = 0.010f * sinf((float)now_ms * 2.0f * (float)M_PI / KACHEL_T_BREATH_PERIOD_MS);
    if (timer_ui == TIMER_DONE)
        breath = 0;
    compute_rows(current, breath, w.condition == KACHEL_COND_FOG);

    // --- mark: clock, demoted to 60% during rush (§D) ---
    lv_obj_set_style_text_color(clock_label, oklch_to_lv(current.clock), LV_PART_MAIN);
    lv_obj_set_style_text_opa(clock_label, dp == DP_RUSH ? (lv_opa_t)153 : LV_OPA_COVER,
                              LV_PART_MAIN);
    char text[8];
    if (time_sync_clock_text(text, sizeof(text)))
        lv_label_set_text(clock_label, text);

    // --- line: temp + precip window (§F) ---
    if (w.valid)
    {
        char line[48];
        int temp = (int)lroundf(w.temp);
        if (w.precip_12h_mm > 0.5f && w.precip_start_h >= 0)
            snprintf(line, sizeof(line), "%d\xC2\xB0 \xC2\xB7 Regen ab %dh", temp,
                     w.precip_start_h);
        else
            snprintf(line, sizeof(line), "%d\xC2\xB0", temp);
        lv_label_set_text(line_label, line);
        lv_obj_set_style_text_color(line_label, oklch_to_lv(current.line), LV_PART_MAIN);
        lv_obj_remove_flag(line_label, LV_OBJ_FLAG_HIDDEN);
    }

    // --- droplets: count = precip tercile; ticks when frozen (§F) ---
    int drops = 0;
    if (w.precip_12h_mm > 8.0f) drops = 3;
    else if (w.precip_12h_mm > 2.0f) drops = 2;
    else if (w.precip_12h_mm > 0.5f) drops = 1;
    else if (w.condition == KACHEL_COND_RAIN || w.condition == KACHEL_COND_SNOW) drops = 1;
    bool frozen = w.condition == KACHEL_COND_SNOW;
    for (int i = 0; i < 3; i++)
    {
        droplet_pts[i][0] = {(lv_value_precise_t)(56 + i * 16 + (frozen ? 0 : 6)),
                             (lv_value_precise_t)378};
        droplet_pts[i][1] = {(lv_value_precise_t)(56 + i * 16), (lv_value_precise_t)394};
        lv_line_set_points(droplet_lines[i], droplet_pts[i], 2);
        if (i < drops)
            lv_obj_remove_flag(droplet_lines[i], LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(droplet_lines[i], LV_OBJ_FLAG_HIDDEN);
    }

    // --- slot: timer preempts, then daypart calendar ladder (§E) ---
    auto tmr = state_timer();
    time_t now = time(nullptr);
    if (tmr.active)
    {
        long remain = (long)(tmr.ends_at - now);
        if (remain > 0)
        {
            lv_label_set_text(slot_label, tmr.label);
            if (remain >= 3600)
                lv_label_set_text_fmt(slot_count_label, "%ld:%02ld:%02ld",
                                      remain / 3600, (remain % 3600) / 60, remain % 60);
            else
                lv_label_set_text_fmt(slot_count_label, "%ld:%02ld", remain / 60, remain % 60);
            if (timer_ui == TIMER_HIDDEN || timer_ui == TIMER_DONE)
            {
                reset_slot_style();
                slot_show(true);
                timer_ui = TIMER_RUNNING;
            }
            if (timer_ui == TIMER_RUNNING && remain <= 60)
            {
                // notable accent (§5.5): countdown warms, no motion
                lv_obj_set_style_text_color(slot_count_label,
                                            oklch_to_lv({0.72f, 0.070f, 70}), LV_PART_MAIN);
                timer_ui = TIMER_NOTABLE;
            }
        }
        else
        {
            if (timer_ui == TIMER_RUNNING || timer_ui == TIMER_NOTABLE)
            {
                lv_label_set_text(slot_count_label, "0:00");
                urgent_pulse();
                timer_ui = TIMER_DONE;
                timer_done_ms = now_ms;
            }
            if (timer_ui == TIMER_DONE && now_ms - timer_done_ms > KACHEL_T_DONE_DECAY_MS)
            {
                slot_hide();
                timer_ui = TIMER_HIDDEN;
            }
        }
    }
    else
    {
        if (timer_ui != TIMER_HIDDEN)
        {
            reset_slot_style();
            slot_hide();
            timer_ui = TIMER_HIDDEN;
        }
        if (timer_ui == TIMER_HIDDEN)
        {
            if (fill_slot_calendar(dp))
                slot_show(false);
            else
                slot_hide();
        }
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

    current = target = target_for_now();
    compute_rows(current, 0, false);

    // MARK: Doto clock, center (240, 200), anchor eternal (§D)
    clock_label = lv_label_create(tile);
    lv_obj_set_style_text_font(clock_label, &font_clock_100, LV_PART_MAIN);
    lv_label_set_text(clock_label, "--:--");
    // +8: digits sit high in the 112px line box (glyph rows 0-97,
    // baseline 101) — this puts the digit block center exactly at y=240,
    // the true center of the square (Josch 2026-07-25)
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_text_color(clock_label, oklch_to_lv(current.clock), LV_PART_MAIN);

    // LINE: temp + precip window, top-left (36, 52) (§F)
    line_label = lv_label_create(tile);
    lv_obj_set_style_text_font(line_label, &font_text_22, LV_PART_MAIN);
    lv_obj_align(line_label, LV_ALIGN_TOP_LEFT, 36, 40);
    lv_obj_add_flag(line_label, LV_OBJ_FLAG_HIDDEN);

    // DROPLETS: 1-3 hairlines at (56, 388) (§F)
    for (int i = 0; i < 3; i++)
    {
        droplet_lines[i] = lv_line_create(tile);
        lv_obj_set_style_line_width(droplet_lines[i], 2, LV_PART_MAIN);
        lv_obj_set_style_line_color(droplet_lines[i], oklch_to_lv(DROPLET), LV_PART_MAIN);
        lv_obj_set_style_line_rounded(droplet_lines[i], true, LV_PART_MAIN);
        lv_obj_add_flag(droplet_lines[i], LV_OBJ_FLAG_HIDDEN);
    }

    // SLOT: full-width amber plinth, bottom (§E)
    slot_band = lv_obj_create(tile);
    lv_obj_remove_style_all(slot_band);
    lv_obj_set_size(slot_band, W, 88);
    lv_obj_align(slot_band, LV_ALIGN_BOTTOM_MID, 0, 0);
    // §4 one-gesture silence: taps on the slot bubble to the tile handler
    lv_obj_add_flag(slot_band, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(slot_band, LV_OBJ_FLAG_HIDDEN);
    slot_count_label = lv_label_create(slot_band);
    lv_obj_set_style_text_font(slot_count_label, &font_timer_44, LV_PART_MAIN);
    lv_obj_align(slot_count_label, LV_ALIGN_TOP_MID, 0, 2);
    slot_label = lv_label_create(slot_band);
    lv_obj_set_style_text_font(slot_label, &font_guest_22, LV_PART_MAIN);
    lv_obj_set_width(slot_label, 440);
    lv_label_set_long_mode(slot_label, LV_LABEL_LONG_WRAP); // 2 lines fit the band
    lv_obj_set_style_text_align(slot_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(slot_label, LV_ALIGN_CENTER, 0, 0);
    reset_slot_style();

    // one-gesture silence (§4): any tap on the face dismisses a done timer
    lv_obj_add_event_cb(tile, [](lv_event_t *)
                        {
        if (timer_ui == TIMER_DONE)
        {
            reset_slot_style();
            slot_hide();
            timer_ui = TIMER_HIDDEN;
        } },
                        LV_EVENT_CLICKED, nullptr);

    lv_timer_create(tick, KACHEL_T_FACE_TICK_MS, nullptr);
}
