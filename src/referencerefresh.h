#pragma once

#include "argumentsvector.h"
#include "referenceableitem.h"

extern void reparseStaleFile(int fileNumber, ArgumentsVector baseArgs);

extern void reparseFile(int fileNumber, ArgumentsVector baseArgs);

extern void ensureFreshReferences(ReferenceableItem *item, ArgumentsVector baseArgs);
