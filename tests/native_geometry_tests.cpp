#include "stdafx.h"
#include "th06/native_geometry.hpp"
#include "th06/planner.hpp"
#include "th06/resources.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace th06::solver;
namespace th06 {
extern ChainElem g_BulletManagerCalcChainElem;
}
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
        // Isolate the original bullet callback from the player's startup
        // invulnerability clear; otherwise the fixture is canceled each tick.
        th06::EnemyShooter shooter;
        shooter.position = {1000, 1000, 0};
        shooter.angle1 = .3f;
        shooter.startOffset = 32;
        shooter.endOffset = 420;
        shooter.startLength = 388;
        shooter.width = 24;
        shooter.startTime = shooter.hitboxStartTime = 2;
        shooter.duration = shooter.despawnDuration = 4;
        shooter.hitboxEndDelay = 2;
        auto *laser = th06::g_BulletManager.SpawnLaserPattern(&shooter);
        require(laser && laser->inUse, "source laser fixture initialized");
        auto forecast = game.observe(true).lasers.back();
        for (unsigned step = 0; step < 12; ++step) {
            const auto expected = advance_laser(forecast);
            frame->laser_count = frame->box_count = 0;
            frame->overflow = false;
            geometry_frame = frame.get();
            auto &chain = th06::g_BulletManagerCalcChainElem;
            require(chain.callback(chain.arg) == th06::CHAIN_CALLBACK_RESULT_CONTINUE,
                    "source bullet callback completed");
            geometry_frame = nullptr;
            unsigned calls = 0;
            float width = 0, height = 0;
            for (unsigned i = 0; i < frame->laser_count; ++i) {
                const auto &actual = frame->lasers[i];
                if (actual.origin_x != 1000 || actual.origin_y != 1000)
                    continue;
                ++calls;
                width = std::max(width, actual.width);
                height = std::max(height, actual.height);
            }
            if (expected.lethal != bool(calls) ||
                (calls && (std::abs(width - 2 * expected.half_width) >= .001f ||
                           std::abs(height - 2 * expected.half_height) >= .001f)))
                std::cerr << "laser step=" << step << " calls=" << calls
                          << " expected=" << expected.lethal << " size=" << width << ',' << height
                          << " forecast=" << 2 * expected.half_width << ','
                          << 2 * expected.half_height << " state=" << unsigned(laser->state)
                          << " timer=" << laser->timer.current << '\n';
            require(expected.lethal == bool(calls) &&
                        (!calls || (std::abs(width - 2 * expected.half_width) < .001f &&
                                    std::abs(height - 2 * expected.half_height) < .001f)),
                    "proposal lifecycle agrees with actual source laser collision calls");
        }
        PlannerOptions options;
        options.horizon = 128;
        options.reuse = 128;
        options.proposals = 1;
        const auto prediction = choose(game, options);
        require(prediction.lookahead_frames == 128, "baseline future complete");
        const auto tracking_digest = game.observation_digest();
        game.enable_laser_guidance();
        require(game.observation_digest() == tracking_digest,
                "enabling proposal history changes no gameplay");
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
