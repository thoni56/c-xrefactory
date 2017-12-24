/*
  enumTxt.c - only includes a generated enumTxt.g.c, or when bootstrapping, enumTxt.bs.c
*/

#ifdef BOOTSTRAP
#include "enumTxt.bs.c"
#else
#include "enumTxt.g.c"
#endif
