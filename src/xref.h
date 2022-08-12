#ifndef XREF_INCLUDED
#define XREF_INCLUDED
#include <stdbool.h>

extern void checkExactPositionUpdate(bool printMessage);
extern void callXref(int argc, char **argv, bool isRefactoring);
extern void xref(int argc, char **argv);

#endif
