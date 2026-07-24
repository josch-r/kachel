# Kachel — Ambient Face Design Specification (M3)

Design pass 2026-07-24 (specialist agent, adopted into firmware `src/ambient_face.cpp`).
All colors: **OKLCH (L 0–1, C, H deg)**. All blends — spatial (between gradient stops) and temporal (phase ramps, weather/air fades) — execute in **OKLab** (lerp a,b; never rotate hue in LCH): chroma is low everywhere, so blends pass near-gray and can never produce accidental green/cyan bands. Canvas 480×480. Opacity as LVGL opa 0–255.

## G. Hue law (one hue = one meaning, forever — SPEC §5.4)

| H (deg) | Family | Single meaning |
|---|---|---|
| 75–90 | Bone (warm neutral) | All is well — resting field base, ground, text, clock |
| 70 | Amber | Time — night clock (`KACHEL_NIGHT_AMBER`), timer card accents, the timer-done pulse |
| 40–60 | Rose-amber | Sun at the horizon — dawn glow (≈45), dusk glow (≈58), nowhere else |
| 240–255 | Slate | Sky/atmosphere — day zenith, rain-cooled field; C never exceeds 0.030 |
| 330 | Dust-violet | Air quality degraded — the only alert hue on the face |
| 90 / 220 / 140 | Layer surfaces | Lights / Air / Household identities (existing tokens, unchanged) |
| — (red, 15–30) | Forbidden | Intentionally unused. Nothing on this face is an emergency. |

## A. Time-of-day gradient phases

Vertical gradient, three stops: **top y=0, horizon y=312 (65%), bottom y=480**. Sunrise/sunset from `kachel/state/weather`; if stale, last-known (§5.6). Rendered with Bayer 8×8 dither, ±1 LSB per channel (§6 banding rule).

| Phase | Window | Top (L, C, H) | Horizon (L, C, H) | Bottom (L, C, H) |
|---|---|---|---|---|
| Night | 00:00–06:00 | — | `KACHEL_BG_BLACK`, screen off (§5.12) | — |
| Pre-dawn | 06:00 → sunrise−40′ | (0.11, 0.015, 75) | (0.15, 0.035, 62) | (0.08, 0.010, 75) |
| Dawn peak | at sunrise | (0.17, 0.025, 250) | (0.26, 0.090, 45) | (0.10, 0.015, 75) |
| Day | sunrise+40′ → sunset−40′ | (0.28, 0.030, 240) | (0.33, 0.020, 85) | (0.15, 0.015, 75) |
| Dusk peak | at sunset | (0.14, 0.022, 255) | (0.24, 0.095, 58) | (0.09, 0.012, 70) |
| Evening | sunset+40′ → 22:00 | = pre-dawn (winter 06:30 stays warm, never blue-white, §5.13) |
| Clock-only | 22:00–00:00 | field → black (schedule overlay) |

Phase ramps: linear OKLab lerp across the ±40 min windows around sunrise/sunset.

## B. Weather condition modifiers

Per stop: ΔL, C multiplier, optional OKLab blend fraction *k* toward an anchor (anchor keeps stop L). After all modifiers clamp L ∈ [0.05, 0.35], C ≤ 0.10.

| Condition | Top ΔL | Horizon ΔL | Bottom ΔL | C × | Hue treatment |
|---|---|---|---|---|---|
| clear / sunny | 0 | 0 | 0 | 1.00 | none (baseline) |
| partlycloudy | 0 | −0.01 | 0 | 0.75 | none |
| cloudy | +0.02 | −0.04 | +0.01 | 0.45 | none — gradient flattens to a lid |
| rain | −0.02 | −0.05 | −0.02 | 0.55 | blend k=0.35 toward (L, 0.030, 245) per stop |
| fog | +0.04 | −0.02 | +0.05 | 0.30 | none — veil lifts ground, erases horizon |
| snow | +0.02 | +0.01 | +0.06 | 0.40 | bottom stop hue → 85 — lightened ground is the cue |

**Rain-later rule:** condition ≠ rain and `precip_12h_mm` > 0.5 → final = lerp(condition-modified, rain-modified, 0.60). Modifier changes crossfade over 8 s (KACHEL_T_AMBIENT_MS).

## C. Air quality hue shift

Per stop: anchor (L = stop L, C = 0.080, H = 330), OKLab blend fraction *k*. **L never changes** — poor air discolors the sky, never brightens it.

| PM2.5 band | Hysteresis | k | C floor after blend |
|---|---|---|---|
| Good ≤ 12 | exit elevated < 10 | 0 | — |
| Elevated 12–35 | enter > 12 | 0.40 | 0.035 |
| Poor > 35 | enter > 35, exit < 31 | 0.85 | 0.075 horizon, 0.060 top/bottom |

Band transitions ride the 8 s ambient fade. Field-only (field is black 22:00–06:00; Air layer holds detail).

**Composition order:** phase stops → phase ramp → weather modifier → rain-later blend → air blend → clamp → breathing offset → dithered render.

## D. Clock

| Property | Value |
|---|---|
| Content | HH:MM, 24 h, no seconds |
| Typeface | Inter Tight Light 300, one family for every numeral on the device (§5.15) |
| Size | em 176 px → digit cap height ≈128 px (≥120 ✓) |
| Anchor | center (240, 200). Never moves. |
| Opacity | 255 — dimming lives in L, not alpha (avoids RGB565 blend banding) |
| Colon | static; no blink — a 60 cpm blink is an idle loop (§5.7). *(v1 deviation: colon full opacity — per-glyph opacity needs span widget, deferred)* |

| Phase | Clock (L, C, H) |
|---|---|
| Pre-dawn / Evening | (0.68, 0.045, 70) |
| Dawn peak | (0.82, 0.028, 80) |
| Day | (0.90, 0.020, 85) |
| Dusk peak | (0.78, 0.035, 72) |
| 22:00–00:00 | `KACHEL_NIGHT_AMBER` (0.55, 0.100, 70) on black |

## E. Guest cards

Fixed anchors in the ground zone, forever (§5.2). Flat translucent fills (no blur on device).

**Timer card:** 280×96 px, center (240, 376), radius 24. Fill (0.20, 0.015, 75) opa 120; border 1 px (0.35, 0.020, 75) opa 60. Label: Inter Tight 22 px `KACHEL_TEXT_DIM` top-center. Countdown: Inter Tight Light 72 px `KACHEL_TEXT_PRIMARY`, M:SS (H:MM:SS ≥ 1 h). Fade-in 600 ms, fade-out 400 ms. T−60 s notable cue: countdown color → (0.72, 0.070, 70).

**Timer-done pulse (the one urgent beat, §5.5 — visual only, relay variant):** t0 text → (0.72, 0.085, 70); fill rises to (0.45, 0.110, 70) opa 216 in 350 ms; decays to done state over 1600 ms. Done (static until tap): fill (0.17, 0.040, 70) opa 160, border (0.45, 0.080, 70) opa 90. Tap anywhere dismisses (§4 one-gesture silence); 60 s self-decay fallback (§5.5).

**Calendar line:** max 400×24 px, center (240, 448). "HH:MM · Titel", Inter Tight 22 px `KACHEL_TEXT_DIM`, bare text on field. Visible when next event ≤ 2 h away, leaves 5 min after start. Fade 1500 ms each way.

## F. Breathing

Horizon stop L += 0.010 · sin(2πt / 10 s) — 6 cycles/min (§5.7 ceiling), horizon only. Suspended during the done-beat and at night. A near-threshold luminance drift reads peripherally as a tended, living object — the mui register — invisible to a direct glance.
