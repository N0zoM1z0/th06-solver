#include "th06/laser_guidance.hpp"
#include <iostream>
#include <stdexcept>
using namespace th06::solver;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
LaserForecast beam() {
    LaserForecast l;
    l.present = true;
    l.x = 192;
    l.y = 80;
    l.start = 32;
    l.end = 420;
    l.length = 388;
    l.width = 24;
    l.state = 1;
    l.duration = 100;
    l.despawn = 40;
    l.hitbox_end = 20;
    return l;
}
int main() {
    try {
        auto l = beam();
        l.angle = 1.57079632679f;
        const auto vertical = advance_laser(l);
        require(laser_proposal_cost(vertical, 192, 380) > 30 &&
                    laser_proposal_cost(vertical, 352, 380) == 0,
                "downward source ray penalizes a player on its length, not the opposite side");
        l = beam();
        l.angular_speed = 1.57079632679f;
        require(laser_proposal_cost(advance_laser(l), 192, 380) > 30,
                "observed rotation advances before collision geometry");
        l = beam();
        l.state = 0;
        l.activation = l.hitbox_start = 60;
        for (unsigned i = 0; i < 60; ++i)
            require(!advance_laser(l).lethal, "warning before native hitbox start stays harmless");
        const auto transition = advance_laser(l);
        require(transition.lethal && transition.half_width == 6,
                "source warning fallthrough retains the thin-length bug");
        require(advance_laser(l).half_width == 194,
                "next active update restores the source's full length");
        l = beam();
        l.timer = l.duration;
        const auto last_active = advance_laser(l);
        require(last_active.lethal && last_active.half_width == 194 && l.state == 2,
                "last active collision is retained through despawn fallthrough");
        require(advance_laser(l).half_width < 6, "despawn shrink changes length, not thickness");
        l = beam();
        l.timer = l.duration;
        l.despawn = 0;
        require(advance_laser(l).lethal && !l.present && !advance_laser(l).lethal,
                "immediate removal retains this collision and removes the following one");
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
