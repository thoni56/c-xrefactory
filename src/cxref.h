#ifndef CXREF_H_INCLUDED
#define CXREF_H_INCLUDED


#include "browsermenu.h"
#include "session.h"
#include "server.h"
#include "symbol.h"
#include "usage.h"


enum menuFilterLevels {
    MenuFilterAllWithName        = 0,  // Protocol value sent from Emacs
    MenuFilterExactMatch         = 1,
    MenuFilterExactMatchSameFile = 2,
    MAX_MENU_FILTER_LEVEL,
};

enum refsFilterLevels {
    ReferenceFilterAll                    = 0,  // Protocol value sent from Emacs
    ReferenceFilterExcludeReads           = 1,  // also toplevel usage (USAGE_TOP_LEVEL_USED)
    ReferenceFilterExcludeReadsAndAddress = 2,  // also extend usage (USAGE_EXTEND_USAGE)
    ReferenceFilterDefinitionsOnly        = 3,
    MAX_REF_LIST_FILTER_LEVEL,
};


extern bool isSameReferenceableItem(ReferenceableItem *p1, ReferenceableItem *p2);
extern bool haveSameBareName(ReferenceableItem *p1, ReferenceableItem *p2);
extern void recomputeSelectedReferenceable(SessionStackEntry *refs );
extern void addReferencesFromFileToList(Reference *references, int fileNumber, Reference **listP);
extern void processSelectedReferences(SessionStackEntry *rstack,
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
extern bool filterLevelAtLeast(unsigned level, unsigned atLeast);
extern void getLineAndColumnCursorPositionFromCommandLineOptions( int *l, int *c );
extern void olcxPushSpecialCheckMenuSym(char *name);

extern void answerEditAction(void);

extern void generateReferences(void);

#endif
