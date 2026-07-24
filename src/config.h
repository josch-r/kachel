// Kachel compile-time configuration (SPEC §7: v1 config is compile-time).
#pragma once

#include <cstdint>

#define KACHEL_FW_VERSION "0.3.0-m3"

// --- schedule (SPEC §5.12; all times configurable here) ---
constexpr int KACHEL_DAY_START_MIN = 6 * 60;    // 06:00 back to day scale
constexpr int KACHEL_NIGHT_START_MIN = 22 * 60; // 22:00 ramp + clock-only
constexpr int KACHEL_BLACK_START_MIN = 0;       // 00:00 screen black
constexpr int KACHEL_RAMP_MINUTES = 30;         // gradual ramp length from 22:00

// brightness scale
constexpr float KACHEL_BRIGHT_DAY = 1.0f;
constexpr float KACHEL_BRIGHT_NIGHT = 0.01f; // ultra-dim floor (min PWM duty 1%)

// --- interaction (SPEC §4) ---
constexpr uint32_t KACHEL_IDLE_RETURN_MS = 20000; // 20 s, valid range 10-30 s

// --- test mode ---
// 1 = compress 24 h into 2 min for the dim-cycle evidence video; 0 = real time
#define KACHEL_SCHEDULE_COMPRESS_TEST 0
