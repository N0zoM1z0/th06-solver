#include "th06/shadow_native.hpp"
#include "th06/shadow_search.hpp"
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
        b::Search backup;
        backup.warm_selected = true;
        backup.prediction.count = 80;
        require(b::covered_warm_backup(backup, 140) && b::covered_warm_backup(backup, 80) &&
                    !b::covered_warm_backup(backup, 79),
                "a nominal backup must add certified time before replacing the old queue");
        backup.warm_selected = false;
        require(!b::covered_warm_backup(backup, 140),
                "fresh alternatives can change a longer queue");
        backup.warm_selected = true;
        backup.prediction.count = 0;
        require(!b::covered_warm_backup(backup, 140), "a failed search is not a covered backup");
        require(b::recovery_horizon(128, 64, 16, 81, true) == 128 &&
                    b::recovery_horizon(128, 64, 16, 80, true) == 64,
                "short native rescue starts only at the declared low-queue boundary");
        require(b::recovery_horizon(128, 64, 16, 72, false) == 128 &&
                    b::recovery_horizon(128, 64, 16, 140, true) == 128,
                "successful replacement retries normal lookahead and a healthy queue keeps it");
        require(b::recovery_horizon(256, 64, 16, 144, true) == 64 &&
                    b::recovery_horizon(256, 0, 16, 0, true) == 256,
                "survival horizon has its own threshold; disabled rescue preserves defaults");
        require(b::start_valid(1, 3, 0, 1) && b::start_valid(6, 3, 3, 0), "Lunatic starts");
        require(b::start_valid(7, 4, 0, 0) && b::start_valid(7, 4, 3, 0), "Extra starts");
        require(!b::start_valid(7, 3, 0, 0) && !b::start_valid(6, 4, 0, 0),
                "reject mismatched Extra stage/difficulty");
        require(!b::start_valid(7, 4, 0, 1) && !b::start_valid(5, 3, 0, 1) &&
                    !b::start_valid(1, 3, 0, 2),
                "reject invalid story starts");
        require(!b::start_valid(0, 3, 0, 0) && !b::start_valid(8, 4, 0, 0) &&
                    !b::start_valid(7, 4, 4, 0),
                "reject out-of-range starts");
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
        require(b::remaining(p, 99) == 0 && b::remaining(p, 100) == 48 &&
                    b::remaining(p, 147) == 1 && b::remaining(p, 148) == 0 &&
                    b::remaining({}, 0) == 0,
                "queue boundaries exclude exhausted fallback holds");
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
        p.stage = 7;
        p.generation = 8;
        b::publish(map.data(), p);
        require(b::read(map.data(), read) && b::compatible(read, {}, 261, 100, 7) &&
                    !b::timely(read, 100, 6),
                "Extra plans round-trip without crossing main-game stage ownership");
        p.stage = 8;
        require(!b::valid(p), "reject stage beyond Extra");
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
