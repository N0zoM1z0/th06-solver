#pragma once
#include <array>
#include <cstddef>
#include <cstring>

namespace th06::retail {
// Copy the exact vertex bytes. Triangle strips alternate winding;
// preserve both triangle order and winding when joining adjacent quads.
template <std::size_t VertexStride> class QuadVertices {
  public:
    static constexpr std::size_t stride = VertexStride, capacity = 512;
    std::array<unsigned char, capacity * 6 * stride> vertices{};
    std::size_t count = 0;
    bool append(const void *source) {
        if (!source || count == capacity)
            return false;
        constexpr unsigned indices[] = {0, 1, 2, 2, 1, 3};
        for (unsigned i = 0; i < 6; ++i)
            std::memcpy(vertices.data() + (count * 6 + i) * stride,
                        static_cast<const unsigned char *>(source) + indices[i] * stride, stride);
        ++count;
        return true;
    }
};
using Quads = QuadVertices<28>;       // XYZRHW / DIFFUSE / TEX1.
using FactorQuads = QuadVertices<24>; // XYZRHW / TEX1; source texture-factor color.
} // namespace th06::retail
