#define MACRO(a, b) a##b

void f(void) {
    int a, b;
    int ab;

    MACRO(a, b) = 1;
}
