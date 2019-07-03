#ifndef CXREF_H
#define CXREF_H

#include "proto.h"

extern int olcxReferenceInternalLessFunction(S_reference *r1, S_reference *r2);
extern int olSymbolRefItemLess(S_symbolRefItem *s1, S_symbolRefItem *s2);
extern void tagSearchCompactShortResults(void);
extern void printTagSearchResults(void);
extern S_olSymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi,int storage);
extern int itIsSameCxSymbol(S_symbolRefItem *p1, S_symbolRefItem *p2);
extern int itIsSameCxSymbolIncludingFunClass(S_symbolRefItem *p1, S_symbolRefItem *p2);
extern int itIsSameCxSymbolIncludingApplClass(S_symbolRefItem *p1, S_symbolRefItem *p2);
extern int olcxItIsSameCxSymbol(S_symbolRefItem *p1, S_symbolRefItem *p2);
extern void olcxRecomputeSelRefs( S_olcxReferences *refs );
extern void olProcessSelectedReferences(S_olcxReferences *rstack,
                                        void (*referencesMapFun)(S_olcxReferences *rstack, S_olSymbolsMenu *ss));
extern void olcxPopOnly(void);
extern S_reference * olcxCopyRefList(S_reference *ll);
extern void olStackDeleteSymbol( S_olcxReferences *refs);
extern int getFileNumberFromName(char *name);
extern void generateOnlineCxref(S_position *p,
                                char *commandString,
                                int usage,
                                char *suffix,
                                char *suffix2
                                );
extern S_reference *olcxAddReferenceNoUsageCheck(S_reference **rlist,
                                                 S_reference *ref,
                                                 int bestMatchFlag);
extern S_reference *olcxAddReference(S_reference **rlist,
                                     S_reference *ref,
                                     int bestMatchFlag);
extern void olcxFreeReferences(S_reference *r);
extern int isSmallerOrEqClass(int inf, int sup);
extern int olcxPushLessFunction(S_reference *r1, S_reference *r2);
extern int olcxListLessFunction(S_reference *r1, S_reference *r2);
extern char *getJavaDocUrl_st(S_symbolRefItem *rr);
extern char *getLocalJavaDocFile_st(char *fileUrl);
extern char *getFullUrlOfJavaDoc_st(char *fileUrl);
extern int htmlJdkDocAvailableForUrl(char *ss);
extern void setIntToZero(void *p);
extern S_reference *duplicateReference(S_reference *r);
extern S_reference * addCxReferenceNew(S_symbol *p, S_position *pos,
                                       S_usageBits *ub, int vFunCl, int vApplCl);
extern S_reference * addCxReference(S_symbol *p, S_position *pos, int usage,
                                    int vFunClass,int vApplClass);
extern S_reference *addSpecialFieldReference(char *name, int storage,
                                             int fnum, S_position *p, int usage);
extern void addClassTreeHierarchyReference(int fnum, S_position *p, int usage);
extern void addCfClassTreeHierarchyRef(int fnum, int usage);
extern void addTrivialCxReference (char *name, int symType, int storage,
                                   S_position *pos, int usage);
extern void olcxAddReferences(S_reference *list, S_reference **dlist, int fnum,
                              int bestMatchFlag);
extern void olSetCallerPosition(S_position *pos);
extern S_olCompletion * olCompletionListPrepend(char *name, char *fullText,
                                                char *vclass, int jindent, S_symbol *s,
                                                S_symbolRefItem *ri, S_reference *dfpos,
                                                int symType, int vFunClass,
                                                S_olcxReferences *stack);
extern S_olSymbolsMenu *olCreateNewMenuItem(S_symbolRefItem *sym, int vApplClass,
                                            int vFunCl, S_position *defpos, int defusage,
                                            int selected, int visible,
                                            unsigned ooBits, int olusage, int vlevel
                                            );
extern S_olSymbolsMenu *olAddBrowsedSymbol(S_symbolRefItem *sym, S_olSymbolsMenu **list,
                                           int selected, int visible, unsigned ooBits,
                                           int olusage, int vlevel,
                                           S_position *defpos, int defusage);
extern void renameCollationSymbols(S_olSymbolsMenu *sss);
extern void olCompletionListReverse(void);
extern S_reference **addToRefList(S_reference **list,
                                  S_usageBits *pusage,
                                  S_position *pos,
                                  int category
                                  );
extern int isInRefList(S_reference *list,
                       S_usageBits *pusage,
                       S_position *pos,
                       int category
                       );
extern char *getXrefEnvironmentValue( char *name );
extern int byPassAcceptableSymbol(S_symbolRefItem *p);
extern int itIsSymbolToPushOlRefences(S_symbolRefItem *p, S_olcxReferences *rstack,
                                      S_olSymbolsMenu **rss, int checkSelFlag);
extern void olcxAddReferenceToOlSymbolsMenu(S_olSymbolsMenu  *cms, S_reference *rr,
                                            int bestFitTlag);
extern void putOnLineLoadedReferences(S_symbolRefItem *p);
extern void genOnLineReferences(S_olcxReferences *rstack, S_olSymbolsMenu *cms);
extern S_olSymbolsMenu *createSelectionMenu(S_symbolRefItem *dd);
extern void mapCreateSelectionMenu(S_symbolRefItem *dd);
extern int olcxFreeOldCompletionItems(S_olcxReferencesStack *stack);
extern void olcxInit(void);
extern S_userOlcx *olcxSetCurrentUser(char *user);
extern S_reference * getDefinitionRef(S_reference *rr);
extern int safetyCheck2ShouldWarn(void);
extern void olCreateSelectionMenu(int command);
extern void olcxPushEmptyStackItem(S_olcxReferencesStack *stack);
extern void olcxPrintSelectionMenu(S_olSymbolsMenu *);
extern int ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2);
extern void olcxPrintClassTree(S_olSymbolsMenu *sss);
extern void olcxReferencesDiff(S_reference **anr1, S_reference **aor2,
                               S_reference **diff);
extern int olcxShowSelectionMenu(void);
extern int getClassNumFromClassLinkName(char *name, int defaultResult);
extern void getLineColCursorPositionFromCommandLineOption( int *l, int *c );
extern void changeClassReferencesUsages(char *linkName, int category, int fnum,
                                        S_symbol *cclass);
extern int isStrictlyEnclosingClass(int enclosedClass, int enclosingClass);
extern void changeMethodReferencesUsages(char *linkName, int category, int fnum,
                                         S_symbol *cclass);
extern void olcxPushSpecialCheckMenuSym(int command, char *symname);
extern int refOccursInRefs(S_reference *r, S_reference *list);
extern void olcxCheck1CxFileReference(S_symbolRefItem *ss, S_reference *r);
extern void olcxPushSpecial(char *fieldName, int command);
extern int isPushAllMethodsValidRefItem(S_symbolRefItem *ri);
extern int symbolsCorrespondWrtMoving(S_olSymbolsMenu *osym, S_olSymbolsMenu *nsym,
                                      int command);
extern void olcxPrintPushingAction(int opt, int afterMenu);
extern void olPushAllReferencesInBetween(int minMemi, int maxMemi);
extern void tpCheckFillMoveClassData(S_tpCheckMoveClassData *dd, char *spack,
                                     char *tpack);
extern S_symbol *getMoveTargetClass(void);
extern int javaGetSuperClassNumFromClassNum(int cn);
extern int javaIsSuperClass(int superclas, int clas);
extern void pushLocalUnusedSymbolsAction(void);
extern void mainAnswerEditAction(void);
extern void freeOldestOlcx(void);
extern S_olSymbolsMenu *olcxFreeSymbolMenuItem(S_olSymbolsMenu *ll);
extern void olcxFreeResolutionMenu( S_olSymbolsMenu *sym );
extern int refCharCode(int usage);
extern int smartReadFileTabFile(void);

#endif
