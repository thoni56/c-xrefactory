#ifndef CXFILE_H
#define CXFILE_H

#include "proto.h"
#include "symbol.h"

extern int searchStringFitness(char *cxtag, int slen);
extern char *crTagSearchLineStatic(char *name, S_position *p,
                                   int *len1, int *len2, int *len3);
extern int symbolNameShouldBeHiddenFromReports(char *name);
extern void searchSymbolCheckReference(S_symbolRefItem  *ss, S_reference *rr);
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

#endif
