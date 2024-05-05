#include "completion.h"

#include <stdio.h>
#include <string.h>

#include "cxfile.h"             /* for cxFileHashnumber() */
#include "filetable.h"
#include "list.h"
#include "log.h"
#include "memory.h"
#include "options.h"
#include "reference.h"
#include "session.h"
#include "symbol.h"


Completion *newCompletion(char *name, char *fullName,
                          short int lineCount, char category, char csymType,
                          struct reference ref, struct referenceItem sym) {
    Completion *completion = olcxAlloc(sizeof(Completion));

    completion->name = name;
    completion->fullName = fullName;
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
Completion *completionListPrepend(Completion *completions, char *name, char *fullText, Symbol *symbol,
                                  ReferenceItem *referenceItem,
                                  Reference *reference, int cType, int vFunClass) {
    Completion    *completion;
    char *ss,*nn, *fullnn;
    ReferenceCategory category;
    ReferenceScope scope;
    Storage storage;
    int slen, nlen;
    ReferenceItem sri;

    nlen = strlen(name);
    nn = olcxAlloc(nlen+1);
    strcpy(nn, name);
    fullnn = NULL;
    if (fullText!=NULL) {
        fullnn = olcxAlloc(strlen(fullText)+1);
        strcpy(fullnn, fullText);
    }
    if (referenceItem!=NULL) {
        // probably a 'search in tag' file item
        slen = strlen(referenceItem->linkName);
        ss = olcxAlloc(slen+1);
        strcpy(ss, referenceItem->linkName);
        fillReferenceItem(&sri, ss, cxFileHashNumber(ss), referenceItem->vApplClass, referenceItem->vFunClass,
                           referenceItem->type, referenceItem->storage, referenceItem->scope,
                           referenceItem->access, referenceItem->category);

        completion = newCompletion(nn, fullnn, 1, referenceItem->category, cType, *reference, sri);
    } else if (symbol==NULL) {
        Reference r = *reference;
        r.next = NULL;
        fillReferenceItem(&sri, "", cxFileHashNumber(""), NO_FILE_NUMBER, NO_FILE_NUMBER, TypeUnknown, StorageDefault,
                           ScopeAuto, AccessDefault, CategoryLocal);
        completion = newCompletion(nn, fullnn, 1, CategoryLocal, cType, r, sri);
    } else {
        Reference r;
        getSymbolCxrefProperties(symbol, &category, &scope, &storage);
        log_trace(":adding sym '%s' %d", symbol->linkName, category);
        slen = strlen(symbol->linkName);
        ss = olcxAlloc(slen+1);
        strcpy(ss, symbol->linkName);
        fillUsage(&r.usage, UsageDefined, 0);
        fillReference(&r, r.usage, symbol->pos, NULL);
        fillReferenceItem(&sri, ss, cxFileHashNumber(ss),
                           vFunClass, vFunClass, symbol->type, storage,
                           scope, symbol->access, category);
        completion = newCompletion(nn, fullnn, 1, category, cType, r, sri);
    }
    if (fullText!=NULL) {
        for (int i=0; fullText[i]; i++) {
            if (fullText[i] == '\n')
                completion->lineCount++;
        }
    }
    completion->next = completions;
    return completion;
}

void olcxFreeCompletion(Completion *r) {
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
