#ifndef CXFILE_H
#define CXFILE_H

#include "proto.h"

extern int searchStringFitness(char *cxtag, int slen);
extern char *crTagSearchLineStatic(char *name, S_position *p,
                                   int *len1, int *len2, int *len3);
extern int symbolNameShouldBeHiddenFromReports(char *name);
extern void searchSymbolCheckReference(S_symbolRefItem  *ss, S_reference *rr);
extern int cxFileHashNumber(char *sym);
extern void genReferenceFile(int updateFlag, char *fname);
extern void addSubClassItemToFileTab( int sup, int inf, int origin);
extern void addSubClassesItemsToFileTab(S_symbol *ss, int origin);
extern void scanCxFile(S_cxScanFileFunctionLink *scanFuns);
extern int scanReferenceFile(char *fname, char *fns1, char *fns2,
                             S_cxScanFileFunctionLink *scanFunTab);
extern void readOneAppropReferenceFile(char *symname,
                                       S_cxScanFileFunctionLink *scanFunTab);
extern void scanReferenceFiles(char *fname, S_cxScanFileFunctionLink *scanFunTab);

extern S_cxScanFileFunctionLink s_cxScanFileTab[];
extern S_cxScanFileFunctionLink s_cxFullScanFunTab[];
extern S_cxScanFileFunctionLink s_cxByPassFunTab[];
extern S_cxScanFileFunctionLink s_cxSymbolMenuCreationTab[];
extern S_cxScanFileFunctionLink s_cxSymbolLoadMenuRefs[];
extern S_cxScanFileFunctionLink s_cxScanFunTabFor2PassMacroUsage[];
extern S_cxScanFileFunctionLink s_cxScanFunTabForClassHierarchy[];
extern S_cxScanFileFunctionLink s_cxFullUpdateScanFunTab[];
extern S_cxScanFileFunctionLink s_cxHtmlGlobRefListScanFunTab[];
extern S_cxScanFileFunctionLink s_cxSymbolSearchScanFunTab[];
extern S_cxScanFileFunctionLink s_cxDeadCodeDetectionScanFunTab[];

#endif
