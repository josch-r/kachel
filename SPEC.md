# Kachel — Spec v1

**Calm-tech ambient kitchen display for the shared flat. One 4" square tile that informs from the periphery and rewards touch with control.**

Status: build-ready draft (grill session 2026-07-23). Source of truth for the build. Changes to this file follow the change protocol in AGENT_KICKOFF.md.

---

## 1. Big idea

A Guition ESP32-4848S040C_I (4" 480x480 capacitive touch) sits on the kitchen desk of a shared flat. At rest it is a calm, mostly-dark generative surface encoding air quality, weather, and time in abstracted color and light — readable in a 1–2 s glance from across the room. Touch recenters it: swipe layers give both residents control of light scenes, the air purifier, and shared household info. It always returns itself to the periphery.

Weiser's one-line test governs every element: **it must inform from the periphery without demanding attention, or reward deliberate attention with control — anything that does neither gets cut.**

## 2. Users and context

- **Josch** — creative technologist, builds and maintains it.
- **Co-resident** — techy, full co-user: glances *and* controls. No feature may require Josch-specific knowledge to operate.
- Placement: kitchen desk, wall power (USB-C 5 V), WiFi. Semi-public surface — guests see it; nothing private on the resting screen (person-specific detail only after deliberate touch).
- Alexa remains the voice channel in the room. Kachel never competes with voice input; it complements it as the glanceable state surface.

## 3. Jobs (v1)

| # | Job | Layer |
|---|-----|-------|
| 1 | Ambient face: air quality (PM2.5), weather, time, running Alexa timer — abstracted, glanceable | Home (resting state) |
| 2 | Light control: 4 scene buttons (2×2 grid) | Lights layer |
| 3 | Air purifier: fan speed control, PM2.5 history, filter life | Air layer |
| 4 | Household: Bring! list summary + next shared iCloud calendar events | Household layer |

**Explicitly cut (decision log §10):** CS2 presence, Claude usage display. **Deferred to roadmap (§11):** one-tap timer presets, LD2410 presence sensor, KVB departures.

## 4. Interaction model

- **Horizontal swipe carousel:** Lights ← **Ambient face (home)** → Air → Household. Swipe = page-turn metaphor, spring physics, interruptible mid-gesture, deltas apply immediately.
- **Tap = recentering.** Tapping the resting face wakes full brightness / reveals detail. Any non-home layer times out back to the ambient face after **20 s idle** (configurable 10–30 s). The device always returns itself to the periphery.
- **Gesture trigger discipline:** lightweight actions (layer reveal, dim) trigger mid-gesture; destructive/stateful actions (scene change, fan change) trigger only on gesture completion (tap release inside target).
- **Abstract commands get widgets, not gestures** (no learned secret gestures — research: no consensus gestures exist for abstract commands).
- **One-gesture silence:** during any alerting state, a single tap anywhere silences it. No UI reading required.

### Touch targets
- Minimum tap target **80×80 px**; primary controls (scene buttons) ~**200×200 px** (2×2 grid on 480×480 = ideal Fitts).
- Invisible hit-area expansion beyond visible bounds on all controls.
- Max **4 primary choices per view** (Hick; Miller/Cowan ~4 chunks).

## 5. Design principles (hard rules)

### Calm rules
1. **Yohaku resting state:** mostly dark/empty. Emptiness signals "all is well." Resist filling the square.
2. **One primary datum per state**, readable in ≤2 s at 2–3 m → primary glyph/numeral ≥ **120 px** tall. Secondary data learnable by fixed position (spatial anchors never move).
3. **Abstract before display:** derived states ("air good", "rain later"), not raw feeds. Peripheral encoding via pre-attentive channels only: hue, position, fill-level, brightness.
4. **One hue = one meaning, forever.** Hue table in §6. Saturation is alert currency — resting palette stays desaturated/warm.
5. **Escalation ladder:** info = silent static change → notable = slow fade-in accent → needs-you-soon = gentle periodic pulse → urgent (timer done; rare) = one motion+sound event, then self-decay. Everything below urgent is ignorable indefinitely. Nothing nags.
6. **Fail calm:** on WiFi/backend loss show clock + last-known data with a subtle staleness mark. Never a full-screen error, never a visible reboot.

### Motion rules
7. Resting motion: none, or sub-perceptual breathing ≤ **6–10 cycles/min**.
8. State changes: one transition, **300–800 ms** ease (ambient shifts may take multi-second fades), then static. Interaction feedback **120–250 ms**. Never idle-loop, bounce, or spin.
9. One timing scale reused everywhere. Interruptible always.
10. Novelty budget ~10%: one signature beat (the ambient face's generative behavior). The daily control loop stays boring and instant.

### Light & night rules
11. Dark UI base, light content. Device reads as an object, not a screen.
12. **Schedule-based dimming (v1):** day full scale → from 22:00 gradual ramp down → **22:00–00:00 ultra-dim clock only** → **00:00–06:00 screen black**. Touch always wakes (full detail), auto-returns per §4. All times configurable. Ramps gradual, never steps. Backlight PWM at ~150 Hz (hardware constraint), min duty 1%.
13. Never blue-white light into a dim room; night rendering warm/amber. Never flash; no full-screen brightness jumps as feedback.

### Sound rules
14. Default silent. Sound only for opted-in urgent events (timer completion). One short soft low-leaning tone, once, volume follows day/night. Every sound has a visual equivalent. (Hardware note: speaker XOR relay jumper — verify board variant; if no speaker, urgent = visual pulse only.)

### Typography & color
15. Distinctive typeface for numerals (mono or grotesk — pick one in design phase; no default-stack fonts). Body text ≥ 18 px at arm's length.
16. All colors defined as **OKLCH tokens** in one palette file; firmware receives precomputed RGB565 ramps (see §7 banding note).

## 6. Ambient face (the signature surface)

Design-phase freedom within these constraints:

- **Primary datum: clock** (recommended — the #1 kitchen glance; open to revision in design phase).
- **Air quality → color temperature/field state** of the generative background (hue ramp: good = calm warm-neutral field → poor = distinct shifted hue; exact ramp fixed in design phase and then never changed).
- **Weather → texture/horizon behavior** of the field (Horizon-style OKLCH gradient derived from real sky/time-of-day is the natural direction).
- **Running Alexa timer → guest card:** slides in as a quiet countdown element, leaves on completion (urgent beat, §5.5). Guest cards are temporary visitors; the face owns the surface.
- **Next calendar event within 2 h → one fading guest line.** Nothing beyond 2 h appears on the resting face.
- RGB565 gradient banding is real: enable LVGL gradient dithering; precompute dithered OKLCH ramps.

## 7. Architecture

```
[Levoit Core 300S]──VeSync cloud──┐
[~8 Alexa bulbs]──(brand TBD)─────┤
[Bring!]──────────────────────────┼──► Home Assistant (Raspberry Pi, HAOS)
[iCloud calendar]──CalDAV─────────┤        │  Statestream / automations
[Open-Meteo (DWD ICON)]───────────┘        ▼
[Alexa timers]──Alexa Media Player──► MQTT broker (Mosquitto add-on)
                                           ▲│
                                 publish   ││  subscribe
                                           │▼
                              Kachel firmware (ESP32-S3, C++ / LVGL 9)
```

- **Backend: Home Assistant OS on Raspberry Pi** (model check pending — needs Pi 4/5, 2 GB+). All integrations live in HA; the display is dumb.
- **Transport: MQTT** (Mosquitto add-on). Device subscribes to state topics, publishes command topics; HA automations map commands to scenes/entities.
- **Firmware: PlatformIO + Arduino core + LVGL 9** via `esp32-smartdisplay` board defs. Not ESPHome (generative face needs free drawing; C++/LVGL is also the most agent-friendly toolchain).

### MQTT contract (draft — implementation may refine, spec updated when it does)
| Topic | Dir | Payload (JSON) | Cadence |
|---|---|---|---|
| `kachel/state/air` | →device | `{pm25, aqi_level, fan, mode, filter_pct}` | on change |
| `kachel/state/weather` | →device | `{temp, condition, precip_12h_mm, sunrise, sunset}` | 15 min |
| `kachel/state/calendar` | →device | `{next:[{title, start, cal}]}` (max 3) | 5 min |
| `kachel/state/bring` | →device | `{count, items:[top 5]}` | 5 min |
| `kachel/state/timer` | →device | `{label, ends_at}` or `{}` | on change |
| `kachel/cmd/scene` | device→ | `{id: 1..4}` | user action |
| `kachel/cmd/air` | device→ | `{fan: 0..3}` | user action |
| `kachel/sys/status` | device→ | `{fw, rssi, uptime}` | 60 s |

- Stale rule: any state topic silent > 3× its cadence ⇒ staleness mark on that datum (§5.6).
- Config (WiFi/MQTT creds): compile-time `secrets.h` for v1; provisioning portal = roadmap.

### Hardware constraints (from research — respect in implementation)
- ESP32-S3, 16 MB flash, 8 MB octal PSRAM. ST7701S over 16-bit RGB parallel + 9-bit SPI init; PCLK 12–16 MHz.
- **Flash writes stall the RGB refill → flicker.** Avoid runtime NVS/filesystem writes during active display; buffer writes to idle moments; consider `CONFIG_SPIRAM_XIP_FROM_PSRAM`, bounce buffers.
- GT911 touch on I2C (SDA 19, SCL 45), polling only (no INT). Touch transform must match display rotation.
- Backlight GPIO38, PWM ~150 Hz, min 1% duty.
- Hard reset needed after flash/OTA (white-screen symptom otherwise).
- Speaker XOR relays via 0-ohm jumpers — verify variant before promising sound.
- CH340 serial (no native USB-CDC).

## 8. Milestones

Each milestone ends with evidence (build log, on-device photo/video from Josch, or serial trace). No milestone is "done" on claim alone.

- **M0 — Backend up.** HAOS on Pi; Mosquitto; integrations connected: VeSync, Bring!, CalDAV (iCloud), Open-Meteo weather, bulbs (IKEA via Hue Bridge); Alexa Media Player deferred to pre-M3 (Amazon account block, §11). Evidence: HA dashboard shows live entities. *(Largely human task, agent-guided.)*
- **M1 — Shell.** Firmware boots, display + touch verified, 4-layer swipe carousel with placeholder content, brightness schedule, no flicker at rest. Evidence: video of swipe + dim cycle.
- **M2 — Nervous system.** MQTT connected; all state topics rendered raw (debug view); `cmd/scene` + `cmd/air` round-trip to HA works. Evidence: serial trace + HA log + video of scene trigger.
- **M3 — Ambient face.** Generative OKLCH surface with clock primary, air/weather encoding, timer + calendar guest cards, dithered gradients (no visible banding), escalation ladder. Evidence: photos at day/dusk/night + across-room glance test.
- **M4 — Control layers.** Lights (4 scenes, 2×2), Air (fan/filter/history), Household (Bring! + calendar). Auto-return to face. Evidence: video of full interaction loop, co-resident usability pass (she operates everything unprompted).
- **M5 — Calm polish.** Night behavior end-to-end, fail-calm (router-off test), one-gesture silence, timing-scale audit, 48 h soak (no crash, no flicker, no memory leak in `sys/status`). Evidence: soak log + checklist against §5.

## 9. Quality gates

- Plan approval before each milestone (conductor mode; Josch signs off).
- Compile + static checks green before flash; on-device verification before "done".
- Scope discipline: touch only what the milestone asks.
- Spec is source of truth; drift = broken loop (SAID). Change protocol in AGENT_KICKOFF.md.

## 10. Decision log (grill session, 2026-07-23)

| Decision | Choice |
|---|---|
| Placement | Kitchen desk, shared living space |
| Users | Both residents; both glance + control |
| Identity | Techy but calm — Calm Technology design language |
| Aesthetic anchor | Calm tech (Weiser/Case/Little Signals); mui Board as closest product precedent; Horizon-style OKLCH generative face |
| Jobs v1 | Ambient face; 4 light scenes; air purifier layer; household (Bring! + iCloud calendar) |
| Timer | Display running Alexa timers (voice stays input channel); one-tap presets → roadmap |
| Cut | CS2 presence ("co-resident doesn't need to know"), Claude usage |
| Backend | Home Assistant OS on Raspberry Pi + Mosquitto MQTT |
| Firmware | Custom C++, PlatformIO + LVGL 9 (esp32-smartdisplay); not ESPHome |
| Night/presence | Schedule-based v1 (dim 22:00, clock-only 22–24, black 00–06, touch wake); LD2410 radar → roadmap |
| Name | Kachel |
| Weather source (M0, 2026-07-23) | Open-Meteo core integration — replaces Bright Sky (HACS-only custom integration; Open-Meteo serves same DWD ICON model for the location, keyless, zero add-on dependency) |
| IKEA bulb path (M0, 2026-07-23) | Existing Hue Bridge — bulb already paired there; HA Hue integration, fully local. No Zigbee dongle needed (roadmap item void). Echo is a Dot (no built-in Zigbee) — Echo path was never viable |
| Alexa Media Player (M0, 2026-07-24) | Deferred to pre-M3 — Amazon risk engine blocks third-party logins on the account (verification loop, then SMS refusal; AMP #2853, account-level, not config). First dependent feature is the M3 timer guest card, so M0 closes without it |
| Board variant (M1, 2026-07-24) | Relay variant confirmed — GPIO40 click test audible + 3 relay connectors on case back, no speaker. §5.14 resolves for v1: urgent = one visual pulse, no sound. Relays unused by Kachel |
| M1 accepted (2026-07-24) | Shell complete: display, touch (GT911 matrix-lie fix), 4-layer carousel, WiFi/SNTP clock, §5.12 schedule, 10.5-min no-flicker soak with WiFi live. Reviewer verdict ACCEPT; deferred notes owned by M3 (120px clock glyph) and M5 (snap-timing audit) |
| Weather contract field (M2, 2026-07-24) | `precip_prob` → `precip_12h_mm` (sum of next 12 hourly precipitation values, mm) — no forecast source in this HA exposes precipitation probability; §6 "rain later" cue derives from mm equally |

## 11. Open items & roadmap

**Homework (not build blockers):**
- [x] Bulb path: IKEA bulb on existing Hue Bridge → HA Hue integration (2026-07-23)
- [x] Purifier model: Core 300S confirmed via sticker (2026-07-23) — has PM2.5 sensor
- [x] Raspberry Pi: Pi 4 B, 4 GB — HAOS 18.1 running (2026-07-23)
- [x] SD card: swapped to 32 GB Intenso 2026-07-24; add-ons unblocked
- [ ] AMP retry (pre-M3): days of cool-down first; region field must read amazon.de (console trace showed amazon.com marketplace ID — recheck on retry); try different browser (WebAuthn `getClientCapabilities` TypeError in proxy); last resort Amazon support to clear the sign-in flag. 2FA already configured |
- [ ] 4 scene names + moods — co-design with co-resident
- [x] Board jumper variant: relay (GPIO40 click test, 2026-07-24) — no speaker, §5.14 = visual pulse only

**Roadmap (v2+):** LD2410 presence sensor (sleep-until-someone's-there), one-tap timer presets, KVB departures (unofficial API), provisioning portal + OTA, enclosure/stand (3D print), ambient-light sensor for true Ambient EQ.
