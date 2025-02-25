_BitInt(32) x;   // 32-bit integer
unsigned _BitInt(64) y;  // 64-bit unsigned integer

auto auto_int = 42;  // Expected: Recognize 'auto_int' as an int variable
auto auto_float = 3.14; // Expected: Recognize 'auto_float' as a float variable

struct S { int f, g; };
auto s = (struct S){ .f = 1, .g = 2 };  // C23: Deduce 's' as 'struct S'

char *utf8 = u8"Hello Marián!";  // UTF-8 string
int c = 'abcd';                  // Multi-character constant
