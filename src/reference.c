#include "reference.h"

#include <stdio.h>

void reset_reference_usage(Reference *reference, UsageKind usage) {
    if (reference != NULL && reference->usage.kind > usage) {
        reference->usage.kind = usage;
    }
}
