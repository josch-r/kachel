// Kachel timing scale — the only source of durations (SPEC §5.8–9).
// One scale, reused everywhere. No ad-hoc millisecond constants in UI code.
// Values 350–2000 ms from the M3 face design pass (docs/DESIGN_FACE.md).
#pragma once

#include <cstdint>

// interaction feedback: 120–250 ms (SPEC §5.8)
constexpr uint32_t KACHEL_T_FEEDBACK_MS = 150;
// state-change transitions: 300–800 ms, one transition then static (SPEC §5.8)
constexpr uint32_t KACHEL_T_TRANSITION_MS = 500;
// guest card entrances/exits (§5.8 transition class)
constexpr uint32_t KACHEL_T_CARD_IN_MS = 600;
constexpr uint32_t KACHEL_T_CARD_OUT_MS = 400;
// info-class ambient arrival (calendar line)
constexpr uint32_t KACHEL_T_INFO_FADE_MS = 1500;
// the one urgent beat: rise + decay (§5.5)
constexpr uint32_t KACHEL_T_PULSE_RISE_MS = 350;
constexpr uint32_t KACHEL_T_PULSE_DECAY_MS = 1600;
// notable-class slow accent (timer T-60 color)
constexpr uint32_t KACHEL_T_NOTABLE_MS = 2000;
// ambient field shifts (weather/air crossfades) — multi-second (SPEC §5.8)
constexpr uint32_t KACHEL_T_AMBIENT_MS = 8000;
