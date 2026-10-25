#include "th06/retail_replay.hpp"
#include <cstring>
#include <stdexcept>

namespace th06::solver {
namespace {
constexpr std::size_t header_size = 0x50, stage_size = 0x10, stage_limit = 0x69780;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
void put(std::vector<std::uint8_t> &b, std::size_t offset, std::uint32_t n, unsigned size = 4) {
    for (unsigned i = 0; i < size; ++i)
        b.at(offset + i) = std::uint8_t(n >> (8 * i));
}
std::uint32_t get(const std::vector<std::uint8_t> &b, std::size_t offset, unsigned size = 4) {
    std::uint32_t n = 0;
    for (unsigned i = 0; i < size; ++i)
        n |= std::uint32_t(b.at(offset + i)) << (8 * i);
    return n;
}
std::uint32_t checksum(const std::vector<std::uint8_t> &b) {
    std::uint32_t n = 0x3f000318;
    for (std::size_t i = 14; i < b.size(); ++i)
        n += b[i];
    return n;
}
} // namespace
std::vector<std::uint8_t> encode_retail_replay(const RetailReplay &r) {
    require(r.shot < 4 && r.difficulty < 4 && r.stages.size() == (r.difficulty ? 6u : 5u),
            "retail export requires a complete standard story");
    std::vector<std::uint8_t> b(header_size);
    std::memcpy(b.data(), "T6RP", 4);
    put(b, 4, 0x102, 2);
    b[6] = r.shot;
    b[7] = r.difficulty;
    b[14] = 0x69;
    std::memcpy(b.data() + 16, "10/25/26", 8);
    std::memcpy(b.data() + 25, "NATIVE", 6);
    put(b, 36, r.score);
    for (unsigned offset : {40u, 44u, 48u})
        put(b, offset, 0x42c80000); // 100.0f; display-only slowdown fields.
    for (std::size_t i = 0; i < r.stages.size(); ++i) {
        const auto &s = r.stages[i];
        require(s.inputs.size() >= 2 && s.inputs.size() <= (stage_limit - stage_size) / 8,
                "retail stage input bounds");
        require(s.inputs.front().frame == 0 && s.inputs.back().frame == 9999999,
                "retail stage requires frame-zero input and terminal sentinel");
        const auto start = b.size();
        put(b, 52 + 4 * i, start);
        b.resize(start + stage_size + s.inputs.size() * 8);
        put(b, start, s.score);
        put(b, start + 4, s.seed, 2);
        put(b, start + 6, s.point_items, 2);
        b[start + 8] = s.power;
        b[start + 9] = std::uint8_t(s.lives);
        b[start + 10] = std::uint8_t(s.bombs);
        b[start + 11] = s.rank;
        b[start + 12] = std::uint8_t(s.power_item_count);
        std::int32_t previous = -1;
        for (std::size_t j = 0; j < s.inputs.size(); ++j) {
            const auto &input = s.inputs[j];
            require(input.frame >= previous && input.frame <= 9999999 && !(input.input & ~0x1f7),
                    "invalid retail input sequence");
            require(j + 1 == s.inputs.size() || input.frame < 9999999,
                    "retail sentinel must be the final input");
            put(b, start + stage_size + j * 8, input.frame);
            put(b, start + stage_size + j * 8 + 4, input.input, 2);
            previous = input.frame;
        }
    }
    put(b, 8, checksum(b));
    std::uint8_t key = b[14];
    for (std::size_t i = 15; i < b.size(); ++i, key = std::uint8_t(key + 7))
        b[i] = std::uint8_t(b[i] + key);
    return b;
}
RetailReplay decode_retail_replay(std::vector<std::uint8_t> b) {
    require(b.size() >= header_size && b.size() <= header_size + 7 * stage_limit,
            "retail replay size bounds");
    require(std::memcmp(b.data(), "T6RP", 4) == 0 && get(b, 4, 2) == 0x102,
            "unsupported retail replay magic/version");
    std::uint8_t key = b[14];
    for (std::size_t i = 15; i < b.size(); ++i, key = std::uint8_t(key + 7))
        b[i] = std::uint8_t(b[i] - key);
    require(get(b, 8) == checksum(b), "retail replay checksum mismatch");
    RetailReplay r;
    r.shot = b[6];
    r.difficulty = b[7];
    r.score = std::int32_t(get(b, 36));
    require(r.shot < 4 && r.difficulty < 4, "unsupported retail story profile");
    const unsigned count = r.difficulty ? 6 : 5;
    std::size_t expected = header_size;
    for (unsigned i = 0; i < 7; ++i) {
        const auto offset = get(b, 52 + 4 * i);
        if (i >= count) {
            require(offset == 0, "unexpected retail stage");
            continue;
        }
        require(offset == expected && offset + stage_size <= b.size(), "retail stage offset");
        RetailStage s;
        s.score = std::int32_t(get(b, offset));
        s.seed = get(b, offset + 4, 2);
        s.point_items = get(b, offset + 6, 2);
        s.power = b[offset + 8];
        s.lives = std::int8_t(b[offset + 9]);
        s.bombs = std::int8_t(b[offset + 10]);
        s.rank = b[offset + 11];
        s.power_item_count = std::int8_t(b[offset + 12]);
        expected = offset + stage_size;
        std::int32_t previous = -1;
        for (;;) {
            require(expected + 8 <= b.size() && expected + 8 - offset <= stage_limit,
                    "missing retail terminal sentinel");
            RetailInput input{std::int32_t(get(b, expected)),
                              std::uint16_t(get(b, expected + 4, 2))};
            require(input.frame >= previous && input.frame <= 9999999 && !(input.input & ~0x1f7),
                    "invalid retail input sequence");
            if (s.inputs.empty())
                require(input.frame == 0, "missing frame-zero retail input");
            s.inputs.push_back(input);
            expected += 8;
            previous = input.frame;
            if (input.frame == 9999999)
                break;
        }
        r.stages.push_back(std::move(s));
    }
    require(expected == b.size(), "trailing retail replay data");
    return r;
}
} // namespace th06::solver
