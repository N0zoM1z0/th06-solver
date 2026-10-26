#pragma once
#include <array>
#include <cstdint>
#include <limits>

namespace th06::render_timing {
// Diagnostic file for the owned 32-bit x86 Wine process: little-endian words,
// a 16-byte header and 64-byte samples. No pointer or compiler struct crosses it.
constexpr std::uint32_t magic = 0x36544d52, version = 1;
using Header = std::array<std::uint32_t, 4>;
using Sample = std::array<std::uint64_t, 8>;
enum Field { Sequence, Stage, ElapsedNs, PeriodNs, DrawNs, PresentNs, DrawCalls, Flags };
enum Flag : std::uint64_t { NoDraw = 1, PresentFailed = 2, RepeatedReceipt = 4 };
constexpr Header header{magic, version, sizeof(Sample), 0};
using Update = std::array<std::uint64_t, 3>;
constexpr Header update_header{0x36545055, 1, sizeof(Update), 0};
constexpr bool frequency_valid(std::uint64_t frequency) {
    return frequency && frequency <= std::numeric_limits<std::uint64_t>::max() / 1000000000;
}
constexpr std::uint64_t nanoseconds(std::uint64_t ticks, std::uint64_t frequency) {
    // Do not let the game's 24-bit x87 mode quantize long profiling clocks.
    return (ticks / frequency) * 1000000000 + (ticks % frequency) * 1000000000 / frequency;
}
} // namespace th06::render_timing
