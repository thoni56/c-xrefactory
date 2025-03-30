#define NUM Two
#define EXPAND(func, test) NUM##num

int EXPAND(Ensure, NUM);
