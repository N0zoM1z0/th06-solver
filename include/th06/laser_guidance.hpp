#pragma once
#include <algorithm>
#include <cmath>

namespace th06::solver {
// Proposal geometry only. Native BulletManager/Player still decide collisions.
// Lifecycle forecasting follows the source, including its thin-length bug;
// observed ECL motion is only a continuation hypothesis, not a certificate.
struct LaserForecast {
    float x = 0, y = 0, angle = 0, start = 0, end = 0, length = 0, width = 0, speed = 0;
    int timer = 0, state = 0, activation = 0, hitbox_start = 0, duration = 0;
    int despawn = 0, hitbox_end = 0;
    unsigned flags = 0;
    bool present = false;
    float dx = 0, dy = 0, angular_speed = 0;
};
struct LaserShape {
    float x = 0, y = 0, half_width = 0, half_height = 0;
    float origin_x = 0, origin_y = 0, cosine = 1, sine = 0;
    bool lethal = false;
};
inline LaserShape advance_laser(LaserForecast &l) {
    LaserShape shape;
    if (!l.present)
        return shape;
    l.x += l.dx;
    l.y += l.dy;
    l.angle += l.angular_speed;
    l.end += l.speed;
    l.start = std::max(0.f, std::max(l.start, l.end - l.length));
    float extent = l.end - l.start;
    shape.x = l.x + (l.start + l.end) * .5f;
    shape.y = l.y;
    shape.origin_x = l.x;
    shape.origin_y = l.y;
    shape.cosine = std::cos(l.angle);
    shape.sine = std::sin(l.angle);
    shape.half_height = l.width * .25f;
    if (l.state == 0) {
        if (!(l.flags & 1))
            extent = (l.timer > l.activation - std::min(l.activation, 30)
                          ? l.timer * l.width / std::max(1, l.activation)
                          : 1.2f) *
                     .5f;
        shape.lethal = l.timer >= l.hitbox_start;
        if (l.timer >= l.activation) {
            l.timer = 0;
            l.state = 1;
        }
    }
    if (l.state == 1) {
        shape.lethal = true;
        if (l.timer >= l.duration) {
            l.timer = 0;
            l.state = 2;
            if (!l.despawn)
                l.present = false;
        }
    }
    if (l.state == 2 && l.present) {
        const bool active = shape.lethal;
        if (!(l.flags & 1) && l.despawn > 0) {
            const float thin = (l.width - l.timer * l.width / l.despawn) * .5f;
            extent = active ? std::max(extent, thin) : thin;
        }
        shape.lethal = active || l.timer < l.hitbox_end;
        if (l.timer >= l.despawn)
            l.present = false;
    }
    if (l.state < 0 || l.state > 2)
        l.present = shape.lethal = false;
    shape.half_width = std::max(0.f, extent * .5f);
    if (l.start >= 640)
        l.present = false;
    ++l.timer;
    return shape;
}
inline double laser_proposal_cost(const LaserShape &l, float x, float y) {
    if (!l.lethal)
        return 0;
    const float rx = x - l.origin_x, ry = y - l.origin_y;
    const float px = l.origin_x + l.cosine * rx + l.sine * ry;
    const float py = l.origin_y - l.sine * rx + l.cosine * ry;
    const float dx = std::max(0.f, std::abs(px - l.x) - l.half_width - 1.5f);
    const float dy = std::max(0.f, std::abs(py - l.y) - l.half_height - 1.5f);
    const double distance = dx * dx + dy * dy;
    return distance < 14400 ? 2400.0 / (distance + 64) : 0;
}
} // namespace th06::solver
