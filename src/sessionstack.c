#include "sessionstack.h"

#include <stdlib.h>
#include <assert.h>

#include "browsermenu.h"
#include "completion.h"
#include "cxref.h"
#include "globals.h"
#include "options.h"
#include "position.h"
#include "session.h"


#define DEFAULT_MENU_FILTER_LEVEL MenuFilterExactMatchSameFile
#define DEFAULT_REFS_FILTER_LEVEL ReferenceFilterAll


/* Generic stack operations for SessionStack and its semantic aliases.
 *
 * Functions here operate on the stack structure itself and don't depend
 * on the semantic context (browser navigation vs completion vs retrieval).
 */

void deleteSessionStackEntry(SessionStack *stack, SessionStackEntry **entryP) {
    SessionStackEntry *entry = *entryP;

    freeReferences(entry->references);
    freeCompletions(entry->completions);
    freeBrowserMenuList(entry->hkSelectedSym);
    freeBrowserMenuList(entry->menu);

    // if deleting second entry point, update it
    if (entry==stack->top) {
        stack->top = entry->previous;
    }
    // this is useless, but one never knows
    if (entry==stack->root) {
        stack->root = entry->previous;
    }
    *entryP = entry->previous;
    free(entry);
}

void freePoppedSessionStackEntries(SessionStack *stack) {
    assert(stack);
    // delete all after top
    while (stack->root != stack->top) {
        //&fprintf(dumpOut,":freeing %s\n", stack->root->hkSelectedSym->referenceable.linkName);
        deleteSessionStackEntry(stack, &stack->root);
    }
}

static SessionStackEntry *pushEmptyReference(SessionStack *stack) {
    SessionStackEntry *res;

    res  = malloc(sizeof(SessionStackEntry));
    *res = (SessionStackEntry){.references      = NULL,
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

void freeOldCompletionStackEntries(SessionStack *stack) {
    SessionStackEntry **stackEntry;

    stackEntry = &stack->top;
    if (*stackEntry == NULL)
        return;
    for (int i=1; i<MAX_COMPLETIONS_HISTORY_DEEP; i++) {
        stackEntry = &(*stackEntry)->previous;
        if (*stackEntry == NULL)
            return;
    }
    deleteSessionStackEntry(stack, stackEntry);
}

void pushEmptySession(SessionStack *stack) {
    SessionStackEntry *references;
    freePoppedSessionStackEntries(stack);
    references = pushEmptyReference(stack);
    stack->top = stack->root = references;
}


SessionStackEntry *getNextTopStackItem(SessionStack *stack) {
    SessionStackEntry *thisReferences, *nextReferences;
    nextReferences = NULL;
    thisReferences = stack->root;
    while (thisReferences!=NULL && thisReferences!=stack->top) {
        nextReferences = thisReferences;
        thisReferences = thisReferences->previous;
    }
    assert(thisReferences==stack->top);
    return nextReferences;
}
