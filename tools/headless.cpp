#include "th06/resources.hpp"
#include "th06/session.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using namespace th06::solver;
int main(int argc, char **argv) {
    try {
        std::string data, trace;
        StartConfig start;
        int frames = 1200, input = 1 | 4 | 256;
        bool audit = false;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--help") {
                std::cout
                    << "th06_headless --data DIR [--audit] [--stage 1..7] [--difficulty 0..4]\n"
                       "  [--shot 0..3] [--seed 0..65535] [--power 0..128] [--frames N]\n"
                       "  [--input MASK] [--trace FILE]\n";
                return 0;
            }
            if (key == "--audit") {
                audit = true;
                continue;
            }
            if (i + 1 == argc)
                throw std::invalid_argument("missing value for " + key);
            const std::string value = argv[++i];
            if (key == "--data")
                data = value;
            else if (key == "--trace")
                trace = value;
            else {
                std::size_t end;
                const auto number = std::stoul(value, &end, 0);
                if (end != value.size() || number > 10000000)
                    throw std::invalid_argument("invalid number");
                if (key == "--stage")
                    start.stage = number;
                else if (key == "--difficulty")
                    start.difficulty = number;
                else if (key == "--shot")
                    start.shot = number;
                else if (key == "--power")
                    start.power = number;
                else if (key == "--seed" && number <= 65535)
                    start.seed = number;
                else if (key == "--frames" && number > 0)
                    frames = number;
                else if (key == "--input" && number <= 65535)
                    input = number;
                else
                    throw std::invalid_argument("unknown/invalid option " + key);
            }
        }
        if (data.empty())
            throw std::invalid_argument("--data is required");
        Resources resources(data);
        if (audit) {
            resources.verify_all();
            std::cout << "{\"archives\":" << resources.archive_count()
                      << ",\"entries\":" << resources.entry_count() << ",\"verified\":true}\n";
            return 0;
        }
        set_resources(&resources);
        Session game(start);
        std::ofstream out;
        if (!trace.empty()) {
            out.open(trace);
            if (!out)
                throw std::runtime_error("cannot write trace");
            out << "frame\tx\ty\ttimeline\tenemies\tbullets\tlasers\tdeaths\trng\trng_calls\n";
            out << std::setprecision(9);
        }
        auto s = game.state();
        int max_bullets = 0, max_enemies = 0;
        const auto begin = std::chrono::steady_clock::now();
        for (int f = 0; f < frames && !s.complete && !s.hit; ++f) {
            s = game.step(input);
            max_bullets = std::max(max_bullets, s.bullets);
            max_enemies = std::max(max_enemies, s.enemies);
            if (out)
                out << s.frame << '\t' << s.x << '\t' << s.y << '\t' << s.timeline << '\t'
                    << s.enemies << '\t' << s.bullets << '\t' << s.lasers << '\t' << s.deaths
                    << '\t' << s.rng << '\t' << s.rng_calls << '\n';
        }
        const auto seconds =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
        std::cout << std::boolalpha << std::setprecision(9)
                  << "{\"profile\":\"native-i386-v1\",\"stage\":" << start.stage
                  << ",\"difficulty\":" << start.difficulty << ",\"shot\":" << start.shot
                  << ",\"seed\":" << start.seed << ",\"power\":" << start.power
                  << ",\"frames\":" << s.frame << ",\"timeline\":" << s.timeline << ",\"x\":" << s.x
                  << ",\"y\":" << s.y << ",\"max_bullets\":" << max_bullets
                  << ",\"max_enemies\":" << max_enemies << ",\"deaths\":" << s.deaths
                  << ",\"bombs\":" << s.bombs << ",\"hit\":" << s.hit
                  << ",\"complete\":" << s.complete << ",\"rng\":" << s.rng
                  << ",\"rng_calls\":" << s.rng_calls << ",\"simulation_seconds\":" << seconds
                  << "}\n";
        return s.hit ? 2 : 0;
    } catch (const std::exception &error) {
        std::cerr << "th06_headless: " << error.what() << '\n';
        return 1;
    }
}
