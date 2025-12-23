#include "completion.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "commons.h"
#include "filetable.h"
#include "list.h"
#include "options.h"
#include "referenceableitem.h"
#include "session.h"
#include "symbol.h"


// Will create malloc():ed copies of name and fullName so caller don't have to
protected Completion *newCompletion(char *name, char *fullName, int lineCount, Visibility visibility,
                                    Reference reference, ReferenceableItem referenceable) {
    Completion *completion = malloc(sizeof(Completion));

    if (name != NULL) {
        completion->name = strdup(name);
    } else
        completion->name = NULL;

    if (fullName != NULL) {
        completion->fullName = strdup(fullName);
    } else
        completion->fullName = NULL;

    completion->lineCount = lineCount;
    completion->visibility = visibility;
    completion->reference = reference;
    completion->referenceable = referenceable;
    completion->next = NULL;

    return completion;
}

// If symbol == NULL, then the pos is taken as default position of this ref !!!
// If symbol != NULL && referenceableItem != NULL then reference can be anything...
Completion *completionListPrepend(Completion *completions, char *name, char *fullName, Symbol *symbol,
                                  ReferenceableItem *referenceableItem, Reference *reference,
                                  int includedFileNumber) {
    Completion *completion;

    if (referenceableItem != NULL) {
        // probably a 'search in tag' file item
        char *linkName = strdup(referenceableItem->linkName);

        ReferenceableItem item = makeReferenceableItem(linkName, referenceableItem->type,
                                                       referenceableItem->storage, referenceableItem->scope,
                                                       referenceableItem->visibility, referenceableItem->includeFileNumber);

        completion = newCompletion(name, fullName, 1, referenceableItem->visibility, *reference, item);
    } else if (symbol==NULL) {
        Reference r = *reference;
        r.next = NULL;

        ReferenceableItem item = makeReferenceableItem("", TypeUnknown, StorageDefault,
                                                       AutoScope, VisibilityLocal, NO_FILE_NUMBER);

        completion = newCompletion(name, fullName, 1, VisibilityLocal, r, item);
    } else {
        Reference r = makeReference(symbol->pos, UsageNone, NULL);
        Visibility visibility;
        Scope scope;
        Storage storage;
        getSymbolCxrefProperties(symbol, &visibility, &scope, &storage);
        char *linkName = strdup(symbol->linkName);

        ReferenceableItem item = makeReferenceableItem(linkName, symbol->type, storage,
                                                       scope, visibility, includedFileNumber);
        completion = newCompletion(name, fullName, 1, visibility, r, item);
    }
    if (fullName!=NULL) {
        for (int i=0; fullName[i]; i++) {
            if (fullName[i] == '\n')
                completion->lineCount++;
        }
    }
    completion->next = completions;
    return completion;
}

protected void freeCompletion(Completion *completion) {
    free(completion->name);
    free(completion->fullName);
    if (completion->visibility == VisibilityGlobal) {
        assert(completion->referenceable.linkName);
        free(completion->referenceable.linkName);
    }
    free(completion);
}


void freeCompletions(Completion *r) {
    Completion *tmp;

    while (r!=NULL) {
        tmp = r->next;
        freeCompletion(r);
        r = tmp;
    }
}

static bool completionIsLessThan(Completion *c1, Completion *c2) {
    return strcmp(c1->name, c2->name) < 0;
}

static void tagSearchShortRemoveMultipleLines(Completion *list) {
    for (Completion *l=list; l!=NULL; l=l->next) {
    again:
        if (l->next!=NULL && strcmp(l->name, l->next->name)==0) {
            // O.K. remove redundant one
            Completion *tmp = l->next;
            l->next = l->next->next;
            freeCompletion(tmp);
            goto again;          /* Again, but don't advance */
        }
    }
}

static void sortCompletionList(Completion **completions,
                               bool (*compareFunction)(Completion *c1, Completion *c2)) {
    LIST_MERGE_SORT(Completion, *completions, compareFunction);
}

void tagSearchCompactShortResults(void) {
    sortCompletionList(&sessionData.retrievingStack.top->completions,
                       completionIsLessThan);
    if (options.searchKind == SEARCH_DEFINITIONS_SHORT
        || options.searchKind == SEARCH_FULL_SHORT) {
        tagSearchShortRemoveMultipleLines(sessionData.retrievingStack.top->completions);
    }
}
