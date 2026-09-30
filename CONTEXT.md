# Kachel

A wall tile and the smart-home setup around it in one shared flat. This glossary fixes the words used for the home's systems, so decisions about devices, logic and voice don't drift.

## Language

### Roles

**Brain**:
The one system that holds all home logic: scenes, schedules, automations. Home Assistant is the Brain.
_Avoid_: hub, Zentrale, controller (as a role name)

**Voice channel**:
Alexa. It may switch any device the Brain exposes, but holds no logic of its own.
_Avoid_: Alexa routine (as a place for logic), assistant

**Radio bridge**:
A device that carries one radio protocol into the LAN and holds no logic. The Hue Bridge is the Zigbee radio bridge; it also carries IKEA KAJPLATS bulbs running in their Zigbee mode.
_Avoid_: hub, Brücke, Zentrale

**Thread radio**:
The Brain's own USB radio. It forms the flat's only Thread network, and every Thread device joins it.
_Avoid_: hub, dongle, border router (as a device name), Thread-Router

**Failsafe**:
The manual way to switch a light when the Brain is down: the button on a smart plug or the lamp's own switch. No app, no network.
_Avoid_: backup, fallback, emergency mode

### Home

**Room**:
One physical room of the flat: Flur, Wohnzimmer, Küche, Büro or Schlafzimmer. Every device belongs to exactly one Room, and the Brain and the Hue Bridge use the same Room names. Küche, Wohnzimmer and Büro are open to each other but are still separate Rooms.
_Avoid_: area, Bereich, zone, furniture names as rooms (e.g. "Josch's Schreibtisch")

**Resident**:
One of the two people who live in the flat. Both count equally; nothing in the home works for only one of them.
_Avoid_: user, owner

**Presence**:
Someone is in a Room right now, as reported by that Room's sensor. It says nothing about who.
_Avoid_: motion, occupancy, Arrival

**Dark**:
The sun is less than 3° above the horizon at the flat's location. It is computed by the Brain, never measured by a sensor.
_Avoid_: night, evening, sunset

### Tile

**Scene**:
A named set of target states for lights and plugs, owned by the Brain. A Scene expresses a mood (e.g. gemütlich) or covers the whole flat (Alles an / Alles aus). A single Room is switched as a Room, never through a per-room Scene.
_Avoid_: preset, mode, room scene

**Scene slot**:
One of the four fixed positions (1–4) on the Kachel lights layer, each bound to one Scene.
_Avoid_: button, tile
