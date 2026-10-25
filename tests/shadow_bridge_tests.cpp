#include "th06/shadow_native.hpp"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace b = th06::shadow;
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
int main() {
    try {
        std::array<std::uint32_t, b::map_words> map{};
        b::Plan p;
        p.generation = 2;
        p.origin = 100;
        p.stage = 5;
        p.first_change = 117;
        p.count = 48;
        p.inputs.fill(261);
        p.inputs[16] = 389;
        require(b::valid(p), "valid literal plan");
        b::publish(map.data(), p);
        b::Plan read;
        require(
            b::read(map.data(), read) &&
                std::equal(read.inputs.begin(), read.inputs.begin() + p.count, p.inputs.begin()),
            "mapped round trip");
        require(b::compatible(read, {}, 261, 100, 5), "unchanged fallback lead");
        require(b::timely(read, 115, 5) && !b::timely(read, 116, 5), "strict lead edge");
        require(!b::timely(read, 99, 5) && !b::timely(read, 100, 6), "origin and stage identity");
        p.inputs[7] = 325;
        require(!b::compatible(p, {}, 261, 100, 5), "reject changed committed lead");
        p.inputs[7] = 261;
        auto accepted = read;
        p.generation = 4;
        p.inputs[20] = 325;
        b::publish(map.data(), p);
        require(accepted.inputs[20] == 261, "writer cannot mutate accepted owner copy");
        auto *slot = map.data() + b::slot(p.generation);
        b::save(slot, 5);
        require(!b::read(map.data(), read), "reject torn write");
        b::save(slot, 4);
        b::save(slot + 8, 65536);
        require(!b::read(map.data(), read), "reject truncated input word");
        p.inputs[0] = 2;
        require(!b::valid(p), "bomb forbidden");
        p.inputs[0] = 261;
        p.origin = UINT32_MAX - 1;
        require(!b::valid(p), "sequence overflow");
        p.origin = 100;
        p.count = b::capacity;
        p.generation = 6;
        p.inputs.back() = 325;
        b::publish(map.data(), p);
        require(b::read(map.data(), read) && read.count == b::capacity && read.inputs.back() == 325,
                "maximum-length literal plan crosses both complete map slots");
        require(b::next_input(accepted, 149, 389) == 389, "explicit exhausted-queue hold");
        b::Receipt native{}, retail{};
        native[b::X] = b::bits(192.f);
        retail[b::X] = b::bits(192.01f);
        require(b::difference(native, retail) == b::FieldCount, "small float tolerance");
        retail[b::X] = b::bits(192.1f);
        require(b::difference(native, retail) == b::X, "large drift rejected");
        retail[b::X] = b::bits(std::numeric_limits<float>::quiet_NaN());
        require(b::difference(native, retail) == b::X, "NaN rejected");
        retail = native;
        retail[b::Rng] = 1;
        require(b::difference(native, retail) == b::Rng, "discrete RNG exact");
        retail = native;
        retail[b::TimeHigh] = 42;
        require(b::difference(native, retail) == b::FieldCount, "time is diagnostic");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
