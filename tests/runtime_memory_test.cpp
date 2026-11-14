#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

extern "C" char* npp_str_i64(int64_t value);
extern "C" char* npp_str_concat(const char* left, const char* right);
extern "C" void npp_free(void* pointer);
extern "C" size_t npp_runtime_test_live_bytes();
extern "C" void npp_runtime_test_collect();
extern "C" int npp_runtime_test_try_free(void* pointer);
extern "C" int npp_runtime_test_range_valid(int64_t address, int64_t bytes);
extern "C" int64_t npp_mem_alloc(int64_t bytes);
extern "C" void npp_mem_free(int64_t pointer);
extern "C" void npp_mem_set(int64_t pointer, int64_t value, int64_t bytes);
extern "C" int64_t npp_mem_read(int64_t address);
extern "C" void npp_mem_copy(int64_t destination, int64_t source, int64_t bytes);

int main() {
    // These strings must remain valid across many collection cycles.
    char* volatile persistent = npp_str_concat("persistent ", "value");
    char* volatile latest = nullptr;

    for (int64_t i = 0; i < 200000; ++i) {
        char* number = npp_str_i64(i);
        char* value = npp_str_concat(number, "-frame");
        if (i == 199999)
            latest = value;
    }

    npp_runtime_test_collect();

    if (std::strcmp(persistent, "persistent value") != 0 ||
        std::strcmp(latest, "199999-frame") != 0) {
        std::fprintf(stderr, "runtime GC reclaimed a live string\n");
        return 1;
    }

    const size_t liveBytes = npp_runtime_test_live_bytes();
    if (liveBytes > 2u * 1024u * 1024u) {
        std::fprintf(stderr, "runtime string heap remained too large: %zu bytes\n", liveBytes);
        return 2;
    }

    char* freedProbe = npp_str_concat("freed ", "probe");
    if (!freedProbe) {
        std::fprintf(stderr, "runtime allocation probe failed\n");
        return 3;
    }
    npp_free(freedProbe);
    if (npp_runtime_test_try_free(freedProbe) != 0) {
        std::fprintf(stderr, "runtime accepted a double free\n");
        return 4;
    }

    int stackValue = 0;
    if (npp_runtime_test_try_free(&stackValue) != 0) {
        std::fprintf(stderr, "runtime accepted a non-managed pointer free\n");
        return 5;
    }

    const int64_t buffer = npp_mem_alloc(16);
    const bool wholeRangeValid = npp_runtime_test_range_valid(buffer, 16) != 0;
    const bool tailRangeValid = npp_runtime_test_range_valid(buffer + 8, 8) != 0;
    const bool partialOverflowValid = npp_runtime_test_range_valid(buffer + 12, 8) != 0;
    const bool hugeRangeValid =
        npp_runtime_test_range_valid(buffer, std::numeric_limits<int64_t>::max()) != 0;
    const bool negativeSizeValid = npp_runtime_test_range_valid(buffer, -1) != 0;
    const bool nullRangeValid = npp_runtime_test_range_valid(0, 8) != 0;
    if (!buffer || !wholeRangeValid || !tailRangeValid || partialOverflowValid || hugeRangeValid ||
        negativeSizeValid || nullRangeValid) {
        std::fprintf(stderr,
                     "runtime memory range validation failed: pointer=%lld whole=%d tail=%d "
                     "partial=%d huge=%d negative=%d null=%d\n",
                     static_cast<long long>(buffer),
                     wholeRangeValid,
                     tailRangeValid,
                     partialOverflowValid,
                     hugeRangeValid,
                     negativeSizeValid,
                     nullRangeValid);
        return 6;
    }
    npp_mem_set(buffer, 0x5A, 16);
    const int64_t repeatedBytePattern = INT64_C(0x5A5A5A5A5A5A5A5A);
    if (npp_mem_read(buffer + 8) != repeatedBytePattern) {
        std::fprintf(stderr, "checked memory read/write returned wrong data\n");
        return 7;
    }
    if (npp_mem_read(buffer + 1) != repeatedBytePattern) {
        std::fprintf(stderr, "checked unaligned memory read returned wrong data\n");
        return 9;
    }
    if (npp_mem_read(0) != 0) {
        std::fprintf(stderr, "null memory read did not preserve zero result\n");
        return 10;
    }
    // Overlap is defined for mem_copy; it behaves like memmove.
    npp_mem_copy(buffer + 4, buffer, 8);
    if (npp_mem_read(buffer + 4) != repeatedBytePattern) {
        std::fprintf(stderr, "checked overlapping memory copy returned wrong data\n");
        return 8;
    }
    npp_mem_free(buffer);

    npp_free(persistent);
    npp_free(latest);
    npp_runtime_test_collect();
    return 0;
}
