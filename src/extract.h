#ifndef EXTRACT_H
#define EXTRACT_H

#include "proto.h"
#include "symbol.h"


enum extractModes {
    EXTRACT_FUNCTION,
    EXTRACT_FUNCTION_ADDRESS_ARGS,
    EXTRACT_MACRO,
};


extern Symbol *addContinueBreakLabelSymbol(int labn, char *name);
extern void actionsBeforeAfterExternalDefinition(void);
extern void extractActionOnBlockMarker(void);
extern void generateInternalLabelReference(int counter, int usage);
extern void deleteContinueBreakLabelSymbol(char *name);
extern void genContinueBreakReference(char *name);
extern void generateSwitchCaseFork(bool isLast);
extern void deleteContinueBreakSymbol(Symbol *symbol);

extern int nextGeneratedLocalSymbol(void);
extern int nextGeneratedLabelSymbol(void);
extern int nextGeneratedGotoSymbol(void);
extern int nextGeneratedForkSymbol(void);

#endif
