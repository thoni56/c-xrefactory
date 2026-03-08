#define EXPROF(x, y) _Generic((x), \
    int: (y + x), \
    float: (y * x), \
    double: (y - x), \
    default: (y) \
)

int test_generic(int a, float b, double c) {
    int result1 = EXPROF(a, b);  // Expected: Tracks `b` and `a` inside `TYPEOF`
    float result2 = EXPROF(b, c);  // Expected: Tracks `c` and `b`
    double result3 = EXPROF(c, a);  // Expected: Tracks `a` and `c`
}
