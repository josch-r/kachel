// Kachel palette — the only source of color in this firmware (SPEC §5.16).
// Tokens defined in OKLCH, precomputed to sRGB hex for lv_color_hex().
// Conversion: OKLab reference transform, sRGB-encoded, clamped.
// Starter set for M1 placeholders; design phase (M3) extends, never bypasses.
#pragma once

#include <lvgl.h>

// token            oklch(L C H)              sRGB      RGB565
#define KACHEL_BG_REST        lv_color_hex(0x0f0a05) // oklch(0.15 0.015  75) 0x0840 resting background, warm near-black
#define KACHEL_SURFACE_LIGHTS lv_color_hex(0x1a160b) // oklch(0.20 0.020  90) 0x18A1 Lights layer placeholder surface
#define KACHEL_SURFACE_AIR    lv_color_hex(0x0b181c) // oklch(0.20 0.020 220) 0x08C3 Air layer placeholder surface
#define KACHEL_SURFACE_HOUSE  lv_color_hex(0x111810) // oklch(0.20 0.020 140) 0x10C2 Household layer placeholder surface
#define KACHEL_TEXT_PRIMARY   lv_color_hex(0xe4ddcf) // oklch(0.90 0.020  85) 0xE6F9 primary text, warm off-white
#define KACHEL_TEXT_DIM       lv_color_hex(0x868073) // oklch(0.60 0.020  85) 0x840E secondary text
#define KACHEL_NIGHT_AMBER    lv_color_hex(0x966626) // oklch(0.55 0.100  70) 0x9324 ultra-dim night clock, warm amber
#define KACHEL_BG_BLACK       lv_color_hex(0x000000) // oklch(0 0 0)                  true black, night blackout only
