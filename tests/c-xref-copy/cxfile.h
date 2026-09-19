#ifndef CXFILE_H_INCLUDED
#define CXFILE_H_INCLUDED

#include "referenceableitem.h"


extern void scanReferencesToCreateMenu(char *symbolName);
extern void scanForMacroUsage(char *symbolName);
extern void scanForGlobalUnused(char *cxrefFileName);
extern void scanForSearch(char *cxrefFileName);

extern int cxFileHashNumberForSymbol(char *symbol);
extern void searchSymbolCheckReference(ReferenceableItem *referenceableItem, Reference *reference);

// Abstract API
extern bool loadFileNumbersFromStore(void);
extern void ensureReferencesAreLoadedFor(char *symbolName);
extern void saveReferencesToStore(bool updating, char *name);

#endif
