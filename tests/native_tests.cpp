#include "EclManager.hpp"
#include "stdafx.h"
#include "test_support.hpp"
#include "th06/platform.hpp"
#include <iostream>
using namespace th06;
int main() {
    try {
        Rng rng;
        rng.Initialize(0);
        check(rng.GetRandomU16() == 0xc374 && rng.GetRandomU16() == 0xbfc7 &&
                  rng.generationCount == 2,
              "retail RNG seed progression");
        rng.Initialize(0);
        check(rng.GetRandomU32() == 0xc374bfc7u && rng.generationCount == 2, "RNG draw ordering");
        g_Supervisor.framerateMultiplier = g_Supervisor.effectiveFramerateMultiplier = 1;
        ZunTimer timer;
        timer.Initialize();
        timer.Increment(3);
        timer.Decrement(1);
        check(timer.current == 2, "full-speed source timer");
        g_Player.playerState = PLAYER_STATE_INVULNERABLE;
        g_Player.hitboxTopLeft = D3DXVECTOR3(99, 99, 0);
        g_Player.hitboxBottomRight = D3DXVECTOR3(101, 101, 0);
        D3DXVECTOR3 size(2, 2, 0), edge(102, 100, 0), gap(102.001f, 100, 0);
        check(g_Player.CheckCollision(&edge, &size) == COLLISION_HIT,
              "touching boxes must collide");
        check(g_Player.CheckCollision(&gap, &size) == COLLISION_NONE,
              "separated boxes must not collide");
        Enemy enemy{};
        EclRawInstr ecl{};
        ecl.opCode = 32767;
        ecl.skipForDifficulty = 0xff;
        enemy.currentContext.currentInstr = &ecl;
        enemy.runInterrupt = -1;
        rejects([&] { g_EclManager.RunEcl(&enemy); }, "unknown ECL opcode was silently accepted");
        AnmManager manager;
        AnmVm vm{};
        AnmRawInstr anm{};
        anm.opcode = 255;
        auto *anm_ptr = &anm;
        rejects([&] { manager.SetAndExecuteScript(&vm, anm_ptr); },
                "unknown ANM opcode was silently accepted");
        std::cout << "native RNG, timers, layout and collision boundary contracts passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
