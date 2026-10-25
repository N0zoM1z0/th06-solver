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
        check(argc == 2, "test needs DAT directory");
        solver::Resources resources(argv[1]);
        solver::set_resources(&resources);
        solver::StartConfig start;
        start.seed = 29768;
        start.story = true;
        solver::Session game(start);
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
