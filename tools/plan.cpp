#include "th06/planner.hpp"
#include "th06/resources.hpp"
#include "th06/tape.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sys/resource.h>
#if __has_include(<valgrind/callgrind.h>)
#include <valgrind/callgrind.h>
#else
#define CALLGRIND_START_INSTRUMENTATION ((void)0)
#define CALLGRIND_STOP_INSTRUMENTATION ((void)0)
#define CALLGRIND_ZERO_STATS ((void)0)
#endif

using namespace th06::solver;
int main(int argc, char **argv) {
    try {
        std::string data, output, prefix_path;
        Tape tape;
        PlannerOptions options;
        unsigned frames = 20000;
        unsigned prefix_frames = 0;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--diverse-first") {
                options.diverse_first = true;
                continue;
            }
            if (key == "--profile") {
                options.profile = true;
                continue;
            }
            if (key == "--reactive") {
                options.reactive = true;
                continue;
            }
            if (key == "--story") {
                tape.start.story = true;
                continue;
            }
            if (key == "--help") {
                std::cout
                    << "th06_plan --data DIR --output TAPE [--stage 1..7] [--difficulty 0..4]\n"
                       "  [--shot 0..3] [--seed 0..65535] [--power 0..128] [--frames N]\n"
                       "  [--horizon 1..600] [--commit 1..horizon] [--proposals N] [--story] "
                       "[--reactive] [--profile] [--diverse-first]\n";
                return 0;
            }
            if (i + 1 == argc)
                throw std::invalid_argument("missing value for " + key);
            const std::string value = argv[++i];
            if (key == "--data")
                data = value;
            else if (key == "--output")
                output = value;
            else if (key == "--prefix")
                prefix_path = value;
            else {
                std::size_t end;
                const auto n = std::stoul(value, &end, 0);
                if (end != value.size() || n > 1000000)
                    throw std::invalid_argument("invalid numeric option");
                if (key == "--stage")
                    tape.start.stage = n;
                else if (key == "--difficulty")
                    tape.start.difficulty = n;
                else if (key == "--shot")
                    tape.start.shot = n;
                else if (key == "--seed" && n <= 65535)
                    tape.start.seed = n;
                else if (key == "--power")
                    tape.start.power = n;
                else if (key == "--frames" && n)
                    frames = n;
                else if (key == "--horizon")
                    options.horizon = n;
                else if (key == "--commit")
                    options.commit = n;
                else if (key == "--proposals" && n)
                    options.proposals = n;
                else if (key == "--prefix-frames")
                    prefix_frames = n;
                else if (key == "--exclude-first" && n <= 65535)
                    options.excluded_input = n;
                else
                    throw std::invalid_argument("unknown/invalid option " + key);
            }
        }
        if (data.empty() || output.empty())
            throw std::invalid_argument("--data and --output are required");
        if (!options.commit || options.commit > options.horizon || options.horizon > 600)
            throw std::invalid_argument("require 1 <= commit <= horizon <= 600");
        Tape prefix;
        if (!prefix_path.empty()) {
            prefix = read_tape(prefix_path);
            if (prefix_frames > prefix.rows.size() || prefix_frames >= frames)
                throw std::invalid_argument("invalid prefix rewind point");
            tape.start = prefix.start;
        } else if (prefix_frames)
            throw std::invalid_argument("--prefix-frames requires --prefix");
        Resources resources(data);
        tape.resource_fingerprint = resources.fingerprint();
        if (!prefix_path.empty() && prefix.resource_fingerprint != tape.resource_fingerprint)
            throw std::invalid_argument("prefix DAT fingerprint mismatch");
        set_resources(&resources);
        Session game(tape.start);
        auto state = game.state();
        std::uint64_t branches = 0, simulated = 0, reactive_branches = 0;
        unsigned next_progress = 600;
        const int excluded_first_input = options.excluded_input;
        const auto begin = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < prefix_frames; ++i) {
            state = game.step(prefix.rows[i].input);
            if (state.hit || game.observation_digest() != prefix.rows[i].digest)
                throw std::runtime_error("prefix replay divergence at frame " +
                                         std::to_string(state.frame));
            tape.rows.push_back(prefix.rows[i]);
        }
        next_progress = unsigned(state.frame / 600 + 1) * 600;
        PlannerTimings timings;
        unsigned decisions = 0, short_horizon_decisions = 0;
        std::uint64_t unique_inputs = 0;
        const auto search_begin = std::chrono::steady_clock::now();
        rusage self_begin{}, children_begin{};
        getrusage(RUSAGE_SELF, &self_begin);
        getrusage(RUSAGE_CHILDREN, &children_begin);
        if (options.profile) {
            CALLGRIND_START_INSTRUMENTATION;
            CALLGRIND_ZERO_STATS;
        }
        while (state.frame < frames && !state.complete && !state.hit) {
            const auto decision = choose(game, options);
            timings += decision.timings;
            ++decisions;
            short_horizon_decisions += decision.lookahead_frames < options.horizon;
            unique_inputs += decision.unique_inputs;
            options.excluded_input = -1;
            branches += decision.branches;
            reactive_branches += decision.reactive_branches;
            simulated += decision.simulated_frames;
            if (!decision.certified_frames)
                break;
            const auto count =
                std::min<std::uint64_t>(decision.certified_frames, frames - state.frame);
            for (unsigned i = 0; i < count; ++i) {
                state = game.step(decision.input);
                const auto digest = game.observation_digest();
                if (state.hit || digest != decision.expected_digests[i])
                    throw std::runtime_error("accepted prefix diverged from child at frame " +
                                             std::to_string(state.frame));
                tape.rows.push_back({decision.input, digest});
                if (state.complete)
                    break;
            }
            if (state.frame >= next_progress) {
                // Persist only the accepted native frontier. A later rollout
                // error can be resumed without inventing or copying game state.
                tape.complete = state.complete;
                write_tape(output, tape);
                std::cerr << "frame=" << state.frame << " timeline=" << state.timeline
                          << " bullets=" << state.bullets << " stage=" << state.stage
                          << " power=" << state.power << " branches=" << branches << '\n';
                next_progress += 600;
            }
        }
        if (options.profile)
            CALLGRIND_STOP_INSTRUMENTATION;
        tape.complete = state.complete;
        if (!tape.rows.empty())
            write_tape(output, tape);
        const auto seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
        std::cout << std::boolalpha << std::setprecision(9) << "{\"profile\":\"" << native_profile
                  << "\",\"stage\":" << tape.start.stage
                  << ",\"difficulty\":" << tape.start.difficulty << ",\"shot\":" << tape.start.shot
                  << ",\"seed\":" << tape.start.seed << ",\"power\":" << tape.start.power
                  << ",\"frames\":" << state.frame << ",\"story\":" << tape.start.story
                  << ",\"replayed_prefix_frames\":" << prefix_frames
                  << ",\"excluded_first_input\":" << excluded_first_input
                  << ",\"cleared_stages\":" << game.cleared_stages()
                  << ",\"complete\":" << state.complete << ",\"deaths\":" << state.deaths
                  << ",\"bombs\":" << state.bombs << ",\"branches\":" << branches
                  << ",\"simulated_frames\":" << simulated << ",\"horizon\":" << options.horizon
                  << ",\"reactive\":" << options.reactive
                  << ",\"reactive_branches\":" << reactive_branches
                  << ",\"diverse_first\":" << options.diverse_first
                  << ",\"decisions\":" << decisions
                  << ",\"short_horizon_decisions\":" << short_horizon_decisions
                  << ",\"unique_inputs\":" << unique_inputs << ",\"commit\":" << options.commit
                  << ",\"proposals\":" << options.proposals << ",\"planning_seconds\":" << seconds;
        if (options.profile) {
            rusage self_end{}, children_end{};
            getrusage(RUSAGE_SELF, &self_end);
            getrusage(RUSAGE_CHILDREN, &children_end);
            auto cpu = [](timeval t) { return t.tv_sec + t.tv_usec * 1e-6; };
            std::cout << ",\"timings\":{\"search_wall_seconds\":"
                      << std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                                       search_begin)
                             .count()
                      << ",\"parent_user_seconds\":"
                      << cpu(self_end.ru_utime) - cpu(self_begin.ru_utime)
                      << ",\"parent_system_seconds\":"
                      << cpu(self_end.ru_stime) - cpu(self_begin.ru_stime)
                      << ",\"children_user_seconds\":"
                      << cpu(children_end.ru_utime) - cpu(children_begin.ru_utime)
                      << ",\"children_system_seconds\":"
                      << cpu(children_end.ru_stime) - cpu(children_begin.ru_stime)
                      << ",\"ranking_seconds\":" << timings.ranking
                      << ",\"rollout_wall_seconds\":" << timings.rollout_wall
                      << ",\"native_step_seconds\":" << timings.native_step
                      << ",\"guidance_seconds\":" << timings.guidance
                      << ",\"scoring_seconds\":" << timings.scoring
                      << ",\"digest_seconds\":" << timings.digest << '}';
        }
        std::cout << "}\n";
        return state.complete ? 0 : 2;
    } catch (const std::exception &error) {
        std::cerr << "th06_plan: " << error.what() << '\n';
        return 1;
    }
}
