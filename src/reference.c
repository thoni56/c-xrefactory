#include "reference.h"

#include <stdio.h>

#include "options.h"
#include "list.h"


void fillReference(Reference *reference, Usage usage, Position position, Reference *next) {
    reference->usage = usage;
    reference->position = position;
    reference->next = next;
}

void reset_reference_usage(Reference *reference, UsageKind usageKind) {
    if (reference != NULL && reference->usage.kind > usageKind) {
        reference->usage.kind = usageKind;
    }
}

Reference **addToRefList(Reference **list,
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
