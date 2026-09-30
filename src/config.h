// Kachel compile-time configuration (SPEC §7: v1 config is compile-time).
#pragma once

#include <cstdint>

#define KACHEL_FW_VERSION "0.7.0-link"

// --- schedule (SPEC §5.12; all times configurable here) ---
// Valid ranges (M5 audit): the schedule state machine assumes the order
// DAY_START < NIGHT_START, with BLACK_START in [0, DAY_START) — i.e. the
// black window wraps midnight (NIGHT->BLACK->DAY). A BLACK_START inside the
// day window is unsupported. RAMP_MINUTES must fit before BLACK_START
// (NIGHT_START + RAMP < 24 h + BLACK_START).
constexpr int KACHEL_DAY_START_MIN = 6 * 60;    // 06:00 back to day scale
constexpr int KACHEL_NIGHT_START_MIN = 22 * 60; // 22:00 ramp + clock-only
constexpr int KACHEL_BLACK_START_MIN = 0;       // 00:00 screen black
constexpr int KACHEL_RAMP_MINUTES = 30;         // gradual ramp length from 22:00

// brightness scale
constexpr float KACHEL_BRIGHT_DAY = 1.0f;
constexpr float KACHEL_BRIGHT_NIGHT = 0.01f; // ultra-dim floor (min PWM duty 1%)

// --- mounting ---
// 1 = panel mounted upside down (USB-C on top); LVGL rotates the frame in
// the flush path (board uses DISPLAY_SOFTWARE_ROTATION) and rotates touch
// points to match (lv_indev -> lv_display_rotate_point, AGENTS rule 3)
#define KACHEL_ROTATE_180 1

// --- interaction (SPEC §4) ---
constexpr uint32_t KACHEL_IDLE_RETURN_MS = 20000; // 20 s, spec range 10-30 s

// --- PM2.5 history ring (§10 decision: device-side, resets on reboot) ---
constexpr uint32_t KACHEL_PM25_SAMPLE_S = 300; // one sample per 5 min
constexpr int KACHEL_PM25_HISTORY_N = 288;     // 24 h window

// --- daypart engine (DESIGN_FACE v2 §H) ---
constexpr int KACHEL_RUSH_START_MIN = 6 * 60; // weekday rush window
constexpr int KACHEL_RUSH_END_MIN = 9 * 60;
constexpr int KACHEL_LEAVE_LEAD_MIN = 15; // leave-by = event start - lead

// --- test mode ---
// 1 = compress 24 h into 2 min for the dim-cycle evidence video; 0 = real time
#define KACHEL_SCHEDULE_COMPRESS_TEST 0
