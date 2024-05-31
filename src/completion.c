#include "completion.h"

#include <stdio.h>
#include <string.h>

#include "filetable.h"
#include "list.h"
#include "log.h"
#include "memory.h"
#include "options.h"
#include "reference.h"
#include "session.h"
#include "symbol.h"


// Will create olcxAlloc():ed copies of name and fullName so caller don't have to
static Completion *newCompletion(char *name, char *fullName,
                          int lineCount, char category, char csymType,
                          struct reference ref, struct referenceItem sym) {
    Completion *completion = olcxAlloc(sizeof(Completion));

    if (name != NULL) {
        char *nameCopy = olcxAlloc(strlen(name)+1);
        strcpy(nameCopy, name);
        completion->name = nameCopy;
    } else
        completion->name = NULL;

    if (fullName != NULL) {
        char *fullNameCopy = olcxAlloc(strlen(fullName)+1);
        strcpy(fullNameCopy, fullName);
        completion->fullName = fullNameCopy;
    } else
        completion->fullName = NULL;

    completion->lineCount = lineCount;
    completion->category = category;
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
                                  Reference *reference, int cType, int vApplClass) {
    Completion    *completion;
    char *ss;
    ReferenceCategory category;
    ReferenceScope scope;
    Storage storage;
    int slen;
    ReferenceItem sri;

    if (referenceItem!=NULL) {
        // probably a 'search in tag' file item
        slen = strlen(referenceItem->linkName);
        ss = olcxAlloc(slen+1);
        strcpy(ss, referenceItem->linkName);
        fillReferenceItem(&sri, ss, referenceItem->vApplClass,
                          referenceItem->type, referenceItem->storage, referenceItem->scope, referenceItem->category);

        completion = newCompletion(name, fullName, 1, referenceItem->category, cType, *reference, sri);
    } else if (symbol==NULL) {
        Reference r = *reference;
        r.next = NULL;
        fillReferenceItem(&sri, "", NO_FILE_NUMBER, TypeUnknown, StorageDefault,
                          ScopeAuto, CategoryLocal);
        completion = newCompletion(name, fullName, 1, CategoryLocal, cType, r, sri);
    } else {
        Reference r;
        getSymbolCxrefProperties(symbol, &category, &scope, &storage);
        log_trace(":adding sym '%s' %d", symbol->linkName, category);
        slen = strlen(symbol->linkName);
        ss = olcxAlloc(slen+1);
        strcpy(ss, symbol->linkName);
        fillUsage(&r.usage, UsageDefined);
        fillReference(&r, r.usage, symbol->pos, NULL);
        fillReferenceItem(&sri, ss,
                          vApplClass, symbol->type, storage,
                          scope, category);
        completion = newCompletion(name, fullName, 1, category, cType, r, sri);
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

static void olcxFreeCompletion(Completion *r) {
    olcxFree(r->name, strlen(r->name)+1);
    if (r->fullName!=NULL)
        olcxFree(r->fullName, strlen(r->fullName)+1);
    if (r->category == CategoryGlobal) {
        assert(r->sym.linkName);
        olcxFree(r->sym.linkName, strlen(r->sym.linkName)+1);
    }
    olcxFree(r, sizeof(Completion));
}


void olcxFreeCompletions(Completion *r) {
    Completion *tmp;

    while (r!=NULL) {
        tmp = r->next;
        olcxFreeCompletion(r);
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
            olcxFreeCompletion(tmp);
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
