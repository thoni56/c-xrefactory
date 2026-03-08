// Attribute usage
__attribute__((noreturn)) void fatal();

// Alignof (Global variable)
int alignment = __alignof__(double);

// Typeof (Global variable)
__typeof__(42) int_like = 10;

// Variadic built-in type (Global declaration)
__builtin_va_list args;

// Restrict keyword (Function parameter)
void *__restrict__ p;

// Leaf attribute (Function definition)
__leaf__ int inline_func() { return 0; }

// Extension keyword (GCC extensions)
__extension__ long long extended_type;

// Thread-local storage (Older syntax)
__thread int tls_var;

// Function to contain statement-level GNUisms
void test_gnu_extensions(int x) {
    // Branch prediction hints
    if (__builtin_expect(x, 0)) {}

    // Type compatibility check
    _Bool compatible = __builtin_types_compatible_p(int, long);

    // Offsetof macro
    long offset = __builtin_offsetof(struct S, field);

    // Mark unreachable code
    if (x == 0) {
        __builtin_unreachable();
    }

    // Built-in memory operations
    char src[10], dest[10];
    __builtin_memcpy(dest, src, 10);
    __builtin_memset(dest, 0, 10);

    // Compiler assumption
    __builtin_assume(x > 0);

    // Atomic operation (pre-C11)
    int var = 0;
    int atomic_inc = __sync_fetch_and_add(&var, 1);

    // Inline assembly
    __asm__("nop");
}

struct Empty {}; /* Empty struct */

/* Compound Statement Expression */
#define SQUARE(x) ({ int _tmp = (x); _tmp * _tmp; })
static void squarer(void) {
    int y = SQUARE(5);  // Expands to: ({ int _tmp = 5; _tmp * _tmp; })
}
