#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <thread>

extern "C" int64_t npp_to_i64(double value);
extern "C" int64_t npp_random_i64(int64_t minimum, int64_t maximum);
extern "C" int64_t npp_time_ms();

namespace {
    bool expect(bool condition, const char* label) {
        if (condition)
            return true;
        std::fprintf(stderr, "runtime numeric check failed: %s\n", label);
        return false;
    }
} // namespace

int main() {
    bool ok = true;
    const auto min = std::numeric_limits<int64_t>::min();
    const auto max = std::numeric_limits<int64_t>::max();

    ok &= expect(npp_to_i64(0.49) == 0, "positive rounding below half");
    ok &= expect(npp_to_i64(0.5) == 1, "positive halfway rounds away from zero");
    ok &= expect(npp_to_i64(-0.5) == -1, "negative halfway rounds away from zero");
    ok &= expect(npp_to_i64(-1.6) == -2, "negative rounding");
    ok &= expect(npp_to_i64(std::numeric_limits<double>::quiet_NaN()) == 0, "NaN converts to zero");
    ok &= expect(npp_to_i64(std::numeric_limits<double>::infinity()) == max,
                 "positive infinity saturates");
    ok &= expect(npp_to_i64(-std::numeric_limits<double>::infinity()) == min,
                 "negative infinity saturates");
    ok &= expect(npp_to_i64(1.0e300) == max, "large positive value saturates");
    ok &= expect(npp_to_i64(-1.0e300) == min, "large negative value saturates");
    ok &= expect(npp_to_i64(-std::ldexp(1.0, 63)) == min, "i64 minimum boundary");
    ok &= expect(npp_to_i64(std::ldexp(1.0, 63)) == max, "i64 upper exclusive boundary");

    const auto timeBeforeSleep = npp_time_ms();
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    const auto timeAfterSleep = npp_time_ms();
    ok &= expect(timeAfterSleep >= timeBeforeSleep, "monotonic time never moves backwards");
    ok &= expect(timeAfterSleep - timeBeforeSleep >= 10, "time_ms advances during sleep");

    for (int i = 0; i < 2000; ++i) {
        const auto one = npp_random_i64(7, 7);
        ok &= expect(one == 7, "singleton random interval");
        const auto normal = npp_random_i64(-9, 5);
        ok &= expect(normal >= -9 && normal <= 5, "normal random interval");
        const auto reversed = npp_random_i64(5, -9);
        ok &= expect(reversed >= -9 && reversed <= 5, "reversed random interval");
        const auto low = npp_random_i64(min, min + 5);
        ok &= expect(low >= min && low <= min + 5, "interval near i64 minimum");
        const auto high = npp_random_i64(max - 5, max);
        ok &= expect(high >= max - 5 && high <= max, "interval near i64 maximum");
        // The full-domain call used to overflow max - min before generating a value.
        (void)npp_random_i64(min, max);
    }

    if (!ok)
        return 1;
    std::puts("N++ runtime numeric: PASS");
    return 0;
}
