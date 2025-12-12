#ifndef CXREF_H_INCLUDED
#define CXREF_H_INCLUDED


#include "browsermenu.h"
#include "session.h"
#include "server.h"
#include "symbol.h"
#include "usage.h"


extern bool isSameReferenceableItem(ReferenceableItem *p1, ReferenceableItem *p2);
extern bool haveSameBareName(ReferenceableItem *p1, ReferenceableItem *p2);
extern void recomputeSelectedReferenceable(SessionStackEntry *refs );
extern void olProcessSelectedReferences(SessionStackEntry *rstack,
                                        void (*referencesMapFun)(SessionStackEntry *rstack,
                                                                 BrowserMenu *ss));
extern void popFromSession(void);
extern void deleteEntryFromSessionStack(SessionStackEntry *refs);
extern int getFileNumberFromName(char *name);
extern Reference *addCxReference(Symbol *symbol, Position position, Usage usage, int includedFileNumber);
extern void addTrivialCxReference (char *name, Type type, Storage storage,
                                   Position position, Usage usage);
extern void olSetCallerPosition(Position position);
extern int itIsSymbolToPushOlReferences(ReferenceableItem *p, SessionStackEntry *rstack,
                                        BrowserMenu **rss, int checkSelFlag);
extern void putOnLineLoadedReferences(ReferenceableItem *p);
extern BrowserMenu *createSelectionMenu(ReferenceableItem *dd);

extern void createSelectionMenuForOperation(ServerOperation command);
extern bool olcxShowSelectionMenu(void);
extern bool ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2);
extern void getLineAndColumnCursorPositionFromCommandLineOptions( int *l, int *c );
extern void olcxPushSpecialCheckMenuSym(char *name);

extern void answerEditAction(void);

extern void generateReferences(void);

#endif
