#define svg(...) printf(__VA_ARGS__)
#define svg_text(format, ...) \
    svg(format, ## __VA_ARGS__);

void test(void) {
    svg_text("firmware");
}
