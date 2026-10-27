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
    float orbit_radius = 0, orbit_direction = 1;
};
struct Orbit {
    float x = 0, y = 0, radius = 0, direction = 1, axis = 4, diagonal = 2.8284271f;
};
const LaserForecast *orbit_laser(const Observation &o) {
    const LaserForecast *best = nullptr;
    for (const auto &l : o.lasers)
        if (l.present && l.duration >= 60 && l.end - l.start >= 128 &&
            (!best || (best->state == 2 && l.state != 2) ||
             ((best->state == 2) == (l.state == 2) && l.width > best->width)))
            best = &l;
    return best;
}
std::uint16_t orbit_input(const Orbit &o, float x, float y) {
    const float angle = std::atan2(y - o.y, x - o.x);
    const float cosine = std::cos(angle), sine = std::sin(angle);
    const float tangent = o.radius * 1.03f * o.direction;
    const float goal_x = o.x + o.radius * cosine - tangent * sine;
    const float goal_y = o.y + o.radius * sine + tangent * cosine;
    constexpr unsigned directions[]{0, 16, 32, 64, 128, 80, 144, 96, 160};
    float best = std::numeric_limits<float>::infinity();
    std::uint16_t selected = 257;
    for (auto d : directions) {
        const int dx = bool(d & 128) - bool(d & 64), dy = bool(d & 32) - bool(d & 16);
        const float speed = dx && dy ? o.diagonal : o.axis;
        const float px = std::clamp(x + dx * speed, 8.f, 376.f);
        const float py = std::clamp(y + dy * speed, 16.f, 432.f);
        const float cost = (px - goal_x) * (px - goal_x) + (py - goal_y) * (py - goal_y);
        if (cost < best) {
            best = cost;
            selected = std::uint16_t(d | 257);
        }
    }
    return selected;
}
// Inexpensive, deliberately approximate proposal ranking. Native rollouts still
// resolve future spawns, acceleration, lasers, collisions and action-dependent RNG.
std::vector<LaserShape> laser_samples(const Observation &o, unsigned horizon, unsigned stride) {
    if (o.lasers.empty())
        return {};
    auto lasers = o.lasers;
    std::vector<LaserShape> samples(((horizon + stride - 1) / stride) * lasers.size());
    for (unsigned t = 1; t <= horizon; ++t)
        for (unsigned i = 0; i < lasers.size(); ++i) {
            const auto shape = advance_laser(lasers[i]);
            if ((t - 1) % stride == 0)
                samples[((t - 1) / stride) * lasers.size() + i] = shape;
        }
    return samples;
}
double proposal_rank(const Observation &o, std::uint16_t input, unsigned moving, unsigned horizon,
                     const std::vector<LaserShape> &lasers, unsigned stride = 4,
                     bool power_aware = false, bool damage_aware = false) {
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
            const auto [px, py] = hazard.position(t);
            const double hx = px - x, hy = py - y;
            const double d2 = hx * hx + hy * hy;
            if (d2 < 14400)
                local += 900.0 / (d2 + 100);
            if (d2 < hazard.radius * hazard.radius)
                local += 100;
        }
        for (unsigned i = 0; i < o.lasers.size(); ++i)
            local += laser_proposal_cost(lasers[samples * o.lasers.size() + i], x, y);
        const double edge = std::max(0.f, 40.f - std::min(x - 8.f, 376.f - x));
        const double alignment = power_aware && o.power < 128 ? .025 : damage_aware ? .02 : .003;
        result += local + std::abs(x - o.target_x) * alignment + std::abs(y - 380.f) * 0.006 +
                  edge * edge * 0.002;
        ++samples;
    }
    return result / samples + (dx || dy ? 0.02 : 0);
}
std::uint16_t reactive_input(const Observation &observation, bool power_aware, bool damage_aware) {
    constexpr std::array<std::uint16_t, 9> directions = {0,       16,       32,      64,      128,
                                                         16 | 64, 16 | 128, 32 | 64, 32 | 128};
    double best = std::numeric_limits<double>::infinity();
    std::uint16_t selected = 261;
    // Every direction shares one conditional forecast from this same native
    // branch state. Reuse geometry, never actor memory or another branch's RNG.
    const auto lasers = laser_samples(observation, 40, 8);
    for (const auto direction : directions)
        for (unsigned focus : {4u, 0u}) {
            if (!direction && !focus)
                continue;
            const auto input = std::uint16_t(direction | focus | 1u | 256u);
            const double cost = std::min(
                proposal_rank(observation, input, 16, 40, lasers, 8, power_aware, damage_aware),
                proposal_rank(observation, input, 40, 40, lasers, 8, power_aware, damage_aware));
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
               bool laser_aware, bool power_aware, bool damage_aware, bool acceleration_aware,
               const std::vector<std::uint16_t> *literal = nullptr, Orbit orbit = {}) {
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
            auto current = game.state();
            const auto initial_progress = damage_aware ? game.boss_progress() : BossProgress{};
            const bool score_damage =
                initial_progress.active && initial_progress.damageable && initial_progress.life;
            BossGoal goal(initial_progress);
            for (unsigned frame = 0; frame < horizon; ++frame) {
                auto start = profile ? Clock::now() : Clock::time_point{};
                if (literal)
                    action = (*literal)[frame];
                else if (orbit.radius > 0) {
                    if (frame % 8 == 0) {
                        const auto observed = game.observe(true);
                        if (const auto *laser = orbit_laser(observed)) {
                            orbit.x = laser->x;
                            orbit.y = laser->y;
                        }
                    }
                    action = orbit_input(orbit, current.x, current.y);
                } else if (reactive) {
                    // Future observations and their action-dependent RNG belong
                    // to this branch. Retention records the literal actions,
                    // including turns; it never repeats just the initial input.
                    if (frame >= commit && (frame - commit) % 16 == 0)
                        action = reactive_input(
                            game.observe(laser_aware, power_aware, acceleration_aware), power_aware,
                            damage_aware);
                } else
                    action = frame < moving ? input : std::uint16_t(input & ~std::uint16_t(0xf0));
                if (profile) {
                    result.timings.guidance += seconds_since(start);
                    start = Clock::now();
                }
                const auto state = game.step(action);
                current = state;
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
                result.cost += game.proposal_cost(laser_aware, power_aware);
                if (score_damage)
                    result.cost += goal.cost(game.boss_progress());
                if (profile)
                    result.timings.scoring += seconds_since(start);
                if (state.complete) {
                    result.complete = true;
                    break;
                }
            }
            if (result.safe) {
                const auto start = profile ? Clock::now() : Clock::time_point{};
                result.cost =
                    result.cost / result.safe + game.proposal_cost(laser_aware, power_aware);
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
Decision choose(Session &game, const PlannerOptions &options,
                const std::function<void(const Decision &)> &first_certified) {
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
    bool notified = false;
    unsigned considered = 0;
    const auto certify = [&] {
        if (!decision.certified_frames || !decision.expected_digests.empty())
            return;
        const auto start = options.profile ? Clock::now() : Clock::time_point{};
        const auto verified =
            rollout(game, decision.input, decision.certified_frames, decision.certified_frames,
                    options.commit, decision.certified_frames, false, options.profile, true,
                    options.laser_aware, options.power_aware, options.damage_aware,
                    options.acceleration_aware, &decision.expected_inputs);
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
    };
    std::vector<Candidate> candidates;
    if (options.laser_aware)
        game.enable_laser_guidance();
    const auto observation =
        game.observe(options.laser_aware, options.power_aware, options.acceleration_aware);
    const auto lasers = laser_samples(observation, options.horizon, 4);
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
                    {input, moving,
                     proposal_rank(observation, input, moving, options.horizon, lasers, 4,
                                   options.power_aware, options.damage_aware)});
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
    // A rotating long beam can outrun a distant player. Propose an inward
    // approach followed by a small orbit, instead of only held/stop motions.
    // These literal, multi-turn futures still require full native execution.
    if (options.laser_orbits)
        if (const auto *laser = orbit_laser(observation)) {
            std::vector<Candidate> orbits;
            const float reach = std::abs(laser->angular_speed) >= .002f
                                    ? observation.axis / std::abs(laser->angular_speed)
                                    : 80.f;
            for (float direction : {1.f, -1.f})
                for (float fraction : {.5f, .65f, .8f}) {
                    Orbit orbit{laser->x,
                                laser->y,
                                std::clamp(reach * fraction, 48.f, 128.f),
                                direction * (laser->angular_speed >= 0 ? 1.f : -1.f),
                                observation.axis,
                                observation.diagonal};
                    const auto input = orbit_input(orbit, observation.x, observation.y);
                    if (int(input) != options.excluded_input)
                        orbits.push_back(
                            {input, options.horizon, 0, false, orbit.radius, orbit.direction});
                }
            candidates.insert(candidates.begin(), orbits.begin(), orbits.end());
        }
    if (options.profile)
        decision.timings.ranking = seconds_since(start);
    std::array<bool, 512> evaluated{};
    for (const auto &candidate : candidates) {
        if (options.ranking_deadline != Clock::time_point::max() &&
            (best_complete || best_safe == options.horizon) &&
            Clock::now() >= options.ranking_deadline) {
            decision.ranking_yielded = true;
            break;
        }
        const auto input = candidate.input;
        const auto moving = candidate.moving;
        Orbit orbit;
        if (candidate.orbit_radius > 0) {
            const auto *laser = orbit_laser(observation);
            orbit = {laser->x,
                     laser->y,
                     candidate.orbit_radius,
                     candidate.orbit_direction,
                     observation.axis,
                     observation.diagonal};
        }
        const bool digests = !options.defer_later_digests || decision.branches == 0;
        start = options.profile ? Clock::now() : Clock::time_point{};
        const auto result =
            rollout(game, input, moving, options.horizon, options.commit,
                    std::max(options.commit, options.reuse), candidate.reactive, options.profile,
                    digests, options.laser_aware, options.power_aware, options.damage_aware,
                    options.acceleration_aware, nullptr, orbit);
        if (options.profile) {
            decision.timings.rollout_wall += seconds_since(start);
            decision.timings += result.timings;
        }
        ++decision.branches;
        ++considered;
        decision.unique_inputs += !evaluated[input];
        evaluated[input] = true;
        decision.reactive_branches += candidate.reactive;
        decision.orbital_branches += candidate.orbit_radius > 0;
        decision.simulated_frames += result.attempted;
        const double cost = result.cost + ((input & 0xf0) ? 0.02 : 0);
        if ((result.complete && !best_complete) ||
            (result.complete == best_complete &&
             (result.safe > best_safe || (result.safe == best_safe && cost < best_cost)))) {
            best_complete = result.complete;
            decision.future_complete = result.complete;
            decision.orbital_selected = candidate.orbit_radius > 0;
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
        if (!notified && first_certified && (best_complete || best_safe == options.horizon)) {
            certify();
            first_certified(decision);
            notified = true;
        }
        if (considered >= options.proposals && (best_complete || best_safe == options.horizon))
            break;
    }
    certify();
    return decision;
}
} // namespace th06::solver
