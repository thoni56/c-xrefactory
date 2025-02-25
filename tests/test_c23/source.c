_BitInt(32) x;   // 32-bit integer
unsigned _BitInt(64) y;  // 64-bit unsigned integer

auto foo() { return 42; }  // Expected: Recognize `foo` as a function symbol
auto bar() { return 3.14; }  // Expected: Recognize `bar` as a function symbol
