int64_t npp_system(const char* c) {
    return c ? std::system(c) : -1;
}
