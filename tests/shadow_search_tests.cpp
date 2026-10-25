#include "stdafx.h"
#include "th06/planner.hpp"
#include "th06/resources.hpp"
#include "th06/shadow_search.hpp"
#include <cerrno>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
namespace b = th06::shadow;
using namespace th06::solver;
namespace {
unsigned calls = 0;
bool delay = false;
std::uint16_t forbidden = 0;
std::uint32_t *blocked = nullptr;
th06::ChainCallbackResult inject(void *) {
    ++calls;
    if (delay && calls >= 9) {
        b::save(blocked, getpid() != getpgrp() ? 2u : 1u);
        usleep(2000000);
    } else if (!delay &&
               ((forbidden && th06::g_CurFrameInput == forbidden) || (!forbidden && calls == 20)))
        th06::g_GameManager.deaths = 1;
    return th06::CHAIN_CALLBACK_RESULT_CONTINUE;
}
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 3)
            return 2;
        Resources resources(argv[1]);
        set_resources(&resources);
        StartConfig config;
        config.stage = 5;
        config.shot = 1;
        config.power = 128;
        config.seed = 7589;
        Session game(config);
        const auto before = game.observation_digest();
        const std::string mode = argv[2];
        if (mode == "literal" || mode == "pulse") {
            b::Plan active;
            if (mode == "pulse") {
                active.count = 48;
                active.inputs.fill(261);
            }
            const auto result =
                b::search(game, active, 261, 16, 32, 1000, false, mode == "pulse", 65);
            require(!result.timeout && result.prediction.count == 48, "full lead and future");
            if (mode == "pulse")
                require(result.prediction.motion && result.motion_attempts == 1 &&
                            result.motion_cursor == 0,
                        "literal pulse and persistent rotation");
            require(game.observation_digest() == before, "fork preserves actual world/RNG");
            for (unsigned i = 0; i < result.prediction.count; ++i) {
                game.step(result.prediction.inputs[i]);
                require(game.observation_digest() == result.prediction.digests[i],
                        "literal digest");
            }
        } else {
            th06::ChainElem callback;
            callback.SetCallback(reinterpret_cast<th06::ChainCallback>(inject));
            require(th06::g_FuncChain.AddToCalcChain(&callback, 15) == ZUN_SUCCESS,
                    "inject calc observer");
            delay = mode == "deadline" || mode == "pulse-deadline";
            require(delay || mode == "failure" || mode == "deferred", "test mode");
            if (delay) {
                blocked = static_cast<std::uint32_t *>(
                    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
                require(blocked != MAP_FAILED, "shared deadline injection receipt");
            }
            if (mode == "deferred") {
                PlannerOptions options;
                options.horizon = 32;
                options.reuse = 32;
                options.proposals = 1;
                // Discover and then invalidate the first ranked action using
                // a source-phase callback; other native futures remain legal.
                forbidden = choose(game, options).input;
                const auto eager = choose(game, options);
                options.defer_later_digests = true;
                const auto deferred = choose(game, options);
                require(eager.lookahead_frames == 32 && deferred.lookahead_frames == 32 &&
                            deferred.branches > 1,
                        "later native candidate wins");
                require(eager.expected_inputs == deferred.expected_inputs &&
                            eager.expected_digests.size() == 32 &&
                            eager.expected_digests == deferred.expected_digests &&
                            deferred.branches == eager.branches + 1,
                        "defer only hashes, preserve literal selected route");
                require(game.observation_digest() == before && calls == 0,
                        "deferred COW isolation");
                for (unsigned i = 0; i < 32; ++i) {
                    game.step(deferred.expected_inputs[i]);
                    require(game.observation_digest() == eager.expected_digests[i],
                            "independent winner replay matches eager certificate");
                }
                return 0;
            }
            for (unsigned i = 0; i < (delay ? 4u : 1u); ++i) {
                const auto begin = std::chrono::steady_clock::now();
                if (delay)
                    b::save(blocked, 0);
                b::Plan active;
                const bool pulse = mode == "pulse-deadline";
                if (pulse) {
                    active.count = 64;
                    active.inputs.fill(261);
                }
                const auto result =
                    b::search(game, active, 261, 8, 32, delay ? 30 : 1000, false, pulse, 65);
                require(result.prediction.count == 0, "failed future publishes no prefix");
                if (delay) {
                    require(result.timeout, "deadline interrupts native calculation");
                    require(b::load(blocked) == 2, "deadline reaches a nested rollout descendant");
                    if (pulse)
                        require(result.motion_attempts == 1 && result.motion_cursor == 0,
                                "timeout retains attempted pulse cursor");
                    require(std::chrono::steady_clock::now() - begin <
                                std::chrono::milliseconds(500),
                            "deadline wall bound including cleanup");
                }
                require(calls == 0 && game.observation_digest() == before,
                        "actual world unchanged");
                int status;
                errno = 0;
                require(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD,
                        "no running or zombie rollout descendants");
            }
            if (blocked)
                munmap(blocked, 4096);
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
