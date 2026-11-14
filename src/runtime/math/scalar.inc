
// Existing runtime API -------------------------------------------------------
static char* nx_strdup(const char* src) {
    if (!src)
        src = "";
    size_t n = std::strlen(src);
    char* out = (char*)nx_managed_alloc(n + 1);
    if (!out)
        return nullptr;
    std::memcpy(out, src, n + 1);
    return out;
}
double npp_sqrt(double x) {
    return std::sqrt(std::max(0.0, x));
}
double npp_sin(double x) {
    return std::sin(x);
}
double npp_cos(double x) {
    return std::cos(x);
}
double npp_tan(double x) {
    return std::tan(x);
}
double npp_atan2(double y, double x) {
    return std::atan2(y, x);
}
double npp_floor(double x) {
    return std::floor(x);
}
double npp_ceil(double x) {
    return std::ceil(x);
}
double npp_pow(double base, double exponent) {
    return std::pow(base, exponent);
}
double npp_to_f64(int64_t x) {
    return double(x);
}
int64_t npp_to_i64(double x) {
    // Define behavior for values outside llround's representable domain.
    // NaN becomes zero; infinities and out-of-range values saturate.
    if (std::isnan(x))
        return 0;
    const double signedLimit = std::ldexp(1.0, 63); // 2^63, exactly representable.
    if (x >= signedLimit)
        return std::numeric_limits<int64_t>::max();
    if (x <= -signedLimit)
        return std::numeric_limits<int64_t>::min();
    // llround preserves the language's existing nearest-integer, halfway-away-from-zero behavior.
    return static_cast<int64_t>(std::llround(x));
}
void npp_print_i64(int64_t x) {
    std::printf("%lld\n", (long long)x);
}
void npp_print_u64(uint64_t x) {
    std::printf("%llu\n", static_cast<unsigned long long>(x));
}
void npp_print_f64(double x) {
    std::printf("%.15g\n", x);
}
void npp_print_bool(bool x) {
    std::puts(x ? "true" : "false");
}
void npp_print_str(const char* x) {
    std::puts(x ? x : "null");
}
