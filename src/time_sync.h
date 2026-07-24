#pragma once

// WiFi + SNTP clock source. Non-blocking; fail-calm (SPEC §5.6): while time
// is invalid the schedule stays on day scale, no error surfaces.
void time_sync_begin();
bool time_sync_valid();
// minutes since local midnight, or -1 while clock invalid
int time_sync_minute_of_day();
// "HH:MM" into buf (>= 6 bytes); false while clock invalid
bool time_sync_clock_text(char *buf, unsigned len);
