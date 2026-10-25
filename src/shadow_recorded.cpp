// Conditional recording and spatial diversity follow th08-solver 969ae466 (MIT).
#include "th06/shadow_recorded.hpp"
#include "Player.hpp"
#include "stdafx.h"
#include "th06/native_geometry.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace th06::shadow {
namespace {
using namespace solver;
using Clock = std::chrono::steady_clock;
constexpr unsigned max_frames = 240, max_tapes = 5;
constexpr unsigned grid_w = 24, grid_h = 28, grid_size = grid_w * grid_h;
constexpr std::size_t max_index_items = 8 * 1024 * 1024;
struct Tape {
    GeometryFrame frames[max_frames];
    unsigned count, attempted;
    bool hit, complete, fault;
    Prediction prediction;
};
double elapsed(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}
void capture(Session &game, Tape &tape, const std::vector<std::uint16_t> &inputs, bool hashes) {
    tape.count = tape.attempted = 0;
    tape.hit = tape.complete = tape.fault = false;
    tape.prediction = {};
    const auto pid = fork();
    if (pid < 0)
        throw std::runtime_error("geometry capture fork");
    if (!pid) {
        try {
            for (unsigned i = 0; i < inputs.size(); ++i) {
                auto &frame = tape.frames[i];
                frame.box_count = frame.laser_count = 0;
                frame.overflow = false;
                frame.movement = false;
                frame.vulnerable = th06::g_Player.playerState == th06::PLAYER_STATE_ALIVE;
                frame.multiplier_x = frame.multiplier_y = frame.frame_multiplier = 1.f;
                geometry_frame = &frame;
                ++tape.attempted;
                const auto state = game.step(inputs[i]);
                geometry_frame = nullptr;
                tape.count = i + 1;
                if (frame.overflow)
                    throw std::runtime_error("geometry capacity exceeded");
                if (state.hit) {
                    tape.hit = true;
                    break;
                }
                tape.prediction.inputs[i] = inputs[i];
                if (hashes)
                    tape.prediction.digests[i] = game.observation_digest();
                if (state.complete) {
                    tape.complete = true;
                    break;
                }
            }
            if (!tape.hit && (tape.count == inputs.size() || tape.complete))
                tape.prediction.count = tape.count;
            _exit(0);
        } catch (...) {
            tape.fault = true;
            _exit(1);
        }
    }
    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) || tape.fault)
        throw std::runtime_error("native geometry recording failed");
}
struct Bounds {
    float x0, y0, x1, y1;
};
struct LaserView {
    GeometryLaser shape;
    float cosine, sine;
};
struct Grid {
    bool movement = true, vulnerable = true;
    float multiplier_x = 1, multiplier_y = 1, frame_multiplier = 1;
    std::vector<Bounds> boxes;
    std::vector<unsigned> starts, items;
    std::vector<LaserView> lasers;
};
int cell(float value, unsigned n) {
    if (value <= 0)
        return 0;
    if (value >= 16.f * (n - 1))
        return int(n) - 1;
    return int(value / 16.f);
}
void build(const GeometryFrame &frame, Grid &grid, float halfx, float halfy,
           std::size_t &remaining_items) {
    grid.movement = frame.movement;
    grid.vulnerable = frame.vulnerable;
    grid.multiplier_x = frame.multiplier_x;
    grid.multiplier_y = frame.multiplier_y;
    grid.frame_multiplier = frame.frame_multiplier;
    grid.boxes.reserve(frame.box_count);
    for (unsigned i = 0; i < frame.box_count; ++i) {
        const auto &b = frame.boxes[i];
        if (!std::isfinite(b.x) || !std::isfinite(b.y) || !std::isfinite(b.width) ||
            !std::isfinite(b.height))
            throw std::runtime_error("nonfinite recorded box");
        grid.boxes.push_back(
            {b.x - b.width / 2.f, b.y - b.height / 2.f, b.x + b.width / 2.f, b.y + b.height / 2.f});
    }
    grid.starts.assign(grid_size + 1, 0);
    const auto each = [&](const Bounds &b, auto &&fn) {
        const auto x0 = b.x0 - halfx - 1.51f, x1 = b.x1 + halfx + 1.51f;
        const auto y0 = b.y0 - halfy - 1.51f, y1 = b.y1 + halfy + 1.51f;
        if (x0 > x1 || y0 > y1 || x1 < 0 || y1 < 0 || x0 > 384 || y0 > 448)
            return;
        for (int y = cell(y0, grid_h); y <= cell(y1, grid_h); ++y)
            for (int x = cell(x0, grid_w); x <= cell(x1, grid_w); ++x)
                fn(unsigned(y) * grid_w + unsigned(x));
    };
    for (const auto &b : grid.boxes)
        each(b, [&](unsigned c) { ++grid.starts[c + 1]; });
    std::partial_sum(grid.starts.begin(), grid.starts.end(), grid.starts.begin());
    // A single large box can cover every cell. Bound the combined index over
    // all frames and refinements before allocating its duplicated entries.
    if (grid.starts.back() > remaining_items)
        throw std::runtime_error("recorded geometry index capacity exceeded");
    remaining_items -= grid.starts.back();
    grid.items.resize(grid.starts.back());
    auto position = grid.starts;
    for (unsigned i = 0; i < grid.boxes.size(); ++i)
        each(grid.boxes[i], [&](unsigned c) { grid.items[position[c]++] = i; });
    for (unsigned i = 0; i < frame.laser_count; ++i) {
        const auto &l = frame.lasers[i];
        for (float value : {l.x, l.y, l.width, l.height, l.origin_x, l.origin_y, l.angle})
            if (!std::isfinite(value))
                throw std::runtime_error("nonfinite recorded laser");
        grid.lasers.push_back({l, std::cos(l.angle), std::sin(l.angle)});
    }
}
bool overlaps(const Bounds &b, float x, float y, float hx, float hy) {
    return !(x - hx > b.x1 || x + hx < b.x0 || y - hy > b.y1 || y + hy < b.y0);
}
int probe(const Grid &grid, float x, float y, float hx, float hy) {
    if (grid.starts.empty() || !grid.vulnerable)
        return 0;
    const auto c = unsigned(cell(y, grid_h)) * grid_w + unsigned(cell(x, grid_w));
    int near = 0;
    for (unsigned i = grid.starts[c]; i < grid.starts[c + 1]; ++i) {
        const auto &b = grid.boxes[grid.items[i]];
        if (overlaps(b, x, y, hx, hy))
            return 2;
        if (!near && overlaps(b, x, y, hx + 1.5f, hy + 1.5f))
            near = 1;
    }
    for (const auto &v : grid.lasers) {
        const auto &l = v.shape;
        // TH06 utils::Rotate is clockwise: x'=c*x+s*y, y'=c*y-s*x.
        const auto dx = x - l.origin_x, dy = y - l.origin_y;
        const auto px = v.cosine * dx + v.sine * dy + l.origin_x;
        const auto py = v.cosine * dy - v.sine * dx + l.origin_y;
        const Bounds b{l.x - l.width / 2.f, l.y - l.height / 2.f, l.x + l.width / 2.f,
                       l.y + l.height / 2.f};
        if (overlaps(b, px, py, hx, hy))
            return 2;
        if (!near && overlaps(b, px, py, hx + 1.5f, hy + 1.5f))
            near = 1;
    }
    return near;
}
struct Node {
    float x, y, cost, rank;
    int parent;
    std::uint16_t input;
};
struct Layer {
    unsigned start, span;
    std::vector<Node> nodes;
};
struct Seen {
    std::uint32_t key = 0, generation = 0, index = 0;
};
std::vector<std::vector<std::uint16_t>> proposals(const std::vector<std::vector<Grid>> &grids,
                                                  unsigned horizon, unsigned beam,
                                                  const Observation &o, float hx, float hy,
                                                  float goalx, float goaly,
                                                  Clock::time_point deadline, unsigned &expanded) {
    constexpr std::uint16_t directions[]{0, 16, 32, 64, 128, 80, 144, 96, 160};
    std::vector<Seen> seen(32768);
    std::vector<Layer> layers;
    layers.push_back({0, 0, {{o.x, o.y, 0, 0, -1, 261}}});
    unsigned generation = 0;
    for (unsigned start = 0; start < horizon;) {
        if (Clock::now() >= deadline)
            return {};
        const unsigned span = std::min(start < 16 ? 2u : 4u, horizon - start);
        const auto &previous = layers.back().nodes;
        std::vector<Node> candidates;
        candidates.reserve(previous.size() * 18);
        ++generation;
        for (unsigned p = 0; p < previous.size(); ++p) {
            if ((p % 16) == 0 && Clock::now() >= deadline)
                return {};
            const auto &node = previous[p];
            for (unsigned focus : {4u, 0u})
                for (auto direction : directions) {
                    const auto input = std::uint16_t(direction | focus | 257u);
                    const int dx = bool(input & 128) - bool(input & 64),
                              dy = bool(input & 32) - bool(input & 16);
                    const float speed = dx && dy ? (focus ? o.focused_diagonal : o.diagonal)
                                                 : (focus ? o.focused_axis : o.axis);
                    float x = node.x, y = node.y,
                          cost = node.cost + (node.input != input ? .035f : 0.f);
                    bool safe = true;
                    for (unsigned k = 0; k < span; ++k) {
                        const auto &context = grids.back()[start + k];
                        if (context.movement) {
                            x = std::clamp(x + dx * speed * context.multiplier_x *
                                                   context.frame_multiplier,
                                           8.f, 376.f);
                            y = std::clamp(y + dy * speed * context.multiplier_y *
                                                   context.frame_multiplier,
                                           16.f, 432.f);
                        }
                        int danger = 0;
                        for (const auto &tape : grids) {
                            danger = std::max(danger, probe(tape[start + k], x, y, hx, hy));
                            if (danger == 2)
                                break;
                        }
                        if (danger == 2) {
                            safe = false;
                            break;
                        }
                        cost += danger ? 1.f : 0.f;
                    }
                    ++expanded;
                    if (!safe)
                        continue;
                    const float edge =
                        std::max(0.f, 24.f - std::min({x - 8.f, 376.f - x, y - 16.f, 432.f - y}));
                    const auto rank = cost + std::abs(x - goalx) * .008f +
                                      std::abs(y - goaly) * .012f + edge * .02f;
                    const auto key =
                        ((unsigned(x * 2.f + .5f) << 11) | unsigned(y * 2.f + .5f)) * 2 +
                        bool(focus);
                    auto slot = (key * 2654435761u) & 32767u;
                    while (seen[slot].generation == generation && seen[slot].key != key)
                        slot = (slot + 1) & 32767u;
                    if (seen[slot].generation == generation) {
                        auto &old = candidates[seen[slot].index];
                        if (rank < old.rank)
                            old = {x, y, cost, rank, int(p), input};
                    } else {
                        seen[slot] = {key, generation, unsigned(candidates.size())};
                        candidates.push_back({x, y, cost, rank, int(p), input});
                    }
                }
        }
        if (candidates.empty())
            return {};
        std::stable_sort(candidates.begin(), candidates.end(),
                         [](const Node &a, const Node &b) { return a.rank < b.rank; });
        std::array<bool, grid_size * 2> occupied{};
        std::vector<bool> selected(candidates.size());
        Layer next{start, span, {}};
        next.nodes.reserve(beam);
        for (unsigned i = 0; i < candidates.size() && next.nodes.size() < beam; ++i) {
            const auto &n = candidates[i];
            const auto key =
                (unsigned(cell(n.y, grid_h)) * grid_w + unsigned(cell(n.x, grid_w))) * 2 +
                bool(n.input & 4);
            if (!occupied[key]) {
                occupied[key] = true;
                selected[i] = true;
                next.nodes.push_back(n);
            }
        }
        for (unsigned i = 0; i < candidates.size() && next.nodes.size() < beam; ++i)
            if (!selected[i])
                next.nodes.push_back(candidates[i]);
        layers.push_back(std::move(next));
        start += span;
    }
    auto &last = layers.back().nodes;
    std::vector<unsigned> order(last.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(),
                     [&](unsigned a, unsigned b) { return last[a].rank < last[b].rank; });
    std::array<bool, grid_size * 2> occupied{};
    std::vector<std::vector<std::uint16_t>> result;
    for (auto index : order) {
        const auto &n = last[index];
        const auto key = (unsigned(cell(n.y, grid_h)) * grid_w + unsigned(cell(n.x, grid_w))) * 2 +
                         bool(n.input & 4);
        if (occupied[key])
            continue;
        occupied[key] = true;
        std::vector<std::uint16_t> inputs(horizon);
        int p = int(index);
        for (unsigned l = layers.size() - 1; l > 0; --l) {
            const auto &layer = layers[l];
            const auto &node = layer.nodes[p];
            std::fill_n(inputs.begin() + layer.start, layer.span, node.input);
            p = node.parent;
        }
        result.push_back(std::move(inputs));
        if (result.size() == 4)
            break;
    }
    return result;
}
} // namespace
RecordedResult recorded_search(solver::Session &game, const std::vector<std::uint16_t> &nominal,
                               unsigned budget_ms, unsigned beam, std::uint32_t *progress) {
    if (nominal.empty() || nominal.size() > max_frames || !beam || beam > 512)
        throw std::invalid_argument("invalid recorded search bounds");
    RecordedResult result;
    if (!budget_ms)
        return result;
    const auto deadline = Clock::now() + std::chrono::milliseconds(budget_ms);
    struct Storage {
        Tape *tapes =
            static_cast<Tape *>(mmap(nullptr, sizeof(Tape) * max_tapes, PROT_READ | PROT_WRITE,
                                     MAP_SHARED | MAP_ANONYMOUS, -1, 0));
        ~Storage() {
            if (tapes != MAP_FAILED)
                munmap(tapes, sizeof(Tape) * max_tapes);
        }
    } storage;
    if (storage.tapes == MAP_FAILED)
        throw std::runtime_error("recorded tape allocation");
    if (progress) {
        save(progress, 3);
        save(progress + 1, load(progress + 1) + 1);
    }
    auto begin = Clock::now();
    capture(game, storage.tapes[0], nominal, false);
    ++result.forecasts;
    result.updates += storage.tapes[0].attempted;
    result.capture_ms += elapsed(begin);
    const auto observation = game.observe();
    const auto hx = th06::g_Player.hitboxSize.x, hy = th06::g_Player.hitboxSize.y;
    std::vector<std::vector<Grid>> grids;
    std::size_t remaining_items = max_index_items;
    unsigned witness = 0;
    for (unsigned refinement = 0; refinement < 3 && Clock::now() < deadline; ++refinement) {
        const auto &record = storage.tapes[witness];
        std::vector<Grid> index(nominal.size());
        if (progress)
            save(progress, 4);
        begin = Clock::now();
        for (unsigned t = 0; t < record.count; ++t)
            build(record.frames[t], index[t], hx, hy, remaining_items);
        grids.push_back(std::move(index));
        const float goalx =
            refinement == 0 ? observation.target_x : (observation.x < 192.f ? 336.f : 48.f);
        const float goaly = refinement == 0 ? 380.f : 240.f;
        auto paths = proposals(grids, nominal.size(), beam, observation, hx, hy, goalx, goaly,
                               deadline, result.nodes);
        result.proposal_ms += elapsed(begin);
        if (paths.empty())
            break;
        bool failed = false;
        unsigned slot = 1, deepest = 0, next_witness = 1;
        for (const auto &path : paths) {
            if (Clock::now() >= deadline)
                break;
            if (progress) {
                save(progress, 5);
                save(progress + 2, load(progress + 2) + 1);
            }
            begin = Clock::now();
            capture(game, storage.tapes[slot], path, false);
            ++result.checks;
            result.updates += storage.tapes[slot].attempted;
            result.verification_ms += elapsed(begin);
            if (storage.tapes[slot].prediction.count) {
                const auto count = storage.tapes[slot].prediction.count;
                const auto complete = storage.tapes[slot].complete;
                if (progress)
                    save(progress + 2, load(progress + 2) + 1);
                begin = Clock::now();
                capture(game, storage.tapes[slot], path, true);
                ++result.checks;
                result.updates += storage.tapes[slot].attempted;
                result.verification_ms += elapsed(begin);
                if (storage.tapes[slot].prediction.count != count ||
                    storage.tapes[slot].complete != complete)
                    throw std::runtime_error("recorded winner replay mismatch");
                result.prediction = storage.tapes[slot].prediction;
                result.prediction.recorded = 1;
                result.prediction.branches = result.forecasts + result.checks;
                result.prediction.updates = result.updates;
                return result;
            }
            failed = true;
            if (storage.tapes[slot].count > deepest) {
                deepest = storage.tapes[slot].count;
                next_witness = slot;
            }
            ++slot;
        }
        if (!failed)
            break;
        // All already computed endpoint-diverse paths receive native checks
        // before paying for another geometry search. Retain the deepest
        // failed conditional future as the next refinement witness.
        witness = next_witness;
    }
    result.prediction.branches = result.forecasts + result.checks;
    result.prediction.updates = result.updates;
    return result;
}
} // namespace th06::shadow
