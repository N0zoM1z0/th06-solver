#include "th06/render_timing.hpp"
#include "th06/retail_clock.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main() {
    try {
        const auto original = th06::retail::x87_control();
        th06::retail::x87_control(std::uint16_t(original & ~0x300u));
        if (th06::render_timing::frequency_valid(0) ||
            th06::render_timing::nanoseconds(9007199254740991ULL, 10000000) !=
                900719925474099100ULL)
            throw std::runtime_error("profiling clocks must stay exact in the gameplay FPU mode");
        const double last = 442039552., period = 1000. / 60.;
        double coarse, precise;
        const double stuck_delta = 538207616.;
        double stuck_subtract, precise_subtract;
        asm volatile("fldl %1; faddl %2; fstpl %0" : "=m"(coarse) : "m"(last), "m"(period) : "st");
        asm volatile("fldl %1; fsubl %2; fstpl %0"
                     : "=m"(stuck_subtract)
                     : "m"(stuck_delta), "m"(period)
                     : "st");
        th06::retail::LimiterPrecision limiter;
        limiter.enter();
        limiter.enter();
        if (!th06::retail::coarse_wait_has_slack(442039564, last) ||
            th06::retail::coarse_wait_has_slack(442039565, last) ||
            th06::retail::coarse_wait_has_slack(442039569, last) ||
            th06::retail::coarse_wait_has_slack(1, 4294967294.))
            throw std::runtime_error("coarse wait must retain final checks and handle clock wrap");
        asm volatile("fldl %1; faddl %2; fstpl %0" : "=m"(precise) : "m"(last), "m"(period) : "st");
        asm volatile("fldl %1; fsubl %2; fstpl %0"
                     : "=m"(precise_subtract)
                     : "m"(stuck_delta), "m"(period)
                     : "st");
        limiter.restore();
        if (th06::retail::x87_control() != std::uint16_t(original & ~0x300u) || limiter.active ||
            limiter.entries != 1 || limiter.restores != 1)
            throw std::runtime_error("limiter must restore the exact gameplay FPU environment");
        th06::retail::x87_control(original);
        if (coarse - last != 32. || std::abs(precise - last - period) > .00001)
            throw std::runtime_error("long-uptime limiter precision regression");
        if (stuck_subtract != stuck_delta || precise_subtract >= stuck_delta ||
            std::abs(stuck_delta - precise_subtract - period) > .00001)
            throw std::runtime_error("startup catch-up subtraction must make progress");
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
