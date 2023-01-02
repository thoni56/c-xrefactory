#include "reference.h"

#include <stdio.h>


void fillReference(Reference *reference, Usage usage, Position position, Reference *next) {
    reference->usage = usage;
    reference->position = position;
    reference->next = next;
}

void reset_reference_usage(Reference *reference, UsageKind usage) {
    if (reference != NULL && reference->usage.kind > usage) {
        reference->usage.kind = usage;
    }
}
