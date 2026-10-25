#include "th06/native_geometry.hpp"
namespace th06::solver {
GeometryFrame *geometry_frame = nullptr;
void record_geometry_box(float x, float y, float width, float height) {
    auto &f = *geometry_frame;
    if (f.box_count >= geometry_boxes) {
        f.overflow = true;
        return;
    }
    f.boxes[f.box_count++] = {x, y, width, height};
}
void record_geometry_laser(float x, float y, float width, float height, float origin_x,
                           float origin_y, float angle) {
    auto &f = *geometry_frame;
    if (f.laser_count >= geometry_lasers) {
        f.overflow = true;
        return;
    }
    f.lasers[f.laser_count++] = {x, y, width, height, origin_x, origin_y, angle};
}
} // namespace th06::solver
