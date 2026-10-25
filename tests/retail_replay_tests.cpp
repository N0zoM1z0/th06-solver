#include "test_support.hpp"
#include "th06/retail_replay.hpp"
#include <iostream>

using namespace th06::solver;
int main() {
    try {
        RetailReplay r;
        r.shot = 2;
        r.difficulty = 3;
        r.score = 123456;
        r.stages.resize(6);
        for (unsigned i = 0; i < 6; ++i) {
            auto &s = r.stages[i];
            s.seed = 29768 + i;
            s.power = i ? 128 : 0;
            s.rank = 16 + i;
            s.score = 100000 * (i + 1);
            s.inputs = {{0, 0}, {0, 261}, {17, 325}, {25, 0}, {9999999, 0}};
        }
        const auto bytes = encode_retail_replay(r);
        check(bytes.size() == 0x50 + 6 * (0x10 + 5 * 8), "retail packed record sizes");
        check(bytes[4] == 2 && bytes[5] == 1 && bytes[14] == 0x69 && bytes[15] == 0x69 &&
                  bytes[16] == 0xa1,
              "source version and rolling obfuscation boundary");
        const auto roundtrip = decode_retail_replay(bytes);
        check(roundtrip.shot == 2 && roundtrip.difficulty == 3 && roundtrip.score == 123456 &&
                  roundtrip.stages.size() == 6,
              "retail header roundtrip");
        for (unsigned i = 0; i < 6; ++i) {
            const auto &s = roundtrip.stages[i];
            check(s.seed == 29768 + i && s.power == (i ? 128 : 0) && s.rank == 16 + i &&
                      s.score == int(100000 * (i + 1)),
                  "stage metadata roundtrip");
            check(s.inputs.size() == 5 && s.inputs[1].frame == 0 && s.inputs[1].input == 261 &&
                      s.inputs[2].frame == 17 && s.inputs[2].input == 325,
                  "same-frame initial action and sparse input changes");
        }
        auto damaged = bytes;
        damaged.back() ^= 1;
        rejects([&] { decode_retail_replay(damaged); }, "damaged checksum accepted");
        damaged.resize(79);
        rejects([&] { decode_retail_replay(damaged); }, "truncated retail header accepted");
        auto bad = r;
        bad.stages.pop_back();
        rejects([&] { encode_retail_replay(bad); }, "incomplete standard story exported");
        bad = r;
        bad.stages[0].inputs.pop_back();
        rejects([&] { encode_retail_replay(bad); }, "missing sentinel accepted");
        bad = r;
        bad.stages[0].inputs[2].input = 8;
        rejects([&] { encode_retail_replay(bad); }, "menu input exported as replay action");
        bad = r;
        bad.stages[0].inputs[3].frame = 1;
        rejects([&] { encode_retail_replay(bad); }, "decreasing input time accepted");
        bad = r;
        bad.stages[0].inputs.insert(bad.stages[0].inputs.end() - 1, {9999999, 0});
        rejects([&] { encode_retail_replay(bad); }, "early sentinel accepted");
        bad = r;
        bad.stages[0].inputs.assign(54000, {0, 0});
        rejects([&] { encode_retail_replay(bad); }, "original stage buffer bound ignored");
        r.difficulty = 0;
        r.stages.resize(5);
        check(decode_retail_replay(encode_retail_replay(r)).stages.size() == 5,
              "Easy five-stage replay rejected");
        std::cout << "retail replay format contracts passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
