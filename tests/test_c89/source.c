// Implicit int (valid in C89, removed in C99+)
foo();  // Expected: Recognize `foo` as a function symbol

// K&R-style function definition (valid in C89, removed in C99+)
int bar(a, b)   // Expected: Recognize `bar` as a function symbol
    int a, b;
{
    return a + b;
}

// Normal function declaration (always valid)
int baz(int x, int y);  // Expected: Recognize `baz` as a function symbol
