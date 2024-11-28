#include "reference.h"

#include <stdio.h>

#include "filetable.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "usage.h"



void fillReference(Reference *reference, Usage usage, Position position, Reference *next) {
    reference->usage = usage;
    reference->position = position;
    reference->next = next;
}

Reference *duplicateReference(Reference *original) {
    // this is used in extract x=x+2; to re-arrange order of references
    // i.e. usage must be first, lValue second.
    original->usage = NO_USAGE;
    Reference *copy = cxAlloc(sizeof(Reference));
    *copy = *original;
    original->next = copy;
    return copy;
}

void fillReferenceItem(ReferenceItem *referencesItem, char *name, int vApplClass,
                       Type symType, Storage storage, ReferenceScope scope,
                       ReferenceCategory category) {
    referencesItem->linkName = name;
    referencesItem->vApplClass = vApplClass;
    referencesItem->references = NULL;
    referencesItem->next = NULL;
    referencesItem->type = symType;
    referencesItem->storage = storage;
    referencesItem->scope = scope;
    referencesItem->category = category;
}

void freeReferences(Reference *references) {
    while (references != NULL) {
        Reference *next = references->next;
        olcxFree(references, sizeof(Reference));
        references = next;
    }
}

void resetReferenceUsage(Reference *reference, UsageKind usageKind) {
    if (reference != NULL && reference->usage.kind > usageKind) {
        reference->usage.kind = usageKind;
    }
}

Reference **addToReferenceList(Reference **list,
                         Usage usage,
                         Position pos) {
    Reference **place;
    Reference reference;

    fillReference(&reference, usage, pos, NULL);
    SORTED_LIST_PLACE2(place,reference,list);
    if (*place==NULL || SORTED_LIST_NEQ((*place),reference)
        || options.serverOperation==OLO_EXTRACT) {
        Reference *r = cxAlloc(sizeof(Reference));
        fillReference(r, usage, pos, NULL);
        LIST_CONS(r, (*place));
    } else {
        assert(*place);
        (*place)->usage = usage;
    }
    return place;
}

bool isReferenceInList(Reference *reference, Reference *list) {
    Reference *place;
    SORTED_LIST_FIND2(place,Reference, (*reference),list);
    if (place==NULL || SORTED_LIST_NEQ(place, *reference))
        return false;
    return true;
}

Reference *olcxAddReferenceNoUsageCheck(Reference **rlist, Reference *ref) {
    Reference **place, *rr;
    rr = NULL;
    SORTED_LIST_PLACE2(place, *ref, rlist);
    if (*place==NULL || SORTED_LIST_NEQ(*place,*ref)) {
        rr = olcxAlloc(sizeof(Reference));
        *rr = *ref;
        LIST_CONS(rr,(*place));
        log_trace("olcx adding %s %s:%d:%d", usageKindEnumName[ref->usage.kind],
                  getFileItem(ref->position.file)->name, ref->position.line,ref->position.col);
    }
    return rr;
}


Reference *olcxAddReference(Reference **rlist, Reference *ref) {
    log_trace("checking ref %s %s:%d:%d at %d", usageKindEnumName[ref->usage.kind],
              simpleFileName(getFileItem(ref->position.file)->name), ref->position.line, ref->position.col, ref);
    if (!OL_VIEWABLE_REFS(ref))
        return NULL; // no regular on-line refs
    return olcxAddReferenceNoUsageCheck(rlist, ref);
}

#if 0
// I don't know why this is out-pre-processed...
void olcxCheck1CxFileReference(ReferenceItem *referenceItem, Reference *reference) {
    ReferenceItem     *sss;
    OlcxReferences    *rstack;
    SymbolsMenu     *cms;
    int pushedKind;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top->previous;
    assert(rstack && rstack->menuSym);
    sss = &rstack->menuSym->references;
    pushedKind = itIsSymbolToPushOlReferences(referenceItem, rstack, &cms, DEFAULT_VALUE);
    // this is very slow to check the symbol name for each reference
    if (pushedKind == 0 && olcxIsSameCxSymbol(referenceItem, sss)) {
        olcxSingleReferenceCheck1(referenceItem, rstack, reference);
    }
}
#endif
