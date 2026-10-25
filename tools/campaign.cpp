#include "th06/tape.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace th06::solver;
namespace fs = std::filesystem;
namespace {
struct Case {
    std::string name;
    StartConfig start;
    std::string status = "pending";
    std::uint64_t frames = 0;
};
struct Running {
    pid_t pid;
    std::size_t index;
};
class CampaignLock {
  public:
    explicit CampaignLock(const fs::path &directory) {
        fd_ = open((directory / ".campaign.lock").c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
        if (fd_ < 0 || flock(fd_, LOCK_EX | LOCK_NB))
            throw std::runtime_error("another campaign owns this output directory");
    }
    ~CampaignLock() {
        if (fd_ >= 0)
            close(fd_);
    }

  private:
    int fd_ = -1;
};
std::uint64_t file_hash(const fs::path &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot snapshot runtime: " + path.string());
    std::uint64_t hash = 14695981039346656037ULL;
    char block[16384];
    while (in.read(block, sizeof(block)) || in.gcount())
        for (std::streamsize i = 0; i < in.gcount(); ++i) {
            hash ^= static_cast<unsigned char>(block[i]);
            hash *= 1099511628211ULL;
        }
    if (!in.eof())
        throw std::runtime_error("runtime snapshot read failed");
    return hash;
}
fs::path freeze_binaries(const fs::path &source, const fs::path &directory) {
    const auto plan_hash = file_hash(source / "th06_plan");
    const auto check_hash = file_hash(source / "th06_replay_check");
    const auto frozen = fs::absolute(
        directory / "bin" / (std::to_string(plan_hash) + "-" + std::to_string(check_hash)));
    fs::create_directories(frozen);
    for (const auto *name : {"th06_plan", "th06_replay_check"}) {
        const auto expected = std::string(name) == "th06_plan" ? plan_hash : check_hash;
        const auto destination = frozen / name;
        if (!fs::exists(destination)) {
            const auto temporary = frozen / (std::string(name) + ".tmp");
            fs::copy_file(source / name, temporary, fs::copy_options::overwrite_existing);
            if (file_hash(temporary) != expected || file_hash(source / name) != expected)
                throw std::runtime_error(
                    "runtime changed during snapshot; retry after build completes");
            fs::permissions(temporary,
                            fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec);
            fs::rename(temporary, destination);
        } else if (file_hash(destination) != expected)
            throw std::runtime_error("runtime snapshot changed");
    }
    std::ofstream receipt(frozen / "runtime.json");
    receipt << "{\"profile\":\"" << native_profile << "\",\"plan_fnv1a64\":" << plan_hash
            << ",\"check_fnv1a64\":" << check_hash << "}\n";
    return frozen;
}
bool same(const StartConfig &a, const StartConfig &b) {
    return a.stage == b.stage && a.difficulty == b.difficulty && a.shot == b.shot &&
           a.seed == b.seed && a.power == b.power && a.story == b.story;
}
pid_t spawn(const std::vector<std::string> &args, const fs::path &out, const fs::path &err) {
    const auto pid = fork();
    if (pid < 0)
        throw std::runtime_error("campaign fork failed");
    if (!pid) {
        setpgid(0, 0);
        const int output = open(out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        const int diagnostic = open(err.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (output < 0 || diagnostic < 0 || dup2(output, STDOUT_FILENO) < 0 ||
            dup2(diagnostic, STDERR_FILENO) < 0)
            _exit(125);
        close(output);
        close(diagnostic);
        std::vector<char *> pointers;
        for (const auto &arg : args)
            pointers.push_back(const_cast<char *>(arg.c_str()));
        pointers.push_back(nullptr);
        execv(pointers[0], pointers.data());
        dprintf(STDERR_FILENO, "cannot execute %s: %s\n", pointers[0], std::strerror(errno));
        _exit(127);
    }
    return pid;
}
int wait_one(pid_t pid) {
    int status;
    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            throw std::runtime_error("campaign waitpid failed");
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
void report(const fs::path &directory, const std::vector<Case> &cases) {
    std::ofstream out(directory / "status.json.tmp");
    unsigned passed = 0, failed = 0;
    for (const auto &c : cases) {
        passed += c.status == "cleared";
        failed += c.status == "failed";
    }
    out << "{\"profile\":\"" << native_profile << "\",\"cases\":" << cases.size()
        << ",\"cleared\":" << passed << ",\"failed\":" << failed << ",\"results\":[\n";
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto &c = cases[i];
        out << (i ? ",\n" : "") << "{\"case\":\"" << c.name << "\",\"status\":\"" << c.status
            << "\",\"frames\":" << c.frames << "}";
    }
    out << "\n]}\n";
    out.close();
    if (!out)
        throw std::runtime_error("campaign status write failed");
    fs::rename(directory / "status.json.tmp", directory / "status.json");
}
void preserve_attempt(const fs::path &directory, const std::string &name) {
    const auto attempts = directory / "attempts";
    fs::create_directories(attempts);
    unsigned attempt = 1;
    while (fs::exists(attempts / (name + "." + std::to_string(attempt) + ".plan.json")))
        ++attempt;
    for (const auto *suffix :
         {".plan.json", ".plan.log", ".actions", ".check.json", ".check.log"}) {
        const auto path = directory / (name + suffix);
        if (fs::exists(path))
            fs::rename(path, attempts / (name + "." + std::to_string(attempt) + suffix));
    }
}
} // namespace

int main(int argc, char **argv) {
    try {
        fs::path data, directory = "reports/local/stages";
        const auto source_binaries = fs::read_symlink("/proc/self/exe").parent_path();
        std::string suite = "stages", only;
        unsigned jobs = 2, seed = 0, frames = 60000, horizon = 90, commit = 8, proposals = 4;
        bool list = false, resume = false;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--list") {
                list = true;
                continue;
            }
            if (key == "--resume") {
                resume = true;
                continue;
            }
            if (key == "--help") {
                std::cout << "th06_campaign --data DIR --output DIR [--suite stages|story|all]\n"
                             "  [--jobs 1..4] [--seed N] [--frames N] [--horizon N] [--commit N]\n"
                             "  [--proposals N] [--case NAME] [--resume] [--list]\n";
                return 0;
            }
            if (i + 1 == argc)
                throw std::invalid_argument("missing value for " + key);
            const std::string value = argv[++i];
            if (key == "--data")
                data = value;
            else if (key == "--output")
                directory = value;
            else if (key == "--suite")
                suite = value;
            else if (key == "--case")
                only = value;
            else {
                std::size_t used;
                const auto n = std::stoul(value, &used);
                if (used != value.size() || n > 1000000)
                    throw std::invalid_argument("invalid campaign value");
                if (key == "--jobs")
                    jobs = n;
                else if (key == "--seed" && n <= 65535)
                    seed = n;
                else if (key == "--frames")
                    frames = n;
                else if (key == "--horizon")
                    horizon = n;
                else if (key == "--commit")
                    commit = n;
                else if (key == "--proposals")
                    proposals = n;
                else
                    throw std::invalid_argument("unknown/invalid option " + key);
            }
        }
        if (suite != "stages" && suite != "story" && suite != "all")
            throw std::invalid_argument("invalid suite");
        if (!jobs || jobs > 4 || !commit || commit > horizon || horizon > 600 || !proposals ||
            !frames)
            throw std::invalid_argument("invalid campaign budget");
        std::vector<Case> cases;
        if (suite != "story") {
            // Hardest standard cases first; unreachable Easy stage 6 is excluded.
            for (int difficulty : {3, 2, 1, 0, 4})
                for (int stage = difficulty == 4 ? 7 : 1; stage <= (difficulty == 4   ? 7
                                                                    : difficulty == 0 ? 5
                                                                                      : 6);
                     ++stage)
                    for (int shot = 0; shot < 4; ++shot) {
                        const int power = stage == 1 ? 0 : stage == 2 ? 64 : 128;
                        cases.push_back(
                            {"s" + std::to_string(stage) + "-d" + std::to_string(difficulty) +
                                 "-shot" + std::to_string(shot) + "-seed" + std::to_string(seed) +
                                 "-p" + std::to_string(power),
                             {stage, difficulty, shot, std::uint16_t(seed), power, false}});
                    }
        }
        if (suite != "stages")
            for (int difficulty : {3, 2, 1, 0})
                for (int shot = 0; shot < 4; ++shot)
                    cases.push_back({"story-d" + std::to_string(difficulty) + "-shot" +
                                         std::to_string(shot) + "-seed" + std::to_string(seed),
                                     {1, difficulty, shot, std::uint16_t(seed), 0, true}});
        if (!only.empty())
            cases.erase(std::remove_if(cases.begin(), cases.end(),
                                       [&](const auto &c) { return c.name != only; }),
                        cases.end());
        if (cases.empty())
            throw std::invalid_argument("no campaign cases selected");
        if (list) {
            for (const auto &c : cases)
                std::cout << c.name << '\n';
            return 0;
        }
        if (data.empty())
            throw std::invalid_argument("--data is required");
        fs::create_directories(directory);
        CampaignLock ownership(directory);
        const auto binaries = freeze_binaries(source_binaries, directory);
        auto verify = [&](Case &c) {
            try {
                const auto tape = read_tape(directory / (c.name + ".actions"));
                if (!tape.complete || !same(tape.start, c.start))
                    return false;
                const auto checker = spawn(
                    {(binaries / "th06_replay_check").string(), "--data", data.string(), "--tape",
                     (directory / (c.name + ".actions")).string(), "--require-complete"},
                    directory / (c.name + ".check.json"), directory / (c.name + ".check.log"));
                if (wait_one(checker))
                    return false;
                c.frames = tape.rows.size();
                return true;
            } catch (const std::exception &) {
                return false;
            }
        };
        std::vector<Running> running;
        std::size_t next = 0;
        report(directory, cases);
        while (next < cases.size() || !running.empty()) {
            while (next < cases.size() && running.size() < jobs) {
                auto &c = cases[next];
                if (resume && verify(c)) {
                    c.status = "cleared";
                    std::cerr << "verified existing " << c.name << " frames=" << c.frames << '\n';
                    ++next;
                    report(directory, cases);
                    continue;
                }
                preserve_attempt(directory, c.name);
                const auto &s = c.start;
                std::vector<std::string> args = {(binaries / "th06_plan").string(),
                                                 "--data",
                                                 data.string(),
                                                 "--stage",
                                                 std::to_string(s.stage),
                                                 "--difficulty",
                                                 std::to_string(s.difficulty),
                                                 "--shot",
                                                 std::to_string(s.shot),
                                                 "--seed",
                                                 std::to_string(s.seed),
                                                 "--power",
                                                 std::to_string(s.power),
                                                 "--frames",
                                                 std::to_string(frames),
                                                 "--horizon",
                                                 std::to_string(horizon),
                                                 "--commit",
                                                 std::to_string(commit),
                                                 "--proposals",
                                                 std::to_string(proposals),
                                                 "--output",
                                                 (directory / (c.name + ".actions")).string()};
                if (s.story)
                    args.push_back("--story");
                const auto pid = spawn(args, directory / (c.name + ".plan.json"),
                                       directory / (c.name + ".plan.log"));
                c.status = "running";
                running.push_back({pid, next});
                std::cerr << "started " << c.name << " pid=" << pid << '\n';
                ++next;
                report(directory, cases);
            }
            if (running.empty())
                continue;
            int status;
            pid_t done;
            do {
                done = waitpid(-1, &status, 0);
            } while (done < 0 && errno == EINTR);
            if (done < 0)
                throw std::runtime_error("campaign wait failed");
            const auto found = std::find_if(running.begin(), running.end(),
                                            [&](const auto &r) { return r.pid == done; });
            if (found == running.end())
                throw std::runtime_error("unowned campaign child");
            auto &c = cases[found->index];
            const bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0 && verify(c);
            c.status = ok ? "cleared" : "failed";
            std::cerr << c.status << ' ' << c.name << " frames=" << c.frames << '\n';
            running.erase(found);
            report(directory, cases);
        }
        unsigned cleared = 0;
        for (const auto &c : cases)
            cleared += c.status == "cleared";
        std::cout << "{\"cases\":" << cases.size() << ",\"cleared\":" << cleared
                  << ",\"failed\":" << cases.size() - cleared << "}\n";
        return cleared == cases.size() ? 0 : 2;
    } catch (const std::exception &e) {
        std::cerr << "th06_campaign: " << e.what() << '\n';
        return 1;
    }
}
