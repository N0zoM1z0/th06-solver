#include "th06/retail_quads.hpp"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
template <std::size_t Stride> void check() {
    th06::retail::QuadVertices<Stride> quads;
    std::array<unsigned char, Stride * 4> input;
    for (unsigned i = 0; i < input.size(); ++i)
        input[i] = static_cast<unsigned char>(i * 73 + 19);
    if (quads.append(nullptr) || quads.count)
        throw std::runtime_error("null quad accepted");
    constexpr unsigned triangles[2][3] = {{0, 1, 2}, {2, 1, 3}};
    for (unsigned q = 0; q < quads.capacity; ++q) {
        input[0] = static_cast<unsigned char>(q);
        if (!quads.append(input.data()))
            throw std::runtime_error("premature batch limit");
        for (unsigned t = 0; t < 2; ++t)
            for (unsigned v = 0; v < 3; ++v)
                if (std::memcmp(quads.vertices.data() + (q * 6 + t * 3 + v) * Stride,
                                input.data() + triangles[t][v] * Stride, Stride))
                    throw std::runtime_error("changed vertex bytes, winding or draw order");
    }
    const auto previous = quads.vertices;
    if (quads.append(input.data()) || quads.count != quads.capacity || previous != quads.vertices)
        throw std::runtime_error("full batch mutated or overflowed");
    quads.count = 0;
    if (!quads.append(input.data()) || quads.count != 1)
        throw std::runtime_error("batch cannot be reused");
}
int main() {
    try {
        check<24>();
        check<28>();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
