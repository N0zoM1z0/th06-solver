#pragma once
#include "th06/shadow_search.hpp"
#include <vector>
namespace th06::shadow {
struct RecordedResult {
    Prediction prediction;
    unsigned forecasts = 0, checks = 0, updates = 0, nodes = 0;
    double capture_ms = 0, proposal_ms = 0, verification_ms = 0;
};
// Called only inside the bounded search process group, after the immutable
// lead. Recorded geometry is conditional guidance; a separate full native
// execution produces every returned input/digest certificate.
RecordedResult recorded_search(solver::Session &game, const std::vector<std::uint16_t> &nominal,
                               unsigned budget_ms, unsigned beam = recorded_beam_width,
                               std::uint32_t *progress = nullptr);
} // namespace th06::shadow
