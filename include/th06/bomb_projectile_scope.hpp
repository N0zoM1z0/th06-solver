#pragma once
namespace th06 {
struct Player;
namespace solver {
// Valid only across a source update segment whose bomb-projectile writes are
// excluded or explicitly invalidate the scope. No scope survives a game step.
class BombProjectileScope {
  public:
    explicit BombProjectileScope(const Player &player);
    ~BombProjectileScope();
    BombProjectileScope(const BombProjectileScope &) = delete;
    BombProjectileScope &operator=(const BombProjectileScope &) = delete;
    static bool known_empty(const Player *player);
    static void invalidate(const Player *player);

  private:
    static BombProjectileScope *current_;
    const Player *player_;
    BombProjectileScope *previous_;
    bool empty_ = true;
};
} // namespace solver
} // namespace th06
