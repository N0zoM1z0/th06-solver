#include "th06/boss_guidance.hpp"
#include <iostream>
#include <stdexcept>
using namespace th06::solver;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main() {
    try {
        BossGoal goal({6000, 58, true, true});
        require(goal.cost({3000, 58, true, true}) > 0, "unfinished phase retains a penalty");
        require(goal.cost({12000, 58, false, false}) == 0,
                "a cleared phase cannot incur replacement HP during a handoff");
        require(goal.cost({12000, 58, true, true}) == 0,
                "completed progress remains latched when an identifier is reused");
        BossGoal immediate({6000, 58, true, true});
        require(immediate.cost({12000, 59, true, true}) == 0,
                "direct transition to the next spell is progress, not healing");
        BossGoal survival({1000, 62, true, false});
        require(survival.cost({900, 62, true, false}) == 0,
                "an invulnerable phase cannot reward incidental HP changes");
        BossGoal empty({0, 58, true, true});
        require(empty.cost({12000, 59, true, true}) == 0,
                "a zero-HP origin must not divide by zero");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
