#ifndef EXTRACT_H_INCLUDED
#define EXTRACT_H_INCLUDED


#include "symbol.h"


typedef enum extractModes {
    EXTRACT_FUNCTION,
    EXTRACT_MACRO,
    EXTRACT_VARIABLE
} ExtractMode;

/* Revealed publicly only to allow unittesting */
typedef struct referencesItemList {
    struct referenceItem		*item;
    struct referencesItemList	*next;
} ReferenceItemList;


extern Symbol *addContinueBreakLabelSymbol(int labn, char *name);
extern void actionsBeforeAfterExternalDefinition(void);
extern void extractActionOnBlockMarker(void);
extern void deleteContinueBreakLabelSymbol(char *name);
extern void generateContinueBreakReference(char *name);
extern void generateSwitchCaseFork(bool isLast);
extern void deleteContinueBreakSymbol(Symbol *symbol);

#endif
