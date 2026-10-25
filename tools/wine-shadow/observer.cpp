#include "../wine-replay/identity.hpp"
#include "retail_abi.hpp"
#include "th06/shadow_bridge.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

// Live input and observation for the pinned Japanese 1.02h image.
// The EXE owns RNG, collisions, clocks, transitions and rendering.
namespace {
namespace b = th06::shadow;
constexpr unsigned game_base = 0x69bca0, supervisor_base = 0x6c6d18;
constexpr unsigned player_base = 0x6ca628, enemy_base = 0x4b79c8, rng_base = 0x69d8f8;
template <class T> T &at(unsigned base, unsigned offset = 0) {
    return *reinterpret_cast<T *>(base + offset);
}
struct ChainElem {
    short priority = 0;
    unsigned short flags = 0;
    int(__cdecl *callback)(void *) = nullptr;
    void *added = nullptr, *deleted = nullptr;
    ChainElem *prev = nullptr, *next = nullptr, *self = this;
    void *arg = nullptr;
};
static_assert(sizeof(ChainElem) == 0x20);
using Callback = int(__cdecl *)(void *);
Callback original_menu = nullptr, original_init = nullptr;
HANDLE rows = INVALID_HANDLE_VALUE, mapping = nullptr;
std::uint32_t *shared = nullptr;
b::Plan active;
unsigned examined = 0, initial_stage = 5;
std::uint16_t previous_input = 261;
bool story = false;
LARGE_INTEGER frequency{}, first_tick{};
ChainElem sampling;
char directory[MAX_PATH];
unsigned frame = 0, local_frame = 0, cleared = 0, menu_frames = 0, maximum = 180000;
unsigned shot = 0, difficulty = 3;
bool requested = false, initialized = false, attached = false;
ChainElem observation;

[[noreturn]] void finish(const char *reason, unsigned code) {
    if (shared) {
        const auto outcome = code == 0   ? b::Complete
                             : code == 2 ? b::Death
                             : code == 4 ? b::Budget
                                         : b::Failure;
        b::save(shared + b::Outcome, outcome);
    }
    if (rows != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(rows);
        CloseHandle(rows);
        rows = INVALID_HANDLE_VALUE;
    }
    char path[MAX_PATH];
    std::snprintf(path, sizeof(path), "%s/outcome.json", directory);
    if (FILE *out = std::fopen(path, "w")) {
        std::fprintf(
            out,
            "{\"reason\":\"%s\",\"frames\":%u,\"stage\":%d,\"cleared_stages\":%u,"
            "\"deaths\":%d,\"bombs\":%d,\"supervisor_state\":%d,\"retail_nmnb_complete\":%s}\n",
            reason, frame, at<int>(game_base, abi::GameManager_currentStage), cleared,
            at<int>(game_base, abi::GameManager_deaths),
            at<int>(game_base, abi::GameManager_bombsUsed),
            at<int>(supervisor_base, abi::Supervisor_curState), code == 0 ? "true" : "false");
        std::fclose(out);
    }
    ExitProcess(code);
}
int __cdecl sample(void *) {
    if (!initialized || !at<unsigned char>(game_base, abi::GameManager_isInMenu))
        return 1;
    if (b::load(shared + b::StopRequested))
        finish("controller_stopped", 8);
    const auto stage = unsigned(at<int>(game_base, abi::GameManager_currentStage));
    b::Plan proposal;
    if (b::read(shared, proposal) && proposal.generation != examined) {
        examined = proposal.generation;
        const bool timely = b::timely(proposal, frame, stage);
        if (timely && b::compatible(proposal, active, previous_input, frame, stage)) {
            active = proposal;
            b::save(shared + b::Accepted, examined);
        } else {
            b::save(shared + b::RejectionReason, timely ? 2 : 1);
            b::save(shared + b::Rejected, examined);
        }
    }
    const bool planned =
        active.count && frame + 1 > active.origin && frame + 1 <= active.origin + active.count;
    const auto input = b::next_input(active, frame + 1, previous_input);
    b::save(shared + (planned ? b::Planned : b::Fallback),
            b::load(shared + (planned ? b::Planned : b::Fallback)) + 1);
    at<unsigned short>(0x69d904) = input;
    at<unsigned short>(0x69d908) = previous_input;
    previous_input = input;
    return 1;
}
int __cdecl observe(void *) {
    if (!initialized || !at<unsigned char>(game_base, abi::GameManager_isInMenu))
        return 1;
    const auto stage = at<int>(game_base, abi::GameManager_currentStage);
    const auto state = at<int>(supervisor_base, abi::Supervisor_curState);
    const auto deaths = at<int>(game_base, abi::GameManager_deaths);
    const auto bombs = at<int>(game_base, abi::GameManager_bombsUsed);
    const auto player_state = at<signed char>(player_base, abi::Player_playerState);
    const bool terminal = state == 7 || state == 10 || (!story && state == 3);
    if (state == 3 || state == 7 || state == 10)
        ++cleared;
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    if (!frame)
        first_tick = time;
    const auto ns = std::uint64_t(double(time.QuadPart - first_tick.QuadPart) * 1e9 /
                                  double(frequency.QuadPart));
    b::Receipt receipt{
        b::magic,
        b::version,
        ++frame,
        at<unsigned short>(0x69d904),
        unsigned(stage),
        unsigned(at<int>(enemy_base, abi::EnemyManager_timelineTime + abi::timer_current)),
        b::bits(at<float>(player_base, abi::Player_positionCenter)),
        b::bits(at<float>(player_base, abi::Player_positionCenter + 4)),
        at<unsigned short>(rng_base),
        at<unsigned>(rng_base + 4),
        at<unsigned short>(game_base, abi::GameManager_currentPower),
        unsigned(deaths),
        unsigned(bombs),
        at<unsigned>(game_base, abi::GameManager_score),
        unsigned(at<int>(game_base, abi::GameManager_rank)),
        unsigned(player_state),
        cleared,
        unsigned(terminal),
        unsigned(ns),
        unsigned(ns >> 32)};
    DWORD written;
    if (!WriteFile(rows, receipt.data(), sizeof(receipt), &written, nullptr) ||
        written != sizeof(receipt))
        finish("journal_write_failed", 9);
    b::save(shared + b::Sequence, frame);
    if (deaths || bombs || player_state == abi::player_dead)
        finish("hit_or_bomb", 2);
    if (terminal) {
        const auto required = story ? 6u : 1u;
        const bool complete = cleared == required && stage == int(story ? 6u : initial_stage);
        finish(complete ? "complete" : "premature_terminal", complete ? 0 : 3);
    }
    if (frame >= maximum)
        finish("frame_budget", 4);
    return 1;
}
int __cdecl init_hook(void *manager) {
    const auto result = original_init(manager);
    if (result != 0)
        finish("retail_initialization_failed", 5);
    const auto mode = at<int>(supervisor_base, abi::Supervisor_curState);
    if (mode != 2 && mode != 3)
        finish("replay_load_rejected", 5);
    if (at<int>(game_base, abi::GameManager_isInReplay))
        finish("unexpected_replay_mode", 6);
    active = {};
    if (!initialized) {
        char path[MAX_PATH], final_path[MAX_PATH];
        std::snprintf(path, sizeof(path), "%s/start.tmp", directory);
        std::snprintf(final_path, sizeof(final_path), "%s/start.txt", directory);
        FILE *start = std::fopen(path, "w");
        if (!start)
            finish("start_write_failed", 9);
        std::fprintf(start, "%d %u %u %u %u %u\n",
                     at<int>(game_base, abi::GameManager_currentStage), difficulty, shot,
                     unsigned(at<unsigned short>(game_base, abi::GameManager_randomSeed)),
                     unsigned(at<unsigned short>(game_base, abi::GameManager_currentPower)),
                     unsigned(story));
        std::fclose(start);
        if (!MoveFileExA(path, final_path, MOVEFILE_REPLACE_EXISTING))
            finish("start_publish_failed", 9);
    }
    local_frame = 0;
    initialized = true;
    if (!attached) {
        sampling.callback = sample;
        observation.callback = observe;
        using Add = int(__thiscall *)(void *, ChainElem *, int);
        if (reinterpret_cast<Add>(0x41c860)(reinterpret_cast<void *>(0x69d918), &sampling, 5))
            finish("input_registration_failed", 7);
        if (reinterpret_cast<Add>(0x41c860)(reinterpret_cast<void *>(0x69d918), &observation, 15))
            finish("observer_registration_failed", 7);
        attached = true;
    }
    return result;
}
int __cdecl menu_hook(void *menu) {
    const auto result = original_menu(menu);
    if (!requested && ++menu_frames >= 120 &&
        at<int>(supervisor_base, abi::Supervisor_curState) == 1) {
        // The normal menu handoff: the original EXE initializes and records gameplay.
        at<int>(game_base, abi::GameManager_isInReplay) = 0;
        at<unsigned char>(game_base, abi::GameManager_isInPracticeMode) = !story;
        at<unsigned char>(game_base, abi::GameManager_demoMode) = 0;
        at<int>(game_base, abi::GameManager_currentStage) = initial_stage - 1;
        at<int>(game_base, abi::GameManager_difficulty) = difficulty;
        at<unsigned char>(game_base, abi::GameManager_character) = shot / 2;
        at<unsigned char>(game_base, abi::GameManager_shotType) = shot % 2;
        at<signed char>(game_base, abi::GameManager_livesRemaining) = 2;
        at<signed char>(game_base, abi::GameManager_bombsRemaining) = 3;
        at<float>(supervisor_base, abi::Supervisor_framerateMultiplier) = 1.f;
        at<int>(supervisor_base, abi::Supervisor_curState) = 2;
        requested = true;
        return 0;
    }
    return result;
}
struct Hook {
    unsigned address, size;
    const unsigned char *signature;
    void *replacement;
    unsigned char *trampoline = nullptr;
};
bool setup() {
    char executable[MAX_PATH], limit[32];
    const auto exe_size = GetModuleFileNameA(nullptr, executable, sizeof(executable));
    const auto dir_size = GetEnvironmentVariableA("TH06_RETAIL_DIR", directory, sizeof(directory));
    if (!exe_size || exe_size >= sizeof(executable) || !dir_size ||
        dir_size > sizeof(directory) - 48 || !retail_probe::pinned_executable(executable))
        return false;
    if (!QueryPerformanceFrequency(&frequency))
        return false;
    const auto setting = [&](const char *name, unsigned &value, unsigned low, unsigned high) {
        char text[32];
        const auto n = GetEnvironmentVariableA(name, text, sizeof(text));
        if (!n || n >= sizeof(text))
            return false;
        char *end;
        const auto parsed = std::strtoul(text, &end, 10);
        if (*end || parsed < low || parsed > high)
            return false;
        value = unsigned(parsed);
        return true;
    };
    unsigned story_setting = 0;
    if (!setting("TH06_SHADOW_STAGE", initial_stage, 1, 6) ||
        !setting("TH06_SHADOW_SHOT", shot, 0, 3) ||
        !setting("TH06_SHADOW_STORY", story_setting, 0, 1))
        return false;
    story = story_setting;
    if (story && initial_stage != 1)
        return false;
    const auto limit_size = GetEnvironmentVariableA("TH06_RETAIL_UPDATES", limit, sizeof(limit));
    if (limit_size >= sizeof(limit))
        return false;
    if (limit_size) {
        char *end;
        const auto n = std::strtoul(limit, &end, 10);
        if (*end || !n || n > 1000000)
            return false;
        maximum = unsigned(n);
    }
    char path[MAX_PATH];
    std::snprintf(path, sizeof(path), "%s/bridge.bin", directory);
    HANDLE file =
        CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE || GetFileSize(file, nullptr) != b::map_bytes)
        return false;
    mapping = CreateFileMappingA(file, nullptr, PAGE_READWRITE, 0, b::map_bytes, nullptr);
    CloseHandle(file);
    if (!mapping)
        return false;
    shared = static_cast<std::uint32_t *>(
        MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, b::map_bytes));
    if (!shared || b::load(shared + b::Magic) != b::magic ||
        b::load(shared + b::Version) != b::version)
        return false;
    std::snprintf(path, sizeof(path), "%s/receipts.bin", directory);
    rows = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
    if (rows == INVALID_HANDLE_VALUE)
        return false;
    b::save(shared + b::Ready, 1);
    const unsigned char menu[]{0x55, 0x8b, 0xec, 0x81, 0xec, 0xb4, 0x00, 0x00, 0x00};
    const unsigned char init[]{0x55, 0x8b, 0xec, 0x83, 0xec, 0x28};
    Hook hooks[]{{0x43579f, sizeof(menu), menu, reinterpret_cast<void *>(menu_hook)},
                 {0x41bb02, sizeof(init), init, reinterpret_cast<void *>(init_hook)}};
    for (auto &h : hooks) {
        if (std::memcmp(reinterpret_cast<void *>(h.address), h.signature, h.size))
            return false;
        h.trampoline = static_cast<unsigned char *>(
            VirtualAlloc(nullptr, h.size + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!h.trampoline)
            return false;
        std::memcpy(h.trampoline, h.signature, h.size);
        h.trampoline[h.size] = 0xe9;
        const auto back = h.address + h.size - unsigned(h.trampoline + h.size + 5);
        std::memcpy(h.trampoline + h.size + 1, &back, 4);
    }
    original_menu = reinterpret_cast<Callback>(hooks[0].trampoline);
    original_init = reinterpret_cast<Callback>(hooks[1].trampoline);
    for (auto &h : hooks) {
        DWORD protection;
        auto *p = reinterpret_cast<unsigned char *>(h.address);
        if (!VirtualProtect(p, h.size, PAGE_EXECUTE_READWRITE, &protection))
            return false;
        p[0] = 0xe9;
        const auto jump = unsigned(h.replacement) - h.address - 5;
        std::memcpy(p + 1, &jump, 4);
        std::memset(p + 5, 0x90, h.size - 5);
        DWORD ignored;
        if (!VirtualProtect(p, h.size, protection, &ignored))
            return false;
        FlushInstructionCache(GetCurrentProcess(), p, h.size);
    }
    return true;
}
} // namespace
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(module);
    return TRUE;
}
extern "C" __declspec(dllexport) DWORD WINAPI Th06RetailInitialize(LPVOID) {
    try {
        return setup() ? 1 : 0;
    } catch (...) {
        return 0;
    }
}
