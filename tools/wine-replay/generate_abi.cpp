#include "EnemyManager.hpp"
#include "Player.hpp"
#include "stdafx.h"
#include <cstddef>
#include <fstream>
#include <stdexcept>

int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    std::ofstream out(argv[1]);
    if (!out)
        return 1;
    out << "#pragma once\nnamespace abi {\n";
#define FIELD(type, member)                                                                        \
    out << "inline constexpr unsigned " #type "_" #member " = " << offsetof(th06::type, member)    \
        << ";\n"
    FIELD(GameManager, currentStage);
    FIELD(GameManager, difficulty);
    FIELD(GameManager, character);
    FIELD(GameManager, shotType);
    FIELD(GameManager, isInReplay);
    FIELD(GameManager, isInPracticeMode);
    FIELD(GameManager, demoMode);
    FIELD(GameManager, replayFile);
    FIELD(GameManager, currentPower);
    FIELD(GameManager, randomSeed);
    FIELD(GameManager, livesRemaining);
    FIELD(GameManager, bombsRemaining);
    FIELD(GameManager, isInMenu);
    FIELD(GameManager, deaths);
    FIELD(GameManager, bombsUsed);
    FIELD(GameManager, score);
    FIELD(GameManager, rank);
    FIELD(Supervisor, curState);
    FIELD(Supervisor, wantedState);
    FIELD(Supervisor, framerateMultiplier);
    FIELD(Supervisor, effectiveFramerateMultiplier);
    FIELD(Player, positionCenter);
    FIELD(Player, playerState);
    FIELD(EnemyManager, timelineTime);
    out << "inline constexpr unsigned timer_current = " << offsetof(th06::ZunTimer, current)
        << ";\n";
    out << "inline constexpr int player_dead = " << int(th06::PLAYER_STATE_DEAD) << ";\n}\n";
    return !out;
}
