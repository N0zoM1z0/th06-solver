#pragma once
#include <cstdint>
#include <vector>

namespace th06::solver {
struct RetailInput {
    std::int32_t frame;
    std::uint16_t input;
};
struct RetailStage {
    std::int32_t score = 0;
    std::uint16_t seed = 0, point_items = 0;
    std::uint8_t power = 0;
    std::int8_t lives = 2, bombs = 3;
    std::uint8_t rank = 16;
    std::int8_t power_item_count = 0;
    std::vector<RetailInput> inputs;
};
struct RetailReplay {
    std::uint8_t shot = 0, difficulty = 3;
    std::int32_t score = 0;
    // Five/six main-game records, or one Extra record mapped to file slot seven.
    std::vector<RetailStage> stages;
};
// Japanese 1.02h T6RP format. Offsets are file-relative, never native pointers.
std::vector<std::uint8_t> encode_retail_replay(const RetailReplay &replay);
RetailReplay decode_retail_replay(std::vector<std::uint8_t> bytes);
} // namespace th06::solver
