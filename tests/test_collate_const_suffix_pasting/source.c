#define MAKE_UL(x)  x##UL
#define MAKE_L(x)   x##L

unsigned long v1 = MAKE_UL(42);     // 42##UL -> 42UL
long v2 = MAKE_L(123);              // 123##L -> 123L
