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
typedef SessionStack ReferencesStack;
typedef SessionStack BrowserStack;
typedef SessionStack CompletionStack;
typedef SessionStack RetrieverStack;

/* Generic stack operations */
extern void deleteOlcxRefs(ReferencesStack *stack, SessionStackEntry **referencesP);
extern void freePoppedReferencesStackItems(ReferencesStack *stack);
extern SessionStackEntry *getNextTopStackItem(ReferencesStack *stack);
extern void olcxFreeOldCompletionItems(ReferencesStack *stack);
extern void pushEmptySession(ReferencesStack *stack);

#endif
