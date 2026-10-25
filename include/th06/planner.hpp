#pragma once
#include "th06/session.hpp"
#include <cstdint>
#include <vector>

namespace th06::solver {
struct PlannerOptions {
    unsigned horizon = 90, commit = 8, proposals = 4;
    int excluded_input = -1;
    bool reactive = false;
    bool profile = false;
    bool diverse_first = false;
    // Retain a literal native future prefix only after a full horizon or terminal.
    // Zero preserves the commit-sized action; nonzero requires commit..horizon.
    unsigned reuse = 0;
};
struct PlannerTimings {
    double ranking = 0, rollout_wall = 0, native_step = 0, guidance = 0, scoring = 0, digest = 0;
    PlannerTimings &operator+=(const PlannerTimings &other) {
        ranking += other.ranking;
        rollout_wall += other.rollout_wall;
        native_step += other.native_step;
        guidance += other.guidance;
        scoring += other.scoring;
        digest += other.digest;
        return *this;
    }
};
struct Decision {
    // Initial input for diagnostics. Execute expected_inputs, since an extended
    // certificate can stop moving or turn after this first action.
    std::uint16_t input = 0;
    unsigned certified_frames = 0;
    std::uint64_t branches = 0, simulated_frames = 0;
    std::uint64_t reactive_branches = 0;
    unsigned lookahead_frames = 0, unique_inputs = 0;
    std::vector<std::uint64_t> expected_digests;
    std::vector<std::uint16_t> expected_inputs;
    PlannerTimings timings;
};
// Candidate generation is heuristic. A successful child executes the original
// game and proves only its consecutive prefix from this exact process state.
Decision choose(Session &game, const PlannerOptions &options);
} // namespace th06::solver
