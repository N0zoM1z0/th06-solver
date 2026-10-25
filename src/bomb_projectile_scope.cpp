#include "th06/bomb_projectile_scope.hpp"
#include "Player.hpp"
#include <cassert>

// Adapted from TH08's scoped cancel-region cache; see docs/THIRD_PARTY_NOTICES.md.
namespace th06::solver {
BombProjectileScope *BombProjectileScope::current_ = nullptr;
BombProjectileScope::BombProjectileScope(const Player &player)
    : player_(&player), previous_(current_) {
    for (const auto &region : player.bombProjectiles)
        if (region.size.x != 0.0f) {
            empty_ = false;
            break;
        }
    current_ = this;
}
BombProjectileScope::~BombProjectileScope() {
    current_ = previous_;
}
bool BombProjectileScope::known_empty(const Player *player) {
    if (!current_ || current_->player_ != player || !current_->empty_)
        return false;
#ifndef NDEBUG
    for (const auto &region : player->bombProjectiles)
        assert(region.size.x == 0.0f);
#endif
    return true;
}
void BombProjectileScope::invalidate(const Player *player) {
    for (auto *scope = current_; scope; scope = scope->previous_)
        if (scope->player_ == player)
            scope->empty_ = false;
}
} // namespace th06::solver
