#ifndef XREF_INCLUDED
#define XREF_INCLUDED
#include <stdbool.h>

#include "options.h"


extern void checkExactPositionUpdate(bool printMessage);
extern void callXref(int argc, char **argv, bool isRefactoring);
extern void xref(ArgumentsVector args);
#endif
