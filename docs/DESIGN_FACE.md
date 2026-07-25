# Kachel — Ambient Face Design Specification v2 "Three Strata"

Design pass 2026-07-25 (research council + colorist/hardware-critic experts; direction and
palette signed off by Josch). Replaces v1 (git history holds it). Color numbers live in
`DESIGN_PALETTE_V2.md` — that file is the palette authority; this file owns structure,
behavior, and typography. All colors OKLCH; all blends OKLab lerp (never rotate hue in LCH).
Canvas 480×480, horizon y=312, weather band y=0–160.

## Architecture: three permanent strata + one line

| Stratum | Owns | Anchor (never moves) |
|---|---|---|
| FIELD | atmosphere: time-of-day phase + weather + air | whole canvas |
| MARK | clock, Doto 100 px | center (240, 200) |
| SLOT | the one next thing that needs you (amber) | bottom band, text center (240, 448) |
| — LINE | secondary text: temp + precip window | top-left (36, 52), left-aligned |
| — DROPLETS | precip ≤12 h, 1–3 hairline strokes | (56, 388) |

Relevance moves through intensity and fill, **never position**. Empty slot, absent droplets,
absent band = the Yohaku all-well signal (§5.1).

## G. Hue law (unchanged in meaning, amended in ceiling)

| H | Family | Single meaning |
|---|---|---|
| 75–90 | Bone | all is well — text, clock, droplets |
| 70 | Amber | time: slot, timer, night |
| 40–60 | Rose-amber | sun at the horizon, ONLY inside the ±40 min sunrise/sunset ramps |
| 240–255 | Slate | sky/atmosphere — phases and the weather band; field C ≤ 0.045, band C ≤ 0.030 |
| 330 | Dust-violet | air degraded — the only alert hue |
| 15–30 | Forbidden | red never appears; enforced by the rose-guard (§C) and the night amber L/C rule |

## A. Field: phase gradients

Five phases, values in PALETTE_V2 §1 (Hush / Rift / Vault / Ember / Hearth). Composition
law: light lives at the horizon; top of frame L ≤ 0.15 always. Phase ramps: linear OKLab
lerp across ±40 min around sunrise/sunset (rose-amber exists only inside these ramps).
Sunrise/sunset from `kachel/state/weather`; stale ⇒ last-known (§5.6).
Breathing: horizon L ± 0.010 · sin(2πt/10 s), 6 cpm, suspended at night and during the
done-beat. Bayer 8×8 dither, ±1 LSB, screen-anchored (never frame-offset).

## B. Field: weather (absolute encoding — v1's relative modifiers are dead)

**Sky band (y 0–160, replaces phase top when present):** values are absolute constants
(PALETTE_V2 §2): clear = absent, partly = L 0.22 (24 px feather), overcast = L 0.29
dead-flat (12 px feather; flatness is the lid cue). Band changes ride the 8 s ambient fade.

**Precip veil:** fixed oklch(0.19 0.026 247) replacing the gradient from y160 down to the
fill line (28 px feather; 36 px during Hush/Hearth): `precip_12h_mm` 0.5–2 ⇒ y214,
2–8 ⇒ y262, >8 ⇒ y312 (veil erases the seam). Veil forces band ≥ partly.

**Warmth bridge:** under overcast, horizon C ×0.75 floored at C ≥ 0.055 during Rift/Ember
(overcast sunrise still says morning), 0.018 @ H90 for Vault.

**Condition mapping:** clear/sunny ⇒ clear; partlycloudy ⇒ partly; cloudy/fog ⇒ overcast;
rain/snow ⇒ overcast + veil per mm. Fog additionally diffuses the horizon seam (feather
×3 on the horizon stop). Snow: ground stop hue → 85, L +0.06 (lightened ground cue, kept from v1).

## C. Field: air quality (PALETTE_V2 §3)

k: good 0 / elevated 0.30 (subliminal by design) / poor 0.80 with (a,b)×0.5 pre-drain.
Hysteresis: enter >12 exit <10; enter >35 exit <31. L re-imposed after blend.
**Rose-guard:** blend skips stops with native H ∈ [30,70] and C > 0.05.
**Cooking suppression:** while a timer is active (and 30 min after it ends), air blend k is
capped at the elevated value — frying spikes are expected, escalation waits.

## D. Mark: clock

| Property | Value |
|---|---|
| Content | HH:MM, 24 h, no seconds, static colon |
| Typeface | Doto (OFL), static instance wght≈320 ROND 25, digits+colon subset |
| Size | ~100 px cap height; dot pitch must land on whole pixels |
| Anchor | center (240, 200), eternal |
| Color | per phase, PALETTE_V2 §4 (bone; ≥6.7:1 local contrast) |
| Demotion | rush daypart: opacity → 60% (position and size never change) |

## E. Slot (bottom band — replaces v1 timer card + calendar line)

One amber element, text Inter Tight 22 px (Josch 2026-07-25 — Departure Mono read too
teletype in the slot; restyle pass may revisit), wrapping to two lines, centered in the
band; timer adds countdown in Doto 44 px amber above a single-line label. Fill/colors PALETTE_V2 §4. Priority (highest wins):

1. **Timer running** — label + countdown M:SS (H:MM:SS ≥1 h). T−60 s notable cue and the
   done-beat per §5.5: urgency = +C +L within H70, never a hue move. Tap anywhere
   dismisses done state; 60 s self-decay.
2. **Rush:** first event today as leave-by — "HH:MM · Titel — los um HH:MM" (leave time =
   event start − configurable lead, default 15 min).
3. **Next event, always** (Josch amendment 2026-07-25, replaces the 2 h day-window and
   the evening tomorrow-first rules): the next upcoming event from the feed —
   today "HH:MM · Titel", tomorrow "Morgen HH:MM · Titel", later "Wd HH:MM · Titel".
   Leaves 5 min after start.
4. **Empty** — nothing renders. Emptiness now means: nothing in the next 48 h.

Fades: in 600 ms, out 400 ms (timing.h tokens).

## F. Line + droplets (weather foreground)

**Secondary line (22 px Departure Mono, top-left, per-phase color PALETTE_V2 §4):**
"T° · Regen ab Hh" when precip window known, else "T°". Hidden when weather never received.

**Droplets:** 1–3 strokes, 2 px, bone 0.80/0.020/80, at (56, 388), 16 px spacing:
count = precip tercile (same thresholds as veil). Frozen precip (snow/hail): strokes become
vertical ticks. Absence = dry. One 500 ms ease on change, then static.

## H. Daypart engine

| Daypart | Window | Primary | Clock | Slot |
|---|---|---|---|---|
| Rush | 06:00–09:00 weekdays (config) | weather (field amplitude ×1, droplets, line) | demoted 60% | leave-by |
| Day | rush-end → sunset−40′ | clock | full | event ≤2 h |
| Evening | sunset−40′ → 22:00 | clock | full | tomorrow first |
| Night | 22:00–06:00 | §5.12 schedule owns display (amber clock → black) | — | — |
| Cooking | overlay: timer active | timer | full | timer |

Transitions: single 500 ms ease (KACHEL_T_TRANSITION_MS); boundaries are clock-driven,
no hysteresis needed except cooking-end (+30 min air suppression tail).

## I. Night set (22:00–00:00)

Total amber tint, nothing escapes H70 (PALETTE_V2 §5). True black ground. Hardware rule:
dim amber L ≥ 0.40 at C 0.10, or C ≤ 0.05 below — RGB565 quantization otherwise rotates
amber into forbidden red. 00:00–06:00 black (unchanged, display.cpp owns).

## J. Typography system

| Instance | Face | Size | Subset | Use |
|---|---|---|---|---|
| font_clock_100 | Doto wght 320 ROND 25 | ~100 px | 0-9 : | clock |
| font_timer_44 | Doto wght 320 ROND 25 | 44 px | 0-9 : | timer countdown |
| font_text_22 | Departure Mono | 22 px | 0x20-0x7E ° · ÄÖÜäöüß | data line (temp/precip) |
| font_guest_22 | Inter Tight | 22 px | (v1 asset, kept) | slot text |

Three instances = the flash budget. Doto dots must land on whole pixels (verify at
conversion; reject sizes where dots straddle). Departure Mono at 22 px = 2× its 11 px
native grid, pixel-crisp at 4 bpp. Inter Tight instances retire with v1.

## K. On-device verification (gates v2 acceptance — PALETTE_V2 §6)

Six photo tests: bottom-histogram (no black-hole spike), weather A/B at 2.5 m/55°
(≥5/255 G-channel delta + 5/5 naming), 0° vs 55° band survival (3 levels), night amber
hue-sample (H ≥ 45), 120 fps crossfade slow-mo (monotonic pixel transitions), night
glow photo (no wall patch; PWM duty drops, not just palette L).
