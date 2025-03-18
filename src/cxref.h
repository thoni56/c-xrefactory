#ifndef CXREF_H_INCLUDED
#define CXREF_H_INCLUDED


#include "menu.h"
#include "proto.h"
#include "server.h"
#include "symbol.h"
#include "usage.h"



extern int  olcxReferenceInternalLessFunction(Reference *r1, Reference *r2);
extern bool isSameCxSymbol(ReferenceItem *p1, ReferenceItem *p2);
extern bool olcxIsSameCxSymbol(ReferenceItem *p1, ReferenceItem *p2);
extern void olcxRecomputeSelRefs(OlcxReferences *refs );
extern void olProcessSelectedReferences(OlcxReferences *rstack,
                                        void (*referencesMapFun)(OlcxReferences *rstack,
                                                                 SymbolsMenu *ss));
extern void olcxPopOnly(void);
extern void olStackDeleteSymbol(OlcxReferences *refs);
extern int getFileNumberFromName(char *name);
extern Reference *addCxReference(Symbol *symbol, Position position, Usage usage, int includedFileNumber);
extern void addTrivialCxReference (char *name, Type type, Storage storage,
                                   Position position, Usage usage);
extern void olSetCallerPosition(Position position);
extern int itIsSymbolToPushOlReferences(ReferenceItem *p, OlcxReferences *rstack,
                                        SymbolsMenu **rss, int checkSelFlag);
extern void putOnLineLoadedReferences(ReferenceItem *p);
extern SymbolsMenu *createSelectionMenu(ReferenceItem *dd);
extern void olcxFreeOldCompletionItems(OlcxReferencesStack *stack);

extern void olCreateSelectionMenu(ServerOperation command);
extern bool olcxShowSelectionMenu(void);
extern void pushEmptySession(OlcxReferencesStack *stack);
extern bool ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2);
extern void getLineAndColumnCursorPositionFromCommandLineOptions( int *l, int *c );
extern void olcxPushSpecialCheckMenuSym(char *symname);

extern void answerEditAction(void);

extern void generateReferences(void);

#endif
