#pragma once

// MQTT transport (SPEC §7 contract). Fail calm: connection loss surfaces
// only as data staleness, never as UI errors. Reconnects with backoff.
void mqtt_begin();
void mqtt_tick();
bool mqtt_connected();
