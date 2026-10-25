#pragma once
// Plan transport adapted from th08-solver 969ae466 (MIT, N0zoM1z0, 2026).
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace th06::shadow {
constexpr std::uint32_t magic = 0x53483036, version = 1, capacity = 256;
enum Header : unsigned {
    Magic,
    Version,
    Published,
    Accepted,
    Rejected,
    Sequence,
    Planned,
    Fallback,
    Outcome,
    StopRequested,
    RejectionReason,
    Ready
};
enum OutcomeCode : unsigned { Running, Complete, Death, Budget, Failure, Stopped };
constexpr unsigned slot_words = 8 + capacity, map_words = 32 + 2 * slot_words;
constexpr unsigned map_bytes = 4 * map_words;
static_assert(__atomic_always_lock_free(4, nullptr));
inline std::uint32_t load(const std::uint32_t *p) {
    return __atomic_load_n(p, __ATOMIC_ACQUIRE);
}
inline void save(std::uint32_t *p, std::uint32_t value) {
    __atomic_store_n(p, value, __ATOMIC_RELEASE);
}
inline bool input_valid(unsigned input) {
    return !(input & ~0x1f5u);
}
struct Plan {
    std::uint32_t generation = 0, origin = 0, stage = 0, first_change = 0, count = 0;
    std::array<std::uint16_t, capacity> inputs{};
};
inline bool valid(const Plan &p) {
    return p.generation && !(p.generation & 1) && p.count && p.count <= capacity &&
           p.origin <= UINT32_MAX - p.count && p.first_change > p.origin &&
           p.first_change <= p.origin + p.count && p.stage >= 1 && p.stage <= 6 &&
           std::all_of(p.inputs.begin(), p.inputs.begin() + p.count, input_valid);
}
inline unsigned slot(unsigned generation) {
    return 32 + ((generation / 2) % 2) * slot_words;
}
inline void publish(std::uint32_t *map, const Plan &p) {
    if (!valid(p) || p.generation <= load(map + Published))
        throw std::runtime_error("invalid shadow plan");
    auto *dst = map + slot(p.generation);
    save(dst, p.generation | 1);
    save(dst + 1, p.origin);
    save(dst + 2, p.stage);
    save(dst + 3, p.first_change);
    save(dst + 4, p.count);
    for (unsigned i = 0; i < p.count; ++i)
        save(dst + 8 + i, p.inputs[i]);
    save(dst, p.generation);
    save(map + Published, p.generation);
}
inline bool read(const std::uint32_t *map, Plan &out) {
    const auto g = load(map + Published);
    if (!g || (g & 1))
        return false;
    const auto *src = map + slot(g);
    if (load(src) != g)
        return false;
    Plan p;
    p.generation = g;
    p.origin = load(src + 1);
    p.stage = load(src + 2);
    p.first_change = load(src + 3);
    p.count = load(src + 4);
    if (!p.count || p.count > capacity)
        return false;
    for (unsigned i = 0; i < p.count; ++i) {
        const auto value = load(src + 8 + i);
        if (value > UINT16_MAX)
            return false;
        p.inputs[i] = std::uint16_t(value);
    }
    if (load(src) != g || load(map + Published) != g || !valid(p))
        return false;
    out = p;
    return true;
}
inline std::uint16_t fallback(std::uint16_t previous) {
    return std::uint16_t((previous & 0xf4) | 0x101);
}
inline std::uint16_t next_input(const Plan &p, unsigned next, std::uint16_t previous) {
    if (p.count && next > p.origin && next <= p.origin + p.count)
        return p.inputs[next - p.origin - 1];
    return fallback(previous);
}
inline bool timely(const Plan &p, unsigned sequence, unsigned stage) {
    return valid(p) && p.stage == stage && p.origin <= sequence && sequence < UINT32_MAX &&
           sequence + 1 < p.first_change;
}
inline bool compatible(const Plan &p, const Plan &active, std::uint16_t previous, unsigned sequence,
                       unsigned stage) {
    if (!timely(p, sequence, stage))
        return false;
    for (unsigned next = sequence + 1; next < p.first_change; ++next) {
        previous = next_input(active, next, previous);
        if (p.inputs[next - p.origin - 1] != previous)
            return false;
    }
    return true;
}
// A projection, not a serialized world. Explicit aligned 32-bit little-endian
// values cross Wine/Linux; no actor pointers or C++ object layouts cross it.
enum Field : unsigned {
    Signature,
    WireVersion,
    Frame,
    Input,
    Stage,
    Timeline,
    X,
    Y,
    Rng,
    RngCalls,
    Power,
    Deaths,
    Bombs,
    Score,
    Rank,
    PlayerState,
    Cleared,
    Terminal,
    TimeLow,
    TimeHigh,
    FieldCount
};
using Receipt = std::array<std::uint32_t, FieldCount>;
constexpr const char *field_names[]{"magic",        "version", "frame",    "input",    "stage",
                                    "timeline",     "x",       "y",        "rng",      "rng_calls",
                                    "power",        "deaths",  "bombs",    "score",    "rank",
                                    "player_state", "cleared", "terminal", "time_low", "time_high"};
inline std::uint32_t bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, 4);
    return result;
}
inline float real(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, 4);
    return result;
}
} // namespace th06::shadow
