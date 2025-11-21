#define PASTE3(a, b, c) a##b##c

int foo123bar;
int x = PASTE3(foo, 123, bar);
