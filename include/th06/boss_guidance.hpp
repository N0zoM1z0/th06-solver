#pragma once
#include <algorithm>

namespace th06::solver {
struct BossProgress {
    unsigned life = 0, spell = 0;
    bool active = false, damageable = false;
};
// Proposal scoring only. Completing a phase cannot incur the next phase's HP
// penalty, even if its spell identifier is reused later in this same future.
class BossGoal {
    BossProgress initial_;
    bool finished_ = false;

  public:
    explicit BossGoal(BossProgress initial) : initial_(initial) {}
    double cost(const BossProgress &current) {
        if (!initial_.active || !initial_.damageable || !initial_.life)
            return 0;
        finished_ = finished_ || !current.active || current.spell != initial_.spell;
        return finished_ ? 0 : 8. * std::min(1., double(current.life) / initial_.life);
    }
};
} // namespace th06::solver
