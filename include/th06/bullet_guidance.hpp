#pragma once
#include <algorithm>
#include <utility>

namespace th06::solver {
struct MovingHazard {
    float x, y, vx, vy, radius;
    float ax = 0, ay = 0;
    unsigned acceleration_updates = 0;
    std::pair<float, float> position(unsigned updates) const {
        const float px = x + vx * updates, py = y + vy * updates;
        if (!acceleration_updates)
            return {px, py};
        const float active = float(std::min(updates, acceleration_updates));
        // Original BulletManager accelerates before moving and stops applying
        // acceleration at exDuration; the accumulated velocity persists.
        const float weight = active * float(updates) - active * (active - 1.f) * .5f;
        return {px + ax * weight, py + ay * weight};
    }
};
} // namespace th06::solver
