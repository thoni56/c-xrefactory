_BitInt(32) x;   // 32-bit integer
unsigned _BitInt(64) y;  // 64-bit unsigned integer

auto auto_int = 42;  // Expected: Recognize 'auto_int' as an int variable
auto auto_float = 3.14; // Expected: Recognize 'auto_float' as a float variable

struct S { int f, g; };
auto s = (struct S){ .f = 1, .g = 2 };  // C23: Deduce 's' as 'struct S'

char *utf8 = u8"Hello Marián!";  // UTF-8 string
int c = 'abcd';                  // Multi-character constant

int *p = nullptr;  // p is now a null pointer
void pfunc(int *ptr);
pfunc(nullptr);  // Safe, no integer confusion

typeof(p) another_p;

#define MAX(a, b) ({    \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b; \
})

int max_value = MAX(10, 20.5);  // Expands with correct type deduction - which we don't care about

static_assert(c == 4); /* New in C23, replaces _Static_assert, message optional */

void foo(void);
foo( ); /* Call to foo with whitespace in parentheses wasn't allowed in the standard
        * before */

static void do_while(void) {
    if (1)
        do { c++; } while (0)  // Now valid in C23 without `;`
        else
            c--;
}

constexpr int square(int x) {
    return x * x;
}

constexpr int y = square(4);  // Expected: `y` should be `16` at compile-time

int squareing() {
    int z = square(5);  // Expected: Normal function call
}

/* C23 adds bool, true and false as keywords that can be used without <stdbool.h> */
bool flag1 = true;
bool flag2 = false;

#embed "file"
