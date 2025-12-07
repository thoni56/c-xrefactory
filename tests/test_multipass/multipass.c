// Test file for multi-pass parsing
// Should collect symbols from both PASS1 and PASS2 without spillover
// Only PASS2 should generate syntax error(s)

#ifdef PASS1
int pass1;
#endif

#ifdef PASS2
int syntax error ! ;
int pass2;
#endif

int common;
