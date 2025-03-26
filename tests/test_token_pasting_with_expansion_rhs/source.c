#define NUM Two
#define EXPAND(func, test) num##NUM

EXPAND(Ensure, NUM);
