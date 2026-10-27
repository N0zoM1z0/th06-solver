#include "stdafx.h"
#include "th06/resources.hpp"
#include "th06/session.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace th06::solver;
namespace th06 {
extern ChainElem g_BulletManagerCalcChainElem;
}
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 2)
            return 2;
        Resources resources(argv[1]);
        set_resources(&resources);
        StartConfig config;
        config.stage = 7;
        config.difficulty = 4;
        config.power = 128;
        Session game(config);
        for (unsigned i = 0; i < 4; ++i) {
            th06::EnemyShooter shooter;
            shooter.position = {float(80 + i * 60), 100, 0};
            shooter.count1 = shooter.count2 = 1;
            shooter.speed1 = shooter.speed2 = 1.2f;
            shooter.flags = th06::EX_ACCELERATION;
            shooter.exFloats[0] = .12f;
            shooter.exFloats[1] = .4f;
            shooter.exInts[0] = int(i * 2 + 1);
            if (i == 3)
                shooter.flags |= th06::EX_SPEEDUP;
            require(th06::g_BulletManager.SpawnSingleBullet(&shooter, 0, 0, .2f) == 0,
                    "original source initializes the bullet fixture");
        }
        const auto before = game.observation_digest();
        const auto linear = game.observe();
        const auto observed = game.observe(false, false, true);
        require(game.observation_digest() == before && observed.hazards.size() == 4,
                "guidance snapshots do not change gameplay");
        for (unsigned i = 0; i < 4; ++i)
            require(!linear.hazards[i].acceleration_updates,
                    "default observation preserves the original guidance");
        require(!observed.hazards[3].acceleration_updates,
                "source speedup precedence excludes acceleration prediction");
        for (unsigned step = 1; step <= 12; ++step) {
            auto &chain = th06::g_BulletManagerCalcChainElem;
            require(chain.callback(chain.arg) == th06::CHAIN_CALLBACK_RESULT_CONTINUE,
                    "original BulletManager callback completes");
            const auto current = game.observe(false, false, true);
            for (unsigned i = 0; i < 3; ++i) {
                const auto [x, y] = observed.hazards[i].position(step);
                require(std::abs(current.hazards[i].x - x) < .002f &&
                            std::abs(current.hazards[i].y - y) < .002f,
                        "forecast agrees with original finite acceleration and expiry");
            }
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
