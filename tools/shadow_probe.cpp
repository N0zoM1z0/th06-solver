#include "th06/resources.hpp"
#include "th06/shadow_search.hpp"
#include "th06/tape.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace th06::solver;
namespace b = th06::shadow;
int main(int argc, char **argv) {
    try {
        std::string data, path, output;
        unsigned origin = 0, lead = 16, horizon = 128, budget = 110, proposals = 6;
        bool anytime = false, laser = false, power = false, damage = false, acceleration = false;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--help") {
                std::cout << "th06_shadow_probe --data DIR --tape TAPE --origin N [--lead N] "
                             "[--horizon N] [--budget-ms N] [--proposals N] [--anytime] "
                             "[--laser-aware] [--power-aware] [--damage-aware] "
                             "[--acceleration-aware] [--output TAPE]\n";
                return 0;
            }
            if (key == "--anytime") {
                anytime = true;
                continue;
            }
            if (key == "--laser-aware") {
                laser = true;
                continue;
            }
            if (key == "--power-aware") {
                power = true;
                continue;
            }
            if (key == "--damage-aware") {
                damage = true;
                continue;
            }
            if (key == "--acceleration-aware") {
                acceleration = true;
                continue;
            }
            if (++i == argc)
                throw std::invalid_argument("missing value for " + key);
            const std::string value = argv[i];
            if (key == "--data")
                data = value;
            else if (key == "--tape")
                path = value;
            else if (key == "--output")
                output = value;
            else {
                std::size_t end = 0;
                const auto n = std::stoul(value, &end);
                if (end != value.size() || n > 1000000)
                    throw std::invalid_argument("invalid number");
                if (key == "--origin")
                    origin = n;
                else if (key == "--lead")
                    lead = n;
                else if (key == "--horizon")
                    horizon = n;
                else if (key == "--budget-ms")
                    budget = n;
                else if (key == "--proposals")
                    proposals = n;
                else
                    throw std::invalid_argument("unknown option: " + key);
            }
        }
        if (data.empty() || path.empty() || !lead || lead > 64 || horizon < 8 ||
            horizon > b::capacity - lead || !budget || budget > 2000 || !proposals ||
            proposals > 50)
            throw std::invalid_argument("invalid probe configuration");
        if (!output.empty() && std::filesystem::exists(output))
            throw std::invalid_argument("output already exists; preserve earlier evidence");
        auto tape = read_tape(path);
        if (origin > tape.rows.size() || lead > tape.rows.size() - origin)
            throw std::invalid_argument("tape does not contain the entire immutable lead");
        Resources resources(data);
        if (resources.fingerprint() != tape.resource_fingerprint)
            throw std::runtime_error("DAT fingerprint mismatch");
        set_resources(&resources);
        Session game(tape.start);
        std::uint16_t previous = 261;
        for (unsigned i = 0; i < origin; ++i) {
            const auto &row = tape.rows[i];
            if (game.step(row.input).hit || game.observation_digest() != row.digest)
                throw std::runtime_error("prefix mismatch/hit at " + std::to_string(i + 1));
            previous = row.input;
        }
        b::Plan committed;
        committed.origin = origin;
        committed.stage = game.state().stage;
        committed.count = lead;
        for (unsigned i = 0; i < lead; ++i)
            committed.inputs[i] = tape.rows[origin + i].input;
        const auto before = game.observation_digest();
        const auto result =
            b::search(game, committed, previous, lead, horizon, budget, false, false, 0, false,
                      proposals, laser, anytime, power, damage, acceleration);
        if (game.observation_digest() != before)
            throw std::runtime_error("search mutated the origin world");
        // Validate the captured lead even when the bounded search times out.
        // Every returned digest then receives independent literal execution.
        const auto count = result.prediction.count;
        for (unsigned i = 0; i < std::max(lead, count); ++i) {
            const auto input = i < lead ? tape.rows[origin + i].input : result.prediction.inputs[i];
            const auto state = game.step(input);
            const auto digest = game.observation_digest();
            if (state.hit || (i < lead && digest != tape.rows[origin + i].digest) ||
                (i < count &&
                 (input != result.prediction.inputs[i] || digest != result.prediction.digests[i])))
                throw std::runtime_error("lead/certificate mismatch/hit at " +
                                         std::to_string(origin + i + 1));
        }
        if (!output.empty() && count) {
            tape.rows.resize(origin);
            for (unsigned i = 0; i < count; ++i)
                tape.rows.push_back({result.prediction.inputs[i], result.prediction.digests[i]});
            tape.complete = game.state().complete;
            write_tape(output, tape);
        }
        std::cout << std::boolalpha << std::setprecision(10)
                  << "{\"type\":\"fixed_prefix_native_probe\",\"retail_proof\":false"
                  << ",\"seed\":" << tape.start.seed << ",\"origin\":" << origin
                  << ",\"lead\":" << lead << ",\"horizon\":" << horizon
                  << ",\"budget_ms\":" << budget << ",\"proposals\":" << proposals
                  << ",\"anytime\":" << anytime << ",\"laser_aware\":" << laser
                  << ",\"power_aware\":" << power << ",\"damage_aware\":" << damage
                  << ",\"acceleration_aware\":" << acceleration
                  << ",\"prefix_and_lead_checked\":true,\"certified\":" << count
                  << ",\"literal_checked\":" << bool(count) << ",\"timeout\":" << result.timeout
                  << ",\"deadline_checkpoint\":" << result.deadline_checkpoint
                  << ",\"last_phase\":" << result.last_phase << ",\"wall_ms\":" << result.wall_ms
                  << ",\"branches\":" << result.prediction.branches
                  << ",\"updates\":" << result.prediction.updates << "}\n";
        return count ? 0 : 2;
    } catch (const std::exception &error) {
        std::cerr << "th06_shadow_probe: " << error.what() << '\n';
        return 1;
    }
}
