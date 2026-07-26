#pragma once

#include <cstdint>
#include <ctime>

// Typed view of the SPEC §7 state payloads. Populated by the MQTT task
// (ArduinoJson), read by UI code through snapshot copies (thread-safe).

enum kachel_condition : uint8_t
{
    KACHEL_COND_CLEAR = 0,
    KACHEL_COND_PARTLYCLOUDY,
    KACHEL_COND_CLOUDY,
    KACHEL_COND_RAIN,
    KACHEL_COND_FOG,
    KACHEL_COND_SNOW,
    KACHEL_COND_UNKNOWN,
};

struct kachel_air
{
    int pm25 = -1; // -1 = unknown
    int fan = 0;
    bool auto_mode = false; // preset "auto" active (§10 decision 2026-07-25)
    int filter_pct = -1;
    bool valid = false;
};

struct kachel_weather
{
    float temp = 0;
    kachel_condition condition = KACHEL_COND_UNKNOWN;
    float precip_12h_mm = 0;
    int precip_start_h = -1;   // local hour precip begins; -1 = none/unknown
    int sunrise_min = 6 * 60;  // minutes of day; defaults keep face sane pre-data
    int sunset_min = 21 * 60;
    bool valid = false;
};

struct kachel_event
{
    char title[64] = "";
    time_t start = 0;      // all-day events: local midnight
    bool all_day = false;  // "start" was date-only (no T)
    bool valid = false;
};

struct kachel_timer
{
    char label[32] = "";
    time_t ends_at = 0;
    bool active = false;
};

constexpr int KACHEL_EVENTS_MAX = 3; // contract: calendar next[] max 3
constexpr int KACHEL_BRING_ITEMS_MAX = 5;

struct kachel_bring
{
    int count = 0;
    char items[KACHEL_BRING_ITEMS_MAX][48];
    int item_count = 0;
    bool valid = false;
};

// Ingest a raw payload (called from MQTT task context).
void state_model_ingest(int topic, const char *payload);

// Snapshot accessors (copy under lock; safe from UI thread).
kachel_air state_air();
kachel_weather state_weather();
kachel_event state_next_event();
kachel_timer state_timer();
kachel_bring state_bring();
// Copies up to KACHEL_EVENTS_MAX events into out; returns count.
int state_events(kachel_event *out);

// PM2.5 history ring (24 h, 5-min samples; PSRAM; resets on reboot — §10).
// Call tick ~1 Hz from the UI thread; it samples last-known pm25 on schedule.
// Returns true when a sample was appended (the chart's redraw trigger).
bool state_history_tick();
// Copies samples oldest-first into out (max n); returns count. -1 = no data.
int state_history(int16_t *out, int n);
