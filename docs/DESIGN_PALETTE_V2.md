# Kachel Face — Palette v2 (Three Strata)

Colorist pass + hardware-critic audit, reconciled 2026-07-25. Pending Josch sign-off,
then folds into DESIGN_FACE v2. All colors oklch(L C H); all blends OKLab lerp.
Composition law: **light lives at the horizon; the top of the frame stays dark
(L ≤ 0.15 always).** Phases differ in seam brightness and the temperature of the dark.

Amendments vs. colorist draft (critic audit): all field stops floored at **L 0.09**
(below 0.08 quantizes to true RGB565 black — bottom stop is deliberate near-black
ground); partly-cloud band **0.21 → 0.22**; elevated-air shift is
**subliminal by design** (poor is the readable alert).

## 1. Phase gradients (top y=0 · horizon y=312 · bottom y=480)

| Phase | Anchor | Top | Horizon | Bottom |
|---|---|---|---|---|
| Pre-dawn | Hush | oklch(0.10 0.020 250) | oklch(0.16 0.032 248) | oklch(0.09 0.012 250) |
| Dawn peak | Rift | oklch(0.14 0.030 250) | oklch(0.30 0.100 45) | oklch(0.10 0.018 75) |
| Day | Vault | oklch(0.15 0.042 243) | oklch(0.35 0.024 90) | oklch(0.13 0.016 80) |
| Dusk peak | Ember | oklch(0.11 0.028 255) | oklch(0.26 0.095 57) | oklch(0.09 0.014 75) |
| Evening | Hearth | oklch(0.09 0.016 75) | oklch(0.17 0.035 75) | oklch(0.09 0.012 78) |

Seam-L arc: 0.16 → 0.30 → 0.35 → 0.26 → 0.17 → black. Rose-amber exists only inside
the ±40 min sunrise/sunset ramps (Hush loses v1's unearned warm seam). Slate field-chroma
ceiling amended 0.030 → **0.045** (field stops only; band stays ≤ 0.030) — §G update.
Breathing unchanged (horizon ±0.010 L, 6 cpm).

## 2. Weather sky band (y 0–160, absolute encoding, replaces phase top)

| Cloud step | Value | Edge |
|---|---|---|
| Clear | band absent — phase top shows | — |
| Partly | oklch(0.22 0.020 244), ±0.015 L internal grade | 24 px feather y148–172 |
| Overcast | oklch(0.29 0.012 250), dead flat — flatness is the lid cue | 12 px feather |

**Precip veil** (fixed oklch(0.19 0.026 247), replaces gradient down to fill line,
28 px feather; 36 px during Hush/Hearth): fill y214 (0.5–2 mm) / y262 (2–8 mm) /
y312 full (>8 mm — the veil erases the seam: heavy rain deletes the day's brightest line).
Veil implies band ≥ partly. Fill-depth is geometry, not lightness — L-budget stays at
3 absolute levels (critic ceiling for a standing off-axis viewer).

**Warmth bridge:** band never suppresses horizon identity. Under overcast, horizon C ×0.75
floored at C ≥ 0.055 during Rift/Ember (overcast sunrise = gray lid over muted rose seam).
Vault floor 0.018 @ H90; Hush/Hearth no floor.

## 3. Air (H330, L re-imposed after blend — discolors, never brightens)

| Band | Hysteresis | k | Anchor C | Post floor |
|---|---|---|---|---|
| Good ≤12 | exit <10 | 0 | — | — |
| Elevated 12–35 | enter >12 | 0.30 | 0.070 | 0.030 — **subliminal by design** |
| Poor >35 | enter >35, exit <31 | 0.80 | 0.090 | 0.080 horizon, 0.060 top/bottom; native (a,b)×0.5 pre-drain |

**Rose-guard (mandatory):** blend skips stops with native H ∈ [30,70] and C > 0.05 —
prevents rose × violet passing through forbidden red at dawn/dusk.

## 4. Foreground

Clock (Doto ~100 px, anchor eternal): Hush 0.72/0.030/85 · Rift 0.84/0.026/80 ·
Vault 0.90/0.018/85 · Ember 0.80/0.034/78 · Hearth 0.68/0.045/75. All ≥ 6.7:1 local.
Secondary 22 px: day 0.70/0.018/85 · ramps 0.64/0.020/80 · Hush/Hearth 0.58/0.022/78 (floor 0.58).
Droplets: 0.80/0.020/80 constant. Staleness dot: 0.45/0.014/80.
Slot: fill 0.17/0.030/70 · text 0.74/0.085/70; urgent timer text 0.79/0.110/70 (C cap 0.112 —
sRGB edge), fill 0.30/0.085/70. Urgency = +C+L within H70, never a hue move.

## 5. Night (22:00–00:00, total H70 tint on true black)

Clock 0.55/0.095/70 · countdown 0.62/0.100/70 · done-pulse peak 0.70/0.112/70 ·
slot text 0.48/0.080/70, fill 0.10/0.030/70 · secondary 0.42/0.060/70 ·
droplets 0.50/0.055/70 · dot 0.30/0.040/70.
Critic rule honored: amber needs **L ≥ 0.40 at C 0.10, or C ≤ 0.05 below** — else RGB565
quantization rotates it into forbidden red (H<45). Every dim amber here passes.

## 6. On-device verification protocol (critic's photo tests — gate before adoption)

1. Bottom-160px histogram: not a single spike at 0 (black-hole check).
2. Split-screen A/B of adjacent weather states, locked-exposure photo at 2.5 m / 55°
   off-axis: crops differ ≥ 5/255 mean G, and 5/5 naming trials.
3. 0° vs 55° photo pair: count surviving distinct band states (need 3).
4. Night ambers photographed and hue-sampled: H ≥ 45 after panel white-balance.
5. 120 fps slow-mo of clock edges during an 8 s crossfade: pixels transition once,
   monotonically — no oscillation (Bayer must be screen-anchored).
6. Night glow: 1 s exposure of dark kitchen — clock casts no readable patch on the wall;
   confirm night mode drops PWM duty, not just palette L. Saccade test for phantom array.

## 7. Taste notes (why it coheres)

One law, five moods — the face composes like a Sugimoto series, not five wallpapers.
The seam-L arc is the single story; the top never exceeds 0.15, so noon's bright line
feels earned. C > 0.05 exists only while the sun crosses the horizon and inside the
amber slot: saturation is spent twice a day and when something needs you. Two temperature
poles (slate/bone), rose-amber as the hinge that exists only during the crossing.
Emptiness is a state: absent band = clear, absent droplets = dry, empty slot = free.
