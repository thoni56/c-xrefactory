#ifndef CXFILE_H
#define CXFILE_H

#include "proto.h"              /* for SymbolReferenceItem */
#include "symbol.h"


typedef struct scanFileFunctionStep {
    int		recordCode;
    void    (*handleFun)(int size,int ri,char**ccc,char**ffin,CharacterBuffer*bbb, int additionalArg);
    int		additionalArg;
} ScanFileFunctionStep;


extern ScanFileFunctionStep normalScanFunctionSequence[];
extern ScanFileFunctionStep fullScanFunctionSequence[];
extern ScanFileFunctionStep byPassFunctionSequence[];
extern ScanFileFunctionStep symbolMenuCreationFunctionSequence[];
extern ScanFileFunctionStep symbolLoadMenuRefsFunctionSequence[];
extern ScanFileFunctionStep secondPassMacroUsageFunctionSequence[];
extern ScanFileFunctionStep classHierarchyFunctionSequence[];
extern ScanFileFunctionStep fullUpdateFunctionSequence[];
extern ScanFileFunctionStep htmlGlobalReferencesFunctionSequence[];
extern ScanFileFunctionStep symbolSearchFunctionSequence[];
extern ScanFileFunctionStep deadCodeDetectionFunctionSequence[];


extern int searchStringFitness(char *cxtag, int slen);
extern char *crTagSearchLineStatic(char *name, Position *p,
                                   int *len1, int *len2, int *len3);
extern bool symbolNameShouldBeHiddenFromReports(char *name);
extern void searchSymbolCheckReference(SymbolReferenceItem  *ss, Reference *rr);
extern int cxFileHashNumber(char *sym);
extern void genReferenceFile(int updateFlag, char *fname);
extern void addSubClassItemToFileTab( int sup, int inf, int origin);
extern void addSubClassesItemsToFileTab(Symbol *ss, int origin);
extern void scanCxFile(ScanFileFunctionStep *scanFuns);
extern int scanReferenceFile(char *fname, char *fns1, char *fns2,
                             ScanFileFunctionStep *scanFunTab);
extern void readOneAppropReferenceFile(char *symname,
                                       ScanFileFunctionStep *scanFunTab);
extern void scanReferenceFiles(char *fname, ScanFileFunctionStep *scanFunTab);
extern int smartReadFileTabFile(void);

#endif
