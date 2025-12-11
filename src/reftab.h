#ifndef REFTAB_H_INCLUDED
#define REFTAB_H_INCLUDED

#include "referenceableitem.h"

#define HASH_TAB_NAME refTab
#define HASH_TAB_TYPE ReferenceTable
#define HASH_ELEM_TYPE ReferenceableItem

#include "hashlist.th"


#ifndef IN_REFTAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern void initReferenceTable(int size);
extern int  addToReferencesTable(ReferenceableItem *referencesItem);
extern void pushReferenceableItem(ReferenceableItem *element, int position);
extern ReferenceableItem *getReferenceableItem(int index);
extern int getNextExistingReferenceableItem(int index);
extern void setReferenceableItem(int index, ReferenceableItem *item);

extern bool isMemberInReferenceTable(ReferenceableItem *item, int *indexP, ReferenceableItem **originP);
extern void mapOverReferenceTable(void (*fun)(ReferenceableItem *));
extern void mapOverReferenceTableWithIndex(void (*fun)(int));
extern void mapOverReferenceTableWithPointer(void (*fun)(ReferenceableItem *, void *), void *pointer);
extern void removeReferencesForFile(int fileNumber);

#endif
