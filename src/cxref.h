#ifndef CXREF_H_INCLUDED
#define CXREF_H_INCLUDED

#include "proto.h"
#include "server.h"
#include "usage.h"
#include "symbol.h"
#include "session.h"
#include "ppc.h"

extern void fillReference(Reference *reference, Usage usage, Position position, Reference *next);
extern void fillReferencesItem(ReferencesItem *referencesItem, char *name, unsigned fileHash, int vApplClass,
                               int vFunClass, Type symType, Storage storage, ReferenceScope scope,
                               Access accessFlags, ReferenceCategory category);
extern void fillSymbolsMenu(SymbolsMenu *symbolsMenu, struct referencesItem s, bool selected, bool visible,
                            unsigned ooBits, char olUsage, short int vlevel, short int refn, short int defRefn,
                            char defUsage, struct position defpos, int outOnLine,
                            struct editorMarkerList *markers, /* for refactory only */
                            SymbolsMenu             *next);
extern int  olcxReferenceInternalLessFunction(Reference *r1, Reference *r2);
extern void tagSearchCompactShortResults(void);
extern void printTagSearchResults(void);
extern SymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi, Storage storage);
extern bool isSameCxSymbol(ReferencesItem *p1, ReferencesItem *p2);
extern bool isSameCxSymbolIncludingFunctionClass(ReferencesItem *p1, ReferencesItem *p2);
extern bool isSameCxSymbolIncludingApplicationClass(ReferencesItem *p1, ReferencesItem *p2);
extern bool olcxIsSameCxSymbol(ReferencesItem *p1, ReferencesItem *p2);
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
extern char *getJavaDocUrl_st(ReferencesItem *rr);
extern char *getLocalJavaDocFile_st(char *fileUrl);
extern char *getFullUrlOfJavaDoc_st(char *fileUrl);
extern bool htmlJdkDocAvailableForUrl(char *ss);
extern Reference *duplicateReference(Reference *r);
extern Reference * addNewCxReference(Symbol *symbol, Position *pos,
                                     Usage usage, int vFunClass, int vApplClass);
extern Reference * addCxReference(Symbol *symbol, Position *pos, UsageKind usage,
                                  int vFunClass,int vApplClass);
extern Reference *addSpecialFieldReference(char *name, int storage,
                                           int fnum, Position *p, int usage);
extern void addClassTreeHierarchyReference(int fnum, Position *p, int usage);
extern void addCfClassTreeHierarchyRef(int fnum, int usage);
extern void addTrivialCxReference (char *name, int symType, int storage,
                                   Position position, UsageKind usageKind);
extern void olcxAddReferences(Reference *list, Reference **dlist, int fnum,
                              int bestMatchFlag);
extern void olSetCallerPosition(Position *pos);
extern Completion * olCompletionListPrepend(char *name, char *fullText,
                                                char *vclass, int jindent, Symbol *s,
                                                ReferencesItem *ri, Reference *dfpos,
                                                int symType, int vFunClass,
                                                OlcxReferences *stack);
extern SymbolsMenu *olCreateNewMenuItem(ReferencesItem *sym, int vApplClass,
                                            int vFunCl, Position *defpos, int defusage,
                                            int selected, int visible,
                                            unsigned ooBits, int olusage, int vlevel
                                            );
extern SymbolsMenu *olAddBrowsedSymbol(ReferencesItem *sym, SymbolsMenu **list,
                                           int selected, int visible, unsigned ooBits,
                                           int olusage, int vlevel,
                                           Position *defpos, int defusage);
extern void renameCollationSymbols(SymbolsMenu *sss);
extern void olCompletionListReverse(void);
extern Reference **addToRefList(Reference **list,
                                Usage usage,
                                Position pos);
extern char *getVariable(char *name);
extern int itIsSymbolToPushOlReferences(ReferencesItem *p, OlcxReferences *rstack,
                                      SymbolsMenu **rss, int checkSelFlag);
extern void olcxAddReferenceToSymbolsMenu(SymbolsMenu  *cms, Reference *rr,
                                            int bestFitFlag);
extern void putOnLineLoadedReferences(ReferencesItem *p);
extern void genOnLineReferences(OlcxReferences *rstack, SymbolsMenu *cms);
extern SymbolsMenu *createSelectionMenu(ReferencesItem *dd);
extern void mapCreateSelectionMenu(ReferencesItem *dd);
extern void olcxFreeOldCompletionItems(OlcxReferencesStack *stack);

extern void olcxInit(void);
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
extern void getLineAndColumnCursorPositionFromCommandLineOptions( int *l, int *c );
extern void changeClassReferencesUsages(char *linkName, int category, int fnum,
                                        Symbol *cclass);
extern bool isStrictlyEnclosingClass(int enclosedClass, int enclosingClass);
extern void changeMethodReferencesUsages(char *linkName, int category, int fnum,
                                         Symbol *cclass);
extern void olcxPushSpecialCheckMenuSym(char *symname);
extern bool refOccursInRefs(Reference *r, Reference *list);
extern void olcxCheck1CxFileReference(ReferencesItem *ss, Reference *r);
extern void olcxPushSpecial(char *fieldName, int command);
extern bool isPushAllMethodsValidRefItem(ReferencesItem *ri);
extern bool symbolsCorrespondWrtMoving(SymbolsMenu *osym, SymbolsMenu *nsym,
                                       ServerOperation operation);
extern void olcxPrintPushingAction(int opt, int afterMenu);
extern void olPushAllReferencesInBetween(int minMemi, int maxMemi);
extern Symbol *getMoveTargetClass(void);
extern int javaGetSuperClassNumFromClassNum(int cn);
extern bool javaIsSuperClass(int superclas, int clas);
extern void pushLocalUnusedSymbolsAction(void);
extern void answerEditAction(void);
extern SymbolsMenu *olcxFreeSymbolMenuItem(SymbolsMenu *ll);
extern void olcxFreeResolutionMenu( SymbolsMenu *sym );
extern int refCharCode(int usage);

extern void clearAvailableRefactorings(void);

extern void generateReferences(void);

#endif
