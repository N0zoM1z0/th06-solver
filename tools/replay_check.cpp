#include "th06/resources.hpp"
#include "th06/session.hpp"
#include "th06/tape.hpp"
#include <iostream>
#include <stdexcept>

using namespace th06::solver;
int main(int argc, char **argv) {
    try {
        std::string data, path;
        bool require_complete = false;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--require-complete") {
                require_complete = true;
                continue;
            }
            if (key == "--help") {
                std::cout << "th06_replay_check --data DIR --tape TAPE [--require-complete]\n";
                return 0;
            }
            if (i + 1 == argc)
                throw std::invalid_argument("missing argument for " + key);
            if (key == "--data")
                data = argv[++i];
            else if (key == "--tape")
                path = argv[++i];
            else
                throw std::invalid_argument("unknown option: " + key);
        }
        if (data.empty() || path.empty())
            throw std::invalid_argument("--data and --tape are required");
        const auto tape = read_tape(path);
        Resources resources(data);
        if (resources.fingerprint() != tape.resource_fingerprint)
            throw std::runtime_error("DAT fingerprint mismatch");
        set_resources(&resources);
        Session game(tape.start);
        auto state = game.state();
        for (const auto &row : tape.rows) {
            state = game.step(row.input);
            if (state.hit || game.observation_digest() != row.digest)
                throw std::runtime_error("replay divergence at frame " +
                                         std::to_string(state.frame));
        }
        if (state.complete != tape.complete)
            throw std::runtime_error("tape completion mismatch");
        std::cout << std::boolalpha << "{\"profile\":\"" << native_profile
                  << "\",\"verified\":true,\"frames\":" << state.frame
                  << ",\"complete\":" << state.complete << ",\"deaths\":" << state.deaths
                  << ",\"bombs\":" << state.bombs << ",\"cleared_stages\":" << game.cleared_stages()
                  << "}\n";
        return require_complete && !state.complete ? 2 : 0;
    } catch (const std::exception &error) {
        std::cerr << "th06_replay_check: " << error.what() << '\n';
        return 1;
    }
}
