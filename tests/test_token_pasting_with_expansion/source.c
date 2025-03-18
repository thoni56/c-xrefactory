#define NUM Two
#define EXPAND(func, test) NUM##NUM

EXPAND(Ensure, NUM);
