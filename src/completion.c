#include "completion.h"

#include <stdio.h>
#include <string.h>

#include "cxfile.h"
#include "filetable.h"
#include "list.h"
#include "log.h"
#include "memory.h"
#include "options.h"
#include "reference.h"
#include "session.h"
#include "symbol.h"


Completion *newCompletion(char *name, char *fullName, char *vclass, short int jindent,
                          short int lineCount, char category, char csymType,
                          struct reference ref, struct referencesItem sym) {
    Completion *completion = olcxAlloc(sizeof(Completion));

    completion->name = name;
    completion->fullName = fullName;
    completion->vclass = vclass;
    completion->jindent = jindent;
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
Completion *completionListPrepend(Completion *completions, char *name, char *fullText, char *vclass,
                                    int jindent, Symbol *symbol, ReferencesItem *referenceItem,
                                    Reference *reference, int cType, int vFunClass) {
    Completion    *completion;
    char *ss,*nn, *fullnn, *vclnn;
    ReferenceCategory category;
    ReferenceScope scope;
    Storage storage;
    int slen, nlen;
    ReferencesItem sri;

    nlen = strlen(name);
    nn = olcxAllocc(nlen+1, sizeof(char));
    strcpy(nn, name);
    fullnn = NULL;
    if (fullText!=NULL) {
        fullnn = olcxAllocc(strlen(fullText)+1, sizeof(char));
        strcpy(fullnn, fullText);
    }
    vclnn = NULL;
    if (vclass!=NULL) {
        vclnn = olcxAllocc(strlen(vclass)+1, sizeof(char));
        strcpy(vclnn, vclass);
    }
    if (referenceItem!=NULL) {
        // probably a 'search in tag' file item
        slen = strlen(referenceItem->name);
        ss = olcxAllocc(slen+1, sizeof(char));
        strcpy(ss, referenceItem->name);
        fillReferencesItem(&sri, ss, cxFileHashNumber(ss), referenceItem->vApplClass, referenceItem->vFunClass,
                           referenceItem->type, referenceItem->storage, referenceItem->scope,
                           referenceItem->access, referenceItem->category);

        completion = newCompletion(nn, fullnn, vclnn, jindent, 1, referenceItem->category, cType, *reference, sri);
    } else if (symbol==NULL) {
        Reference r = *reference;
        r.next = NULL;
        fillReferencesItem(&sri, "", cxFileHashNumber(""), noFileIndex, noFileIndex, TypeUnknown, StorageNone,
                           ScopeAuto, AccessDefault, CategoryLocal);
        completion = newCompletion(nn, fullnn, vclnn, jindent, 1, CategoryLocal, cType, r, sri);
    } else {
        Reference r;
        getSymbolCxrefProperties(symbol, &category, &scope, &storage);
        log_trace(":adding sym '%s' %d", symbol->linkName, category);
        slen = strlen(symbol->linkName);
        ss = olcxAllocc(slen+1, sizeof(char));
        strcpy(ss, symbol->linkName);
        fillUsage(&r.usage, UsageDefined, 0);
        fillReference(&r, r.usage, symbol->pos, NULL);
        fillReferencesItem(&sri, ss, cxFileHashNumber(ss),
                           vFunClass, vFunClass, symbol->type, storage,
                           scope, symbol->access, category);
        completion = newCompletion(nn, fullnn, vclnn, jindent, 1, category, cType, r, sri);
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
    if (r->vclass!=NULL)
        olcxFree(r->vclass, strlen(r->vclass)+1);
    if (r->category == CategoryGlobal) {
        assert(r->sym.name);
        olcxFree(r->sym.name, strlen(r->sym.name)+1);
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
    if (options.tagSearchSpecif == TSS_SEARCH_DEFS_ONLY_SHORT
        || options.tagSearchSpecif == TSS_FULL_SEARCH_SHORT) {
        tagSearchShortRemoveMultipleLines(sessionData.retrieverStack.top->completions);
    }
}