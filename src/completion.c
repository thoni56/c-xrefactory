#include "completion.h"

#include <stdio.h>
#include <string.h>

#include "filetable.h"
#include "list.h"
#include "log.h"
#include "options.h"
#include "reference.h"
#include "session.h"
#include "symbol.h"


// Will create malloc():ed copies of name and fullName so caller don't have to
protected Completion *newCompletion(char *name, char *fullName,
                                    int lineCount, Visibility visibility, Type csymType,
                                    struct reference ref, struct referenceItem sym) {
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
    completion->csymType = csymType;
    completion->ref = ref;
    completion->sym = sym;
    completion->next = NULL;

    return completion;
}

// if s==NULL, then the pos is taken as default position of this ref !!!
/* If symbol != NULL && referenceItem != NULL then dfref can be anything... */
Completion *completionListPrepend(Completion *completions, char *name, char *fullName, Symbol *symbol,
                                  ReferenceItem *referenceItem,
                                  Reference *reference, Type cType, int vApplClass) {
    Completion *completion;
    char *linkName;
    Visibility visibility;
    Scope scope;
    Storage storage;
    ReferenceItem item;

    if (referenceItem!=NULL) {
        // probably a 'search in tag' file item
        linkName = strdup(referenceItem->linkName);

        item = makeReferenceItem(linkName, referenceItem->vApplClass, referenceItem->type, referenceItem->storage, referenceItem->scope, referenceItem->visibility);

        completion = newCompletion(name, fullName, 1, referenceItem->visibility, cType, *reference, item);
    } else if (symbol==NULL) {
        Reference r = *reference;
        r.next = NULL;

        item = makeReferenceItem("", NO_FILE_NUMBER, TypeUnknown, StorageDefault,
                                 AutoScope, LocalVisibility);

        completion = newCompletion(name, fullName, 1, LocalVisibility, cType, r, item);
    } else {
        Reference r;
        getSymbolCxrefProperties(symbol, &visibility, &scope, &storage);
        linkName = strdup(symbol->linkName);

        fillReference(&r, r.usage, symbol->pos, NULL);

        item = makeReferenceItem(linkName, vApplClass, symbol->type, storage,
                                 scope, visibility);
        completion = newCompletion(name, fullName, 1, visibility, cType, r, item);
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
    if (completion->visibility == GlobalVisibility) {
        assert(completion->sym.linkName);
        free(completion->sym.linkName);
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
    sortCompletionList(&sessionData.retrieverStack.top->completions,
                       completionIsLessThan);
    if (options.searchKind == SEARCH_DEFINITIONS_SHORT
        || options.searchKind == SEARCH_FULL_SHORT) {
        tagSearchShortRemoveMultipleLines(sessionData.retrieverStack.top->completions);
    }
}
