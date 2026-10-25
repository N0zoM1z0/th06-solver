#include "th06/planner.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace th06::solver {
namespace {
using Clock = std::chrono::steady_clock;
double seconds_since(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}
struct Candidate {
    std::uint16_t input;
    unsigned moving;
    double rank;
    bool reactive = false;
};
// Inexpensive, deliberately approximate proposal ranking. Native rollouts still
// resolve future spawns, acceleration, lasers, collisions and action-dependent RNG.
double proposal_rank(const Observation &o, std::uint16_t input, unsigned moving, unsigned horizon,
                     unsigned stride = 4) {
    const int dx = bool(input & 128) - bool(input & 64), dy = bool(input & 32) - bool(input & 16);
    const bool focus = input & 4;
    const float speed =
        dx && dy ? (focus ? o.focused_diagonal : o.diagonal) : (focus ? o.focused_axis : o.axis);
    double result = 0;
    unsigned samples = 0;
    for (unsigned t = 1; t <= horizon; t += stride) {
        const auto duration = float(std::min(t, moving));
        const float x = std::clamp(o.x + dx * speed * duration, 8.f, 376.f);
        const float y = std::clamp(o.y + dy * speed * duration, 16.f, 432.f);
        double local = 0;
        for (const auto &hazard : o.hazards) {
            const double hx = hazard.x + hazard.vx * t - x, hy = hazard.y + hazard.vy * t - y;
            const double d2 = hx * hx + hy * hy;
            if (d2 < 14400)
                local += 900.0 / (d2 + 100);
            if (d2 < hazard.radius * hazard.radius)
                local += 100;
        }
        const double edge = std::max(0.f, 40.f - std::min(x - 8.f, 376.f - x));
        result += local + std::abs(x - o.target_x) * 0.003 + std::abs(y - 380.f) * 0.006 +
                  edge * edge * 0.002;
        ++samples;
    }
    return result / samples + (dx || dy ? 0.02 : 0);
}
std::uint16_t reactive_input(const Observation &observation) {
    constexpr std::array<std::uint16_t, 9> directions = {0,       16,       32,      64,      128,
                                                         16 | 64, 16 | 128, 32 | 64, 32 | 128};
    double best = std::numeric_limits<double>::infinity();
    std::uint16_t selected = 261;
    for (const auto direction : directions)
        for (unsigned focus : {4u, 0u}) {
            if (!direction && !focus)
                continue;
            const auto input = std::uint16_t(direction | focus | 1u | 256u);
            const double cost = std::min(proposal_rank(observation, input, 16, 40, 8),
                                         proposal_rank(observation, input, 40, 40, 8));
            if (cost < best) {
                best = cost;
                selected = input;
            }
        }
    return selected;
}
struct Result {
    unsigned safe = 0, attempted = 0;
    double cost = 0;
    bool complete = false;
    std::array<std::uint64_t, 600> digests{};
    std::array<std::uint16_t, 600> inputs{};
    PlannerTimings timings;
};
void transfer(int fd, void *data, std::size_t size, bool output) {
    auto *bytes = static_cast<char *>(data);
    while (size) {
        const auto n = output ? ::write(fd, bytes, size) : ::read(fd, bytes, size);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            throw std::runtime_error("native rollout pipe failed");
        bytes += n;
        size -= n;
    }
}
Result rollout(Session &game, std::uint16_t input, unsigned moving, unsigned horizon,
               unsigned commit, unsigned retained, bool reactive, bool profile, bool digests,
               const std::vector<std::uint16_t> *literal = nullptr) {
    int pipefd[2];
    if (::pipe(pipefd))
        throw std::runtime_error("pipe failed");
    // Exactly one child at a time: all mutable native memory, RNG, scripts and
    // actor pointers remain at their original addresses under copy-on-write.
    const auto child = ::fork();
    if (child < 0) {
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        throw std::runtime_error("fork failed");
    }
    if (!child) {
        ::close(pipefd[0]);
        try {
            Result result;
            std::uint16_t action = input;
            for (unsigned frame = 0; frame < horizon; ++frame) {
                auto start = profile ? Clock::now() : Clock::time_point{};
                if (literal)
                    action = (*literal)[frame];
                else if (reactive) {
                    // Future observations and their action-dependent RNG belong
                    // to this branch. Retention records the literal actions,
                    // including turns; it never repeats just the initial input.
                    if (frame >= commit && (frame - commit) % 16 == 0)
                        action = reactive_input(game.observe());
                } else
                    action = frame < moving ? input : std::uint16_t(input & ~std::uint16_t(0xf0));
                if (profile) {
                    result.timings.guidance += seconds_since(start);
                    start = Clock::now();
                }
                const auto state = game.step(action);
                if (profile)
                    result.timings.native_step += seconds_since(start);
                ++result.attempted;
                if (state.hit)
                    break;
                if (frame < retained) {
                    start = profile ? Clock::now() : Clock::time_point{};
                    result.inputs[frame] = action;
                    if (digests)
                        result.digests[frame] = game.observation_digest();
                    if (profile)
                        result.timings.digest += seconds_since(start);
                }
                ++result.safe;
                start = profile ? Clock::now() : Clock::time_point{};
                result.cost += game.proposal_cost();
                if (profile)
                    result.timings.scoring += seconds_since(start);
                if (state.complete) {
                    result.complete = true;
                    break;
                }
            }
            if (result.safe) {
                const auto start = profile ? Clock::now() : Clock::time_point{};
                result.cost = result.cost / result.safe + game.proposal_cost();
                if (profile)
                    result.timings.scoring += seconds_since(start);
            }
            transfer(pipefd[1], &result, sizeof(result), true);
            ::close(pipefd[1]);
            ::_exit(0);
        } catch (const std::exception &error) {
            std::fprintf(stderr, "native rollout: %s\n", error.what());
            ::_exit(1);
        }
    }
    ::close(pipefd[1]);
    Result result;
    bool received = true;
    try {
        transfer(pipefd[0], &result, sizeof(result), false);
    } catch (...) {
        received = false;
    }
    ::close(pipefd[0]);
    int status;
    while (::waitpid(child, &status, 0) < 0) {
        if (errno != EINTR)
            throw std::runtime_error("waitpid failed");
    }
    if (!received || !WIFEXITED(status) || WEXITSTATUS(status))
        throw std::runtime_error("native rollout failed; no prefix certified");
    if (!std::isfinite(result.cost))
        throw std::runtime_error("non-finite rollout cost");
    return result;
}
} // namespace
Decision choose(Session &game, const PlannerOptions &options) {
    if (!options.commit || options.commit > options.horizon || options.horizon > 600 ||
        !options.proposals)
        throw std::invalid_argument("require 1 <= commit <= horizon <= 600 and proposals > 0");
    if (options.reuse && (options.reuse < options.commit || options.reuse > options.horizon))
        throw std::invalid_argument("require reuse = 0 or commit <= reuse <= horizon");
    constexpr std::array<std::uint16_t, 9> directions = {0,       16,       32,      64,      128,
                                                         16 | 64, 16 | 128, 32 | 64, 32 | 128};
    Decision decision;
    auto start = options.profile ? Clock::now() : Clock::time_point{};
    unsigned best_safe = 0;
    double best_cost = std::numeric_limits<double>::infinity();
    bool best_complete = false;
    std::vector<Candidate> candidates;
    const auto observation = game.observe();
    for (const auto direction : directions) {
        for (unsigned focus : {4u, 0u}) {
            if (!direction && !focus)
                continue;
            for (unsigned moving : {options.commit, options.horizon}) {
                if (!direction && moving != options.commit)
                    continue;
                const auto input = std::uint16_t(direction | focus | 1u | 256u);
                if (int(input) == options.excluded_input)
                    continue;
                candidates.push_back(
                    {input, moving, proposal_rank(observation, input, moving, options.horizon)});
            }
        }
    }
    if (options.reactive) {
        const auto constant = candidates;
        std::array<bool, 512> seen{};
        for (const auto &candidate : constant) {
            if (seen[candidate.input])
                continue;
            seen[candidate.input] = true;
            double rank = candidate.rank;
            for (const auto &other : constant)
                if (other.input == candidate.input)
                    rank = std::min(rank, other.rank);
            candidates.push_back({candidate.input, options.commit, rank + 0.01, true});
        }
    }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const auto &a, const auto &b) { return a.rank < b.rank; });
    if (options.diverse_first) {
        // The short, constant and reactive tails can certify the same first
        // action. Spend the first budget on distinct committed actions before
        // considering additional futures for an already represented action.
        std::array<bool, 512> represented{};
        std::vector<Candidate> distinct, repeated;
        for (const auto &candidate : candidates) {
            (represented[candidate.input] ? repeated : distinct).push_back(candidate);
            represented[candidate.input] = true;
        }
        distinct.insert(distinct.end(), repeated.begin(), repeated.end());
        candidates = std::move(distinct);
    }
    if (options.profile)
        decision.timings.ranking = seconds_since(start);
    std::array<bool, 512> evaluated{};
    for (const auto &candidate : candidates) {
        const auto input = candidate.input;
        const auto moving = candidate.moving;
        const bool digests = !options.defer_later_digests || decision.branches == 0;
        start = options.profile ? Clock::now() : Clock::time_point{};
        const auto result = rollout(game, input, moving, options.horizon, options.commit,
                                    std::max(options.commit, options.reuse), candidate.reactive,
                                    options.profile, digests);
        if (options.profile) {
            decision.timings.rollout_wall += seconds_since(start);
            decision.timings += result.timings;
        }
        ++decision.branches;
        decision.unique_inputs += !evaluated[input];
        evaluated[input] = true;
        decision.reactive_branches += candidate.reactive;
        decision.simulated_frames += result.attempted;
        const double cost = result.cost + ((input & 0xf0) ? 0.02 : 0);
        if ((result.complete && !best_complete) ||
            (result.complete == best_complete &&
             (result.safe > best_safe || (result.safe == best_safe && cost < best_cost)))) {
            best_complete = result.complete;
            decision.future_complete = result.complete;
            best_safe = result.safe;
            decision.lookahead_frames = best_safe;
            best_cost = cost;
            decision.input = input;
            decision.certified_frames = std::min({options.commit, moving, result.safe});
            // An incomplete future retains only its original short prefix.
            // A full native future may publish a longer literal action sequence.
            if (options.reuse && (result.complete || result.safe == options.horizon))
                decision.certified_frames = std::min(options.reuse, result.safe);
            decision.expected_digests.clear();
            if (digests)
                decision.expected_digests.assign(
                    result.digests.begin(), result.digests.begin() + decision.certified_frames);
            decision.expected_inputs.assign(result.inputs.begin(),
                                            result.inputs.begin() + decision.certified_frames);
        }
        if (decision.branches >= options.proposals &&
            (best_complete || best_safe == options.horizon))
            break;
    }
    if (decision.certified_frames && decision.expected_digests.empty()) {
        const auto start = options.profile ? Clock::now() : Clock::time_point{};
        const auto verified =
            rollout(game, decision.input, decision.certified_frames, decision.certified_frames,
                    options.commit, decision.certified_frames, false, options.profile, true,
                    &decision.expected_inputs);
        if (verified.safe != decision.certified_frames ||
            verified.complete != (decision.future_complete &&
                                  decision.certified_frames == decision.lookahead_frames))
            throw std::runtime_error("winner certificate replay diverged");
        decision.expected_digests.assign(verified.digests.begin(),
                                         verified.digests.begin() + decision.certified_frames);
        ++decision.branches;
        decision.simulated_frames += verified.attempted;
        if (options.profile) {
            decision.timings.rollout_wall += seconds_since(start);
            decision.timings += verified.timings;
        }
    }
    return decision;
}
} // namespace th06::solver
