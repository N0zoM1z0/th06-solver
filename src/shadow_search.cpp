#include "th06/shadow_search.hpp"
#include "th06/planner.hpp"
#include "th06/shadow_recorded.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <poll.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>
namespace th06::shadow {
namespace b = th06::shadow;
using namespace solver;
namespace {
using Clock = std::chrono::steady_clock;
double milliseconds(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}
bool transfer(int fd, void *data, unsigned count, bool output) {
    auto *p = static_cast<char *>(data);
    while (count) {
        const auto n = output ? ::write(fd, p, count) : ::read(fd, p, count);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return false;
        count -= unsigned(n);
        p += n;
    }
    return true;
}
// This helper runs only inside the bounded search process group. A nominal
// failure must not change the common lead world used by fresh proposals.
Prediction nominal(Session &game, const b::Plan &active, unsigned origin, unsigned lead,
                   unsigned horizon, std::uint16_t previous, bool digests = true) {
    int pipefd[2];
    if (pipe(pipefd))
        throw std::runtime_error("nominal pipe");
    const auto pid = fork();
    if (pid < 0)
        throw std::runtime_error("nominal fork");
    if (!pid) {
        close(pipefd[0]);
        Prediction p;
        p.nominal = 1;
        try {
            for (unsigned i = 0; i < horizon; ++i) {
                previous = b::next_input(active, origin + lead + i + 1, previous);
                ++p.updates;
                const auto state = game.step(previous);
                if (state.hit)
                    break;
                p.inputs[i] = previous;
                if (digests)
                    p.digests[i] = game.observation_digest();
                ++p.count;
                if (state.complete)
                    break;
            }
            if (p.count != horizon && !game.state().complete)
                p.count = 0;
            transfer(pipefd[1], &p, sizeof(p), true);
            _exit(0);
        } catch (...) {
            _exit(1);
        }
    }
    close(pipefd[1]);
    Prediction p;
    const bool received = transfer(pipefd[0], &p, sizeof(p), false);
    close(pipefd[0]);
    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (!received || !WIFEXITED(status) || WEXITSTATUS(status))
        throw std::runtime_error("nominal rollout failed");
    return p;
}
} // namespace
Search search(Session &game, const b::Plan &active, std::uint16_t previous, unsigned lead,
              unsigned horizon, unsigned budget, bool reuse, bool pulse, unsigned cursor,
              bool recorded, unsigned proposals, bool laser_aware) {
    if (horizon < 8 || horizon + lead > capacity || !lead || !budget || budget > 2000 ||
        !proposals || proposals > 50)
        throw std::invalid_argument("invalid bounded search configuration");
    if (prctl(PR_SET_CHILD_SUBREAPER, 1))
        throw std::runtime_error("set child subreaper");
    if (laser_aware)
        game.enable_laser_guidance();
    const auto begin = Clock::now();
    struct Progress {
        std::uint32_t *words = static_cast<std::uint32_t *>(
            mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        ~Progress() {
            if (words != MAP_FAILED)
                munmap(words, 4096);
        }
    } progress;
    if (progress.words == MAP_FAILED)
        throw std::runtime_error("search progress map");
    b::save(progress.words, cursor % 66);
    int pipefd[2];
    // A sequenced packet keeps the complete larger certificate atomic. A
    // readable pipe prefix must not bypass the parent's whole-search deadline.
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, pipefd))
        throw std::runtime_error("search socket");
    const auto pid = fork();
    if (pid < 0)
        throw std::runtime_error("search fork");
    if (!pid) {
        setpgid(0, 0);
        close(pipefd[0]);
        try {
            const auto origin = unsigned(game.state().frame);
            Prediction result;
            bool safe = true;
            b::save(progress.words + 2, 1);
            for (unsigned i = 0; i < lead; ++i) {
                previous = b::next_input(active, origin + i + 1, previous);
                const auto state = game.step(previous);
                ++result.updates;
                if (state.hit || state.complete) {
                    safe = false;
                    break;
                }
                result.inputs[i] = previous;
                result.digests[i] = game.observation_digest();
            }
            if (safe) {
                Prediction tail;
                if (reuse && active.count && origin + lead < active.origin + active.count) {
                    b::save(progress.words + 2, 2);
                    tail = nominal(game, active, origin, lead, horizon, previous);
                    ++result.branches;
                    result.updates += tail.updates;
                }
                if (!tail.count && recorded && milliseconds(begin) < budget) {
                    std::vector<std::uint16_t> baseline;
                    auto held = previous;
                    for (unsigned i = 0; i < horizon; ++i) {
                        held = b::next_input(active, origin + lead + i + 1, held);
                        baseline.push_back(held);
                    }
                    const auto proposal = recorded_search(
                        game, baseline, std::max(1, int(budget - milliseconds(begin))),
                        recorded_beam_width, progress.words + 2);
                    tail = proposal.prediction;
                    result.branches += tail.branches;
                    result.updates += tail.updates;
                }
                if (!tail.count && pulse && active.count) {
                    b::save(progress.words + 2, 7);
                    // TH08's native portfolio: 18 held motions and 16/32/64
                    // pulses. Advance shared progress before each attempt so
                    // deadlines cannot repeatedly starve later directions.
                    constexpr std::uint16_t directions[]{0, 16, 32, 64, 128, 80, 144, 96, 160};
                    std::vector<std::pair<std::uint16_t, unsigned>> motions;
                    for (unsigned focus : {4u, 0u})
                        for (auto direction : directions) {
                            const auto input = std::uint16_t(direction | focus | 257u);
                            motions.emplace_back(input, horizon);
                            if (direction)
                                for (unsigned duration : {16u, 32u, 64u})
                                    motions.emplace_back(input, duration);
                        }
                    for (unsigned attempt = 0; attempt < motions.size(); ++attempt) {
                        const auto index = b::load(progress.words);
                        b::save(progress.words, (index + 1) % unsigned(motions.size()));
                        b::save(progress.words + 1, b::load(progress.words + 1) + 1);
                        const auto [input, duration] = motions[index];
                        b::Plan motion;
                        motion.origin = origin + lead;
                        motion.count = horizon;
                        for (unsigned i = 0; i < horizon; ++i)
                            motion.inputs[i] = i < duration ? input : std::uint16_t(input & ~0xf0u);
                        auto trial = nominal(game, motion, origin, lead, horizon, previous, false);
                        ++result.branches;
                        result.updates += trial.updates;
                        if (!trial.count)
                            continue;
                        tail = nominal(game, motion, origin, lead, horizon, previous, true);
                        ++result.branches;
                        result.updates += tail.updates;
                        if (tail.count != trial.count || tail.inputs != trial.inputs)
                            throw std::runtime_error("motion winner replay diverged");
                        tail.nominal = 0;
                        tail.motion = 1;
                        break;
                    }
                }
                if (!tail.count) {
                    b::save(progress.words + 2, 6);
                    PlannerOptions options;
                    options.horizon = horizon;
                    options.commit = 8;
                    options.reuse = horizon;
                    options.proposals = proposals;
                    options.reactive = true;
                    options.defer_later_digests = true;
                    options.laser_aware = laser_aware;
                    options.laser_orbits = laser_aware;
                    const auto decision = choose(game, options);
                    result.branches += unsigned(decision.branches);
                    result.updates += unsigned(decision.simulated_frames);
                    // A safe short prefix of a failed future never publishes.
                    if (decision.lookahead_frames == horizon || decision.future_complete) {
                        tail.count = decision.certified_frames;
                        tail.orbital = decision.orbital_selected;
                        std::copy(decision.expected_inputs.begin(), decision.expected_inputs.end(),
                                  tail.inputs.begin());
                        std::copy(decision.expected_digests.begin(),
                                  decision.expected_digests.end(), tail.digests.begin());
                    }
                }
                if (tail.count) {
                    result.count = lead + tail.count;
                    result.nominal = tail.nominal;
                    result.motion = tail.motion;
                    result.recorded = tail.recorded;
                    result.orbital = tail.orbital;
                    std::copy_n(tail.inputs.begin(), tail.count, result.inputs.begin() + lead);
                    std::copy_n(tail.digests.begin(), tail.count, result.digests.begin() + lead);
                }
            }
            transfer(pipefd[1], &result, sizeof(result), true);
            _exit(0);
        } catch (...) {
            _exit(1);
        }
    }
    close(pipefd[1]);
    setpgid(pid, pid);
    Search result;
    pollfd fd{pipefd[0], POLLIN, 0};
    int ready = 0;
    while (milliseconds(begin) < budget) {
        ready = poll(&fd, 1, std::max(1, int(budget - milliseconds(begin))));
        if (ready >= 0 || errno != EINTR)
            break;
    }
    bool received = false;
    if (ready > 0 && (fd.revents & POLLIN))
        received = transfer(pipefd[0], &result.prediction, sizeof(Prediction), false);
    if (!received) {
        result.timeout = ready == 0;
        kill(-pid, SIGKILL);
        kill(pid, SIGKILL);
    }
    close(pipefd[0]);
    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (!received) {
        // Adopted rollout descendants are reaped after the entire owned search
        // group is killed; repeated deadlines must not accumulate zombies.
        while (waitpid(-pid, &status, 0) > 0 || errno == EINTR) {
        }
    }
    result.wall_ms = milliseconds(begin);
    result.motion_attempts = b::load(progress.words + 1);
    result.motion_cursor = b::load(progress.words);
    result.last_phase = b::load(progress.words + 2);
    result.recorded_forecasts = b::load(progress.words + 3);
    result.recorded_checks = b::load(progress.words + 4);
    if ((!received && !result.timeout) || (received && (!WIFEXITED(status) || WEXITSTATUS(status))))
        throw std::runtime_error("shadow search child failed");
    return result;
}
} // namespace th06::shadow
