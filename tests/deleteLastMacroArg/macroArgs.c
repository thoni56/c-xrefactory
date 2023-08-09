#define macro(arg1, arg2, arg3) { \
    arg1 = arg2 = arg3; \
}

static void f(void) {
    macro(1, 2, 3);
}
