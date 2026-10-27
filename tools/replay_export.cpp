#include "stdafx.h"
#include "th06/resources.hpp"
#include "th06/retail_replay.hpp"
#include "th06/session.hpp"
#include "th06/tape.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace th06;
using namespace th06::solver;
int main(int argc, char **argv) {
    try {
        std::string data, path, output, trace;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--help") {
                std::cout << "th06_replay_export --data DIR --tape TAPE --output FILE.rpy [--trace "
                             "CSV]\n";
                return 0;
            }
            if (i + 1 == argc)
                throw std::invalid_argument("missing argument for " + key);
            const std::string value = argv[++i];
            if (key == "--data")
                data = value;
            else if (key == "--tape")
                path = value;
            else if (key == "--output")
                output = value;
            else if (key == "--trace")
                trace = value;
            else
                throw std::invalid_argument("unknown option: " + key);
        }
        if (data.empty() || path.empty() || output.empty())
            throw std::invalid_argument("--data, --tape and --output are required");
        const auto tape = read_tape(path);
        const bool extra = tape.start.stage == 7 && tape.start.difficulty == 4 &&
                           !tape.start.story && tape.start.power == 0;
        if ((!tape.start.story && !extra) || !tape.complete)
            throw std::runtime_error(
                "retail export requires a complete story or normal-start Extra");
        Resources resources(data);
        if (resources.fingerprint() != tape.resource_fingerprint)
            throw std::runtime_error("DAT fingerprint mismatch");
        set_resources(&resources);
        Session game(tape.start);
        RetailReplay replay;
        replay.shot = tape.start.shot;
        replay.difficulty = tape.start.difficulty;
        auto snapshot = [&](bool pending) {
            RetailStage s;
            const auto &g = g_GameManager;
            s.seed = pending ? g_Rng.seed : g.randomSeed;
            s.point_items = g.pointItemsCollected;
            s.power = g.currentPower;
            s.lives = g.livesRemaining;
            s.bombs = g.bombsRemaining;
            s.rank = g.rank;
            s.power_item_count = g.powerItemCountForScore;
            s.inputs.push_back({0, 0});
            replay.stages.push_back(std::move(s));
        };
        snapshot(false);
        unsigned local_frame = 0;
        std::ofstream csv;
        if (!trace.empty()) {
            csv.open(trace);
            if (!csv)
                throw std::runtime_error("cannot open trace");
            csv << "frame,stage,local_frame,input,x,y,timeline,rng,rng_calls,power,deaths,bombs,"
                   "score,rank,player_state\n";
            csv.precision(9);
        }
        auto state = game.state();
        for (const auto &row : tape.rows) {
            const bool pending = g_Supervisor.curState == SUPERVISOR_STATE_NEXT_STAGE;
            if (pending) {
                auto &previous = replay.stages.back();
                previous.score = g_GameManager.score;
                previous.inputs.push_back({std::int32_t(local_frame), 0});
                previous.inputs.push_back({9999999, 0});
                snapshot(true);
                local_frame = 0;
            }
            auto &s = replay.stages.back();
            if (s.inputs.back().input != row.input)
                s.inputs.push_back({std::int32_t(local_frame), row.input});
            state = game.step(row.input);
            if (state.hit || game.observation_digest() != row.digest)
                throw std::runtime_error("native replay divergence at frame " +
                                         std::to_string(state.frame));
            if (pending && s.seed != g_GameManager.randomSeed)
                throw std::runtime_error("stage cut changed replay seed; unsupported boundary");
            if (csv.is_open())
                csv << state.frame << ',' << state.stage << ',' << local_frame << ',' << row.input
                    << ',' << state.x << ',' << state.y << ',' << state.timeline << ',' << state.rng
                    << ',' << state.rng_calls << ',' << state.power << ',' << state.deaths << ','
                    << state.bombs << ',' << state.score << ',' << g_GameManager.rank << ','
                    << state.player_state << '\n';
            ++local_frame;
        }
        if (!state.complete)
            throw std::runtime_error("native run did not complete");
        if (csv.is_open() && !csv)
            throw std::runtime_error("native trace write failed");
        auto &last = replay.stages.back();
        last.score = g_GameManager.score;
        last.inputs.push_back({std::int32_t(local_frame), 0});
        last.inputs.push_back({9999999, 0});
        replay.score = g_GameManager.guiScore;
        const auto bytes = encode_retail_replay(replay);
        const auto decoded = decode_retail_replay(bytes);
        if (decoded.stages.size() != game.cleared_stages())
            throw std::runtime_error("retail stage count mismatch");
        std::ofstream file(output, std::ios::binary);
        file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        file.close();
        if (!file)
            throw std::runtime_error("cannot write retail replay");
        std::cout << "{\"native_verified\":true,\"retail_verified\":false,\"frames\":"
                  << state.frame << ",\"stages\":" << decoded.stages.size()
                  << ",\"bytes\":" << bytes.size() << "}\n";
    } catch (const std::exception &e) {
        std::cerr << "th06_replay_export: " << e.what() << '\n';
        return 1;
    }
}
