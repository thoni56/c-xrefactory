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

void deleteOlcxRefs(OlcxReferences **refsP, ReferencesStack *stack) {
    OlcxReferences    *refs = *refsP;

    freeReferences(refs->references);
    freeCompletions(refs->completions);
    freeSymbolsMenuList(refs->hkSelectedSym);
    freeSymbolsMenuList(refs->symbolsMenu);

    // if deleting second entry point, update it
    if (refs==stack->top) {
        stack->top = refs->previous;
    }
    // this is useless, but one never knows
    if (refs==stack->root) {
        stack->root = refs->previous;
    }
    *refsP = refs->previous;
    free(refs);
}

void freePoppedReferencesStackItems(ReferencesStack *stack) {
    assert(stack);
    // delete all after top
    while (stack->root != stack->top) {
        //&fprintf(dumpOut,":freeing %s\n", stack->root->hkSelectedSym->references.linkName);
        deleteOlcxRefs(&stack->root, stack);
    }
}

static OlcxReferences *pushEmptyReference(ReferencesStack *stack) {
    OlcxReferences *res;

    res  = malloc(sizeof(OlcxReferences));
    *res = (OlcxReferences){.references      = NULL,
                            .current          = NULL,
                            .operation       = options.serverOperation,
                            .accessTime      = fileProcessingStartTime,
                            .callerPosition  = noPosition,
                            .completions     = NULL,
                            .hkSelectedSym   = NULL,
                            .menuFilterLevel = DEFAULT_MENU_FILTER_LEVEL,
                            .refsFilterLevel = DEFAULT_REFS_FILTER_LEVEL,
                            .previous        = stack->top};
    return res;
}

void olcxFreeOldCompletionItems(ReferencesStack *stack) {
    OlcxReferences **references;

    references = &stack->top;
    if (*references == NULL)
        return;
    for (int i=1; i<MAX_COMPLETIONS_HISTORY_DEEP; i++) {
        references = &(*references)->previous;
        if (*references == NULL)
            return;
    }
    deleteOlcxRefs(references, stack);
}

void pushEmptySession(ReferencesStack *stack) {
    OlcxReferences *res;
    freePoppedReferencesStackItems(stack);
    res = pushEmptyReference(stack);
    stack->top = stack->root = res;
}


OlcxReferences *getNextTopStackItem(ReferencesStack *stack) {
    OlcxReferences *rr, *nextrr;
    nextrr = NULL;
    rr = stack->root;
    while (rr!=NULL && rr!=stack->top) {
        nextrr = rr;
        rr = rr->previous;
    }
    assert(rr==stack->top);
    return nextrr;
}
