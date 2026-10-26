#include "identity.hpp"
#include "quad_batch.hpp"
#include "render_timing.hpp"
#include "retail_abi.hpp"
#include "th06/retail_clock.hpp"
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
Callback original_bullets = nullptr;
wine_replay::QuadBatch quad_batch;
bool software_bullets = false, batch_bullets = false, software_drawing = false;
unsigned software_entries = 0, software_restores = 0;
struct StateRow {
    std::uint32_t frame, stage, local_frame, input;
    float x, y;
    std::int32_t timeline;
    std::uint32_t rng, rng_calls, power;
    std::int32_t deaths, bombs;
    std::uint32_t score;
    std::int32_t rank, player_state;
};
static_assert(sizeof(StateRow) == 60);
wine_replay::MappedJournal<StateRow> states;
wine_replay::MappedJournal<th06::render_timing::Update> update_times;
wine_replay::RenderTiming render_timing;
th06::retail::LimiterPrecision limiter_precision;
LARGE_INTEGER frequency{}, first_tick{};
bool profile_render = false, precise_limiter = false, timer_hooked = false;
bool sleep_limiter = false;
using Milliseconds = DWORD(WINAPI *)();
Milliseconds limiter_milliseconds = nullptr;
unsigned limiter_polls = 0;
unsigned limiter_sleeps = 0;
unsigned initial_stage = 1;
bool single_stage = false;
using BeginPeriod = unsigned(WINAPI *)(unsigned);
BeginPeriod original_begin_period = nullptr;
unsigned WINAPI begin_period(unsigned period) {
    const auto result = original_begin_period(period);
    if (__builtin_return_address(0) == reinterpret_cast<void *>(0x4208f5)) {
        limiter_precision.enter();
        ++limiter_polls;
        if (sleep_limiter &&
            th06::retail::coarse_wait_has_slack(limiter_milliseconds(), at<double>(0x6c6bf8))) {
            Sleep(1);
            ++limiter_sleeps;
        }
    }
    return result;
}
void restore_precision() {
    limiter_precision.restore();
}
char directory[MAX_PATH], replay_path[256];
unsigned frame = 0, local_frame = 0, cleared = 0, menu_frames = 0, maximum = 180000;
unsigned shot = 0, difficulty = 3;
bool requested = false, initialized = false, attached = false;
ChainElem observation;

[[noreturn]] void finish(const char *reason, unsigned code) {
    if (!render_timing.close()) {
        reason = "render_timing_write_failed";
        code = 9;
    }
    const bool updates_good = update_times.close(), states_good = states.close();
    if (!updates_good || !states_good) {
        reason = "state_or_update_journal_close_failed";
        code = 9;
    }
    char path[MAX_PATH];
    std::snprintf(path, sizeof(path), "%s/outcome.json", directory);
    if (FILE *out = std::fopen(path, "w")) {
        std::fprintf(
            out,
            "{\"reason\":\"%s\",\"frames\":%u,\"stage\":%d,\"cleared_stages\":%u,"
            "\"deaths\":%d,\"bombs\":%d,\"supervisor_state\":%d,\"retail_nmnb_complete\":%s,"
            "\"journal_mode\":\"mapped_v2\",\"state_records\":%u,\"initial_stage\":%u,\"single_"
            "stage\":%s,\"precise_limiter\":%s,"
            "\"limiter_entries\":%u,\"limiter_restores\":%u,\"limiter_polls\":%u,"
            "\"sleep_limiter\":%s,\"limiter_sleeps\":%u,"
            "\"software_bullets\":%s,\"software_entries\":%u,\"software_restores\":%u,"
            "\"batch_bullets\":%s,\"batch_draw_calls\":%llu,\"batch_submitted\":%llu,"
            "\"batch_quads\":%llu,\"batch_scopes\":%llu}\n",
            reason, frame, at<int>(game_base, abi::GameManager_currentStage), cleared,
            at<int>(game_base, abi::GameManager_deaths),
            at<int>(game_base, abi::GameManager_bombsUsed),
            at<int>(supervisor_base, abi::Supervisor_curState), code == 0 ? "true" : "false",
            states.count(), initial_stage, single_stage ? "true" : "false",
            precise_limiter ? "true" : "false", limiter_precision.entries,
            limiter_precision.restores, limiter_polls, sleep_limiter ? "true" : "false",
            limiter_sleeps, software_bullets ? "true" : "false", software_entries,
            software_restores, batch_bullets ? "true" : "false", quad_batch.draw_calls,
            quad_batch.submitted, quad_batch.copied_quads, quad_batch.scopes);
        std::fclose(out);
    }
    ExitProcess(code);
}
int __cdecl observe(void *) {
    if (!initialized || !at<unsigned char>(game_base, abi::GameManager_isInMenu))
        return 1;
    if (limiter_precision.active)
        finish("limiter_precision_reached_gameplay", 9);
    if (software_drawing || quad_batch.active())
        finish("software_draw_scope_reached_gameplay", 9);
    const auto stage = at<int>(game_base, abi::GameManager_currentStage);
    const auto input = at<unsigned short>(0x69d904);
    const auto x = at<float>(player_base, abi::Player_positionCenter);
    const auto y = at<float>(player_base, abi::Player_positionCenter + 4);
    const auto state = at<int>(supervisor_base, abi::Supervisor_curState);
    const auto deaths = at<int>(game_base, abi::GameManager_deaths);
    const auto bombs = at<int>(game_base, abi::GameManager_bombsUsed);
    const auto player_state = at<signed char>(player_base, abi::Player_playerState);
    ++frame;
    if (profile_render) {
        LARGE_INTEGER time;
        QueryPerformanceCounter(&time);
        if (frame == 1) {
            first_tick = time;
            char path[MAX_PATH];
            std::snprintf(path, sizeof(path), "%s/clock.json", directory);
            if (FILE *out = std::fopen(path, "w")) {
                using Milliseconds = DWORD(WINAPI *)();
                const auto clock = reinterpret_cast<Milliseconds>(
                    GetProcAddress(GetModuleHandleA("winmm.dll"), "timeGetTime"));
                std::fprintf(
                    out,
                    "{\"x87_control\":%u,\"milliseconds\":%lu,\"limiter_previous\":%.12g,"
                    "\"graphics_opts\":%u,\"windowed\":%u,\"color_16bit\":%u}\n",
                    unsigned(th06::retail::x87_control()), clock ? clock() : 0ul,
                    at<double>(0x6c6bf8),
                    at<unsigned>(supervisor_base,
                                 abi::Supervisor_cfg + abi::GameConfiguration_opts),
                    unsigned(at<unsigned char>(
                        supervisor_base, abi::Supervisor_cfg + abi::GameConfiguration_windowed)),
                    unsigned(at<unsigned char>(supervisor_base,
                                               abi::Supervisor_cfg +
                                                   abi::GameConfiguration_colorMode16bit)));
                std::fclose(out);
            }
        }
        render_timing.context(frame, unsigned(stage), first_tick.QuadPart);
        const auto ns = render_timing.elapsed_ns(time.QuadPart - first_tick.QuadPart);
        const th06::render_timing::Update row{frame, unsigned(stage), ns};
        if (!update_times.append(row) || render_timing.failed())
            finish("render_timing_write_failed", 9);
    }
    const StateRow row{frame,
                       unsigned(stage),
                       local_frame++,
                       input,
                       x,
                       y,
                       at<int>(enemy_base, abi::EnemyManager_timelineTime + abi::timer_current),
                       unsigned(at<unsigned short>(rng_base)),
                       at<unsigned>(rng_base + 4),
                       unsigned(at<unsigned short>(game_base, abi::GameManager_currentPower)),
                       deaths,
                       bombs,
                       at<unsigned>(game_base, abi::GameManager_score),
                       at<int>(game_base, abi::GameManager_rank),
                       int(player_state)};
    if (!states.append(row))
        finish("state_journal_write_failed", 9);
    if (deaths || bombs || player_state == abi::player_dead)
        finish("hit_or_bomb", 2);
    if (state == 3 || state == 8 || state == 7 || state == 10)
        ++cleared;
    if (single_stage && (state == 3 || state == 8 || state == 7 || state == 10))
        finish(stage == int(initial_stage) && cleared == 1 ? "complete" : "premature_terminal",
               stage == int(initial_stage) && cleared == 1 ? 0 : 3);
    if (state == 8 || state == 7 || state == 10) {
        const auto required = difficulty == 0 ? 5u : 6u;
        const auto remaining = required - initial_stage + 1;
        finish(stage == int(required) && cleared == remaining ? "complete" : "premature_terminal",
               stage == int(required) && cleared == remaining ? 0 : 3);
    }
    if (frame >= maximum)
        finish("frame_budget", 4);
    return 1;
}
int __cdecl bullets_hook(void *manager) {
    if (software_drawing)
        finish("software_draw_scope_reentered", 9);
    if (batch_bullets && !quad_batch.enter())
        finish("quad_batch_scope_failed", 9);
    // Use the source's CPU-transformed branch only within its draw callback.
    // The one-byte flag and every draw submission are restored before gameplay.
    auto &hardware =
        at<unsigned char>(supervisor_base, abi::Supervisor_hasD3dHardwareVertexProcessing);
    const auto previous = hardware;
    software_drawing = true;
    ++software_entries;
    hardware = 0;
    const auto result = original_bullets(manager);
    hardware = previous;
    software_drawing = false;
    ++software_restores;
    if (batch_bullets)
        quad_batch.leave();
    if (quad_batch.failed())
        finish("quad_batch_submission_failed", 9);
    return result;
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
    if (software_bullets) {
        auto *root = reinterpret_cast<ChainElem *>(0x69d938);
        ChainElem *found = nullptr;
        unsigned visited = 0;
        for (auto *node = root->next; node && node != root; node = node->next) {
            if (++visited > 128)
                finish("software_draw_chain_invalid", 9);
            if (node->priority == 9) {
                if (found)
                    finish("software_draw_chain_ambiguous", 9);
                found = node;
            }
        }
        if (!found || !found->callback || !found->arg)
            finish("software_draw_chain_missing", 9);
        if (found->callback != bullets_hook) {
            const auto address = reinterpret_cast<std::uintptr_t>(found->callback);
            if (address < 0x401000 || address >= 0x469000)
                finish("software_draw_callback_identity_failed", 9);
            original_bullets = found->callback;
            found->callback = bullets_hook;
        }
    }
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
    if (profile_render &&
        !render_timing.install(at<void *>(supervisor_base, abi::Supervisor_d3dDevice)))
        finish("render_timing_install_failed", 9);
    if (batch_bullets &&
        !quad_batch.install(at<void *>(supervisor_base, abi::Supervisor_d3dDevice)))
        finish("quad_batch_install_failed", 9);
    if (precise_limiter && !timer_hooked) {
        if (sleep_limiter) {
            limiter_milliseconds = reinterpret_cast<Milliseconds>(
                GetProcAddress(GetModuleHandleA("winmm.dll"), "timeGetTime"));
            if (!limiter_milliseconds)
                finish("limiter_clock_identity_failed", 9);
        }
        if (at<unsigned char>(supervisor_base,
                              abi::Supervisor_cfg + abi::GameConfiguration_frameskipConfig))
            finish("precise_limiter_requires_no_frameskip", 9);
        auto **entry = reinterpret_cast<void **>(0x46a250);
        const auto expected = GetProcAddress(GetModuleHandleA("winmm.dll"), "timeBeginPeriod");
        if (*entry != reinterpret_cast<void *>(expected))
            finish("limiter_import_identity_failed", 9);
        DWORD protection;
        if (!VirtualProtect(entry, sizeof(void *), PAGE_READWRITE, &protection))
            finish("limiter_import_protection_failed", 9);
        original_begin_period = reinterpret_cast<BeginPeriod>(*entry);
        render_timing.before_present(restore_precision);
        InterlockedExchangePointer(entry, reinterpret_cast<void *>(begin_period));
        DWORD ignored;
        if (!VirtualProtect(entry, sizeof(void *), protection, &ignored))
            finish("limiter_import_protection_failed", 9);
        timer_hooked = true;
    }
    const auto result = original_menu(menu);
    if (!requested && ++menu_frames >= 120 &&
        at<int>(supervisor_base, abi::Supervisor_curState) == 1) {
        // Same fields and removal result as MainMenu::ReplayHandling. The
        // original replay manager subsequently validates and restores the RPY.
        at<int>(game_base, abi::GameManager_isInReplay) = 1;
        at<unsigned char>(game_base, abi::GameManager_isInPracticeMode) = 0;
        at<unsigned char>(game_base, abi::GameManager_demoMode) = 0;
        at<int>(game_base, abi::GameManager_currentStage) = int(initial_stage) - 1;
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
    if (!QueryPerformanceFrequency(&frequency))
        return false;
    char option[32];
    profile_render = GetEnvironmentVariableA("TH06_REPLAY_RENDER_TIMING", option, sizeof(option)) &&
                     std::strcmp(option, "1") == 0;
    precise_limiter =
        GetEnvironmentVariableA("TH06_REPLAY_PRECISE_LIMITER", option, sizeof(option)) &&
        std::strcmp(option, "1") == 0;
    sleep_limiter = GetEnvironmentVariableA("TH06_REPLAY_SLEEP_LIMITER", option, sizeof(option)) &&
                    std::strcmp(option, "1") == 0;
    if (sleep_limiter && !precise_limiter)
        return false;
    software_bullets =
        GetEnvironmentVariableA("TH06_REPLAY_SOFTWARE_BULLETS", option, sizeof(option)) &&
        std::strcmp(option, "1") == 0;
    batch_bullets = GetEnvironmentVariableA("TH06_REPLAY_BATCH_BULLETS", option, sizeof(option)) &&
                    std::strcmp(option, "1") == 0;
    if (batch_bullets && !software_bullets)
        return false;
    single_stage = GetEnvironmentVariableA("TH06_REPLAY_SINGLE_STAGE", option, sizeof(option)) &&
                   std::strcmp(option, "1") == 0;
    if (precise_limiter && !profile_render)
        return false;
    const auto stage_size =
        GetEnvironmentVariableA("TH06_REPLAY_START_STAGE", option, sizeof(option));
    if (stage_size >= sizeof(option))
        return false;
    if (stage_size) {
        char *end;
        const auto n = std::strtoul(option, &end, 10);
        if (*end || n < 1 || n > 6)
            return false;
        initial_stage = unsigned(n);
    }
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
    if (profile_render) {
        std::snprintf(path, sizeof(path), "%s/render-times.bin", directory);
        if (!render_timing.open(path, frequency.QuadPart, maximum))
            return false;
        std::snprintf(path, sizeof(path), "%s/update-times.bin", directory);
        if (!update_times.open(path, th06::render_timing::update_header[0], maximum))
            return false;
    }
    std::snprintf(path, sizeof(path), "%s/retail-states.bin", directory);
    if (!states.open(path, 0x36545354, maximum))
        return false;
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
