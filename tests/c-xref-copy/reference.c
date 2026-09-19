#include "reference.h"

#include <stdio.h>
#include <stdlib.h>

#include "commons.h"
#include "filetable.h"
#include "list.h"
#include "log.h"
#include "memory.h"
#include "misc.h"
#include "parsing.h"
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
    if (reference != NULL && isLessImportantUsageThan(reference->usage, usage)) {
        reference->usage = usage;
    }
}

Reference **addToReferenceList(Reference **list,
                               Position position, Usage usage) {
    Reference **place;
    Reference reference = makeReference(position, usage, NULL);

    SORTED_LIST_PLACE2(place, reference, list);
    if (*place==NULL || SORTED_LIST_NEQ((*place),reference)
        || allowsDuplicateReferences(parsingConfig.operation)) {
        Reference *r = cxAlloc(sizeof(Reference));
        *r = makeReference(position, usage, NULL);
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

static Reference *addReferenceToListWithoutUsageCheck(Reference *ref, Reference **listP) {
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


Reference *addReferenceToList(Reference *reference, Reference **listP) {
    log_debug("checking ref %s %s:%d:%d at %d", usageKindEnumName[reference->usage],
              simpleFileName(getFileItemWithFileNumber(reference->position.file)->name),
              reference->position.line, reference->position.col, reference);
    if (!isVisibleUsage(reference->usage))
        return NULL; // no regular on-line refs
    return addReferenceToListWithoutUsageCheck(reference, listP);
}

int fileNumberOfReference(Reference reference) {
    return reference.position.file;
}
