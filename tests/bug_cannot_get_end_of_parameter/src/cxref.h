#ifndef CXREF_H_INCLUDED
#define CXREF_H_INCLUDED

#include "completion.h"
#include "menu.h"
#include "ppc.h"
#include "proto.h"
#include "server.h"
#include "session.h"
#include "symbol.h"
#include "usage.h"


extern char olSymbolType[COMPLETION_STRING_SIZE];
extern char olSymbolClassType[COMPLETION_STRING_SIZE];


extern void olcxInit(void);
extern int  olcxReferenceInternalLessFunction(Reference *r1, Reference *r2);
extern void printTagSearchResults(void);
extern bool isSameCxSymbol(ReferenceItem *p1, ReferenceItem *p2);
extern bool olcxIsSameCxSymbol(ReferenceItem *p1, ReferenceItem *p2);
extern void olcxRecomputeSelRefs(OlcxReferences *refs );
extern void olProcessSelectedReferences(OlcxReferences *rstack,
                                        void (*referencesMapFun)(OlcxReferences *rstack,
                                                                 SymbolsMenu *ss));
extern void olcxPopOnly(void);
extern void olStackDeleteSymbol(OlcxReferences *refs);
extern int getFileNumberFromName(char *name);
extern void gotoOnlineCxref(Position *p, UsageKind usageKind, char *suffix);
extern Reference *addNewCxReference(Symbol *symbol, Position *pos,
                                    Usage usage, int vFunClass, int vApplClass);
extern Reference *addCxReference(Symbol *symbol, Position *pos, UsageKind usage,
                                 int vFunClass,int vApplClass);
extern Reference *addSpecialFieldReference(char *name, int storage,
                                           int fnum, Position *p, int usage);
extern void addTrivialCxReference (char *name, int symType, int storage,
                                   Position position, UsageKind usageKind);
extern void olcxAddReferences(Reference *list, Reference **dlist, int fnum,
                              int bestMatchFlag);
extern void olSetCallerPosition(Position *pos);
extern SymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi, Storage storage);
extern void renameCollationSymbols(SymbolsMenu *sss);
extern void olCompletionListReverse(void);
extern int itIsSymbolToPushOlReferences(ReferenceItem *p, OlcxReferences *rstack,
                                      SymbolsMenu **rss, int checkSelFlag);
extern void putOnLineLoadedReferences(ReferenceItem *p);
extern void genOnLineReferences(OlcxReferences *rstack, SymbolsMenu *cms);
extern SymbolsMenu *createSelectionMenu(ReferenceItem *dd);
extern void mapCreateSelectionMenu(ReferenceItem *dd);
extern void olcxFreeOldCompletionItems(OlcxReferencesStack *stack);

extern Reference *getDefinitionRef(Reference *rr);
extern void olCreateSelectionMenu(int command);
extern bool olcxShowSelectionMenu(void);
extern void pushEmptySession(OlcxReferencesStack *stack);
extern bool ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2);
extern void olcxReferencesDiff(Reference **anr1, Reference **aor2,
                               Reference **diff);
extern void getLineAndColumnCursorPositionFromCommandLineOptions( int *l, int *c );
extern void olcxPushSpecialCheckMenuSym(char *symname);
extern bool isPushAllMethodsValidRefItem(ReferenceItem *ri);
extern void olcxPrintPushingAction(ServerOperation operation);
extern void olPushAllReferencesInBetween(int minMemi, int maxMemi);

extern void pushLocalUnusedSymbolsAction(void);
extern void answerEditAction(void);

extern void generateReferences(void);

#endif
