#ifndef REFERENCESSTACK_H
#define REFERENCESSTACK_H

/* Forward declaration */
typedef struct olcxReferences OlcxReferences;

typedef struct OlcxReferencesStack {
    OlcxReferences *top;
    OlcxReferences *root;
} OlcxReferencesStack;

/* Type aliases for semantic clarity - each stack uses the same structure
 * but for different purposes (different fields of OlcxReferences are used) */
typedef OlcxReferencesStack ReferencesStack;
typedef OlcxReferencesStack BrowserStack;
typedef OlcxReferencesStack CompletionStack;
typedef OlcxReferencesStack RetrieverStack;

/* Generic stack operations */
extern void deleteOlcxRefs(OlcxReferences **refsP, ReferencesStack *stack);
extern void freePoppedReferencesStackItems(ReferencesStack *stack);
extern OlcxReferences *getNextTopStackItem(ReferencesStack *stack);
extern void olcxFreeOldCompletionItems(ReferencesStack *stack);
extern void pushEmptySession(ReferencesStack *stack);

#endif
