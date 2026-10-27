#include "th06/session.hpp"
#include "AsciiManager.hpp"
#include "BulletManager.hpp"
#include "ChainPriorities.hpp"
#include "EclManager.hpp"
#include "EffectManager.hpp"
#include "EnemyManager.hpp"
#include "Stage.hpp"
#include "stdafx.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace th06 {
ChainCallbackResult GameManager_OnUpdate(GameManager *);
}
namespace th06::solver {
namespace {
bool active = false;
struct PowerTarget {
    double squared_distance = 1e30;
    float x = 192;
    bool found = false;
};
PowerTarget power_target(float x, float y) {
    PowerTarget target;
    // Item positions guide proposals only. Original ItemManager owns movement,
    // collision, power thresholds and rewards. Exclude its failed-spawn dummy.
    for (unsigned i = 0; i < MAX_ITEMS; ++i) {
        const auto &item = g_ItemManager.items[i];
        if (!item.isInUse || (item.itemType != ITEM_POWER_SMALL &&
                              item.itemType != ITEM_POWER_BIG && item.itemType != ITEM_FULL_POWER))
            continue;
        const double dx = item.currentPosition.x - x, dy = item.currentPosition.y - y;
        const double weight = item.itemType == ITEM_POWER_SMALL ? 1. : 1.7;
        const double distance = (dx * dx + dy * dy * .25) / (weight * weight);
        if (distance < target.squared_distance)
            target = {distance, item.currentPosition.x, true};
    }
    return target;
}
ChainElem game_chain;
void require(ZunResult result, const char *message) {
    if (result != ZUN_SUCCESS)
        throw std::runtime_error(message);
}
LaserForecast laser_snapshot(const Laser &l) {
    LaserForecast out;
    out.x = l.pos.x;
    out.y = l.pos.y;
    out.angle = l.angle;
    out.start = l.startOffset;
    out.end = l.endOffset;
    out.length = l.startLength;
    out.width = l.width;
    out.speed = l.speed;
    out.timer = l.timer.current;
    out.state = l.state;
    out.activation = l.startTime;
    out.hitbox_start = l.hitboxStartTime;
    out.duration = l.duration;
    out.despawn = l.despawnDuration;
    out.hitbox_end = l.hitboxEndDelay;
    out.flags = l.flags;
    out.present = l.inUse;
    return out;
}
} // namespace
Session::Session(const StartConfig &c) {
    if (active)
        throw std::logic_error("one Session per process");
    if (c.stage < 1 || c.stage > 7 || c.shot < 0 || c.shot > 3 || c.difficulty < 0 ||
        c.difficulty > 4 || (c.stage == 7) != (c.difficulty == 4) || c.power < 0 || c.power > 128)
        throw std::invalid_argument("invalid stage/difficulty/shot/power");
    if (c.story && c.stage != 1)
        throw std::invalid_argument("complete story must start at stage 1");
    active = true;
    story_ = c.story;
    required_stages_ = c.story ? (c.difficulty == EASY ? 5 : 6) : 1;
    g_Supervisor.curState = SUPERVISOR_STATE_GAMEMANAGER;
    g_Supervisor.framerateMultiplier = g_Supervisor.effectiveFramerateMultiplier = 1.f;
    g_Supervisor.cfg.lifeCount = g_Supervisor.defaultConfig.lifeCount = 2;
    g_Supervisor.cfg.bombCount = g_Supervisor.defaultConfig.bombCount = 3;
    g_Supervisor.d3dIface = Direct3DCreate8(D3D_SDK_VERSION);
    if (!g_Supervisor.d3dIface || FAILED(g_Supervisor.d3dIface->CreateDevice(
                                      0, D3DDEVTYPE_HAL, nullptr, 0,
                                      &g_Supervisor.presentParameters, &g_Supervisor.d3dDevice)))
        throw std::runtime_error("CPU D3D resource initialization failed");
    g_AnmManager = new AnmManager;
    g_GameManager.currentStage = c.stage;
    g_GameManager.difficulty = static_cast<Difficulty>(c.difficulty);
    g_GameManager.character = c.shot / 2;
    g_GameManager.shotType = c.shot % 2;
    g_GameManager.currentPower = c.power;
    g_GameManager.livesRemaining = 2;
    g_GameManager.bombsRemaining = 3;
    // GameManager_AddedCallback disables score extends in Extra by selecting
    // the final (unreachable) 1,900,000,000-point entry in its five-score table.
    g_GameManager.livesEarned = c.difficulty == EXTRA ? 4 : 0;
    g_GameManager.rank = 16;
    g_GameManager.minRank = c.difficulty == 0 ? 12 : c.difficulty == 4 ? 14 : 10;
    g_GameManager.maxRank = c.difficulty == 0 ? 20 : c.difficulty == 4 ? 18 : 32;
    // Extra uses the normal source entry and result path, not practice records.
    g_GameManager.isInPracticeMode = !c.story && c.stage != 7;
    g_GameManager.isInMenu = true;
    g_GameManager.gameRegionScreenPos = {32, 16};
    g_GameManager.gameRegionSize = {384, 448};
    g_GameManager.playerMovementAreaTopLeftPos = {8, 16};
    g_GameManager.playerMovementAreaSize = {368, 416};
    g_Rng.Initialize(c.seed);
    require(g_AnmManager->LoadAnm(ANM_FILE_TEXT, "data/text.anm", ANM_OFFSET_TEXT),
            "text ANM initialization failed");
    require(AsciiManager_RegisterChain(), "ASCII initialization failed");
    game_chain.SetCallback(reinterpret_cast<ChainCallback>(GameManager_OnUpdate));
    game_chain.arg = &g_GameManager;
    require(g_FuncChain.AddToCalcChain(&game_chain, TH_CHAIN_PRIO_CALC_GAMEMANAGER),
            "game chain failed");
    initialize_stage();
    started_ = true;
}
void Session::initialize_stage() {
    const auto stage = g_GameManager.currentStage;
    g_Rng.ResetGenerationCount();
    g_GameManager.randomSeed = g_Rng.GetSeed();
    require(Stage_RegisterChain(stage), "stage initialization failed");
    require(Player_RegisterChain(0), "player initialization failed");
    require(BulletManager_RegisterChain("data/etama.anm"), "bullet initialization failed");
    static const char *enemy[][2] = {{"data/stg1enm.anm", "data/stg1enm2.anm"},
                                     {"data/stg2enm.anm", "data/stg2enm2.anm"},
                                     {"data/stg3enm.anm", nullptr},
                                     {"data/stg4enm.anm", nullptr},
                                     {"data/stg5enm.anm", "data/stg5enm2.anm"},
                                     {"data/stg6enm.anm", "data/stg6enm2.anm"},
                                     {"data/stg7enm.anm", "data/stg7enm2.anm"}};
    require(EnemyManager_RegisterChain(enemy[stage - 1][0], enemy[stage - 1][1]),
            "enemy initialization failed");
    const auto ecl = "data/ecldata" + std::to_string(stage) + ".ecl";
    require(g_EclManager.Load(ecl.c_str()), "ECL initialization failed");
    require(EffectManager_RegisterChain(), "effect initialization failed");
    require(Gui_RegisterChain(), "GUI initialization failed");
}
void Session::advance_stage() {
    // This is the NEXT_STAGE path of GameManager's deleted/added callbacks.
    // Keep that state set while cutting/registering: Player and GUI use it to
    // retain their shared ANMs. The source resets stage actors and bullet pools,
    // but carries gameplay RNG, rank, resources, score and player statistics.
    Stage_CutChain();
    BulletManager_CutChain();
    Player_CutChain();
    EnemyManager_CutChain();
    g_EclManager.Unload();
    EffectManager_CutChain();
    Gui_CutChain();
    auto &g = g_GameManager;
    g.guiScore = g.score;
    g.nextScoreIncrement = 0;
    g.subRank = 0;
    g.pointItemsCollectedInStage = 0;
    g.grazeInStage = 0;
    g.isInGameMenu = false;
    ++g.currentStage;
    initialize_stage();
    g.isInRetryMenu = false;
    g.isInMenu = true;
    g.isTimeStopped = false;
    g.score = 0;
    g.isGameCompleted = false;
    g_AsciiManager.InitializeVms();
    g_Supervisor.curState = SUPERVISOR_STATE_GAMEMANAGER;
}
Session::~Session() {
    if (started_) {
        g_FuncChain.Release();
        g_EclManager.Unload();
        delete g_AnmManager;
        g_AnmManager = nullptr;
        g_Supervisor.d3dDevice->Release();
        g_Supervisor.d3dIface->Release();
    }
    active = false;
}
State Session::state() const {
    State s;
    s.frame = frame_;
    s.x = g_Player.positionCenter.x;
    s.y = g_Player.positionCenter.y;
    s.stage = g_GameManager.currentStage;
    s.timeline = g_EnemyManager.timelineTime.current;
    for (const auto &enemy : g_EnemyManager.enemies)
        s.enemies += enemy.flags.isSlotOccupied;
    for (const auto &bullet : g_BulletManager.bullets)
        s.bullets += bullet.state != BULLET_STATE_INACTIVE;
    for (const auto &laser : g_BulletManager.lasers)
        s.lasers += laser.inUse;
    s.deaths = g_GameManager.deaths;
    s.bombs = g_GameManager.bombsUsed;
    s.power = g_GameManager.currentPower;
    s.player_state = g_Player.playerState;
    s.rng = g_Rng.seed;
    s.rng_calls = g_Rng.generationCount;
    s.score = g_GameManager.score;
    s.hit = s.deaths != 0 || s.bombs != 0 || s.player_state == PLAYER_STATE_DEAD;
    s.complete = terminal();
    return s;
}
bool Session::terminal() const {
    return g_Supervisor.curState == SUPERVISOR_STATE_RESULTSCREEN_FROMGAME ||
           g_Supervisor.curState == SUPERVISOR_STATE_ENDING ||
           (!story_ && g_Supervisor.curState == SUPERVISOR_STATE_NEXT_STAGE);
}
State Session::step(std::uint16_t input) {
    if (terminal())
        throw std::logic_error("stage already complete");
    constexpr std::uint16_t supported =
        TH_BUTTON_DIRECTION | TH_BUTTON_SHOOT | TH_BUTTON_FOCUS | TH_BUTTON_SKIP;
    if (input & ~supported)
        throw std::invalid_argument("unsupported input (bomb/menu disallowed)");
    if (story_ && g_Supervisor.curState == SUPERVISOR_STATE_NEXT_STAGE)
        advance_stage();
    if (track_lasers_)
        for (unsigned i = 0; i < previous_lasers_.size(); ++i)
            previous_lasers_[i] = laser_snapshot(g_BulletManager.lasers[i]);
    g_PreviousFrameKeys = g_CurFrameInput;
    g_CurFrameInput = input;
    if (g_FuncChain.RunCalcChain() < 0)
        throw std::runtime_error("calc chain failed");
    ++frame_;
    if (track_lasers_)
        for (unsigned i = 0; i < laser_motion_.size(); ++i) {
            auto &motion = laser_motion_[i];
            motion = {};
            const auto &before = previous_lasers_[i];
            const auto &after = g_BulletManager.lasers[i];
            // Pooled/restarted lasers and phase resets have no measured motion.
            if (before.present && after.inUse && before.state == after.state &&
                after.timer.current == before.timer + 1 && before.width == after.width &&
                before.activation == after.startTime && before.duration == after.duration) {
                motion.dx = after.pos.x - before.x;
                motion.dy = after.pos.y - before.y;
                motion.angular_speed = std::remainder(after.angle - before.angle, 6.28318530718f);
            }
        }
    if (g_Supervisor.curState == SUPERVISOR_STATE_NEXT_STAGE || terminal())
        ++cleared_stages_;
    if (story_ && terminal() &&
        (unsigned(g_GameManager.currentStage) != required_stages_ ||
         cleared_stages_ != required_stages_))
        throw std::runtime_error("story terminated before all required stages were cleared");
    return state();
}

std::uint64_t Session::observation_digest() const {
    // Deliberately hash named observable scalars, never padding or pointers.
    // This detects native replay divergence; it is not a retail physics oracle.
    std::uint64_t hash = 14695981039346656037ULL;
    auto integer = [&](std::uint64_t value) {
        for (unsigned i = 0; i < 8; ++i) {
            hash ^= (value >> (i * 8)) & 255;
            hash *= 1099511628211ULL;
        }
    };
    auto scalar = [&](float value) {
        std::uint32_t bits;
        std::memcpy(&bits, &value, 4);
        integer(bits);
    };
    const auto s = state();
    integer(s.frame);
    scalar(s.x);
    scalar(s.y);
    integer(s.stage);
    integer(s.timeline);
    integer(s.enemies);
    integer(s.bullets);
    integer(s.lasers);
    integer(s.deaths);
    integer(s.bombs);
    integer(s.power);
    integer(s.player_state);
    integer(s.rng);
    integer(s.rng_calls);
    integer(s.score);
    integer(s.complete);
    integer(s.hit);
    integer(g_Player.invulnerabilityTimer.current);
    integer(g_Player.fireBulletTimer.current);
    integer(g_GameManager.rank);
    integer(g_GameManager.subRank);
    for (const auto &enemy : g_EnemyManager.enemies) {
        integer(enemy.flags.isSlotOccupied);
        if (!enemy.flags.isSlotOccupied)
            continue;
        scalar(enemy.position.x);
        scalar(enemy.position.y);
        scalar(enemy.axisSpeed.x);
        scalar(enemy.axisSpeed.y);
        integer(enemy.life);
        integer(enemy.phaseTimer.current);
        integer(enemy.currentContext.subId);
        integer(enemy.currentContext.time.current);
        integer(enemy.primaryVm.currentTimeInScript.current);
        integer(enemy.primaryVm.activeSpriteIndex);
    }
    for (const auto &bullet : g_BulletManager.bullets) {
        integer(bullet.state);
        if (bullet.state == BULLET_STATE_INACTIVE)
            continue;
        scalar(bullet.pos.x);
        scalar(bullet.pos.y);
        scalar(bullet.velocity.x);
        scalar(bullet.velocity.y);
        scalar(bullet.speed);
        scalar(bullet.angle);
        integer(bullet.timer.current);
        integer(bullet.exFlags);
        integer(bullet.sprites.spriteBullet.activeSpriteIndex);
    }
    for (const auto &laser : g_BulletManager.lasers) {
        integer(laser.inUse);
        if (!laser.inUse)
            continue;
        scalar(laser.pos.x);
        scalar(laser.pos.y);
        scalar(laser.angle);
        scalar(laser.width);
        scalar(laser.startOffset);
        scalar(laser.endOffset);
        integer(laser.timer.current);
        integer(laser.state);
    }
    for (const auto &bullet : g_Player.bullets) {
        integer(bullet.bulletState);
        if (bullet.bulletState == PLAYER_BULLET_STATE_INACTIVE)
            continue;
        scalar(bullet.position.x);
        scalar(bullet.position.y);
        scalar(bullet.velocity.x);
        scalar(bullet.velocity.y);
        integer(bullet.damage);
        integer(bullet.lifetime.current);
    }
    return hash;
}

double Session::proposal_cost(bool laser_aware, bool power_aware) const {
    const float x = g_Player.positionCenter.x, y = g_Player.positionCenter.y;
    double cost = 0;
    for (const auto &bullet : g_BulletManager.bullets) {
        if (bullet.state == BULLET_STATE_INACTIVE || bullet.state == BULLET_STATE_DESPAWNING)
            continue;
        const double dx = bullet.pos.x - x, dy = bullet.pos.y - y, d2 = dx * dx + dy * dy;
        if (d2 < 14400)
            cost += 900.0 / (d2 + 100);
    }
    for (const auto &enemy : g_EnemyManager.enemies) {
        if (!enemy.flags.isSlotOccupied || !enemy.flags.isCollidable)
            continue;
        const double dx = enemy.position.x - x, dy = enemy.position.y - y;
        cost += 1600.0 / (dx * dx + dy * dy + 100);
    }
    float target_x = 192;
    for (const auto *boss : g_EnemyManager.bosses)
        if (boss && boss->flags.isSlotOccupied) {
            target_x = boss->position.x;
            break;
        }
    cost += std::abs(x - target_x) * 0.003 + std::abs(y - 380.f) * 0.006;
    const double edge = std::max(0.f, 40.f - std::min(x - 8.f, 376.f - x));
    cost += edge * edge * 0.002;
    if (laser_aware)
        for (unsigned i = 0; i < laser_motion_.size(); ++i) {
            const auto &laser = g_BulletManager.lasers[i];
            if (!laser.inUse)
                continue;
            auto forecast = laser_snapshot(laser);
            forecast.dx = laser_motion_[i].dx;
            forecast.dy = laser_motion_[i].dy;
            forecast.angular_speed = laser_motion_[i].angular_speed;
            cost += laser_proposal_cost(advance_laser(forecast), x, y);
        }
    if (power_aware && g_GameManager.currentPower < 128) {
        cost += (128 - g_GameManager.currentPower) * .15;
        const auto target = power_target(x, y);
        // No item means no attraction penalty, including immediately after
        // collecting the last one. Actual native power is rewarded separately.
        if (target.found)
            cost += std::min(240., std::sqrt(target.squared_distance)) * .02;
    }
    return cost;
}
Observation Session::observe(bool laser_aware, bool power_aware) const {
    const auto &speed = g_Player.characterData;
    Observation out{g_Player.positionCenter.x,
                    g_Player.positionCenter.y,
                    speed.orthogonalMovementSpeed,
                    speed.diagonalMovementSpeed,
                    speed.orthogonalMovementSpeedFocus,
                    speed.diagonalMovementSpeedFocus,
                    192,
                    {},
                    {}};
    out.power = g_GameManager.currentPower;
    out.hazards.reserve(700);
    for (const auto &bullet : g_BulletManager.bullets) {
        if (bullet.state == BULLET_STATE_INACTIVE || bullet.state == BULLET_STATE_DESPAWNING)
            continue;
        out.hazards.push_back(
            {bullet.pos.x, bullet.pos.y, bullet.velocity.x, bullet.velocity.y,
             std::max(bullet.sprites.grazeSize.x, bullet.sprites.grazeSize.y) * 0.5f + 1.25f});
    }
    for (const auto &enemy : g_EnemyManager.enemies) {
        if (!enemy.flags.isSlotOccupied)
            continue;
        if (enemy.flags.isBoss)
            out.target_x = enemy.position.x;
        if (enemy.flags.isCollidable)
            out.hazards.push_back(
                {enemy.position.x, enemy.position.y, enemy.axisSpeed.x, enemy.axisSpeed.y,
                 std::max(enemy.hitboxDimensions.x, enemy.hitboxDimensions.y) * 0.5f + 1.25f});
    }
    if (laser_aware)
        for (unsigned i = 0; i < laser_motion_.size(); ++i) {
            if (!g_BulletManager.lasers[i].inUse)
                continue;
            auto laser = laser_snapshot(g_BulletManager.lasers[i]);
            laser.dx = laser_motion_[i].dx;
            laser.dy = laser_motion_[i].dy;
            laser.angular_speed = laser_motion_[i].angular_speed;
            out.lasers.push_back(laser);
        }
    if (power_aware && out.power < 128) {
        const auto target = power_target(out.x, out.y);
        if (target.found)
            out.target_x = target.x;
    }
    return out;
}
} // namespace th06::solver
