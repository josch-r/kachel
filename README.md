# Kachel

**A calm-tech ambient display for the kitchen. One 4″ square tile that informs from the periphery and rewards touch with control.**

Kachel is a 480×480 ESP32-S3 touch tile that sits on a kitchen desk. At rest it is a mostly-dark generative surface encoding air quality, weather, and time as abstracted color and light — readable in a 1–2 second glance from across the room. Touch recenters it: horizontal swipes reveal control layers for light scenes, the air purifier, and shared household info. It always returns itself to the periphery.

Every element passes Weiser's one-line test: *it must inform from the periphery without demanding attention, or reward deliberate attention with control — anything that does neither gets cut.*

Firmware is C++ / LVGL 9 on PlatformIO. All integrations live in Home Assistant; the device is deliberately dumb and speaks only MQTT.

---

## Status

| | |
|---|---|
| Firmware | `0.6.0-face2` |
| Milestones | M0–M4 accepted; M5 (calm polish, 48 h soak) in progress |
| Spec | [`SPEC.md`](SPEC.md) is the source of truth |

---

## Hardware

Guition **ESP32-4848S040C_I** — ESP32-S3-WROOM-1 N16R8, 16 MB flash, 8 MB octal PSRAM, 4″ 480×480 ST7701S panel, GT911 capacitive touch, USB-C (CH340 serial, no native USB-CDC).

Board definition is vendored in [`boards/esp32-4848S040CIY1.json`](boards/esp32-4848S040CIY1.json) from [rzeldent/platformio-espressif32-sunton](https://github.com/rzeldent/platformio-espressif32-sunton) @ `0d9a9b1`.

| Function | Pin |
|---|---|
| Backlight PWM | GPIO38 — ~150 Hz only, min 1 % duty |
| Touch GT911 (I²C) | SDA 19, SCL 45, addr `0x5D` — polling only, INT/RST not wired |
| SD (SPI) | CS 42, CLK 48, MOSI 47, MISO 41 |
| Relay (relay variant) | GPIO40 |
| Serial | CH340, 115200 |

Nearly every other GPIO is consumed by the 16-bit RGB bus.

**Hardware gotchas that cost debugging rounds** (full list in [`AGENTS.md`](AGENTS.md)):

- **No runtime flash writes while the display is active.** Flash and PSRAM share a bus; any NVS/LittleFS write stalls the RGB refill and produces visible flicker. `WiFi.persistent(false)` + `esp_wifi_set_storage(WIFI_STORAGE_RAM)` are mandatory.
- **Hard reset required after every flash.** RGB + octal PSRAM boots to a white screen otherwise, and CH340 does not auto-reset reliably.
- **GT911 self-reports a bogus 1085×600 matrix.** Raw coordinates are native 480×480; `fix_gt911_scaling()` in `src/main.cpp` detaches the library's coordinate adjustment. Symptom if it regresses: the right/bottom half of the screen becomes untouchable.

---

## Architecture

```
[Levoit Core 300S]──VeSync cloud──┐
[Smart bulbs]──────Hue Bridge─────┤
[Bring!]──────────────────────────┼──► Home Assistant (Raspberry Pi, HAOS)
[iCloud calendar]──CalDAV─────────┤        │  automations
[Open-Meteo (DWD ICON)]───────────┘        ▼
[Alexa timers]──Alexa Media Player──► MQTT broker (Mosquitto add-on)
                                           ▲│
                                 publish   ││  subscribe
                                           │▼
                              Kachel firmware (ESP32-S3, C++ / LVGL 9)
```

Home Assistant owns every integration and pushes derived state to retained MQTT topics. The firmware renders state and publishes commands — it never talks to a cloud API directly. LVGL 9 rather than ESPHome, because the generative face needs free drawing.

### MQTT contract

| Topic | Direction | Payload (JSON) | Cadence |
|---|---|---|---|
| `kachel/state/air` | → device | `{pm25, aqi_level, fan, mode, filter_pct}` | on change |
| `kachel/state/weather` | → device | `{temp, condition, precip_12h_mm, precip_start_h, sunrise, sunset}` | 15 min |
| `kachel/state/calendar` | → device | `{next:[{title, start, cal}]}` (max 3) | 5 min |
| `kachel/state/bring` | → device | `{count, items:[top 5]}` | 5 min |
| `kachel/state/timer` | → device | `{label, ends_at}` or `{}` | on change |
| `kachel/cmd/scene` | device → | `{id: 1..4}` | user action |
| `kachel/cmd/air` | device → | `{fan: 0..3}` or `{mode: "auto"}` | user action |
| `kachel/sys/status` | device → | `{fw, rssi, uptime, heap, psram_free}` | 60 s |

All state topics are retained, so a rebooting device paints real data immediately. Any state topic silent for more than 3× its cadence earns a staleness mark on that datum rather than an error screen.

`fan` is `0..3` (off / low / mid / high); `mode: "auto"` maps to the purifier's auto preset. `precip_start_h` is the first forecast hour with > 0.1 mm precipitation, or `-1` for none.

---

## Interaction model

Horizontal swipe carousel, spring physics, interruptible mid-gesture:

```
Lights  ←  [ Ambient face — home ]  →  Air  →  Household
```

- **Tap = recentering.** Tapping the resting face wakes full brightness and reveals detail.
- **Auto-return.** Any non-home layer falls back to the ambient face after 20 s idle.
- **Gesture discipline.** Lightweight actions (layer reveal, dim) trigger mid-gesture; stateful ones (scene change, fan change) trigger only on gesture completion.
- **One-gesture silence.** During any alerting state, a single tap anywhere silences it — no UI reading required.
- Minimum tap target 80×80 px; scene buttons ~200×200 px (2×2 grid on 480×480). Max 4 primary choices per view.

### The ambient face

Three strata, specified in [`docs/DESIGN_FACE.md`](docs/DESIGN_FACE.md):

- **FIELD** — 5-phase daypart gradient, absolute weather band and precipitation veil, air-quality hue shift.
- **MARK** — Doto dot-matrix clock ≈100 px, anchored at true center, never moves.
- **SLOT** — one amber band for the next thing: upcoming calendar event, or a running timer that preempts it.

Colors are OKLCH tokens ([`docs/DESIGN_PALETTE_V2.md`](docs/DESIGN_PALETTE_V2.md)) compiled to RGB565 ramps and Bayer 8×8 dithered on a PSRAM canvas to kill banding. Weather is encoded absolutely — fixed sky-band lightness steps, fill-depth veil, 1–3 droplet hairlines, and a 22 px temperature/precipitation text line.

Night behavior: full scale by day → gradual ramp from 22:00 → ultra-dim clock only 22:00–00:00 → screen black 00:00–06:00. Touch always wakes. Never blue-white light into a dim room.

---

## Build and flash

Requires [PlatformIO](https://platformio.org/).

```sh
pio run                   # build
pio run -t upload         # flash, then HARD RESET the device (RTS pulse works)
pio device monitor        # serial console, 115200
```

Adjust `upload_port` / `monitor_port` in [`platformio.ini`](platformio.ini) — the CH340 device path drifts between replugs.

### Configuration

Copy the example secrets header and fill it in. `include/secrets.h` is gitignored and never committed.

```sh
cp include/secrets.h.example include/secrets.h
```

```c
#define WIFI_SSID     "your-ssid"
#define WIFI_PASSWORD "your-password"

#define MQTT_HOST     "your-broker-ip"
#define MQTT_PORT     1883
#define MQTT_USER     "kachel"
#define MQTT_PASSWORD "your-mqtt-password"
```

Everything else — brightness schedule, idle timeout, PM2.5 ring size, rush window — lives in [`src/config.h`](src/config.h) as compile-time constants. There is no provisioning portal in v1.

### Pinned versions

These move as a set; bumping one alone breaks the build.

- `espressif32@7.0.1`
- `esp32-smartdisplay` @ git `20c728e` — the registry 2.1.0 release predates the `DISPLAY_*` define rename and fails against the vendored board JSON
- `lvgl/lvgl@9.2.2` — `LV_CONF_PATH` points at `include/lv_conf.h`, defined *without* inner quotes
- `knolleary/PubSubClient@2.8`, `bblanchon/ArduinoJson@^7.4`

---

## Home Assistant setup

[`ha/setup_m2.py`](ha/setup_m2.py) installs the publish automations and scene stubs through the HA config API. Point `BASE` at your instance and put a long-lived access token in `.kachel/ha_token` (gitignored), then:

```sh
python3 ha/setup_m2.py
```

It creates one automation per state topic (air, weather, calendar, Bring!, timer), two command handlers (`cmd/scene`, `cmd/air`), and four scene slots. The entity IDs in that file are placeholders (`switch.lamp_a`, `calendar.shared`, `todo.shared_list`, …) — replace them with your own before running it.

Requires the Mosquitto add-on plus whichever integrations you actually use: VeSync, Bring!, CalDAV, Open-Meteo, Hue, Alexa Media Player.

---

## Tools

| Script | Purpose |
|---|---|
| [`tools/timer_demo.py`](tools/timer_demo.py) | Publish retained `state/timer` payloads to exercise the timer card (`start 5 "Pasta"`, `soon`, `clear`) |
| [`tools/soak_log.py`](tools/soak_log.py) | Append every `sys/status` heartbeat to JSONL for the 48 h soak; `--report` prints uptime resets, heap slope, and gaps |
| [`tools/patch_doto_colon.py`](tools/patch_doto_colon.py) | Rewrite the Doto colon glyph to a single dot — reference implementation for editing `lv_font_conv` bitstreams |

The first two read broker credentials from `include/secrets.h` and never print them. Both need `paho-mqtt`.

---

## Repository layout

```
src/                firmware — face, layers, carousel, MQTT, state model, palette
include/            lv_conf.h, secrets.h.example
boards/             vendored PlatformIO board definition
ha/                 Home Assistant automation + scene installer
tools/              MQTT test harnesses, font patching
docs/               design authority for the face, palette, and research council
SPEC.md             source of truth — jobs, rules, contract, decision log
AGENTS.md           hard constraints and gotchas (ratchet file)
AGENT_KICKOFF.md    change protocol and working agreement
RESEARCH.md         hardware and design research backing the spec
```

`src/font_*.c` are `lv_font_conv` bitmap dumps of Doto (clock, timer), Departure Mono (data text), and Inter Tight (prose lines). Their bitmaps are continuous 4bpp bitstreams — rows are **not** byte-aligned, so patching them naively renders garbage.

---

## Design rules

The hard rules live in [`SPEC.md`](SPEC.md) §5. The short version:

1. **Yohaku resting state** — mostly dark and empty. Emptiness signals "all is well."
2. **One primary datum per state**, readable in ≤ 2 s at 2–3 m.
3. **Abstract before display** — derived states ("air good", "rain later"), never raw feeds. Peripheral encoding uses pre-attentive channels only: hue, position, fill level, brightness.
4. **One hue = one meaning, forever.** Saturation is alert currency; the resting palette stays desaturated and warm. Red is forbidden.
5. **Escalation ladder** — silent static change → slow fade-in accent → gentle periodic pulse → one urgent motion event that self-decays. Nothing nags.
6. **Fail calm** — on WiFi or broker loss, show the clock and last-known data with a subtle staleness mark. Never a full-screen error, never a visible reboot.
7. **Motion**: one transition, 300–800 ms; interaction feedback 120–250 ms; then static. Never idle-loop, bounce, or spin. All durations come from `src/timing.h`; all colors from `src/palette.h`.

---

## Roadmap

LD2410 presence sensor (sleep until someone is there), one-tap timer presets, KVB departures, provisioning portal + OTA, 3D-printed enclosure, ambient-light sensor for true adaptive brightness.

---

## Notes

This is a personal build for one specific flat, published as a reference. The Home Assistant entity IDs, scene semantics, and schedule constants are not portable — treat `ha/setup_m2.py` and `src/config.h` as things you rewrite, not things you run.

The embedded fonts (Doto, Departure Mono, Inter Tight) are third-party works under their own licenses; the converted bitmaps in `src/font_*.c` inherit those terms.
