#ifndef CXREF_H_INCLUDED
#define CXREF_H_INCLUDED


#include "browsermenu.h"
#include "proto.h"
#include "server.h"
#include "symbol.h"
#include "usage.h"



extern int  olcxReferenceInternalLessFunction(Reference *r1, Reference *r2);
extern bool isSameReferenceableItem(ReferenceableItem *p1, ReferenceableItem *p2);
extern bool haveSameBareName(ReferenceableItem *p1, ReferenceableItem *p2);
extern void olcxRecomputeSelRefs(OlcxReferences *refs );
extern void olProcessSelectedReferences(OlcxReferences *rstack,
                                        void (*referencesMapFun)(OlcxReferences *rstack,
                                                                 BrowserMenu *ss));
extern void olcxPopOnly(void);
extern void olStackDeleteSymbol(OlcxReferences *refs);
extern int getFileNumberFromName(char *name);
extern Reference *addCxReference(Symbol *symbol, Position position, Usage usage, int includedFileNumber);
extern void addTrivialCxReference (char *name, Type type, Storage storage,
                                   Position position, Usage usage);
extern void olSetCallerPosition(Position position);
extern int itIsSymbolToPushOlReferences(ReferenceableItem *p, OlcxReferences *rstack,
                                        BrowserMenu **rss, int checkSelFlag);
extern void putOnLineLoadedReferences(ReferenceableItem *p);
extern BrowserMenu *createSelectionMenu(ReferenceableItem *dd);

extern void olCreateSelectionMenu(ServerOperation command);
extern bool olcxShowSelectionMenu(void);
extern bool ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2);
extern void getLineAndColumnCursorPositionFromCommandLineOptions( int *l, int *c );
extern void olcxPushSpecialCheckMenuSym(char *symname);

extern void answerEditAction(void);

extern void generateReferences(void);

#endif
