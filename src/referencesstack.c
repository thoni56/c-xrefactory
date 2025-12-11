#include "referencesstack.h"

#include <stdlib.h>
#include <assert.h>

#include "proto.h"
#include "reference.h"
#include "completion.h"
#include "menu.h"
#include "options.h"
#include "position.h"
#include "globals.h"

/* Generic stack operations for ReferencesStack and its semantic aliases
 * (BrowserStack, CompletionStack, RetrieverStack).
 *
 * Functions here operate on the stack structure itself and don't depend
 * on the semantic context (browser navigation vs completion vs retrieval).
 */

void deleteOlcxRefs(ReferencesStack *stack, OlcxReferences **referencesP) {
    OlcxReferences *references = *referencesP;

    freeReferences(references->references);
    freeCompletions(references->completions);
    freeSymbolsMenuList(references->hkSelectedSym);
    freeSymbolsMenuList(references->symbolsMenu);

    // if deleting second entry point, update it
    if (references==stack->top) {
        stack->top = references->previous;
    }
    // this is useless, but one never knows
    if (references==stack->root) {
        stack->root = references->previous;
    }
    *referencesP = references->previous;
    free(references);
}

void freePoppedReferencesStackItems(ReferencesStack *stack) {
    assert(stack);
    // delete all after top
    while (stack->root != stack->top) {
        //&fprintf(dumpOut,":freeing %s\n", stack->root->hkSelectedSym->references.linkName);
        deleteOlcxRefs(stack, &stack->root);
    }
}

static OlcxReferences *pushEmptyReference(ReferencesStack *stack) {
    OlcxReferences *res;

    res  = malloc(sizeof(OlcxReferences));
    *res = (OlcxReferences){.references      = NULL,
                            .current          = NULL,
                            .operation       = options.serverOperation,
                            .callerPosition  = noPosition,
                            .completions     = NULL,
                            .hkSelectedSym   = NULL,
                            .menuFilterLevel = DEFAULT_MENU_FILTER_LEVEL,
                            .refsFilterLevel = DEFAULT_REFS_FILTER_LEVEL,
                            .previous        = stack->top};
    return res;
}

void olcxFreeOldCompletionItems(ReferencesStack *stack) {
    OlcxReferences **referencesP;

    referencesP = &stack->top;
    if (*referencesP == NULL)
        return;
    for (int i=1; i<MAX_COMPLETIONS_HISTORY_DEEP; i++) {
        referencesP = &(*referencesP)->previous;
        if (*referencesP == NULL)
            return;
    }
    deleteOlcxRefs(stack, referencesP);
}

void pushEmptySession(ReferencesStack *stack) {
    OlcxReferences *references;
    freePoppedReferencesStackItems(stack);
    references = pushEmptyReference(stack);
    stack->top = stack->root = references;
}


OlcxReferences *getNextTopStackItem(ReferencesStack *stack) {
    OlcxReferences *thisReferences, *nextReferences;
    nextReferences = NULL;
    thisReferences = stack->root;
    while (thisReferences!=NULL && thisReferences!=stack->top) {
        nextReferences = thisReferences;
        thisReferences = thisReferences->previous;
    }
    assert(thisReferences==stack->top);
    return nextReferences;
}
