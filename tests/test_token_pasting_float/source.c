#define FLOAT(x, y) x##.##y
double pi = FLOAT(3, 14); // Should become "double pi = 3.14;"
