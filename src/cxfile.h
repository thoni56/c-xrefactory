#ifndef CXFILE_H_INCLUDED
#define CXFILE_H_INCLUDED

#include "referenceableitem.h"


extern void scanReferencesToCreateMenu(char *symbolName);
extern void scanForMacroUsage(char *symbolName);

extern int cxFileHashNumberForSymbol(char *symbol);
extern void searchSymbolCheckReference(ReferenceableItem *referenceableItem, Reference *reference);

// Abstract API
extern bool loadFileNumbersFromStore(void);
extern bool loadSnapshotFromStore(void);
extern void saveReferencesToStore(bool updating, char *name);

#endif
