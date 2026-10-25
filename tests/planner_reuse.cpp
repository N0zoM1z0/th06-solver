#include "Chain.hpp"
#include "stdafx.h"
#include "test_support.hpp"
#include "th06/planner.hpp"
#include "th06/resources.hpp"
#include <algorithm>
#include <iostream>

using namespace th06;
using namespace th06::solver;
int main(int argc, char **argv) {
    try {
        check(argc == 3, "reuse fixture needs DAT and mixed/failed mode");
        Resources resources(argv[1]);
        set_resources(&resources);
        StartConfig start;
        start.stage = 5;
        start.shot = 1;
        start.seed = 7589;
        start.power = 128;
        unsigned failure_updates = 0;
        ChainElem failure;
        Session game(start);
        PlannerOptions options;
        options.horizon = 64;
        options.proposals = 6;
        options.reactive = true;
        options.reuse = 32;
        // These controlled states test native prefix contracts, not stage clears.
        const bool failed = std::string(argv[2]) == "failed";
        if (failed) {
            failure.SetCallback(+[](void *arg) {
                if (++*static_cast<unsigned *>(arg) == 20)
                    g_GameManager.deaths = 1;
                return CHAIN_CALLBACK_RESULT_CONTINUE;
            });
            failure.arg = &failure_updates;
            check(g_FuncChain.AddToCalcChain(&failure, 15) == ZUN_SUCCESS,
                  "failure injection callback registration");
        } else {
            check(std::string(argv[2]) == "mixed", "unknown reuse fixture");
            g_Player.positionCenter.y = 420;
        }
        const auto original = game.observation_digest();
        for (unsigned invalid : {7u, 65u}) {
            auto bad = options;
            bad.reuse = invalid;
            rejects([&] { choose(game, bad); }, "invalid retained prefix accepted");
            check(game.observation_digest() == original, "invalid request changed parent");
        }
        const auto decision = choose(game, options);
        check(game.observation_digest() == original, "rollout changed parent world or RNG");
        check(failure_updates == 0, "rollout leaked branch callback state");
        check(decision.expected_inputs.size() == decision.certified_frames &&
                  decision.expected_digests.size() == decision.certified_frames,
              "input/digest certificate lengths differ");
        if (failed) {
            check(decision.lookahead_frames == 19 && decision.certified_frames == 8,
                  "incomplete future extended beyond the original commit");
        } else {
            check(decision.certified_frames == 32 && decision.lookahead_frames == 64,
                  "full native horizon did not retain the requested prefix");
            check(std::any_of(decision.expected_inputs.begin(), decision.expected_inputs.end(),
                              [&](auto input) { return input != decision.input; }),
                  "fixture did not exercise a multi-action future");
        }
        for (unsigned i = 0; i < decision.certified_frames; ++i) {
            const auto state = game.step(decision.expected_inputs[i]);
            check(!state.hit && game.observation_digest() == decision.expected_digests[i],
                  "retained action sequence diverged in parent replay");
        }
        if (failed)
            g_FuncChain.Cut(&failure);
        std::cout << "synthetic native retained-future contract passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
