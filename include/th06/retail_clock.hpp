#pragma once
#include <cstdint>
namespace th06::retail {
inline std::uint16_t x87_control() {
    std::uint16_t value;
    asm volatile("fnstcw %0" : "=m"(value));
    return value;
}
inline void x87_control(std::uint16_t value) {
    asm volatile("fldcw %0" : : "m"(value));
}
// D3D8 selects 24-bit arithmetic. The source limiter accumulates an absolute
// millisecond timestamp in a double: at long uptimes 16.67-ms additions lose
// precision. Widen only that arithmetic, restoring before Present/BeginScene
// and every subsequent gameplay callback. Original time APIs/limits stay intact.
struct LimiterPrecision {
    bool active = false;
    std::uint16_t original = 0;
    unsigned entries = 0, restores = 0;
    void enter() {
        if (!active) {
            original = x87_control();
            x87_control(std::uint16_t((original & ~0x300u) | 0x200u));
            active = true;
            ++entries;
        }
    }
    void restore() {
        if (active) {
            x87_control(original);
            active = false;
            ++restores;
        }
    }
};
} // namespace th06::retail
