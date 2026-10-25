#pragma once
#include "th06/session.hpp"
#include "th06/shadow_bridge.hpp"
namespace th06::shadow {
Receipt project(const solver::Session &game, std::uint16_t input);
// Returns the first mismatching field, or FieldCount. Coordinates allow small
// float drift; discrete gameplay/RNG/lifecycle values must agree.
unsigned difference(const Receipt &native, const Receipt &retail);
} // namespace th06::shadow
