#pragma once

namespace th06::solver {
// Conditional collision geometry only. It proposes paths and never replaces
// native collision predicates or serializes a mutable game world.
constexpr unsigned geometry_boxes = 2048, geometry_lasers = 512;
struct GeometryBox {
    float x, y, width, height;
};
struct GeometryLaser {
    float x, y, width, height, origin_x, origin_y, angle;
};
struct GeometryFrame {
    unsigned box_count, laser_count;
    bool overflow;
    bool movement, vulnerable;
    float multiplier_x, multiplier_y, frame_multiplier;
    GeometryBox boxes[geometry_boxes];
    GeometryLaser lasers[geometry_lasers];
};
extern GeometryFrame *geometry_frame;
void record_geometry_box(float x, float y, float width, float height);
void record_geometry_laser(float x, float y, float width, float height, float origin_x,
                           float origin_y, float angle);
} // namespace th06::solver
