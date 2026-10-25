#include "th06/shadow_native.hpp"
#include "stdafx.h"
#include <cmath>

namespace th06::shadow {
Receipt project(const solver::Session &game, std::uint16_t input) {
    const auto s = game.state();
    return {magic,
            version,
            unsigned(s.frame),
            input,
            unsigned(s.stage),
            unsigned(s.timeline),
            bits(s.x),
            bits(s.y),
            s.rng,
            s.rng_calls,
            unsigned(s.power),
            unsigned(s.deaths),
            unsigned(s.bombs),
            s.score,
            unsigned(th06::g_GameManager.rank),
            unsigned(s.player_state),
            game.cleared_stages(),
            unsigned(s.complete),
            0,
            0};
}
unsigned difference(const Receipt &native, const Receipt &retail) {
    for (unsigned i = 0; i < TimeLow; ++i) {
        if (i == X || i == Y) {
            if (!std::isfinite(real(native[i])) || !std::isfinite(real(retail[i])) ||
                std::abs(real(native[i]) - real(retail[i])) > 0.025f)
                return i;
        } else if (native[i] != retail[i])
            return i;
    }
    return FieldCount;
}
} // namespace th06::shadow
