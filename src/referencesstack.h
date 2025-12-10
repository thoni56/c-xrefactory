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

#endif
