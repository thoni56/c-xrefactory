#ifndef CXFILE_H_INCLUDED
#define CXFILE_H_INCLUDED

#include "referenceableitem.h"


extern void searchSymbolCheckReference(ReferenceableItem *referenceableItem, Reference *reference);

// Abstract API
extern bool loadFileNumbersFromStore(void);
extern bool loadSnapshotFromStore(void);
extern void saveReferencesToStore(char *name);

#endif
