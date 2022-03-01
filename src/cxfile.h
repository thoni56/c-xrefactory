#ifndef CXFILE_H_INCLUDED
#define CXFILE_H_INCLUDED

#include "proto.h"              /* for SymbolReferenceItem */
#include "symbol.h"
#include "characterreader.h"


extern void normalScanFor(char *fileName, char *suffix);
extern void scanForClassHierarchy(void);
extern void fullScanFor(char *symbolName);
extern void scanForBypass(char *symbolName);
extern void scanReferencesToCreateMenu(char *symbolName);
extern void scanForMacroUsage(char *symbolName);
extern void scanForClassHierarchy(void);
extern void scanForGlobalUnused(char *cxrefFileName);
extern void scanForSearch(char *cxrefFileName);

extern void genReferenceFile(bool updating, char *filename);
extern int cxFileHashNumber(char *symbol);
extern char *createTagSearchLineStatic(char *name, Position *position,
                                       int *len1, int *len2, int *len3);
extern bool symbolNameShouldBeHiddenFromReports(char *name);
extern void searchSymbolCheckReference(SymbolReferenceItem  *symbolReference, Reference *reference);
extern void addSubClassItemToFileTab(int superior, int inferior, int origin);
extern void addSubClassesItemsToFileTab(Symbol *symbol, int origin);

#endif
