#pragma once

// Brightness schedule + night states (SPEC §5.12) and idle auto-return
// (SPEC §4). Owns the backlight: 150 Hz PWM, 10-bit, 1% duty floor
// (RESEARCH §1 — panel dims properly only at low PWM frequency).
void display_schedule_init();
void display_schedule_tick();
