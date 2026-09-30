---
status: accepted (radio deferred to backlog, 2026-09-29)
---

# The Brain owns the flat's only Thread network

IKEA's new devices (KAJPLATS bulbs, BILRESA remotes, GRILLPLATS plugs, MYGGSPRAY sensors) are Matter over Thread, and the flat has no Thread border router. We decided that once Thread comes in, Home Assistant runs the one Thread network itself, through its own USB radio, and every Thread device joins that network. Home Assistant is the Brain (see CONTEXT.md), so the network its devices live on must not belong to another vendor's hub. Moving later would mean re-commissioning every Thread device, which is why this is recorded.

Until then, KAJPLATS bulbs run in their hidden Zigbee mode on the Hue Bridge. The first one ("Frau Marschik") has switched on and off reliably for two weeks, so the reported on/off failures of that mode don't hold for our bulbs. The radio stays in the backlog until a device really needs Thread; the kitchen plug is the likely trigger.

## Considered Options

- **KAJPLATS on the Hue Bridge via the Zigbee/Touchlink hack** (Hue Essentials Premium). Accepted as the interim path, not as the Thread network. Costs: the BILRESA remote stops working, the bulbs get no firmware updates, and new IKEA units may not keep the Zigbee mode.
- **IKEA DIRIGERA hub.** It would be a second Brain-like hub with its own app and a second Thread network, and sharing single Thread devices to HA often fails. Rejected.
- **An Echo with a built-in Thread border router** (Echo 4th gen, Echo Dot Max, Echo Hub). Amazon's Thread network is closed to HA, so devices would belong to Alexa and reach HA only through Matter sharing, which is unproven for this setup. Rejected. The Echo Dot 3rd gen in the flat is not a border router anyway.
- **Which radio.** An ESP32-C6 flashed with Espressif's OpenThread RCP (about €5–10) is tried first, because the ESP32 toolchain is already in this repo and the network will hold only a few devices. If it is not stable for two weeks or does not reach the kitchen, the official Connect ZBT-2 (about €60) replaces it. A Sonoff Dongle Plus MG24 was rejected: it needs the same manual flashing as the C6 for several times the price.

## Consequences

- While everything is on Hue or Tapo, Alexa gets its devices through their own skills. Once Brain-only Thread devices exist, they are exposed to Alexa through `emulated_hue` with an explicit entity list, never `expose_by_default`.
- The radio runs Thread only. Neither the ESP32-C6 nor the ZBT-2 can serve Zigbee at the same time, so the Hue Bridge stays for as long as any Zigbee device remains.
- A self-flashed radio can stop working after a Home Assistant update until its firmware is re-flashed to match.
- Pairing a Thread device needs the HA Companion app on an iPhone that holds this network's Thread credentials.
