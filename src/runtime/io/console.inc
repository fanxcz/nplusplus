// Interactive terminal helpers remain available.
void npp_screen_begin() {}
void npp_screen_end() {}
void npp_screen_clear() {
    npp_clear();
}
void npp_screen_put(int64_t x, int64_t y, const char* t) {
    std::printf("\033[%lld;%lldH%s", (long long)(y + 1), (long long)(x + 1), t ? t : "");
}
void npp_screen_present() {
    std::fflush(stdout);
}
void npp_screen_set_title(const char* t) {
    (void)t;
}
int64_t npp_screen_width() {
    return 80;
}
int64_t npp_screen_height() {
    return 24;
}
bool npp_key_pressed() {
    return false;
}
int64_t npp_read_key() {
    return -1;
}
void npp_beep() {
    std::fputs("\a", stdout);
    std::fflush(stdout);
}
