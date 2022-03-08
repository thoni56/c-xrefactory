#ifndef CXREF_H_INCLUDED
#define CXREF_H_INCLUDED

#include "proto.h"
#include "usage.h"
#include "symbol.h"
#include "olcxtab.h"
#include "ppc.h"


extern void fillReference(Reference *reference, Usage usage, Position position, Reference *next);
extern void fillSymbolRefItem(SymbolReferenceItem *symbolRefItem, char *name, unsigned fileHash, int vApplClass,
                              int vFunClass);
extern void fillSymbolRefItemBits(SymbolReferenceItemBits *symbolRefItemBits, unsigned symType,
                                  unsigned storage, unsigned scope, unsigned accessFlags,
                                  unsigned category);
extern void fillSymbolsMenu(SymbolsMenu *symbolsMenu, struct symbolReferenceItem s,
                            char selected, char visible, unsigned ooBits, char olUsage,
                            short int vlevel, short int refn, short int defRefn,
                            char defUsage, struct position defpos, int outOnLine,
                            struct editorMarkerList *markers,	/* for refactory only */
                            SymbolsMenu *next);
extern int olcxReferenceInternalLessFunction(Reference *r1, Reference *r2);
extern bool olSymbolRefItemLess(SymbolReferenceItem *s1, SymbolReferenceItem *s2);
extern void tagSearchCompactShortResults(void);
extern void printTagSearchResults(void);
extern SymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi, int storage);
extern bool isSameCxSymbol(SymbolReferenceItem *p1, SymbolReferenceItem *p2);
extern bool isSameCxSymbolIncludingFunctionClass(SymbolReferenceItem *p1, SymbolReferenceItem *p2);
extern bool isSameCxSymbolIncludingApplicationClass(SymbolReferenceItem *p1, SymbolReferenceItem *p2);
extern bool olcxIsSameCxSymbol(SymbolReferenceItem *p1, SymbolReferenceItem *p2);
extern void olcxRecomputeSelRefs(OlcxReferences *refs );
extern void olProcessSelectedReferences(OlcxReferences *rstack,
                                        void (*referencesMapFun)(OlcxReferences *rstack,
                                                                 SymbolsMenu *ss));
extern void olcxPopOnly(void);
extern Reference * olcxCopyRefList(Reference *ll);
extern void olStackDeleteSymbol(OlcxReferences *refs);
extern int getFileNumberFromName(char *name);
extern void gotoOnlineCxref(Position *p, int usage, char *suffix);
extern Reference *olcxAddReferenceNoUsageCheck(Reference **rlist,
                                               Reference *ref,
                                               int bestMatchFlag);
extern Reference *olcxAddReference(Reference **rlist,
                                   Reference *ref,
                                   int bestMatchFlag);
extern void olcxFreeReferences(Reference *r);
extern bool isSmallerOrEqClass(int inf, int sup);
extern char *getJavaDocUrl_st(SymbolReferenceItem *rr);
extern char *getLocalJavaDocFile_st(char *fileUrl);
extern char *getFullUrlOfJavaDoc_st(char *fileUrl);
extern bool htmlJdkDocAvailableForUrl(char *ss);
extern Reference *duplicateReference(Reference *r);
extern Reference * addCxReferenceNew(Symbol *symbol, Position *pos,
                                     Usage usage, int vFunClass, int vApplClass);
extern Reference * addCxReference(Symbol *symbol, Position *pos, UsageKind usage,
                                  int vFunClass,int vApplClass);
extern Reference *addSpecialFieldReference(char *name, int storage,
                                           int fnum, Position *p, int usage);
extern void addClassTreeHierarchyReference(int fnum, Position *p, int usage);
extern void addCfClassTreeHierarchyRef(int fnum, int usage);
extern void addTrivialCxReference (char *name, int symType, int storage,
                                   Position *pos, UsageKind usageKind);
extern void olcxAddReferences(Reference *list, Reference **dlist, int fnum,
                              int bestMatchFlag);
extern void olSetCallerPosition(Position *pos);
extern S_olCompletion * olCompletionListPrepend(char *name, char *fullText,
                                                char *vclass, int jindent, Symbol *s,
                                                SymbolReferenceItem *ri, Reference *dfpos,
                                                int symType, int vFunClass,
                                                OlcxReferences *stack);
extern SymbolsMenu *olCreateNewMenuItem(SymbolReferenceItem *sym, int vApplClass,
                                            int vFunCl, Position *defpos, int defusage,
                                            int selected, int visible,
                                            unsigned ooBits, int olusage, int vlevel
                                            );
extern SymbolsMenu *olAddBrowsedSymbol(SymbolReferenceItem *sym, SymbolsMenu **list,
                                           int selected, int visible, unsigned ooBits,
                                           int olusage, int vlevel,
                                           Position *defpos, int defusage);
extern void renameCollationSymbols(SymbolsMenu *sss);
extern void olCompletionListReverse(void);
extern Reference **addToRefList(Reference **list,
                                Usage usage,
                                Position pos);
extern char *getXrefEnvironmentValue(char *name);
extern int itIsSymbolToPushOlReferences(SymbolReferenceItem *p, OlcxReferences *rstack,
                                      SymbolsMenu **rss, int checkSelFlag);
extern void olcxAddReferenceToSymbolsMenu(SymbolsMenu  *cms, Reference *rr,
                                            int bestFitFlag);
extern void putOnLineLoadedReferences(SymbolReferenceItem *p);
extern void genOnLineReferences(OlcxReferences *rstack, SymbolsMenu *cms);
extern SymbolsMenu *createSelectionMenu(SymbolReferenceItem *dd);
extern void mapCreateSelectionMenu(SymbolReferenceItem *dd);
extern void olcxFreeOldCompletionItems(OlcxReferencesStack *stack);

extern void olcxInit(void);
extern UserOlcxData *olcxSetCurrentUser(char *user);
extern Reference * getDefinitionRef(Reference *rr);
extern bool safetyCheck2ShouldWarn(void);
extern void olCreateSelectionMenu(int command);
extern void olcxPushEmptyStackItem(OlcxReferencesStack *stack);
extern void olcxPrintSelectionMenu(SymbolsMenu *sss);
extern bool ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2);
extern void olcxPrintClassTree(SymbolsMenu *sss);
extern void olcxReferencesDiff(Reference **anr1, Reference **aor2,
                               Reference **diff);
extern bool olcxShowSelectionMenu(void);
extern int getClassNumFromClassLinkName(char *name, int defaultResult);
extern void getLineColCursorPositionFromCommandLineOption( int *l, int *c );
extern void changeClassReferencesUsages(char *linkName, int category, int fnum,
                                        Symbol *cclass);
extern bool isStrictlyEnclosingClass(int enclosedClass, int enclosingClass);
extern void changeMethodReferencesUsages(char *linkName, int category, int fnum,
                                         Symbol *cclass);
extern void olcxPushSpecialCheckMenuSym(char *symname);
extern bool refOccursInRefs(Reference *r, Reference *list);
extern void olcxCheck1CxFileReference(SymbolReferenceItem *ss, Reference *r);
extern void olcxPushSpecial(char *fieldName, int command);
extern bool isPushAllMethodsValidRefItem(SymbolReferenceItem *ri);
extern bool symbolsCorrespondWrtMoving(SymbolsMenu *osym, SymbolsMenu *nsym,
                                      int command);
extern void olcxPrintPushingAction(int opt, int afterMenu);
extern void olPushAllReferencesInBetween(int minMemi, int maxMemi);
extern Symbol *getMoveTargetClass(void);
extern int javaGetSuperClassNumFromClassNum(int cn);
extern bool javaIsSuperClass(int superclas, int clas);
extern void pushLocalUnusedSymbolsAction(void);
extern void mainAnswerEditAction(void);
extern void freeOldestOlcx(void);
extern SymbolsMenu *olcxFreeSymbolMenuItem(SymbolsMenu *ll);
extern void olcxFreeResolutionMenu( SymbolsMenu *sym );
extern int refCharCode(int usage);

extern void initAvailableRefactorings(void);

#endif
