#include "reference.h"

#include <stdio.h>

#include "filetable.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "usage.h"


Reference makeReference(Position position, Usage usage, Reference *next) {
    Reference reference;
    reference.usage = usage;
    reference.position = position;
    reference.next = next;
    return reference;
}

void fillReference(Reference *reference, Position position, Usage usage, Reference *next) {
    reference->usage = usage;
    reference->position = position;
    reference->next = next;
}

Reference *duplicateReference(Reference *original) {
    // this is used in extract x=x+2; to re-arrange order of references
    // i.e. usage must be first, lValue second.
    original->usage = UsageNone;
    Reference *copy = cxAlloc(sizeof(Reference));
    *copy = *original;
    original->next = copy;
    return copy;
}

void freeReferences(Reference *references) {
    while (references != NULL) {
        Reference *next = references->next;
        free(references);
        references = next;
    }
}

void resetReferenceUsage(Reference *reference,  Usage usage) {
    if (reference != NULL && reference->usage > usage) {
        reference->usage = usage;
    }
}

Reference **addToReferenceList(Reference **list,
                               Position pos, Usage usage) {
    Reference **place;
    Reference reference = makeReference(pos, usage, NULL);

    SORTED_LIST_PLACE2(place, reference, list);
    if (*place==NULL || SORTED_LIST_NEQ((*place),reference)
        || options.serverOperation==OLO_EXTRACT) {
        Reference *r = cxAlloc(sizeof(Reference));
        fillReference(r, pos, usage, NULL);
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
        rr = malloc(sizeof(Reference));
        *rr = *ref;
        LIST_CONS(rr,(*place));
        log_trace("olcx adding %s %s:%d:%d", usageKindEnumName[ref->usage],
                  getFileItem(ref->position.file)->name, ref->position.line,ref->position.col);
    }
    return rr;
}


Reference *olcxAddReference(Reference **rlist, Reference *ref) {
    log_trace("checking ref %s %s:%d:%d at %d", usageKindEnumName[ref->usage],
              simpleFileName(getFileItem(ref->position.file)->name), ref->position.line, ref->position.col, ref);
    if (!OL_VIEWABLE_REFS(ref))
        return NULL; // no regular on-line refs
    return olcxAddReferenceNoUsageCheck(rlist, ref);
}

ReferenceItem makeReferenceItem(char *name, int vApplClass, Type type, Storage storage, Scope scope,
                                Visibility visibility) {
    ReferenceItem item;
    item.linkName = name;
    item.vApplClass = vApplClass;
    item.type = type;
    item.storage = storage;
    item.scope = scope;
    item.visibility = visibility;
    item.next = NULL;
    item.references = NULL;

    return item;
}


void fillReferenceItem(ReferenceItem *referencesItem, char *name, int vApplClass,
                       Type symType, Storage storage, Scope scope,
                       Visibility visibility) {
    referencesItem->linkName = name;
    referencesItem->vApplClass = vApplClass;
    referencesItem->references = NULL;
    referencesItem->next = NULL;
    referencesItem->type = symType;
    referencesItem->storage = storage;
    referencesItem->scope = scope;
    referencesItem->visibility = visibility;
}
