#include "test_support.hpp"
#include "th06/tape.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace th06::solver;
int main(int argc, char **argv) {
    try {
        check(argc == 2, "test needs work file");
        Tape original;
        original.start = {7, 4, 3, 65535, 128};
        original.resource_fingerprint = 123456;
        original.rows = {{261, 123}, {325, 456}};
        original.complete = true;
        write_tape(argv[1], original);
        const auto loaded = read_tape(argv[1]);
        check(loaded.start.stage == 7 && loaded.start.seed == 65535 && loaded.start.power == 128 &&
                  loaded.resource_fingerprint == 123456 && loaded.rows.size() == 2 &&
                  loaded.rows[1].input == 325 && loaded.rows[1].digest == 456 && loaded.complete,
              "tape roundtrip");
        const std::string header = "TH06ACTIONS 1 native-i386-v1\n1 3 0 0 0\n123 1 0\n";
        for (const auto &bad :
             {header + "2 261 123\n", header + "1 263 123\n", header + "1 65536 123\n",
              header + "1 261 123 extra\n", header + "1 261 123\ntrailing\n", header,
              std::string("TH06ACTIONS 2 other-profile\n")}) {
            {
                std::ofstream out(argv[1]);
                out << bad;
            }
            rejects([&] { read_tape(argv[1]); }, "invalid tape accepted");
        }
        std::filesystem::remove(argv[1]);
        std::cout << "action tape roundtrip, sequencing and rejection contracts passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
