#include "stdafx.h"
#include "th06/planner.hpp"
#include "th06/resources.hpp"
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
using namespace th06::solver;
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
unsigned replay_power(Session &game, const Decision &decision) {
    const auto pid = fork();
    if (pid < 0)
        throw std::runtime_error("power replay fork");
    if (!pid) {
        try {
            for (unsigned i = 0; i < decision.certified_frames; ++i)
                if (game.step(decision.expected_inputs[i]).hit ||
                    game.observation_digest() != decision.expected_digests[i])
                    _exit(250);
            _exit(game.state().power);
        } catch (...) {
            _exit(250);
        }
    }
    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    require(WIFEXITED(status) && WEXITSTATUS(status) <= 128,
            "literal native power replay must match every digest without a hit");
    return unsigned(WEXITSTATUS(status));
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 3)
            return 2;
        Resources resources(argv[1]);
        set_resources(&resources);
        const bool full = std::string(argv[2]) == "full";
        StartConfig config;
        config.stage = 5;
        config.shot = 1;
        config.seed = 7589;
        config.power = full ? 128 : 64;
        Session game(config);
        D3DXVECTOR3 position(240.f, 400.f, 0.f);
        th06::g_ItemManager.SpawnItem(&position, th06::ITEM_POWER_BIG, th06::ITEM_STATE_FALLING);
        const auto before = game.observation_digest();
        PlannerOptions options;
        options.horizon = options.reuse = 64;
        options.proposals = 3;
        options.defer_later_digests = true;
        const auto ordinary = choose(game, options);
        options.power_aware = true;
        const auto guided = choose(game, options);
        require(game.observation_digest() == before, "proposal forks preserve actual power/RNG");
        require(ordinary.certified_frames == 64 && guided.certified_frames == 64,
                "both routes require complete native futures");
        const auto ordinary_power = replay_power(game, ordinary);
        const auto guided_power = replay_power(game, guided);
        if (full)
            require(ordinary.expected_inputs == guided.expected_inputs &&
                        ordinary.expected_digests == guided.expected_digests &&
                        ordinary_power == 128 && guided_power == 128,
                    "full power disables item attraction and preserves the original route");
        else
            require(ordinary_power == 64 && guided_power >= 72,
                    "original ItemManager awards the actual large-power pickup");
        require(game.observation_digest() == before, "independent replay remains fork-owned");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
