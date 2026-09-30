#pragma once

#include <cstdint>

// MQTT transport (SPEC §7 contract). Fail calm: connection loss surfaces
// as data staleness plus the control layers' link mark (§10 2026-09-29),
// never as full-screen errors. Reconnects with backoff.
void mqtt_begin();
void mqtt_tick();
bool mqtt_connected();

// Link diagnostics snapshot (copied under lock; safe from UI thread).
struct kachel_link
{
    bool broker_up = false;
    bool ever_connected = false;
    uint32_t changed_ms = 0;  // millis() of the last up/down transition (0 = boot)
    int last_rc = 0;          // PubSubClient state at last failure/drop
    uint32_t failed_tries = 0; // connect attempts since last success
};
kachel_link mqtt_link();

// State topics per SPEC §7 contract
enum kachel_topic
{
    KACHEL_TOPIC_AIR = 0,
    KACHEL_TOPIC_WEATHER,
    KACHEL_TOPIC_CALENDAR,
    KACHEL_TOPIC_BRING,
    KACHEL_TOPIC_TIMER,
    KACHEL_TOPIC_COUNT,
};

struct kachel_state
{
    char payload[512];
    uint32_t received_ms; // millis() at last message
    bool ever_received;
};

// Latest raw payload for a state topic (always valid pointer).
const kachel_state *mqtt_state(kachel_topic topic);

// Seconds since last message, or -1 if never received.
int32_t mqtt_state_age_s(kachel_topic topic);

// True when topic violates the §5.6 stale rule (silent > 3x cadence).
// On-change topics (air, timer) never report stale.
bool mqtt_state_stale(kachel_topic topic);

// Command topics (SPEC §7): fire on gesture completion only (§4).
// Dropped while the broker is down — a tap never replays later.
void mqtt_cmd_scene(uint8_t id); // 1..4
void mqtt_cmd_air(uint8_t fan);  // 0..3 (manual level; 0 = off)
void mqtt_cmd_air_auto();        // {"mode":"auto"} — §10 decision 2026-07-25
