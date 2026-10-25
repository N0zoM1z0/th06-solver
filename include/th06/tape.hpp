#pragma once
#include "th06/session.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

namespace th06::solver {
inline constexpr const char *native_profile = "native-i386-v1";
struct ActionRow {
    std::uint16_t input;
    std::uint64_t digest;
};
struct Tape {
    StartConfig start;
    std::uint64_t resource_fingerprint = 0;
    std::vector<ActionRow> rows;
    bool complete = false;
};
void write_tape(const std::filesystem::path &path, const Tape &tape);
Tape read_tape(const std::filesystem::path &path);
} // namespace th06::solver
