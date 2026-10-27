#pragma once
#include "th06/laser_guidance.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace th06::solver {
struct StartConfig {
    int stage = 1, difficulty = 3, shot = 0;
    std::uint16_t seed = 0;
    int power = 0;
    bool story = false;
};
struct State {
    std::uint64_t frame = 0;
    float x = 0, y = 0;
    int stage = 0, timeline = 0, enemies = 0, bullets = 0, lasers = 0;
    int deaths = 0, bombs = 0, power = 0, player_state = 0;
    std::uint16_t rng = 0;
    std::uint32_t rng_calls = 0, score = 0;
    bool complete = false, hit = false;
};
struct MovingHazard {
    float x, y, vx, vy, radius;
};
struct Observation {
    float x, y, axis, diagonal, focused_axis, focused_diagonal, target_x;
    std::vector<MovingHazard> hazards;
    std::vector<LaserForecast> lasers;
    unsigned power = 128;
};
// One source game per process. Branches use fork(), never byte copies of the
// managers: their pointers refer to mutable actors and loaded script storage.
class Session {
  public:
    explicit Session(const StartConfig &config);
    ~Session();
    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;
    State state() const;
    State step(std::uint16_t input);
    std::uint64_t observation_digest() const;
    double proposal_cost(bool laser_aware = false, bool power_aware = false) const;
    Observation observe(bool laser_aware = false, bool power_aware = false) const;
    void enable_laser_guidance() {
        track_lasers_ = true;
    }
    unsigned cleared_stages() const {
        return cleared_stages_;
    }

  private:
    void initialize_stage();
    void advance_stage();
    bool terminal() const;
    std::uint64_t frame_ = 0;
    bool started_ = false;
    bool story_ = false;
    unsigned cleared_stages_ = 0;
    unsigned required_stages_ = 1;
    bool track_lasers_ = false;
    struct LaserMotion {
        float dx = 0, dy = 0, angular_speed = 0;
    };
    std::array<LaserMotion, 64> laser_motion_{};
    std::array<LaserForecast, 64> previous_lasers_{};
};
} // namespace th06::solver
