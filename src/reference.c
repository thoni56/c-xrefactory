#include "reference.h"

#include <stdio.h>

#include "options.h"
#include "list.h"
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

void fillReferencesItem(ReferencesItem *referencesItem, char *name, unsigned fileHash, int vApplClass,
                        int vFunClass, Type symType, Storage storage, ReferenceScope scope, Access accessFlags,
                        ReferenceCategory category) {
    referencesItem->name = name;
    referencesItem->fileHash = fileHash;
    referencesItem->vApplClass = vApplClass;
    referencesItem->vFunClass = vFunClass;
    referencesItem->references = NULL;
    referencesItem->next = NULL;
    referencesItem->type = symType;
    referencesItem->storage = storage;
    referencesItem->scope = scope;
    referencesItem->access = accessFlags;
    referencesItem->category = category;
}

void reset_reference_usage(Reference *reference, UsageKind usageKind) {
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

/*&
void deleteFromRefList(void *p) {
  Reference **pp, *ff;
  pp = (S_reference **) p;
  ff = *pp;
  *pp = (*pp)->next;
  CX_FREE(ff);
}
&*/
