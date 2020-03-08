#ifndef EXTRACT_H
#define EXTRACT_H

#include "proto.h"

enum extractModes {
    EXTR_FUNCTION,
    EXTR_FUNCTION_ADDRESS_ARGS,
    EXTR_MACRO,
};

extern Symbol * addContinueBreakLabelSymbol(int labn, char *name);
extern void actionsBeforeAfterExternalDefinition(void);
extern void extractActionOnBlockMarker(void);
extern void genInternalLabelReference(int counter, int usage);
extern void deleteContinueBreakLabelSymbol(char *name);
extern void genContinueBreakReference(char *name);
extern void genSwitchCaseFork(int lastFlag);

#endif
