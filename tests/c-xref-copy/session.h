#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

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

    /* ===== Completion and Searching-specific fields (completionStack & searchingStack) ===== */
    struct Match          *matches;
} SessionStackEntry;

typedef struct SessionStack {
    SessionStackEntry *top;
    SessionStackEntry *root;
} SessionStack;

/* Type aliases for semantic clarity - each stack uses the same structure
 * but for different purposes (different fields of OlcxReferences are used).
 * All stacks use generic fields: previous, callerPosition, operation, accessTime.
 *
 * BrowsingStack (code navigation, references, refactoring):
 *   - references, current, hkSelectedSym, symbolsMenu, menuFilterLevel, refsFilterLevel
 *
 * CompletionStack (code completion):
 *   - completions
 *
 * RetrievingStack (tag search):
 *   - completions
 */
typedef SessionStack BrowsingStack;
typedef SessionStack CompletionStack;
typedef SessionStack SearchingStack;

typedef struct SessionData {
    BrowsingStack	browsingStack;
    CompletionStack	completionStack;
    SearchingStack	searchingStack;
} SessionData;


extern SessionData sessionData;


/* Generic stack operations */
extern void deleteSessionStackEntry(SessionStack *stack, SessionStackEntry **referencesP);
extern void freePoppedSessionStackEntries(SessionStack *stack);
extern SessionStackEntry *getNextTopStackItem(SessionStack *stack);
extern void freeOldCompletionStackEntries(SessionStack *stack);
extern void pushEmptySession(SessionStack *stack);

#endif
