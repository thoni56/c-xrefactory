#ifdef PASS1
int only1;
void f1(void) { only1 = 1; }
#endif

#ifdef PASS2
int only2;
void f2(void) { only2 = 2; }
#endif
