#ifndef EXTRACT_H
#define EXTRACT_H

#include "proto.h"

extern S_symbol * addContinueBreakLabelSymbol(int labn, char *name);
extern void actionsBeforeAfterExternalDefinition();
extern void extractActionOnBlockMarker();
extern void genInternalLabelReference(int counter, int usage);
extern S_symbol * addContinueBreakLabelSymbol(int labn, char *name);
extern void deleteContinueBreakLabelSymbol(char *name);
extern void genInternalLabelReference(int counter, int usage);
extern void genContinueBreakReference(char *name);
extern void genSwitchCaseFork(int lastFlag);

#endif
