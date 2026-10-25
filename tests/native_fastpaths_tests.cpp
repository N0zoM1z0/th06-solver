#include "stdafx.h"
#include "test_support.hpp"
#include "th06/bomb_projectile_scope.hpp"
#include <iostream>

using namespace th06;
using solver::BombProjectileScope;
namespace {
void clear_regions(Player &player) {
    for (auto &region : player.bombProjectiles)
        region.size.x = 0;
}
void check_regions() {
    clear_regions(g_Player);
    g_Player.playerState = PLAYER_STATE_INVULNERABLE;
    g_Player.hitboxTopLeft = D3DXVECTOR3(99, 99, 0);
    g_Player.hitboxBottomRight = D3DXVECTOR3(101, 101, 0);
    D3DXVECTOR3 center(100, 100, 0), size(2, 2, 0);
    check(!BombProjectileScope::known_empty(&g_Player), "unscoped queries must use source scan");
    {
        BombProjectileScope outer(g_Player);
        check(BombProjectileScope::known_empty(&g_Player), "empty regions not recognized");
        check(g_Player.CheckCollision(&center, &size) == COLLISION_HIT,
              "empty regions must preserve player collision");
        Player other{};
        clear_regions(other);
        {
            BombProjectileScope nested(other);
            check(!BombProjectileScope::known_empty(&g_Player), "scope crossed player ownership");
            BombProjectileScope::invalidate(&g_Player);
            g_Player.bombProjectiles[0].pos.x = center.x;
            g_Player.bombProjectiles[0].pos.y = center.y;
            g_Player.bombProjectiles[0].size.x = size.x;
            g_Player.bombProjectiles[0].size.y = size.y;
        }
        check(!BombProjectileScope::known_empty(&g_Player), "nested writer left outer scope valid");
        check(g_Player.CheckCollision(&center, &size) == COLLISION_GRAZE,
              "bomb region must retain priority over player collision");
        g_Player.playerState = PLAYER_STATE_DEAD;
        check(g_Player.CheckGraze(&center, &size) == CANCELED_BY_BOMB,
              "bomb cancellation must precede dead-player graze rejection");
    }
    clear_regions(g_Player);
    {
        BombProjectileScope refreshed(g_Player);
        check(BombProjectileScope::known_empty(&g_Player), "new update did not refresh emptiness");
        check(g_Player.CheckGraze(&center, &size) == GRAZE_FALSE, "dead player grazed");
    }
    check(!BombProjectileScope::known_empty(&g_Player), "scope survived update lifetime");
}
void check_colors() {
    AnmManager manager;
    AnmRawInstr halt{};
    halt.opcode = ANM_OPCODE_ANM_HALT;
    for (const auto initial : {0x80604020u, 0x10203040u}) {
        AnmVm vm{};
        vm.Initialize();
        vm.currentInstruction = &halt;
        vm.beginingOfScript = &halt;
        vm.alphaInterpInitial = initial;
        vm.alphaInterpFinal = initial == 0x80604020u ? 0x10604020u : 0x90a0b0c0u;
        vm.alphaInterpEndTime = 2;
        vm.alphaInterpTime.Initialize();
        check(manager.ExecuteScript(&vm) == 0, "alpha VM ended early");
        check(vm.color == (initial == 0x80604020u ? 0x48604020u : 0x50607080u),
              "alpha/RGB midpoint or identity component changed");
        check(manager.ExecuteScript(&vm) == 0 && vm.color == vm.alphaInterpFinal &&
                  vm.alphaInterpEndTime == 0 && vm.currentTimeInScript.current == 0,
              "alpha terminal value or halted script timer changed");
    }
}
} // namespace
int main() {
    try {
        g_Supervisor.framerateMultiplier = g_Supervisor.effectiveFramerateMultiplier = 1;
        check_regions();
        check_colors();
        std::cout << "native empty-region scope and ANM color contracts passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
