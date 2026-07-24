# Reviewer Memory Index

- [Flicker / WiFi-NVS gotcha](gotcha-flicker-wifi-nvs.md) — "no flicker at rest" tests must run WiFi-on and name the evidence + build-flag mitigations
- [Carousel gesture rules gotcha](gotcha-carousel-gesture-rules.md) — check swipe tasks against SPEC §4 spring/interruptible/immediate-delta and §5.12 clock-only night state
- [HA fan percentage mapping gotcha](gotcha-ha-fan-percentage-mapping.md) — state/air fan must map HA % to 0..3; the /33 ceil trap emits 4 at 100%
- [One-gesture-silence clickable child gotcha](gotcha-one-gesture-silence-clickable-child.md) — §4 tile CLICKED handler is swallowed by clickable overlay children (timer card) lacking EVENT_BUBBLE
