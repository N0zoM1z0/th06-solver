#include "BulletManager.hpp"
#include "EnemyManager.hpp"
#include "stdafx.h"
#include "test_support.hpp"
#include "th06/platform.hpp"
#include "th06/resources.hpp"
#include "th06/session.hpp"
#include <iostream>

using namespace th06;
int main(int argc, char **argv) {
    try {
        check(argc == 2 || argc == 3, "test needs DAT directory and optional profile");
        solver::Resources resources(argv[1]);
        solver::set_resources(&resources);
        solver::StartConfig start;
        start.seed = 29768;
        start.story = true;
        const bool extra = argc == 3 && std::string(argv[2]) == "extra";
        if (extra) {
            start.story = false;
            start.stage = 7;
            start.difficulty = 4;
            start.power = 128;
        }
        solver::Session game(start);
        if (extra) {
            check(g_GameManager.livesEarned == 4 && g_GameManager.livesRemaining == 2 &&
                      g_GameManager.bombsRemaining == 3 && g_GameManager.rank == 16 &&
                      g_GameManager.minRank == 14 && g_GameManager.maxRank == 18,
                  "Extra bootstrap must preserve source rank/extend rules");
            std::cout << "Extra source bootstrap contracts passed\n";
            return 0;
        }
        for (unsigned i = 0; i < 30; ++i)
            game.step(261);
        g_GameManager.currentPower = 17;
        g_GameManager.livesRemaining = 1;
        g_GameManager.bombsRemaining = 2;
        g_GameManager.rank = 20;
        g_GameManager.score = g_GameManager.guiScore = 123450;
        g_GameManager.grazeInStage = 9;
        g_GameManager.pointItemsCollectedInStage = 7;
        g_BulletManager.bullets[0].state = BULLET_STATE_FIRED;
        g_BulletManager.bullets[0].pos = D3DXVECTOR3(-9876, -9876, 0);
        Rng expected;
        expected.Initialize(g_Rng.seed);
        g_Rng.generationCount = 1000000;
        // This is a lifecycle regression, not evidence that the skipped stage
        // was cleared. Full story acceptance still requires natural MSG endings.
        g_Supervisor.curState = SUPERVISOR_STATE_NEXT_STAGE;
        const auto pending_seed = g_Rng.seed;
        rejects([&] { game.step(2); }, "unsupported input accepted at handoff");
        check(g_GameManager.currentStage == 1 && g_Rng.seed == pending_seed &&
                  g_Supervisor.curState == SUPERVISOR_STATE_NEXT_STAGE,
              "rejected input partially advanced the story");
        const auto state = game.step(261);
        check(state.stage == 2 && state.frame == 31 && !state.complete, "story handoff boundary");
        check(g_GameManager.currentPower == 17 && g_GameManager.livesRemaining == 1 &&
                  g_GameManager.bombsRemaining == 2 && g_GameManager.rank == 20,
              "carried player resources/rank reset");
        check(g_GameManager.score == 123450 && g_GameManager.grazeInStage == 0 &&
                  g_GameManager.pointItemsCollectedInStage == 0,
              "score/stage counters handoff");
        check(g_Rng.generationCount < 1000, "source RNG generation counter not reset");
        for (unsigned i = 0; i < g_Rng.generationCount; ++i)
            expected.GetRandomU16();
        check(expected.seed == g_Rng.seed, "gameplay RNG reseeded or skipped at handoff");
        for (const auto &bullet : g_BulletManager.bullets)
            check(bullet.pos.x != -9876 || bullet.state == BULLET_STATE_INACTIVE,
                  "old bullet pool survived source reset");
        check(state.deaths == 0 && state.bombs == 0, "handoff caused a miss/bomb");
        std::cout << "story carry/reset/RNG lifecycle contracts passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
