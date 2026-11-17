#if defined(PASS1) && defined(PASS2)
int x;
#else
int x;
#endif
int use(void) { return x; }
