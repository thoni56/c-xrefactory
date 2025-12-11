#include "reference.h"

#include <stdio.h>
#include <stdlib.h>

#include "commons.h"
#include "filetable.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "usage.h"


Reference *newReference(Position position, Usage usage, Reference *next) {
    Reference *reference = malloc(sizeof(Reference));
    reference->usage = usage;
    reference->position = position;
    reference->next = next;
    return reference;
}

Reference makeReference(Position position, Usage usage, Reference *next) {
    Reference reference;
    reference.usage = usage;
    reference.position = position;
    reference.next = next;
    return reference;
}

Reference *duplicateReferenceInCxMemory(Reference *original) {
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

void setReferenceUsage(Reference *reference,  Usage usage) {
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
        *r = makeReference(pos, usage, NULL);
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

static Reference *addReferenceWithoutUsageCheck(Reference **listP, Reference *ref) {
    Reference **placeInList;
    Reference *r = NULL;

    SORTED_LIST_PLACE2(placeInList, *ref, listP);
    if (*placeInList==NULL || SORTED_LIST_NEQ(*placeInList, *ref)) {
        r = malloc(sizeof(Reference));
        *r = *ref;
        LIST_CONS(r, *placeInList);
        log_debug("olcx adding %s %s:%d:%d", usageKindEnumName[ref->usage],
                  getFileItemWithFileNumber(ref->position.file)->name, ref->position.line,ref->position.col);
    }
    return r;
}


Reference *addReferenceToList(Reference **listP, Reference *ref) {
    log_debug("checking ref %s %s:%d:%d at %d", usageKindEnumName[ref->usage],
              simpleFileName(getFileItemWithFileNumber(ref->position.file)->name), ref->position.line, ref->position.col, ref);
    if (!isVisibleUsage(ref->usage))
        return NULL; // no regular on-line refs
    return addReferenceWithoutUsageCheck(listP, ref);
}

int fileNumberOfReference(Reference reference) {
    return reference.position.file;
}
