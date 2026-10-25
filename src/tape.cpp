#include "th06/tape.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace th06::solver {
namespace {
template <class... Values> void line(std::istream &in, Values &...values) {
    std::string text;
    if (!std::getline(in, text))
        throw std::runtime_error("truncated action tape");
    std::istringstream row(text);
    if (!(row >> ... >> values))
        throw std::runtime_error("invalid action tape row");
    row >> std::ws;
    if (!row.eof())
        throw std::runtime_error("extra fields in action tape");
}
} // namespace
void write_tape(const std::filesystem::path &path, const Tape &tape) {
    const auto temporary = path.string() + ".tmp";
    std::ofstream out(temporary, std::ios::trunc);
    if (!out)
        throw std::runtime_error("cannot write action tape: " + temporary);
    out << "TH06ACTIONS 2 " << native_profile << '\n';
    const auto &s = tape.start;
    out << s.stage << ' ' << s.difficulty << ' ' << s.shot << ' ' << s.seed << ' ' << s.power << ' '
        << s.story << '\n';
    out << tape.resource_fingerprint << ' ' << tape.rows.size() << ' ' << tape.complete << '\n';
    for (std::size_t i = 0; i < tape.rows.size(); ++i)
        out << i + 1 << ' ' << tape.rows[i].input << ' ' << tape.rows[i].digest << '\n';
    out.close();
    if (!out)
        throw std::runtime_error("action tape write failed");
    std::filesystem::rename(temporary, path);
}
Tape read_tape(const std::filesystem::path &path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot read action tape: " + path.string());
    std::string magic, profile;
    unsigned version;
    line(in, magic, version, profile);
    if (magic != "TH06ACTIONS" || (version != 1 && version != 2) || profile != native_profile)
        throw std::runtime_error("unsupported tape profile/version");
    Tape tape;
    unsigned seed;
    if (version == 1)
        line(in, tape.start.stage, tape.start.difficulty, tape.start.shot, seed, tape.start.power);
    else {
        unsigned story;
        line(in, tape.start.stage, tape.start.difficulty, tape.start.shot, seed, tape.start.power,
             story);
        if (story > 1)
            throw std::runtime_error("invalid story flag");
        tape.start.story = story;
    }
    if (seed > 65535)
        throw std::runtime_error("invalid tape seed");
    tape.start.seed = seed;
    std::size_t count;
    unsigned complete;
    line(in, tape.resource_fingerprint, count, complete);
    if (!count || count > 1000000 || complete > 1)
        throw std::runtime_error("invalid tape length/status");
    tape.complete = complete;
    tape.rows.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t frame;
        unsigned input;
        std::uint64_t digest;
        line(in, frame, input, digest);
        if (frame != i + 1 || input > 65535 || (input & ~0x1f5u))
            throw std::runtime_error("invalid tape frame/input");
        tape.rows.push_back({std::uint16_t(input), digest});
    }
    in >> std::ws;
    if (!in.eof())
        throw std::runtime_error("trailing action tape data");
    return tape;
}
} // namespace th06::solver
