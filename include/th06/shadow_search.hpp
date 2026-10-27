#pragma once
#include "th06/session.hpp"
#include "th06/shadow_bridge.hpp"
namespace th06::shadow {
inline constexpr unsigned recorded_beam_width = 64;
struct Prediction {
    unsigned count = 0, branches = 0, updates = 0, nominal = 0, motion = 0;
    unsigned recorded = 0;
    unsigned orbital = 0;
    unsigned ranking_yielded = 0;
    std::array<std::uint16_t, capacity> inputs{};
    std::array<std::uint64_t, capacity> digests{};
};
static_assert(sizeof(Prediction) < 65536);
struct Search {
    Prediction prediction;
    double wall_ms = 0;
    bool timeout = false;
    bool deadline_checkpoint = false;
    unsigned motion_attempts = 0, motion_cursor = 0;
    unsigned recorded_forecasts = 0, recorded_checks = 0, last_phase = 0;
};
// The caller owns a single native world. Search forks it and enforces a wall
// budget on the whole owned process group, including proposal descendants.
Search search(solver::Session &game, const Plan &active, std::uint16_t previous, unsigned lead,
              unsigned horizon, unsigned budget_ms, bool reuse, bool pulse = false,
              unsigned motion_cursor = 0, bool recorded = false, unsigned proposals = 1,
              bool laser_aware = false, bool anytime = false, bool power_aware = false);
} // namespace th06::shadow
