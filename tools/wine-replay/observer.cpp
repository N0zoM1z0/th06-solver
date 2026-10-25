#include "identity.hpp"
#include "retail_abi.hpp"
#include <cstdint>
#include <cstdlib>
#include <cstring>

// Read-only gameplay observation for the pinned Japanese 1.02h image. Startup
// reproduces ReplayHandling's menu handoff; the EXE owns replay input, RNG,
// collisions, stage transitions and rendering throughout playback.
namespace {
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
FILE *rows = nullptr;
char directory[MAX_PATH], replay_path[256];
unsigned frame = 0, local_frame = 0, cleared = 0, menu_frames = 0, maximum = 180000;
unsigned shot = 0, difficulty = 3;
bool requested = false, initialized = false, attached = false;
ChainElem observation;

[[noreturn]] void finish(const char *reason, unsigned code) {
    if (rows) {
        std::fflush(rows);
        std::fclose(rows);
        rows = nullptr;
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
int __cdecl observe(void *) {
    if (!initialized || !at<unsigned char>(game_base, abi::GameManager_isInMenu))
        return 1;
    const auto stage = at<int>(game_base, abi::GameManager_currentStage);
    const auto input = at<unsigned short>(0x69d904);
    const auto x = at<float>(player_base, abi::Player_positionCenter);
    const auto y = at<float>(player_base, abi::Player_positionCenter + 4);
    const auto state = at<int>(supervisor_base, abi::Supervisor_curState);
    const auto deaths = at<int>(game_base, abi::GameManager_deaths);
    const auto bombs = at<int>(game_base, abi::GameManager_bombsUsed);
    const auto player_state = at<signed char>(player_base, abi::Player_playerState);
    ++frame;
    std::fprintf(rows, "%u,%d,%u,%u,%.9g,%.9g,%d,%u,%u,%u,%d,%d,%u,%d,%d\n", frame, stage,
                 local_frame++, input, x, y,
                 at<int>(enemy_base, abi::EnemyManager_timelineTime + abi::timer_current),
                 unsigned(at<unsigned short>(rng_base)), at<unsigned>(rng_base + 4),
                 unsigned(at<unsigned short>(game_base, abi::GameManager_currentPower)), deaths,
                 bombs, at<unsigned>(game_base, abi::GameManager_score),
                 at<int>(game_base, abi::GameManager_rank), int(player_state));
    if (frame % 600 == 0)
        std::fflush(rows);
    if (deaths || bombs || player_state == abi::player_dead)
        finish("hit_or_bomb", 2);
    if (state == 3 || state == 8 || state == 7 || state == 10)
        ++cleared;
    if (state == 8 || state == 7 || state == 10) {
        const auto required = difficulty == 0 ? 5u : 6u;
        finish(stage == int(required) && cleared == required ? "complete" : "premature_terminal",
               stage == int(required) && cleared == required ? 0 : 3);
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
    if (!at<int>(game_base, abi::GameManager_isInReplay))
        finish("not_replay_mode", 6);
    local_frame = 0;
    initialized = true;
    if (!attached) {
        observation.callback = observe;
        using Add = int(__thiscall *)(void *, ChainElem *, int);
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
        // Same fields and removal result as MainMenu::ReplayHandling. The
        // original replay manager subsequently validates and restores the RPY.
        at<int>(game_base, abi::GameManager_isInReplay) = 1;
        at<unsigned char>(game_base, abi::GameManager_isInPracticeMode) = 0;
        at<unsigned char>(game_base, abi::GameManager_demoMode) = 0;
        at<int>(game_base, abi::GameManager_currentStage) = 0;
        at<int>(game_base, abi::GameManager_difficulty) = difficulty;
        at<unsigned char>(game_base, abi::GameManager_character) = shot / 2;
        at<unsigned char>(game_base, abi::GameManager_shotType) = shot % 2;
        at<signed char>(game_base, abi::GameManager_livesRemaining) = 2;
        at<signed char>(game_base, abi::GameManager_bombsRemaining) = 3;
        std::strcpy(reinterpret_cast<char *>(game_base + abi::GameManager_replayFile), replay_path);
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
    const auto replay_size =
        GetEnvironmentVariableA("TH06_RETAIL_REPLAY", replay_path, sizeof(replay_path));
    if (!exe_size || exe_size >= sizeof(executable) || !dir_size ||
        dir_size > sizeof(directory) - 48 || !replay_size || replay_size >= sizeof(replay_path) ||
        !retail_probe::pinned_executable(executable))
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
    if (FILE *rpy = std::fopen(replay_path, "rb")) {
        unsigned char header[8];
        const auto n = std::fread(header, 1, sizeof(header), rpy);
        std::fclose(rpy);
        if (n != sizeof(header) || std::memcmp(header, "T6RP", 4) || header[6] > 3 || header[7] > 3)
            return false;
        shot = header[6];
        difficulty = header[7];
    } else
        return false;
    char path[MAX_PATH];
    std::snprintf(path, sizeof(path), "%s/retail.csv", directory);
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
        return false;
    rows = std::fopen(path, "w");
    if (!rows)
        return false;
    std::fprintf(rows, "frame,stage,local_frame,input,x,y,timeline,rng,rng_calls,power,deaths,"
                       "bombs,score,rank,player_state\n");
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
