# Kachel — Agent Rules (ratchet file)

Every line here traces to a real failure or a hard constraint from RESEARCH.md §1.
Grow only via the ratchet rule (AGENT_KICKOFF.md): mistake costs a debugging round → line lands here.

## Board

Guition ESP32-4848S040C_I — ESP32-S3-WROOM-1 N16R8, 16 MB flash, 8 MB octal PSRAM.
Board def: `boards/esp32-4848S040CIY1.json`, vendored from
rzeldent/platformio-espressif32-sunton @ `0d9a9b1`. CIY1 vs CIY3 differ only in
relay defines (1 vs 3) — display identical. Actual jumper variant (speaker XOR
relay): unresolved until board-back photo (SPEC §11).

| Function | Pin |
|---|---|
| Backlight PWM | GPIO38 — ~150 Hz only, min 1 % duty (board JSON handles init) |
| Touch GT911 I2C | SDA 19, SCL 45, addr 0x5D — polling only, INT/RST not wired |
| SD (SPI) | CS 42, CLK 48, MOSI 47, MISO 41 |
| Relays (if relay variant) | GPIO40 (CIY3 adds 2, 1) |
| Serial | CH340 USB-C bridge, 115200 — no native USB-CDC |

Nearly all other GPIOs consumed by the 16-bit RGB bus. ST7701S: RGB parallel + 9-bit SPI init, PCLK 12–16 MHz.

## Hard rules

1. **No runtime flash writes while display active.** Flash and PSRAM share a bus; any NVS/LittleFS/OTA write stalls RGB refill → visible flicker (RESEARCH §1 quirk 1). WiFi stack: `WiFi.persistent(false)` + `esp_wifi_set_storage(WIFI_STORAGE_RAM)` mandatory. Escalation path if flicker still observed: `CONFIG_SPIRAM_XIP_FROM_PSRAM`/bounce buffers — needs custom core build, not stock Arduino; treat as last resort.
2. **Hard reset after every flash/OTA.** RGB + octal PSRAM: without physical reset → white screen. Flashing over CH340 does not auto-reset reliably. Josch presses reset/replugs.
3. **Touch rotation must match display rotation** or coordinates offset (quirk 3).
4. **GT911 self-reports a bogus 1085×600 matrix — never trust it.** Raw coordinates are native 480×480; esp32-smartdisplay's auto "coordinate adjustment" compresses x to ≤212 / y to ≤384. `fix_gt911_scaling()` in main.cpp detaches `th->config.process_coordinates` after `smartdisplay_init()`. Symptom if it regresses: right/bottom screen half untouchable. (Cost a debugging round 2026-07-24.)
5. Colors only via `src/palette.h` tokens (OKLCH-derived). No inline color literals anywhere else.
6. Animation/transition durations only via `src/timing.h`. No ad-hoc millisecond constants in UI code.
7. **Read the full compiler warning output, never just the tail.** A missing `return` in a `bool` function compiled with `-Wall` only (warning scrolled past unseen) → UB at `-Ofast` → `IllegalInstruction` crash loop on device. `-Werror=return-type` now in platformio.ini so it can't recur silently. (Cost a debugging round 2026-07-25.)
8. **lv_font_conv glyph bitmaps are continuous 4bpp bitstreams, and its hex is minimal-width.** Rows are NOT byte-aligned; `0x0` single-digit bytes break `{2}`-digit regexes. Patching a glyph with byte-aligned rows renders garbage. Decode/encode as a bitstream (`tools/patch_doto_colon.py` is the reference); always verify by decoding the patched bitmap before flashing. (Cost a debugging round + one bad flash 2026-07-25.)

## Versions (exact pins — update only deliberately)

- platform: `espressif32@7.0.1`
- esp32-smartdisplay: git `20c728e` (2.1.1-dev; registry 2.1.0 predates the `DISPLAY_*` define rename and fails against boards repo @ `0d9a9b1` — lib and board JSON must move as a pair)
- `lvgl/lvgl@9.2.2` (LV_CONF_PATH → `include/lv_conf.h`, defined WITHOUT inner quotes — LVGL stringifies the macro itself)

## Build / flash

```sh
pio run                   # build
pio run -t upload         # flash over /dev/cu.usbserial-110 (then: hard reset!)
pio device monitor        # serial console, 115200
```
