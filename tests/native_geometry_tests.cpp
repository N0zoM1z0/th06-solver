#include "stdafx.h"
#include "th06/native_geometry.hpp"
#include "th06/planner.hpp"
#include "th06/resources.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace th06::solver;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char **argv) {
    try {
        if (argc != 2)
            return 2;
        Resources resources(argv[1]);
        set_resources(&resources);
        StartConfig config;
        config.stage = 5;
        config.shot = 1;
        config.power = 128;
        config.seed = 7589;
        Session game(config);
        auto frame = std::make_unique<GeometryFrame>();
        const auto before = game.observation_digest();
        geometry_frame = frame.get();
        D3DXVECTOR3 far(1000.f, 1000.f, 0.f), size(4.f, 6.f, 0.f), origin(0.f, 0.f, 0.f);
        th06::g_Player.CheckGraze(&far, &size);
        require(frame->box_count == 1 && frame->boxes[0].width == 4.f &&
                    frame->boxes[0].height == 6.f,
                "distant ungrazed bullet recorded with lethal size, not graze expansion");
        th06::g_Player.CheckCollision(&far, &size);
        th06::g_Player.CalcLaserHitbox(&far, &size, &origin, .5f, false);
        require(frame->box_count == 2 && frame->laser_count == 1 && frame->lasers[0].angle == .5f,
                "source bullet and rotated laser calls captured");
        require(game.observation_digest() == before, "capture hooks have no gameplay effect");
        frame->box_count = geometry_boxes;
        th06::g_Player.CheckCollision(&far, &size);
        require(frame->overflow && frame->box_count == geometry_boxes,
                "capacity failure stays explicit");
        geometry_frame = nullptr;
        PlannerOptions options;
        options.horizon = 128;
        options.reuse = 128;
        options.proposals = 1;
        const auto prediction = choose(game, options);
        require(prediction.lookahead_frames == 128, "baseline future complete");
        for (unsigned i = 0; i < prediction.certified_frames; ++i) {
            frame->box_count = frame->laser_count = 0;
            frame->overflow = false;
            geometry_frame = frame.get();
            game.step(prediction.expected_inputs[i]);
            geometry_frame = nullptr;
            require(!frame->overflow && game.observation_digest() == prediction.expected_digests[i],
                    "capture preserves every original native future digest");
        }
        return 0;
    } catch (const std::exception &e) {
        geometry_frame = nullptr;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
