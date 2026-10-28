#include "th06/resources.hpp"
#include "th06/shadow_native.hpp"
#include "th06/shadow_search.hpp"
#include "th06/tape.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <poll.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace b = th06::shadow;
using namespace th06::solver;
namespace {
using Clock = std::chrono::steady_clock;
double milliseconds(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}
double percentile(std::vector<double> values, double fraction) {
    if (values.empty())
        return 0;
    std::sort(values.begin(), values.end());
    return values[std::min(values.size() - 1, std::size_t(fraction * (values.size() - 1)))];
}
struct Mapping {
    int fd = -1;
    std::uint32_t *words = nullptr;
    explicit Mapping(const std::string &path) {
        fd = open(path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd < 0 || ftruncate(fd, b::map_bytes))
            throw std::runtime_error("create bridge map");
        words = static_cast<std::uint32_t *>(
            mmap(nullptr, b::map_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        if (words == MAP_FAILED) {
            words = nullptr;
            throw std::runtime_error("mmap bridge");
        }
        b::save(words + b::Magic, b::magic);
        b::save(words + b::Version, b::version);
    }
    ~Mapping() {
        if (words) {
            b::save(words + b::StopRequested, 1);
            munmap(words, b::map_bytes);
        }
        if (fd >= 0)
            close(fd);
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        if (prctl(PR_SET_CHILD_SUBREAPER, 1))
            throw std::runtime_error("set child subreaper");
        std::string data, directory;
        unsigned horizon = 128, lead = 16, budget = 110, interval = 16;
        unsigned survival_horizon = 0;
        unsigned rescue_horizon = 0;
        unsigned refresh_updates = 0;
        bool pulse = false;
        bool pulse_on_failure = false;
        bool laser_aware = false;
        bool anytime = false;
        bool warm_start = false;
        bool save_certificates = false;
        bool power_aware = false;
        bool damage_aware = false;
        bool acceleration_aware = false;
        bool diverse_first = false;
        bool recorded = false;
        bool fresh = false;
        unsigned proposals = 1;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--save-certificates") {
                save_certificates = true;
                continue;
            }
            if (key == "--warm-start") {
                warm_start = true;
                continue;
            }
            if (key == "--diverse-first") {
                diverse_first = true;
                continue;
            }
            if (key == "--fresh") {
                fresh = true;
                continue;
            }
            if (key == "--recorded") {
                recorded = true;
                continue;
            }
            if (key == "--pulse") {
                pulse = true;
                continue;
            }
            if (key == "--pulse-on-failure") {
                pulse_on_failure = true;
                continue;
            }
            if (key == "--laser-aware") {
                laser_aware = true;
                continue;
            }
            if (key == "--anytime") {
                anytime = true;
                continue;
            }
            if (key == "--power-aware") {
                power_aware = true;
                continue;
            }
            if (key == "--acceleration-aware") {
                acceleration_aware = true;
                continue;
            }
            if (key == "--damage-aware") {
                damage_aware = true;
                continue;
            }
            if (key == "--help") {
                std::cout
                    << "th06_shadow_online --data DIR --directory DIR [--horizon 32..592] "
                       "[--lead 8..64] [--budget-ms N] [--interval N] [--proposals 1..50] "
                       "[--fresh] [--pulse | --pulse-on-failure] [--recorded] [--laser-aware] "
                       "[--anytime] [--warm-start] [--power-aware] [--damage-aware] "
                       "[--acceleration-aware] "
                       "[--survival-horizon N] [--rescue-horizon N] [--refresh-updates N] "
                       "[--diverse-first] "
                       "[--save-certificates]\n";
                return 0;
            }
            if (++i == argc)
                throw std::runtime_error("missing option value");
            const std::string value = argv[i];
            if (key == "--data")
                data = value;
            else if (key == "--directory")
                directory = value;
            else {
                std::size_t end = 0;
                const auto n = std::stoul(value, &end);
                if (end != value.size() || n > 10000)
                    throw std::runtime_error("invalid number");
                if (key == "--horizon")
                    horizon = n;
                else if (key == "--survival-horizon")
                    survival_horizon = n;
                else if (key == "--rescue-horizon")
                    rescue_horizon = n;
                else if (key == "--refresh-updates")
                    refresh_updates = n;
                else if (key == "--lead")
                    lead = n;
                else if (key == "--budget-ms")
                    budget = n;
                else if (key == "--interval")
                    interval = n;
                else if (key == "--proposals")
                    proposals = n;
                else
                    throw std::runtime_error("unknown option");
            }
        }
        if (data.empty() || directory.empty() || horizon < 32 || horizon > 592 || lead < 8 ||
            lead > 64 || horizon + lead > b::capacity || !budget || budget > 2000 || !interval ||
            interval > horizon || !proposals || proposals > 50)
            throw std::runtime_error("invalid shadow configuration");
        if (survival_horizon &&
            (survival_horizon < horizon || survival_horizon + lead > b::capacity))
            throw std::runtime_error("invalid survival horizon");
        if (rescue_horizon && (rescue_horizon < 32 || rescue_horizon > horizon))
            throw std::runtime_error("rescue horizon requires 32..base horizon");
        if (recorded && std::max(horizon, survival_horizon) > 240)
            throw std::runtime_error("recorded proposals require horizon <= 240");
        if (pulse && pulse_on_failure)
            throw std::runtime_error("choose one pulse policy");
        if (warm_start && !anytime)
            throw std::runtime_error("warm start requires anytime certificates");
        if (refresh_updates && (fresh || refresh_updates < interval || refresh_updates > horizon))
            throw std::runtime_error(
                "refresh interval requires non-fresh mode and interval..horizon");
        Mapping mapping(directory + "/bridge.bin");
        auto *map = mapping.words;
        Resources resources(data);
        set_resources(&resources);
        // Retail publishes its pre-stage-init RNG after its real initialization.
        // It keeps running while this process creates the common-start world.
        StartConfig config;
        unsigned story = 0, seed = 0;
        const auto startup = Clock::now();
        for (;;) {
            std::ifstream start(directory + "/start.txt");
            if (start >> config.stage >> config.difficulty >> config.shot >> seed >> config.power >>
                story)
                break;
            if (milliseconds(startup) > 30000)
                throw std::runtime_error("retail start timeout");
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if (!b::start_valid(config.stage, config.difficulty, config.shot, story) || seed > 65535)
            throw std::runtime_error("unsupported retail start");
        config.seed = std::uint16_t(seed);
        config.story = story;
        Session game(config);
        // Only actual receipts mutate the parent world. Cache its scalar state
        // while waiting; rescanning every actor/bullet pool on each 1-ms poll
        // adds CPU without producing a new observation.
        auto current = game.state();
        Tape tape;
        tape.start = config;
        tape.resource_fingerprint = resources.fingerprint();
        int journal = open((directory + "/receipts.bin").c_str(), O_RDONLY);
        if (journal < 0)
            throw std::runtime_error("open retail journal");
        std::ofstream decisions(directory + "/decisions.jsonl");
        std::ofstream certificates;
        if (save_certificates) {
            certificates.exceptions(std::ios::failbit | std::ios::badbit);
            certificates.open(directory + "/certificates.jsonl");
        }
        std::vector<double> times, gaps;
        unsigned timeouts = 0, attempts = 0, submitted = 0, stale = 0, failed = 0, reused = 0;
        unsigned motion_attempts = 0, motion_selected = 0, motion_cursor = 0;
        unsigned recovery_searches = 0, recovery_publications = 0;
        unsigned orbital_publications = 0;
        unsigned deadline_checkpoints = 0, checkpoint_publications = 0;
        unsigned ranking_yields = 0;
        unsigned warm_attempts = 0, warm_certificates = 0, warm_publications = 0;
        unsigned survival_searches = 0, survival_publications = 0;
        unsigned short_searches = 0, short_publications = 0;
        bool last_unpublished = false;
        bool recovery_armed = false;
        unsigned forecasts = 0, checks = 0, recorded_selected = 0;
        std::array<unsigned, 8> timeouts_by_phase{};
        std::uint64_t branches = 0, updates = 0, offset = 0, prior_time = 0;
        b::Plan active, pending;
        std::uint16_t previous = 261;
        struct History {
            unsigned frame = 0;
            std::uint16_t input = 0;
            std::uint64_t digest = 0;
        };
        std::array<History, 512> history{};
        const auto begin = Clock::now();
        auto last_receipt = begin;
        std::string error;
        const auto catch_up = [&]() {
            b::Receipt row;
            while (true) {
                const auto n = pread(journal, row.data(), sizeof(row), off_t(offset));
                if (n < 0 && errno == EINTR)
                    continue;
                if (n < 0)
                    throw std::runtime_error("retail journal read");
                if (n != sizeof(row))
                    break;
                offset += sizeof(row);
                if (row[b::Signature] != b::magic || row[b::WireVersion] != b::version ||
                    row[b::Frame] != current.frame + 1 || !b::input_valid(row[b::Input]))
                    throw std::runtime_error("invalid or nonconsecutive retail receipt");
                const auto state = game.step(std::uint16_t(row[b::Input]));
                current = state;
                const auto expected = b::project(game, std::uint16_t(row[b::Input]));
                const auto field = b::difference(expected, row);
                if (field != b::FieldCount) {
                    std::ofstream detail(directory + "/divergence.json");
                    detail << "{\"frame\":" << state.frame << ",\"field\":\""
                           << b::field_names[field] << "\",\"native\":" << expected[field]
                           << ",\"retail\":" << row[field] << "}\n";
                    throw std::runtime_error(std::string("retail divergence: ") +
                                             b::field_names[field]);
                }
                previous = std::uint16_t(row[b::Input]);
                const auto digest = game.observation_digest();
                history[state.frame % history.size()] = {unsigned(state.frame), previous, digest};
                tape.rows.push_back({previous, digest});
                const auto time =
                    std::uint64_t(row[b::TimeLow]) | (std::uint64_t(row[b::TimeHigh]) << 32);
                if (prior_time)
                    gaps.push_back(double(time - prior_time) / 1e6);
                prior_time = time;
                last_receipt = Clock::now();
            }
            if (pending.generation && b::load(map + b::Accepted) == pending.generation) {
                active = pending;
                pending = {};
            } else if (pending.generation && b::load(map + b::Rejected) == pending.generation)
                pending = {};
            // The owner discards its old queue on a natural stage handoff.
            // Mirror that epoch change before constructing the next fixed lead.
            const auto stage = unsigned(current.stage);
            if (active.stage != stage)
                active = {};
            if (pending.stage != stage)
                pending = {};
        };
        unsigned last_search = 0, last_refresh = 0;
        try {
            while (true) {
                catch_up();
                if (b::load(map + b::Outcome) != b::Running) {
                    catch_up();
                    if (b::load(map + b::Sequence) != current.frame)
                        throw std::runtime_error("incomplete final retail journal");
                    if (b::load(map + b::Outcome) == b::Complete && !current.complete)
                        throw std::runtime_error("retail terminal missing from native follow");
                    if (b::load(map + b::Outcome) == b::Failure)
                        throw std::runtime_error("retail bridge failed");
                    break;
                }
                if (milliseconds(last_receipt) > 10000)
                    throw std::runtime_error("retail receipt timeout");
                const auto state = current;
                if (!state.frame || pending.generation || state.complete ||
                    state.frame < last_search + interval) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                // Reconstruct the origin from actual receipts, then preserve
                // the accepted queue/fallback through the immutable lead.
                const auto origin = unsigned(state.frame);
                const auto queue_before = b::remaining(active, origin);
                const auto active_generation = active.generation;
                const auto cursor_before = motion_cursor;
                last_search = origin;
                const bool reuse = refresh_updates
                                       ? attempts && origin - last_refresh < refresh_updates
                                       : !fresh && attempts % 4 != 0;
                if (!reuse)
                    last_refresh = origin;
                const bool recovery = pulse_on_failure && recovery_armed;
                recovery_searches += recovery;
                const unsigned normal_horizon =
                    survival_horizon && game.survival_spell() ? survival_horizon : horizon;
                const bool previous_failed = last_unpublished;
                const unsigned selected_horizon = b::recovery_horizon(
                    normal_horizon, rescue_horizon, lead, queue_before, previous_failed);
                const bool shortened = selected_horizon < normal_horizon;
                const bool extended = selected_horizon > horizon;
                short_searches += shortened;
                survival_searches += extended;
                const auto result = b::search(
                    game, active, previous, lead, selected_horizon, budget, reuse,
                    pulse || recovery, motion_cursor, recorded, proposals, laser_aware, anytime,
                    power_aware, damage_aware, acceleration_aware, diverse_first, warm_start);
                warm_attempts += result.warm_attempted;
                warm_certificates += result.warm_certified;
                deadline_checkpoints += result.deadline_checkpoint;
                if (result.prediction.count && !result.prediction.nominal)
                    last_refresh = origin;
                ranking_yields += result.prediction.ranking_yielded;
                // Try motion once after an unsuccessful ordinary search, then
                // restore ordinary proposals even when that rescue also fails.
                recovery_armed = pulse_on_failure && !recovery &&
                                 (!result.prediction.count || result.warm_selected);
                forecasts += result.recorded_forecasts;
                checks += result.recorded_checks;
                if (result.timeout && result.last_phase < timeouts_by_phase.size())
                    ++timeouts_by_phase[result.last_phase];
                motion_attempts += result.motion_attempts;
                motion_cursor = result.motion_cursor;
                ++attempts;
                times.push_back(result.wall_ms);
                timeouts += result.timeout;
                branches += result.prediction.branches;
                updates += result.prediction.updates;
                catch_up();
                const auto now = current;
                bool matches = result.prediction.count && now.stage == state.stage &&
                               now.frame < origin + lead - 1;
                for (auto f = origin + 1; matches && f <= now.frame; ++f) {
                    const auto &h = history[f % history.size()];
                    const auto i = f - origin - 1;
                    matches = h.frame == f && h.input == result.prediction.inputs[i] &&
                              h.digest == result.prediction.digests[i];
                }
                bool published = false;
                if (matches && b::load(map + b::Outcome) == b::Running) {
                    pending = {};
                    pending.generation = (submitted + 1) * 2;
                    pending.origin = origin;
                    pending.stage = state.stage;
                    pending.first_change = origin + lead + 1;
                    pending.count = result.prediction.count;
                    pending.inputs = result.prediction.inputs;
                    b::publish(map, pending);
                    ++submitted;
                    published = true;
                    reused += result.prediction.nominal;
                    motion_selected += result.prediction.motion;
                    recorded_selected += result.prediction.recorded;
                    recovery_publications += recovery;
                    orbital_publications += result.prediction.orbital;
                    checkpoint_publications += result.deadline_checkpoint;
                    survival_publications += extended;
                    warm_publications += result.warm_selected;
                    short_publications += shortened;
                    if (save_certificates) {
                        certificates << "{\"version\":1,\"generation\":" << pending.generation
                                     << ",\"origin\":" << pending.origin
                                     << ",\"stage\":" << pending.stage
                                     << ",\"first_change\":" << pending.first_change
                                     << ",\"count\":" << pending.count << ",\"lead\":" << lead
                                     << ",\"horizon\":" << selected_horizon << ",\"inputs\":[";
                        for (unsigned i = 0; i < pending.count; ++i)
                            certificates << (i ? "," : "") << pending.inputs[i];
                        certificates << "],\"digests\":[";
                        for (unsigned i = 0; i < pending.count; ++i)
                            certificates << (i ? "," : "") << result.prediction.digests[i];
                        certificates << "]}\n";
                        certificates.flush();
                    }
                } else if (result.prediction.count)
                    ++stale;
                else if (!result.timeout)
                    ++failed;
                last_unpublished = !published;
                decisions << "{\"origin\":" << origin << ",\"caught_up\":" << now.frame
                          << ",\"normal_horizon\":" << normal_horizon
                          << ",\"previous_replacement_failed\":"
                          << (previous_failed ? "true" : "false")
                          << ",\"shortened\":" << (shortened ? "true" : "false")
                          << ",\"horizon\":" << selected_horizon
                          << ",\"active_generation\":" << active_generation
                          << ",\"published_generation\":" << (published ? pending.generation : 0)
                          << ",\"motion_cursor_before\":" << cursor_before
                          << ",\"motion_cursor_after\":" << motion_cursor
                          << ",\"warm_attempted\":" << (result.warm_attempted ? "true" : "false")
                          << ",\"warm_certified\":" << (result.warm_certified ? "true" : "false")
                          << ",\"warm_selected\":" << (result.warm_selected ? "true" : "false")
                          << ",\"warm_checkpoint\":" << (result.warm_checkpoint ? "true" : "false")
                          << ",\"nominal_attempt\":" << (reuse ? "true" : "false")
                          << ",\"nominal\":" << (result.prediction.nominal ? "true" : "false")
                          << ",\"wall_ms\":" << result.wall_ms
                          << ",\"cleanup_ms\":" << result.cleanup_ms
                          << ",\"completion_ms\":" << result.completion_ms
                          << ",\"late_completion\":" << (result.late_completion ? "true" : "false")
                          << ",\"last_phase\":" << result.last_phase
                          << ",\"queue_before\":" << queue_before
                          << ",\"queue_after\":" << b::remaining(active, unsigned(now.frame))
                          << ",\"consumed_lead\":" << now.frame - origin
                          << ",\"owner_sequence\":" << b::load(map + b::Sequence)
                          << ",\"fallback_total\":" << b::load(map + b::Fallback)
                          << ",\"result_reason\":\""
                          << (published                  ? "published"
                              : now.stage != state.stage ? "stage_reset"
                              : result.prediction.count  ? "stale"
                              : result.timeout           ? "timeout"
                                                         : "no_certificate")
                          << "\""
                          << ",\"timeout\":" << (result.timeout ? "true" : "false")
                          << ",\"certified\":" << result.prediction.count
                          << ",\"unique_inputs\":" << result.prediction.unique_inputs
                          << ",\"pulse_recovery\":" << (recovery ? "true" : "false")
                          << ",\"orbital\":" << (result.prediction.orbital ? "true" : "false")
                          << ",\"deadline_checkpoint\":"
                          << (result.deadline_checkpoint ? "true" : "false")
                          << ",\"ranking_yielded\":"
                          << (result.prediction.ranking_yielded ? "true" : "false")
                          << ",\"published\":" << (published ? "true" : "false") << "}\n";
                decisions.flush();
            }
        } catch (const std::exception &e) {
            error = e.what();
            b::save(map + b::StopRequested, 1);
        }
        close(journal);
        const auto state = game.state();
        tape.complete = state.complete && !state.hit && error.empty();
        write_tape(directory + "/actual.actions", tape);
        std::ofstream report(directory + "/controller.json");
        rusage self{}, children{};
        if (getrusage(RUSAGE_SELF, &self) || getrusage(RUSAGE_CHILDREN, &children))
            throw std::runtime_error("read controller CPU usage");
        const auto cpu = [](const rusage &r) {
            return r.ru_utime.tv_sec + r.ru_stime.tv_sec +
                   (r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1e6;
        };
        report << std::boolalpha << std::setprecision(10) << "{\"profile\":\"native-shadow-h"
               << horizon << "\",\"frames\":" << state.frame << ",\"complete\":" << tape.complete
               << ",\"error\":\"" << error << "\",\"horizon\":" << horizon << ",\"lead\":" << lead
               << ",\"budget_ms\":" << budget << ",\"interval\":" << interval
               << ",\"fresh_only\":" << fresh << ",\"proposals\":" << proposals
               << ",\"refresh_updates\":" << refresh_updates << ",\"searches\":" << attempts
               << ",\"warm_start\":" << warm_start << ",\"warm_attempts\":" << warm_attempts
               << ",\"warm_certificates\":" << warm_certificates
               << ",\"warm_publications\":" << warm_publications
               << ",\"save_certificates\":" << save_certificates << ",\"timeouts\":" << timeouts
               << ",\"failed\":" << failed << ",\"published\":" << submitted
               << ",\"accepted_generation\":" << b::load(map + b::Accepted)
               << ",\"rejected_generation\":" << b::load(map + b::Rejected)
               << ",\"stale\":" << stale << ",\"nominal_publications\":" << reused
               << ",\"recorded\":" << recorded << ",\"recorded_forecasts\":" << forecasts
               << ",\"recorded_checks\":" << checks
               << ",\"recorded_publications\":" << recorded_selected
               << ",\"timeouts_lead\":" << timeouts_by_phase[1]
               << ",\"timeouts_literal\":" << timeouts_by_phase[2]
               << ",\"timeouts_recording\":" << timeouts_by_phase[3]
               << ",\"timeouts_geometry\":" << timeouts_by_phase[4]
               << ",\"timeouts_recorded_verification\":" << timeouts_by_phase[5]
               << ",\"timeouts_heuristic\":" << timeouts_by_phase[6]
               << ",\"timeouts_motion\":" << timeouts_by_phase[7] << ",\"pulse\":" << pulse
               << ",\"pulse_on_failure\":" << pulse_on_failure
               << ",\"recovery_searches\":" << recovery_searches
               << ",\"recovery_publications\":" << recovery_publications
               << ",\"laser_aware\":" << laser_aware
               << ",\"orbital_publications\":" << orbital_publications << ",\"anytime\":" << anytime
               << ",\"power_aware\":" << power_aware << ",\"damage_aware\":" << damage_aware
               << ",\"acceleration_aware\":" << acceleration_aware
               << ",\"diverse_first\":" << diverse_first
               << ",\"deadline_checkpoints\":" << deadline_checkpoints
               << ",\"checkpoint_publications\":" << checkpoint_publications
               << ",\"ranking_yields\":" << ranking_yields
               << ",\"survival_horizon\":" << survival_horizon
               << ",\"rescue_horizon\":" << rescue_horizon
               << ",\"short_searches\":" << short_searches
               << ",\"short_publications\":" << short_publications
               << ",\"survival_searches\":" << survival_searches
               << ",\"survival_publications\":" << survival_publications
               << ",\"motion_attempts\":" << motion_attempts
               << ",\"motion_publications\":" << motion_selected
               << ",\"motion_cursor\":" << motion_cursor
               << ",\"completed_search_branches\":" << branches
               << ",\"completed_search_updates\":" << updates
               << ",\"interrupted_search_work_excluded\":true,\"planned_inputs\":"
               << b::load(map + b::Planned) << ",\"fallback_inputs\":" << b::load(map + b::Fallback)
               << ",\"search_p50_ms\":" << percentile(times, .5)
               << ",\"search_p99_ms\":" << percentile(times, .99)
               << ",\"search_max_ms\":" << percentile(times, 1)
               << ",\"receipt_gap_p99_ms\":" << percentile(gaps, .99)
               << ",\"receipt_gap_max_ms\":" << percentile(gaps, 1)
               << ",\"search_wall_total_ms\":" << std::accumulate(times.begin(), times.end(), 0.0)
               << ",\"search_over_budget\":"
               << std::count_if(times.begin(), times.end(), [&](double t) { return t > budget; })
               << ",\"controller_cpu_seconds\":" << cpu(self)
               << ",\"children_cpu_seconds\":" << cpu(children)
               << ",\"wall_seconds\":" << milliseconds(begin) / 1000 << "}\n";
        std::cout << "shadow frames=" << state.frame << " complete=" << tape.complete
                  << " searches=" << attempts << " timeouts=" << timeouts << " error=" << error
                  << '\n';
        return error.empty() ? 0 : 1;
    } catch (const std::exception &e) {
        std::cerr << "th06_shadow_online: " << e.what() << '\n';
        return 1;
    }
}
