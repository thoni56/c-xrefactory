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
    Completion *completion = olcx_alloc(sizeof(Completion));

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
    nn = olcx_memory_allocc(nlen+1, sizeof(char));
    strcpy(nn, name);
    fullnn = NULL;
    if (fullText!=NULL) {
        fullnn = olcx_memory_allocc(strlen(fullText)+1, sizeof(char));
        strcpy(fullnn, fullText);
    }
    vclnn = NULL;
    if (vclass!=NULL) {
        vclnn = olcx_memory_allocc(strlen(vclass)+1, sizeof(char));
        strcpy(vclnn, vclass);
    }
    if (referenceItem!=NULL) {
        // probably a 'search in tag' file item
        slen = strlen(referenceItem->name);
        ss = olcx_memory_allocc(slen+1, sizeof(char));
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
        ss = olcx_memory_allocc(slen+1, sizeof(char));
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
    olcx_memory_free(r->name, strlen(r->name)+1);
    if (r->fullName!=NULL)
        olcx_memory_free(r->fullName, strlen(r->fullName)+1);
    if (r->vclass!=NULL)
        olcx_memory_free(r->vclass, strlen(r->vclass)+1);
    if (r->category == CategoryGlobal) {
        assert(r->sym.name);
        olcx_memory_free(r->sym.name, strlen(r->sym.name)+1);
    }
    olcx_memory_free(r, sizeof(Completion));
}


void olcxFreeCompletions(Completion *r) {
    Completion *tmp;

    while (r!=NULL) {
        tmp = r->next;
        olcxFreeCompletion(r);
        r = tmp;
    }
}

#if 0
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

static int olTagSearchSortFunction(Completion *c1, Completion *c2) {
    return strcmp(c1->name, c2->name) < 0;
}

void tagSearchCompactShortResults(void) {
    LIST_MERGE_SORT(Completion, sessionData.retrieverStack.top->completions, olTagSearchSortFunction);
    if (options.tagSearchSpecif==TSS_SEARCH_DEFS_ONLY_SHORT
        || options.tagSearchSpecif==TSS_FULL_SEARCH_SHORT) {
        tagSearchShortRemoveMultipleLines(sessionData.retrieverStack.top->completions);
    }
}
#endif
