#define NUM(n) n##0
int ten = NUM(1);        // Should become "int ten = 10;"

#define EXP(x, y) x##e##y
double big = EXP(2, 10); // Should become "double big = 2e10;"
