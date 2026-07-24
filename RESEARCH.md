# Kachel — Research Reference

Distilled from three research passes (2026-07-23): hardware, integrations, calm technology. Reference shelf for the coding agent — SPEC.md governs; this file explains and links.

---

## 1. Hardware: Guition ESP32-4848S040C_I

- **SoC:** ESP32-S3-WROOM-1 N16R8 — dual LX7 @ 240 MHz, 16 MB QIO flash, 8 MB octal PSRAM (OPI, 80 MHz).
- **Display:** 4.0" square IPS 480×480, **ST7701S**. Hybrid interface: 16-bit RGB parallel (DPI) pixel bus + 9-bit SPI init ("3-wire SPI + RGB"). PCLK 12–16 MHz in working configs (~22 MHz ceiling with octal PSRAM).
- **Touch:** GT911 capacitive, I2C 0x5D, SDA GPIO19 / SCL GPIO45. INT/RST not wired → polling only.
- **Backlight:** GPIO38; dims properly only at low PWM freq (~150–250 Hz); reference config 150 Hz, 1% min duty.
- **USB:** USB-C 5 V via **CH340 UART bridge** (native USB pins repurposed for touch I2C) — serial only, 115200–921600 baud.
- **Variants:** relays (GPIO40/2/1) XOR audio (NS4168 amp + speaker) via 0-ohm jumpers. `_I` suffix = IPS panel designation (moderate confidence). Nearly all other GPIOs consumed by RGB bus; practical breakout = relay pins, SD, UART.
- **SD slot:** SPI — CS 42, CLK 48, MOSI 47, MISO 41.

### Known quirks (design around these)
1. **Flash-write flicker:** flash and PSRAM share a bus; any NVS/LittleFS/OTA write stalls RGB refill → flicker/tear. Mitigations: `CONFIG_SPIRAM_XIP_FROM_PSRAM`, bounce buffers, `DATA_CACHE_LINE_64B`, avoid runtime writes. ([esp-bsp #570](https://github.com/espressif/esp-bsp/issues/570), [arduino-esp32 #12323](https://github.com/espressif/arduino-esp32/issues/12323), [Espressif LCD FAQ](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/lcd.html))
2. **Reset-after-flash:** RGB + octal PSRAM needs hard reset post-flash/OTA or white screen. ([espcontrol notes](https://jtenniswood.github.io/espcontrol/screens/4848s040))
3. Touch rotation must match display rotation or coordinates offset.
4. WiFi current spikes can brown out weak USB ports (fine on wall supply).
5. Steady-state display + WiFi coexist fine; flash writes are the flicker trigger.

### Toolchain verdict
- **Chosen: PlatformIO + Arduino + LVGL 9** via [rzeldent/esp32-smartdisplay](https://github.com/rzeldent/esp32-smartdisplay) (board defs for 4848S040). Plain C/C++, huge LVGL training corpus, deterministic builds — best agent-assisted path. Minimal example: [sand1812/ESP32-4848S040](https://github.com/sand1812/ESP32-4848S040).
- Rejected: **ESPHome** (2026.4/2026.5 LVGL+PSRAM+st7701s regressions — [#15917](https://github.com/esphome/esphome/issues/15917), [#16328](https://github.com/esphome/esphome/issues/16328); YAML cages generative drawing; swipe/button conflicts [#6777](https://github.com/esphome/issues/issues/6777)). **openHASP** (JSONL pages too rigid). **Tasmota** (weak UI). **MicroPython+LVGL** (GT911 init failures on this panel). **Web tech: impossible — no browser on device.**
- GUI designer if ever wanted: [EEZ Studio](https://github.com/eez-open/studio) (SquareLine lost the LVGL partnership Feb 2024).

### Reference projects
- [kroon040/HA-Panel-GUITION](https://github.com/kroon040/HA-Panel-GUITION) — polished HA wall panel, de-facto reference.
- [EspControl](https://jtenniswood.github.io/espcontrol/screens/4848s040) — packaged dashboard firmware + 3D-printable stand (MakerWorld).
- [davidegat/ESP32-4848S040-Fun](https://github.com/davidegat/ESP32-4848S040-Fun) — photo frame, RSS, games, backlight demos.
- [ESPHome device page](https://devices.esphome.io/devices/guition-esp32-s3-4848s040/) · [HA community mega-thread](https://community.home-assistant.io/t/guition-4-480x480-esp32-s3-4848s040-smart-display-with-lvgl/729271) · [Teardown](https://michiel.vanderwulp.be/domotica/Modules/SmartDisplay-ESP32-S3-4.0inch/)

---

## 2. Integrations

### Bulbs (~8, Alexa-paired, brand TBD)
- **No API to command Alexa cloud** — dead-end direction. (Smart Home Skill API works the opposite way.)
- **With HA (chosen path):** native brand integrations (local where possible) + [Alexa Media Player](https://github.com/alandtse/alexa_media_player) (HACS — also exposes **timers/alarms**, routines, TTS) + built-in [Alexa Devices integration](https://www.home-assistant.io/integrations/alexa_devices/).
- Local control by brand if identified: WiZ = trivial (UDP 38899, [pywizlight](https://github.com/sbidy/pywizlight)); Hue = bridge REST; Tapo/Kasa = [python-kasa](https://github.com/python-kasa/python-kasa); Tuya = tinytuya + local key, or reflash (often blocked on new firmware).
- Fallback without HA: [Apollon77/alexa-remote](https://github.com/Apollon77/alexa-remote) (unofficial, fragile, actively patched).

### Levoit purifier ("S300" = almost certainly Core 300S)
- No official API. [pyvesync](https://github.com/webdjoe/pyvesync) v3.4.x = reference; [HA VeSync integration](https://www.home-assistant.io/integrations/vesync/) (core, cloud-polling) exposes **PM2.5 + AQI, fan speed/mode, filter life %, display/child-lock**. Core 300S has the dust sensor (base Core 300 doesn't).
- Direct ESP32→VeSync: possible in principle, no client exists — not worth it.

### Weather
- **[Bright Sky](https://brightsky.dev/)** — free, keyless JSON over DWD open data. HA integration exists (DWD).

### iCloud calendar
- CalDAV `caldav.icloud.com` + app-specific password → [HA CalDAV integration](https://www.home-assistant.io/integrations/caldav/); or public-share ICS URL polling. Both easy.

### Bring!
- No official API; [bring-api](https://github.com/miaucl/bring-api) powers the core [HA Bring! integration](https://www.home-assistant.io/integrations/bring) (email/password auth). Easy via HA.

### KVB departures (roadmap)
- No official live API. [KoelnAPI/kvb-api](https://github.com/KoelnAPI/kvb-api) scrapes `kvb.koeln/qr/{station_id}`; VRS/EFA interface as alternative. Workable, unofficial.

### Architecture pattern (universal in the field)
**Dumb display + smart backend.** Multi-source dashboards on bare ESP32 = brittle (TLS/OAuth juggling). One HA instance replaces 4+ bespoke companion scripts. Display polls/subscribes; backend aggregates. Precedents: openHASP MQTT model, all Claude-monitor gadgets, [cs2mqtt](https://github.com/lupusbytes/cs2mqtt).

---

## 3. Calm Technology (design foundation)

### Canon
- **Weiser & Brown 1996, "The Coming Age of Calm Technology"** ([full text](https://calmtech.com/papers/coming-age-calm-technology)): calm tech "engages both the center and the periphery of our attention, and moves back and forth between the two." Encalming = empowering the periphery. Three mechanisms: attunement without overload; **control by recentering** (a direct look must reward with detail, then recede); locatedness. Canonical examples: inner office window, Dangling String (diagnostic on demand — glance answers one question instantly, then fades).
- **Amber Case, 8 principles** ([calmtech.com](https://calmtech.com), [Calm Tech Institute](https://www.calmtech.institute/calm-tech-principles)): smallest possible attention; inform and create calm; use the periphery; amplify best of tech + humanity (don't act human); communicate without speaking; **work even when it fails**; minimum tech needed; respect social norms. Calm Tech Certified™ dimensions: attention, periphery, durability, light, sound, materials (certified: reMarkable Paper Pro, Time Timer, Aura, Mudita).
- **Google Little Signals 2022** ([site](https://littlesignals.withgoogle.com/), open-sourced): six objects (Air/Button/Movement/Rhythm/Shadow/Tap). Meta-takeaways: every signal has an urgency-mapped intensity dial; every object has a one-gesture "shut up"; react to presence, not schedules; information arrives as *change of state*, not interruption.

### Screen-applied research
- **Matthews, Berkeley DIS 2006** ([paper](https://dl.acm.org/doi/10.1145/1142405.1142457), [PDF](https://digitalassets.lib.berkeley.edu/techreports/ucb/text/EECS-2006-113.pdf)): glanceable = low cognitive effort; **abstraction beats detail**; pre-attentive variables (color, shape, position, size) read fastest; **fixed spatial layout = learned glance targets**.
- Wear OS guidance: 1–2 s glances, one primary datum, hierarchy via weight/size/whitespace.

### Product precedents
- **mui Board** (Kyoto — closest precedent): wood surface, display **fades to nothing at rest** ("Yohaku" — design of emptiness); circadian brightness with gradual-never-binary transitions. ([mui on calm tech](https://muilab.com/en/journal/calmtechnology/), [Dezeen](https://www.dezeen.com/2023/01/16/mui-board-lab-smart-home-control-ces/))
- **Ambient Orb** (David Rose): one variable → one hue, zero reading, pre-attentive.
- **TRMNL / Tidbyt**: curated cadence over stream; low resolution/refresh as deliberate information limiter; sunset-linked dimming.
- **Nest Hub "Ambient EQ"**: light sensor drives brightness AND color temperature; dark room → dim clock or off; user-set threshold. (Kachel v1 approximates via schedule; sensor on roadmap.)

### Hard numbers absorbed into SPEC §5
Glance ≤2 s at 2–3 m → ≥120 px primary glyph; resting motion ≤6–10 cycles/min; transitions 300–800 ms; interaction feedback 120–250 ms (<400 ms Doherty; <100 ms touch acknowledgment); targets ≥80 px, ~4 choices/view (Hick, Cowan 4±1); novelty budget ~10%; one hue = one meaning.

---

## 4. Wiki grounding (Josch's own knowledge base — for context)

Spec process follows [[wiki/concepts/Spec-Driven Development]] (SAID lifecycle, 4D fluency, concentric loops), quality gates + AGENTS.md ratchet from [[wiki/concepts/Long-running Agents]], prompt craft from [[wiki/concepts/Prompt Engineering]]. UX numbers from [[wiki/concepts/Laws of UX (concept)]] cluster, gesture rules from [[wiki/concepts/Gesture Design Principles]], ambient-media register from [[wiki/concepts/Tangible AI Interaction]] (Ishii). Taste constraints: presence-not-person, structural trust, anti-slop, OKLCH tokens ([[wiki/meta/About Joschua]]).
