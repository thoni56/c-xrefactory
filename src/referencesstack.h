#ifndef REFERENCESSTACK_H
#define REFERENCESSTACK_H

/* Forward declaration */
typedef struct SessionStackEntry SessionStackEntry;

typedef struct SessionStack {
    SessionStackEntry *top;
    SessionStackEntry *root;
} SessionStack;

/* Type aliases for semantic clarity - each stack uses the same structure
 * but for different purposes (different fields of OlcxReferences are used).
 * All stacks use generic fields: previous, callerPosition, operation, accessTime.
 *
 * BrowserStack (code navigation, references, refactoring):
 *   - references, current, hkSelectedSym, symbolsMenu, menuFilterLevel, refsFilterLevel
 *
 * CompletionStack (code completion):
 *   - completions
 *
 * RetrieverStack (tag search):
 *   - completions
 */
typedef SessionStack BrowsingStack;
typedef SessionStack CompletionStack;
typedef SessionStack RetrievingStack;

/* Generic stack operations */
extern void deleteSessionStackEntry(SessionStack *stack, SessionStackEntry **referencesP);
extern void freePoppedSessionStackEntries(SessionStack *stack);
extern SessionStackEntry *getNextTopStackItem(SessionStack *stack);
extern void freeOldCompletionStackEntries(SessionStack *stack);
extern void pushEmptySession(SessionStack *stack);

#endif
