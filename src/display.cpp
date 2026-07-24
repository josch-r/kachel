#include "display.h"

#include <Arduino.h>
#include <lvgl.h>

#include "carousel.h"
#include "config.h"
#include "palette.h"
#include "time_sync.h"
#include "timing.h"

#ifndef DISPLAY_BCKL
#define DISPLAY_BCKL 38
#endif

// 150 Hz / 10-bit: the panel only dims cleanly at low PWM frequency, and
// 10 bits keep the 1% floor resolvable (RESEARCH §1). Replaces the lib's
// 400 Hz / 8-bit setup from smartdisplay_init().
static constexpr uint32_t BCKL_FREQ_HZ = 150;
static constexpr uint8_t BCKL_BITS = 10;
static constexpr uint32_t BCKL_MAX = (1u << BCKL_BITS) - 1;

enum sched_state
{
    SCHED_DAY,
    SCHED_NIGHT_CLOCK, // 22:00-24:00 ultra-dim, clock only (SPEC §5.12)
    SCHED_BLACK,       // 00:00-06:00
};

static lv_obj_t *night_overlay;
static lv_obj_t *night_clock_label;
static float brightness_now = 1.0f;
static uint32_t wake_until_ms; // nonzero while touch-wake is active
static uint32_t last_tick_ms;

// same LEDC channel the lib configured, re-tuned to 150 Hz / 10-bit
static constexpr uint8_t BCKL_CHANNEL = SOC_LEDC_CHANNEL_NUM - 1;

static void backlight_set(float duty)
{
    if (duty > 0.0f && duty < KACHEL_BRIGHT_NIGHT)
        duty = KACHEL_BRIGHT_NIGHT; // 1% floor, but true zero allowed (black)
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(DISPLAY_BCKL, (uint32_t)(duty * BCKL_MAX));
#else
    ledcWrite(BCKL_CHANNEL, (uint32_t)(duty * BCKL_MAX));
#endif
}

// minute-of-day source; test mode compresses 24 h into 2 min starting 20:00
static int schedule_minute_of_day()
{
#if KACHEL_SCHEDULE_COMPRESS_TEST
    return (int)((20 * 60 + (millis() / (120000 / 1440))) % 1440);
#else
    return time_sync_minute_of_day();
#endif
}

static sched_state state_for_minute(int minute)
{
    if (minute < 0)
        return SCHED_DAY; // fail calm: no clock -> stay on day scale
    if (minute >= KACHEL_BLACK_START_MIN && minute < KACHEL_DAY_START_MIN)
        return SCHED_BLACK;
    if (minute >= KACHEL_NIGHT_START_MIN)
        return SCHED_NIGHT_CLOCK;
    return SCHED_DAY;
}

static float brightness_target(sched_state state, int minute)
{
    switch (state)
    {
    case SCHED_BLACK:
        return 0.0f;
    case SCHED_NIGHT_CLOCK:
    {
        // gradual ramp from day scale down to the ultra-dim floor
        int into = minute - KACHEL_NIGHT_START_MIN;
        if (into < KACHEL_RAMP_MINUTES)
        {
            float t = (float)into / KACHEL_RAMP_MINUTES;
            return KACHEL_BRIGHT_DAY + t * (KACHEL_BRIGHT_NIGHT - KACHEL_BRIGHT_DAY);
        }
        return KACHEL_BRIGHT_NIGHT;
    }
    default:
        return KACHEL_BRIGHT_DAY;
    }
}

void display_schedule_init()
{
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcDetach(DISPLAY_BCKL);
    ledcAttach(DISPLAY_BCKL, BCKL_FREQ_HZ, BCKL_BITS);
#else
    ledcSetup(BCKL_CHANNEL, BCKL_FREQ_HZ, BCKL_BITS);
    ledcAttachPin(DISPLAY_BCKL, BCKL_CHANNEL);
#endif
    backlight_set(brightness_now);

    // night overlay lives above the carousel on the top layer
    night_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(night_overlay);
    lv_obj_set_size(night_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(night_overlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(night_overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_add_flag(night_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(night_overlay, LV_OBJ_FLAG_CLICKABLE);

    night_clock_label = lv_label_create(night_overlay);
    lv_label_set_text(night_clock_label, "--:--");
    lv_obj_set_style_text_color(night_clock_label, KACHEL_NIGHT_AMBER, LV_PART_MAIN);
    lv_obj_set_style_text_font(night_clock_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_center(night_clock_label);

    last_tick_ms = millis();
}

void display_schedule_tick()
{
    auto const now = millis();
    auto const dt = now - last_tick_ms;
    last_tick_ms = now;

    auto const minute = schedule_minute_of_day();
    auto const state = state_for_minute(minute);

    // touch anywhere wakes to full detail (SPEC §5.12)
    bool user_active = lv_display_get_inactive_time(nullptr) < 200;
    if (user_active)
        wake_until_ms = now + KACHEL_IDLE_RETURN_MS;
    bool awake = wake_until_ms != 0 && (int32_t)(wake_until_ms - now) > 0;
    if (!awake)
        wake_until_ms = 0;

    // content: night states show the clock-only overlay unless woken
    bool overlay_wanted = (state != SCHED_DAY) && !awake;
    bool overlay_shown = !lv_obj_has_flag(night_overlay, LV_OBJ_FLAG_HIDDEN);
    if (overlay_wanted != overlay_shown)
    {
        if (overlay_wanted)
        {
            lv_obj_remove_flag(night_overlay, LV_OBJ_FLAG_HIDDEN);
            carousel_return_home(false); // return to periphery behind the overlay
        }
        else
            lv_obj_add_flag(night_overlay, LV_OBJ_FLAG_HIDDEN);
    }

    // clock text: hidden while black (label off keeps true darkness)
    if (state == SCHED_BLACK && !awake)
        lv_obj_add_flag(night_clock_label, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_remove_flag(night_clock_label, LV_OBJ_FLAG_HIDDEN);
    char clock_text[6];
#if KACHEL_SCHEDULE_COMPRESS_TEST
    snprintf(clock_text, sizeof(clock_text), "%02d:%02d", minute / 60, minute % 60);
    lv_label_set_text(night_clock_label, clock_text);
#else
    if (time_sync_clock_text(clock_text, sizeof(clock_text)))
        lv_label_set_text(night_clock_label, clock_text);
#endif

    // brightness: slew toward target, never step (SPEC §5.12)
    float target = awake ? KACHEL_BRIGHT_DAY : brightness_target(state, minute);
    float max_step = (float)dt / KACHEL_T_TRANSITION_MS; // full range per transition beat
    float delta = target - brightness_now;
    if (delta > max_step)
        delta = max_step;
    else if (delta < -max_step)
        delta = -max_step;
    brightness_now += delta;
    backlight_set(brightness_now);

    // §4 idle auto-return: any non-home layer snaps back after the timeout
    if (state == SCHED_DAY && !user_active &&
        lv_display_get_inactive_time(nullptr) > KACHEL_IDLE_RETURN_MS)
        carousel_return_home(true);
}
