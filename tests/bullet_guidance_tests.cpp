#include "th06/bullet_guidance.hpp"
#include <iostream>
#include <stdexcept>
using namespace th06::solver;
int main() {
    try {
        const MovingHazard inert{10, 20, 2, -1, 3};
        if (inert.position(8) != std::pair<float, float>{26, 12})
            throw std::runtime_error("default guidance preserves constant velocity");
        const MovingHazard finite{10, 20, 2, -1, 3, 1, -2, 3};
        if (finite.position(1) != std::pair<float, float>{13, 17})
            throw std::runtime_error("acceleration must precede movement");
        if (finite.position(3) != std::pair<float, float>{22, 5})
            throw std::runtime_error("finite acceleration accumulates through its last update");
        if (finite.position(5) != std::pair<float, float>{32, -9})
            throw std::runtime_error(
                "expiry retains accumulated velocity without further acceleration");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
