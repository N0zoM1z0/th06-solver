#include "th06/resources.hpp"
#include "th06/tape.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace th06::solver;
int main(int argc, char **argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("th06_runtime_bench DATA TAPE");
        const auto tape = read_tape(argv[2]);
        if (tape.rows.empty())
            throw std::invalid_argument("benchmark requires a nonempty tape");
        Resources resources(argv[1]);
        if (resources.fingerprint() != tape.resource_fingerprint)
            throw std::invalid_argument("benchmark DAT fingerprint mismatch");
        set_resources(&resources);
        // One source game per process, as in the planner and replay checker.
        Session game(tape.start);
        State state;
        const auto begin = std::chrono::steady_clock::now();
        for (const auto &row : tape.rows) {
            state = game.step(row.input);
            if (state.hit)
                throw std::runtime_error("benchmark route hit");
        }
        const auto seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
        // This is a timing workload, not a substitute for full fresh replay.
        // Check the terminal digest outside the measured native update loop.
        if (game.observation_digest() != tape.rows.back().digest || state.complete != tape.complete)
            throw std::runtime_error("benchmark terminal divergence");
        std::cout << std::boolalpha << std::setprecision(9) << "{\"profile\":\"" << native_profile
                  << "\",\"stage\":" << tape.start.stage
                  << ",\"difficulty\":" << tape.start.difficulty << ",\"shot\":" << tape.start.shot
                  << ",\"seed\":" << tape.start.seed << ",\"updates\":" << tape.rows.size()
                  << ",\"complete\":" << tape.complete << ",\"terminal_verified\":true"
                  << ",\"native_seconds_excluding_init_digest_io\":" << seconds << "}\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
