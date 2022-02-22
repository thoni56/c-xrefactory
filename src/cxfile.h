#ifndef CXFILE_H_INCLUDED
#define CXFILE_H_INCLUDED

#include "proto.h"              /* for SymbolReferenceItem */
#include "symbol.h"
#include "characterreader.h"


typedef struct scanFileFunctionStep {
    int		recordCode;
    void    (*handleFun)(int size, int ri, CharacterBuffer *cb, int additionalArg); /* TODO: Break out a type */
    int		additionalArg;
} ScanFileFunctionStep;


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
extern int cxFileHashNumber(char *sym);
extern bool smartReadFileTabFile(void);
extern bool searchStringFitness(char *cxtag, int slen);
extern char *createTagSearchLineStatic(char *name, Position *p,
                                       int *len1, int *len2, int *len3);
extern bool symbolNameShouldBeHiddenFromReports(char *name);
extern void searchSymbolCheckReference(SymbolReferenceItem  *ss, Reference *rr);
extern void addSubClassItemToFileTab( int sup, int inf, int origin);
extern void addSubClassesItemsToFileTab(Symbol *ss, int origin);

#endif
