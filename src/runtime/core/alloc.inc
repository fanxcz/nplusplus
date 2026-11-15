
#ifdef NPP_RUNTIME_TESTING
size_t npp_runtime_test_live_bytes() {
    return nx_test_live_managed_bytes();
}
void npp_runtime_test_collect() {
    nx_gc_collect();
}
int npp_runtime_test_try_free(void* pointer) {
    return nx_managed_try_free(pointer) ? 1 : 0;
}
int npp_runtime_test_range_valid(int64_t address, int64_t bytes) {
    return nx_managed_range_is_valid(address, bytes) ? 1 : 0;
}
#endif

void* npp_alloc(size_t n) {
    return nx_managed_alloc(n, n <= kNxGcMaxScannedBlock);
}
void npp_free(void* p) {
    nx_managed_free(p);
}
void npp_bounds_check(int64_t i, int64_t n) {
    if (i < 0 || i >= n) {
        std::fprintf(stderr,
                     "N++ runtime error: array index %lld out of bounds for size %lld\n",
                     (long long)i,
                     (long long)n);
        std::exit(1);
    }
}
void npp_slice_bounds_check(int64_t start, int64_t end, int64_t length) {
    if (start < 0 || end < start || end > length) {
        std::fprintf(stderr,
                     "N++ runtime error: invalid slice range [%lld..%lld] for length %lld\n",
                     (long long)start, (long long)end, (long long)length);
        std::exit(1);
    }
}
int64_t npp_division_check(int64_t dividend, int64_t divisor) {
    if (divisor == 0) {
        std::fprintf(stderr, "N++ runtime error: division by zero\n");
        std::exit(1);
    }
    if (dividend == INT64_MIN && divisor == -1) {
        std::fprintf(stderr, "N++ runtime error: signed integer division overflow\n");
        std::exit(1);
    }
    return divisor;
}
int64_t npp_division_check_typed(
    int64_t dividend, int64_t divisor, int64_t bits, bool isSigned) {
    if (divisor == 0) {
        std::fprintf(stderr, "N++ runtime error: division by zero\n");
        std::exit(1);
    }
    if (isSigned && bits >= 2 && bits <= 64) {
        const int64_t minimum = bits == 64
            ? INT64_MIN
            : -(INT64_C(1) << (bits - 1));
        if (dividend == minimum && divisor == -1) {
            std::fprintf(stderr, "N++ runtime error: signed integer division overflow\n");
            std::exit(1);
        }
    }
    return divisor;
}
char* npp_str_concat(const char* a, const char* b) {
    if (!a)
        a = "";
    if (!b)
        b = "";
    size_t la = std::strlen(a), lb = std::strlen(b);
    char* out = (char*)nx_managed_alloc(la + lb + 1);
    if (!out)
        return nullptr;
    std::memcpy(out, a, la);
    std::memcpy(out + la, b, lb);
    out[la + lb] = 0;
    return out;
}
