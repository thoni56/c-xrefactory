#define NUM Two, Three, Four
#define EXPAND(func, test) NUM##NUM

int EXPAND(Ensure, NUM);
