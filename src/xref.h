#ifndef XREF_INCLUDED
#define XREF_INCLUDED
#include <stdbool.h>

#include "argumentsvector.h"


extern void checkExactPositionUpdate(bool printMessage);
extern void callXref(ArgumentsVector args, bool isRefactoring);
extern void xref(ArgumentsVector args);
#endif
