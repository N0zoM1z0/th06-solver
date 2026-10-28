#include "stdafx.h"
#include "th06/native_geometry.hpp"
#include "th06/planner.hpp"
#include "th06/resources.hpp"
#include "th06/shadow_recorded.hpp"
#include "th06/shadow_search.hpp"
#include <cerrno>
#include <chrono>
#include <csignal>
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
bool saturate = false;
bool checkpoint_mode = false, checkpoint_delay = false;
bool late_finish = false;
pid_t owner = 0;
std::uint16_t forbidden = 0;
std::uint32_t *blocked = nullptr;
th06::ChainCallbackResult inject(void *) {
    ++calls;
    if (late_finish) {
        if (calls == 9 && !b::load(blocked)) {
            b::save(blocked, 1);
            kill(owner, SIGSTOP);
            usleep(400000);
        }
        return th06::CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (checkpoint_mode) {
        if (checkpoint_delay && calls == 9) {
            const auto branch = b::load(blocked + 1) + 1;
            b::save(blocked + 1, branch);
            if (branch >= 2) {
                b::save(blocked, 2);
                usleep(2000000);
            }
        }
        return th06::CHAIN_CALLBACK_RESULT_CONTINUE;
    }
    if (saturate) {
        if (geometry_frame)
            while (geometry_frame->box_count < geometry_boxes)
                record_geometry_box(192.f, 224.f, 1000.f, 1000.f);
        return th06::CHAIN_CALLBACK_RESULT_CONTINUE;
    }
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
        if (mode == "warm-short" || mode == "warm-short-deadline" || mode == "warm-short-fresh") {
            b::Plan active;
            active.generation = 2;
            active.stage = config.stage;
            active.first_change = 9;
            active.count = 64;
            active.inputs.fill(261);
            for (const auto cap : {7u, b::capacity}) {
                bool rejected = false;
                try {
                    b::search(game, active, 261, 8, 32, 1000, false, false, 0, false, 1, false,
                              true, false, false, false, false, true, cap);
                } catch (const std::invalid_argument &) {
                    rejected = true;
                }
                require(rejected, "invalid backup cap fails before forking");
            }
            const bool timed = mode == "warm-short-deadline";
            const bool fresh = mode == "warm-short-fresh";
            th06::ChainElem callback;
            if (!fresh) {
                callback.SetCallback(reinterpret_cast<th06::ChainCallback>(inject));
                require(th06::g_FuncChain.AddToCalcChain(&callback, 15) == ZUN_SUCCESS,
                        "inject long-future failure after a complete short backup");
            }
            if (timed) {
                checkpoint_mode = checkpoint_delay = true;
                blocked = static_cast<std::uint32_t *>(
                    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
                require(blocked != MAP_FAILED, "shared short backup deadline receipt");
            }
            const auto result =
                b::search(game, active, 261, 8, 32, timed ? 150 : 1000, false, false, 0, false, 1,
                          false, true, false, false, false, false, true, 8);
            require(game.observation_digest() == before && !calls && result.warm_certified &&
                        result.warm_horizon == 8 && result.certified_horizon == (fresh ? 32 : 8) &&
                        result.prediction.count == (fresh ? 40 : 16) &&
                        result.warm_selected == !fresh && result.timeout == timed &&
                        result.warm_checkpoint == timed,
                    "short backup never claims the fresh horizon; full fresh success wins");
            if (timed) {
                require(b::load(blocked) == 2, "fresh work blocks after the short certificate");
                checkpoint_delay = false;
            }
            for (unsigned i = 0; i < result.prediction.count; ++i)
                require(!game.step(result.prediction.inputs[i]).hit &&
                            game.observation_digest() == result.prediction.digests[i],
                        "short or fresh certificate independently replays");
            int status;
            errno = 0;
            require(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD,
                    "short backup reaps all descendants");
            if (blocked)
                munmap(blocked, 4096);
            return 0;
        }
        if (mode == "warm" || mode == "warm-deadline" || mode == "warm-failure") {
            b::Plan active;
            active.generation = 2;
            active.stage = config.stage;
            active.first_change = 9;
            active.count = 64;
            active.inputs.fill(261);
            bool rejected = false;
            try {
                b::search(game, active, 261, 8, 32, 1000, false, false, 0, false, 1, false, false,
                          false, false, false, false, true);
            } catch (const std::invalid_argument &) {
                rejected = true;
            }
            require(rejected && game.observation_digest() == before,
                    "warm start requires an anytime owner without mutating the world");
            th06::ChainElem callback;
            if (mode != "warm") {
                callback.SetCallback(reinterpret_cast<th06::ChainCallback>(inject));
                require(th06::g_FuncChain.AddToCalcChain(&callback, 15) == ZUN_SUCCESS,
                        "inject warm failure/deadline");
            }
            if (mode == "warm-deadline") {
                checkpoint_mode = checkpoint_delay = true;
                blocked = static_cast<std::uint32_t *>(
                    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
                require(blocked != MAP_FAILED, "shared warm deadline receipt");
            }
            const auto result =
                b::search(game, active, 261, 8, 32, mode == "warm-deadline" ? 150 : 1000, false,
                          false, 0, false, 1, false, true, false, false, false, false, true);
            require(result.warm_attempted && game.observation_digest() == before && !calls,
                    "warm attempt preserves the actual world and RNG");
            if (mode == "warm-failure") {
                require(!result.prediction.count && !result.warm_certified &&
                            !result.warm_selected && !result.warm_checkpoint,
                        "an incomplete nominal future cannot become a backup");
            } else {
                const bool timed = mode == "warm-deadline";
                require(
                    result.prediction.count == 40 && result.warm_certified &&
                        result.timeout == timed && result.warm_selected == timed &&
                        result.warm_checkpoint == timed && result.deadline_checkpoint == timed,
                    "fresh success wins; only a blocked fresh search falls back to verified warm");
                if (timed) {
                    require(b::load(blocked) == 2, "fresh ranking reaches a nested blocked child");
                    checkpoint_delay = false;
                }
                for (unsigned i = 0; i < result.prediction.count; ++i) {
                    require(!game.step(result.prediction.inputs[i]).hit &&
                                game.observation_digest() == result.prediction.digests[i],
                            "independent literal warm/fresh certificate replay");
                }
            }
            int status;
            errno = 0;
            require(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD,
                    "warm completion reaps every rollout descendant");
            if (blocked)
                munmap(blocked, 4096);
            return 0;
        }
        if (mode == "nominal") {
            const auto fresh = b::search(game, {}, 261, 16, 32, 1000, false);
            require(fresh.prediction.count == 48, "nominal fixture has a full fresh future");
            b::Plan active;
            active.count = fresh.prediction.count;
            active.inputs = fresh.prediction.inputs;
            active.stage = config.stage;
            auto previous = std::uint16_t(261);
            for (unsigned i = 0; i < 4; ++i) {
                previous = active.inputs[i];
                game.step(previous);
            }
            const auto origin = game.observation_digest();
            const auto result = b::search(game, active, previous, 16, 32, 1000, true);
            require(!result.timeout && result.prediction.count == 48 && result.prediction.nominal,
                    "nominal re-executes the remaining queue and extends its tail");
            require(game.observation_digest() == origin, "nominal search preserves actual state");
            for (unsigned i = 0; i < result.prediction.count; ++i) {
                require(!game.step(result.prediction.inputs[i]).hit &&
                            game.observation_digest() == result.prediction.digests[i],
                        "extended nominal receives independent literal verification");
            }
            return 0;
        }
        if (mode == "literal" || mode == "pulse" || mode == "recorded" || mode == "diverse") {
            b::Plan active;
            if (mode == "pulse") {
                active.count = 48;
                active.inputs.fill(261);
            }
            const auto result = b::search(game, active, 261, 16, 32, 1000, false, mode == "pulse",
                                          65, mode == "recorded", mode == "diverse" ? 6 : 1, false,
                                          false, false, false, false, mode == "diverse");
            if (mode == "diverse")
                require(result.prediction.unique_inputs == 6,
                        "bounded diversity covers six distinct initial inputs");
            require(!result.timeout && result.prediction.count == 48, "full lead and future");
            if (mode == "pulse")
                require(result.prediction.motion && result.motion_attempts == 1 &&
                            result.motion_cursor == 0,
                        "literal pulse and persistent rotation");
            if (mode == "recorded")
                require(result.prediction.recorded && result.recorded_forecasts &&
                            result.recorded_checks >= 2,
                        "geometry proposal receives independent native verification");
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
            if (mode == "late-completion") {
                late_finish = true;
                owner = getpid();
                blocked = static_cast<std::uint32_t *>(
                    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
                require(blocked != MAP_FAILED, "shared late completion receipt");
                for (bool anytime : {false, true}) {
                    b::save(blocked, 0);
                    const auto resumer = fork();
                    require(resumer >= 0, "fork independent owner resumer");
                    if (!resumer) {
                        // The deadline enforcer is deliberately descheduled.
                        // Search can finish late before its owner reads it.
                        usleep(1200000);
                        kill(owner, SIGCONT);
                        _exit(0);
                    }
                    const auto result = b::search(game, {}, 261, 8, 32, 200, false, false, 0, false,
                                                  1, false, anytime);
                    int status;
                    while (waitpid(resumer, &status, 0) < 0 && errno == EINTR) {
                    }
                    std::cout << "late result: count=" << result.prediction.count
                              << " timeout=" << result.timeout
                              << " completion_ms=" << result.completion_ms << " anytime=" << anytime
                              << '\n';
                    require(b::load(blocked) == 1 && result.timeout && result.late_completion &&
                                result.completion_ms > 200 && !result.prediction.count &&
                                !result.deadline_checkpoint,
                            "a readable late certificate cannot bypass the deadline");
                    require(game.observation_digest() == before && !calls,
                            "late completion preserves actual state and RNG");
                    errno = 0;
                    require(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD,
                            "late completion leaves no descendant");
                }
                munmap(blocked, 4096);
                return 0;
            }
            if (mode == "checkpoint") {
                checkpoint_mode = checkpoint_delay = true;
                blocked = static_cast<std::uint32_t *>(
                    mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
                require(blocked != MAP_FAILED, "shared checkpoint injection receipt");
                for (bool anytime : {false, true}) {
                    b::save(blocked, 0);
                    b::save(blocked + 1, 0);
                    const auto result = b::search(game, {}, 261, 8, 32, 100, false, false, 0, false,
                                                  3, false, anytime);
                    require(result.timeout && b::load(blocked) == 2,
                            "ranking blocks after a full first candidate");
                    require(result.deadline_checkpoint == anytime &&
                                result.prediction.count == (anytime ? 40u : 0u),
                            "only opt-in timeout retains the complete certificate");
                    require(calls == 0 && game.observation_digest() == before,
                            "checkpoint preserves actual world and RNG");
                    int status;
                    errno = 0;
                    require(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD,
                            "checkpoint recovery reaps every rollout descendant");
                    if (anytime) {
                        checkpoint_delay = false;
                        for (unsigned i = 0; i < result.prediction.count; ++i) {
                            require(!game.step(result.prediction.inputs[i]).hit,
                                    "checkpoint route is safe");
                            require(game.observation_digest() == result.prediction.digests[i],
                                    "independent checkpoint literal digest");
                        }
                    }
                }
                munmap(blocked, 4096);
                return 0;
            }
            if (mode == "recorded-capacity") {
                saturate = true;
                bool rejected = false;
                try {
                    b::recorded_search(game, std::vector<std::uint16_t>(32, 261), 1000);
                } catch (const std::runtime_error &e) {
                    rejected = std::string(e.what()) == "recorded geometry index capacity exceeded";
                }
                require(rejected, "large overlapping shapes cannot allocate an unbounded index");
                require(calls == 0 && game.observation_digest() == before,
                        "capacity failure preserves actual world");
                int status;
                errno = 0;
                require(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD,
                        "capacity failure reaps its capture child");
                return 0;
            }
            delay = mode == "deadline" || mode == "pulse-deadline" || mode == "recorded-deadline" ||
                    mode == "checkpoint-deadline";
            require(delay || mode == "failure" || mode == "deferred" ||
                        mode == "recorded-failure" || mode == "checkpoint-failure",
                    "test mode");
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
                unsigned notifications = 0;
                Decision saved;
                const auto checkpointed = choose(game, options, [&](const Decision &candidate) {
                    ++notifications;
                    saved = candidate;
                });
                require(notifications == 1 && saved.lookahead_frames == 32 &&
                            saved.expected_inputs == deferred.expected_inputs &&
                            saved.expected_digests == deferred.expected_digests &&
                            checkpointed.expected_inputs == deferred.expected_inputs &&
                            checkpointed.expected_digests == deferred.expected_digests,
                        "first later full candidate receives its own literal certificate once");
                options.ranking_deadline = std::chrono::steady_clock::time_point::min();
                options.proposals = 50;
                const auto yielded = choose(game, options);
                require(yielded.ranking_yielded && yielded.lookahead_frames == 32 &&
                            yielded.expected_inputs == deferred.expected_inputs &&
                            yielded.expected_digests == deferred.expected_digests,
                        "expired ranking budget still finds and verifies the first full future");
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
                const bool recorded = mode == "recorded-deadline" || mode == "recorded-failure";
                const auto result = b::search(
                    game, active, 261, 8, 32, delay ? 150 : 1000, false, pulse, 65, recorded, 1,
                    false, mode == "checkpoint-failure" || mode == "checkpoint-deadline");
                require(result.prediction.count == 0, "failed future publishes no prefix");
                require(!result.deadline_checkpoint,
                        "failed or unfinished future has no checkpoint");
                if (delay) {
                    require(result.timeout, "deadline interrupts native calculation");
                    require(b::load(blocked) == 2, "deadline reaches a nested rollout descendant");
                    if (pulse)
                        require(result.motion_attempts == 1 && result.motion_cursor == 0 &&
                                    result.last_phase == 7,
                                "timeout retains attempted pulse cursor");
                    if (recorded)
                        require(result.recorded_forecasts == 1 && result.last_phase == 3,
                                "deadline accounts for interrupted recording");
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
