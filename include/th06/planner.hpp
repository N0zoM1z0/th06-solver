#pragma once
#include "th06/session.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace th06::solver {
struct PlannerOptions {
    unsigned horizon = 90, commit = 8, proposals = 4;
    int excluded_input = -1;
    bool reactive = false;
    bool profile = false;
    bool diverse_first = false;
    bool laser_aware = false;
    bool laser_orbits = false;
    bool power_aware = false;
    // Stop further ranking only after a complete native future exists. The
    // caller still owns the hard deadline, verification and descendant cleanup.
    std::chrono::steady_clock::time_point ranking_deadline =
        std::chrono::steady_clock::time_point::max();
    // Retain a literal native future prefix only after a full horizon or terminal.
    // Zero preserves the commit-sized action; nonzero requires commit..horizon.
    unsigned reuse = 0;
    // Skip hashes after the first candidate; independently replay a later
    // winner to produce its full digest certificate. Defaults preserve offline
    // cost/selection behavior. Verification work is included in the counters.
    bool defer_later_digests = false;
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
    std::uint64_t orbital_branches = 0;
    bool orbital_selected = false;
    bool ranking_yielded = false;
    unsigned lookahead_frames = 0, unique_inputs = 0;
    bool future_complete = false;
    std::vector<std::uint64_t> expected_digests;
    std::vector<std::uint16_t> expected_inputs;
    PlannerTimings timings;
};
// Candidate generation is heuristic. A successful child executes the original
// game and proves only its consecutive prefix from this exact process state.
// Optional notification occurs once, after the first full native future has
// its literal digest certificate. Further ranking can still select a better one.
Decision choose(Session &game, const PlannerOptions &options,
                const std::function<void(const Decision &)> &first_certified = {});
} // namespace th06::solver
