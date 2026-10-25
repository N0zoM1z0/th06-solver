#include "th06/planner.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

namespace th06::solver {
namespace {
struct Candidate {
    std::uint16_t input;
    unsigned moving;
    double rank;
};
// Inexpensive, deliberately approximate proposal ranking. Native rollouts still
// resolve future spawns, acceleration, lasers, collisions and action-dependent RNG.
double proposal_rank(const Observation &o, std::uint16_t input, unsigned moving, unsigned horizon) {
    const int dx = bool(input & 128) - bool(input & 64), dy = bool(input & 32) - bool(input & 16);
    const bool focus = input & 4;
    const float speed =
        dx && dy ? (focus ? o.focused_diagonal : o.diagonal) : (focus ? o.focused_axis : o.axis);
    double result = 0;
    unsigned samples = 0;
    for (unsigned t = 1; t <= horizon; t += 4) {
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
struct Result {
    unsigned safe = 0, attempted = 0;
    double cost = 0;
    bool complete = false;
    std::array<std::uint64_t, 600> digests{};
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
               unsigned commit) {
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
            for (unsigned frame = 0; frame < horizon; ++frame) {
                const auto action =
                    frame < moving ? input : std::uint16_t(input & ~std::uint16_t(0xf0));
                const auto state = game.step(action);
                ++result.attempted;
                if (state.hit)
                    break;
                if (frame < commit)
                    result.digests[frame] = game.observation_digest();
                ++result.safe;
                result.cost += game.proposal_cost();
                if (state.complete) {
                    result.complete = true;
                    break;
                }
            }
            if (result.safe)
                result.cost = result.cost / result.safe + game.proposal_cost();
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
    constexpr std::array<std::uint16_t, 9> directions = {0,       16,       32,      64,      128,
                                                         16 | 64, 16 | 128, 32 | 64, 32 | 128};
    Decision decision;
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
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const auto &a, const auto &b) { return a.rank < b.rank; });
    for (const auto &candidate : candidates) {
        const auto input = candidate.input;
        const auto moving = candidate.moving;
        const auto result = rollout(game, input, moving, options.horizon, options.commit);
        ++decision.branches;
        decision.simulated_frames += result.attempted;
        const double cost = result.cost + ((input & 0xf0) ? 0.02 : 0);
        if ((result.complete && !best_complete) ||
            (result.complete == best_complete &&
             (result.safe > best_safe || (result.safe == best_safe && cost < best_cost)))) {
            best_complete = result.complete;
            best_safe = result.safe;
            best_cost = cost;
            decision.input = input;
            // A short motion followed by a neutral tail proves only
            // the initial motion for this returned action.
            decision.certified_frames = std::min({options.commit, moving, result.safe});
            decision.expected_digests.assign(result.digests.begin(),
                                             result.digests.begin() + decision.certified_frames);
        }
        if (decision.branches >= options.proposals &&
            (best_complete || best_safe == options.horizon))
            break;
    }
    return decision;
}
} // namespace th06::solver
