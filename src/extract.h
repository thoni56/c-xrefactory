#ifndef EXTRACT_H
#define EXTRACT_H

#include "proto.h"
#include "symbol.h"


extern Symbol * addContinueBreakLabelSymbol(int labn, char *name);
extern void actionsBeforeAfterExternalDefinition(void);
extern void extractActionOnBlockMarker(void);
extern void genInternalLabelReference(int counter, int usage);
extern void deleteContinueBreakLabelSymbol(char *name);
extern void genContinueBreakReference(char *name);
extern void genSwitchCaseFork(int lastFlag);

#endif
