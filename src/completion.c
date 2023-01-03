#include "completion.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "reference.h"


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

#if 0
// if s==NULL, then the pos is taken as default position of this ref !!!

/* If symbol != NULL && referenceItem != NULL then dfref can be anything... */
Completion *completionListPrepend(Completion *completions, char *name, char *fullText, char *vclass,
                                    int jindent, Symbol *symbol, ReferencesItem *referenceItem,
                                    Reference *reference, int cType, int vFunClass) {
    Completion    *completion;
    char *ss,*nn, *fullnn, *vclnn;
    int category, scope, storage, slen, nlen;
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
#endif
