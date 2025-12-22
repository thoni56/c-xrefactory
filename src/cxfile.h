#ifndef CXFILE_H_INCLUDED
#define CXFILE_H_INCLUDED

#include "referenceableitem.h"


extern void fullScanFor(char *symbolName);
extern void scanReferencesToCreateMenu(char *symbolName);
extern void scanForMacroUsage(char *symbolName);
extern void scanForGlobalUnused(char *cxrefFileName);
extern void scanForSearch(char *cxrefFileName);

extern void writeCxFile(bool updating, char *filename);
extern int cxFileHashNumberForSymbol(char *symbol);
extern bool smartReadFileNumbersFromStore(void);
extern bool searchStringMatch(char *cxtag, int slen);
extern bool symbolShouldBeHiddenFromSearchResults(char *name);
extern void searchSymbolCheckReference(ReferenceableItem *symbolReference, Reference *reference);

#endif
