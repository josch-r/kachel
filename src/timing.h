// Kachel timing scale — the only source of durations (SPEC §5.8–9).
// One scale, reused everywhere. No ad-hoc millisecond constants in UI code.
#pragma once

#include <cstdint>

// interaction feedback: 120–250 ms (SPEC §5.8)
constexpr uint32_t KACHEL_T_FEEDBACK_MS = 150;
// state-change transitions: 300–800 ms, one transition then static (SPEC §5.8)
constexpr uint32_t KACHEL_T_TRANSITION_MS = 500;
// ambient shifts may take multi-second fades (SPEC §5.8)
constexpr uint32_t KACHEL_T_AMBIENT_MS = 3000;
