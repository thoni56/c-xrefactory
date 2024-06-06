#ifndef CXFILE_H_INCLUDED
#define CXFILE_H_INCLUDED

#include "reference.h"


extern void normalScanReferenceFile(char *name);
extern void fullScanFor(char *symbolName);
extern void scanForBypass(char *symbolName);
extern void scanReferencesToCreateMenu(char *symbolName);
extern void scanForMacroUsage(char *symbolName);
extern void scanForGlobalUnused(char *cxrefFileName);
extern void scanForSearch(char *cxrefFileName);

extern void writeReferenceFile(bool updating, char *filename);
extern int cxFileHashNumber(char *symbol);
extern bool smartReadReferences(void);
extern bool searchStringMatch(char *cxtag, int slen);
extern bool symbolShouldBeHiddenFromSearchResults(char *name);
extern void searchSymbolCheckReference(ReferenceItem *symbolReference, Reference *reference);

#endif
