#pragma once

#include <cstdint>

// MQTT transport (SPEC §7 contract). Fail calm: connection loss surfaces
// only as data staleness, never as UI errors. Reconnects with backoff.
void mqtt_begin();
void mqtt_tick();
bool mqtt_connected();

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
