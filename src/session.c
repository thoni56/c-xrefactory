#include "session.h"

#include <stdlib.h>

#include "browsermenu.h"
#include "commons.h"
#include "cxref.h"
#include "globals.h"
#include "match.h"
#include "options.h"
#include "reference.h"


SessionData sessionData;


#define MAX_COMPLETIONS_HISTORY 10   /* maximal length of completion history */

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
    freeMatches(entry->matches);
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

protected SessionStackEntry *newEmptySessionStackEntry(void) {
    SessionStackEntry *entry  = malloc(sizeof(SessionStackEntry));
    *entry = (SessionStackEntry){
        .references      = NULL,
        .current         = NULL,
        .operation       = options.serverOperation,
        .callerPosition  = NO_POSITION,
        .matches         = NULL,
        .hkSelectedSym   = NULL,
        .menuFilterLevel = DEFAULT_MENU_FILTER_LEVEL,
        .refsFilterLevel = DEFAULT_REFS_FILTER_LEVEL,
        .previous        = NULL};
    return entry;
}

static SessionStackEntry *pushEntryWithNoReferences(SessionStack *stack) {
    SessionStackEntry *entry = newEmptySessionStackEntry();
    entry->previous = stack->top;
    return entry;
}

void freeOldCompletionStackEntries(SessionStack *stack) {
    SessionStackEntry **entry;

    entry = &stack->top;
    if (*entry == NULL)
        return;
    for (int i=1; i<MAX_COMPLETIONS_HISTORY; i++) {
        entry = &(*entry)->previous;
        if (*entry == NULL)
            return;
    }
    deleteSessionStackEntry(stack, entry);
}

void pushEmptySession(SessionStack *stack) {
    SessionStackEntry *entry;
    freePoppedSessionStackEntries(stack);
    entry = pushEntryWithNoReferences(stack);
    stack->top = stack->root = entry;
}


SessionStackEntry *getNextTopStackItem(SessionStack *stack) {
    SessionStackEntry *this, *next;
    next = NULL;
    this = stack->root;
    while (this!=NULL && this!=stack->top) {
        next = this;
        this = this->previous;
    }
    assert(this==stack->top);
    return next;
}

SessionStackEntry *getSessionEntryForOperation(ServerOperation operation) {
    if (operation == OP_COMPLETION
        || operation == OP_COMPLETION_SELECT
        || operation == OP_COMPLETION_GOTO_N
        || operation == OP_SEARCH)
        return sessionData.completionStack.top;
    return sessionData.browsingStack.top;
}
