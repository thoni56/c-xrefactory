#define HEX(x) 0x##x
int h = HEX(FF); // Should become "int h = 0xFF;"
int i = 0xFF;

#define NUM(n) n##0
#define TXT(a) b##a
int TXT(z) = NUM(1); // Should become "int bz = 10;"

#define FLOAT(x, y) x##.##y
double d = FLOAT(3, 14); // Expected: double d = 3.14;

#define EXP(x, y) x##e##y
double e = EXP(2, 10);  // Expected: double d = 2e10;
