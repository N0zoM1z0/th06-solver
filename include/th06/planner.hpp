#pragma once
#include "th06/session.hpp"
#include <cstdint>
#include <vector>

namespace th06::solver {
struct PlannerOptions {
    unsigned horizon = 90, commit = 8, proposals = 4;
    int excluded_input = -1;
};
struct Decision {
    std::uint16_t input = 0;
    unsigned certified_frames = 0;
    std::uint64_t branches = 0, simulated_frames = 0;
    std::vector<std::uint64_t> expected_digests;
};
// Candidate generation is heuristic. A successful child executes the original
// game and proves only its consecutive prefix from this exact process state.
Decision choose(Session &game, const PlannerOptions &options);
} // namespace th06::solver
