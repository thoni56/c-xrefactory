#define PASTE(a, b) a##b
#define FOO bar

// Undefine FOO
#undef FOO

// Try to use undefined FOO in token pasting - should not crash
int x = PASTE(FOO, 123);
