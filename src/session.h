#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "referencesstack.h"

#include "position.h"
#include "server.h"


typedef struct SessionStackEntry {
    /* ===== Generic stack fields (used by all stack types) ===== */
    struct SessionStackEntry *previous;      /* linked-list for stack mechanics */
    struct position        callerPosition; /* where the operation was initiated */
    ServerOperation        operation;      /* OLO_PUSH/OLO_LIST/OLO_COMPLETION/etc */

    /* ===== Browser-specific fields (browserStack only) ===== */
    struct reference      *references;     /* list of references for browsing */
    struct reference      *current;        /* current reference position */
    struct BrowserMenu    *hkSelectedSym;  /* resolved symbols under the cursor */
    struct BrowserMenu    *menu;    /* hkSelectedSyms plus same name */
    // following two lists should be probably split into hashed tables of lists
    // because of bad performances for class tree and global unused symbols
    int                    menuFilterLevel; /* filter level for menu display */
    int                    refsFilterLevel; /* filter level for references display */

    /* ===== Completion-specific fields (completionsStack & retrieverStack) ===== */
    struct completion     *completions;    /* completions list for OLO_COMPLETION */
} SessionStackEntry;

typedef struct SessionData {
    BrowsingStack	browsingStack;
    CompletionStack	completionStack;
    RetrievingStack	retrievingStack;
} SessionData;


extern SessionData sessionData;

#endif
