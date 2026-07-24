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
    int filter_pct = -1;
    bool valid = false;
};

struct kachel_weather
{
    float temp = 0;
    kachel_condition condition = KACHEL_COND_UNKNOWN;
    float precip_12h_mm = 0;
    int sunrise_min = 6 * 60;  // minutes of day; defaults keep face sane pre-data
    int sunset_min = 21 * 60;
    bool valid = false;
};

struct kachel_event
{
    char title[64] = "";
    time_t start = 0;
    bool valid = false;
};

struct kachel_timer
{
    char label[32] = "";
    time_t ends_at = 0;
    bool active = false;
};

// Ingest a raw payload (called from MQTT task context).
void state_model_ingest(int topic, const char *payload);

// Snapshot accessors (copy under lock; safe from UI thread).
kachel_air state_air();
kachel_weather state_weather();
kachel_event state_next_event();
kachel_timer state_timer();
