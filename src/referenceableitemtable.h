#ifndef REFTAB_H_INCLUDED
#define REFTAB_H_INCLUDED

#include "referenceableitem.h"

#define HASH_TAB_NAME referenceableItemTable
#define HASH_TAB_TYPE ReferenceableItemTable
#define HASH_ELEM_TYPE ReferenceableItem

#include "hashlist.th"


#ifndef IN_REFTAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern void initReferenceableItemTable(int size);
extern int  addToReferenceableItemTable(ReferenceableItem *referencesItem);
extern void pushReferenceableItem(ReferenceableItem *element, int position);
extern ReferenceableItem *getReferenceableItem(int index);
extern int getNextExistingReferenceableItem(int index);
extern void setReferenceableItem(int index, ReferenceableItem *item);

extern bool isMemberInReferenceableItemTable(ReferenceableItem *item, int *indexP, ReferenceableItem **originP);
extern void mapOverReferenceableItemTable(void (*fun)(ReferenceableItem *));
extern void mapOverReferenceableItemTableWithIndex(void (*fun)(int));
extern void mapOverReferenceableItemTableWithPointer(void (*fun)(ReferenceableItem *, void *), void *pointer);
extern void removeReferenceableItemsForFile(int fileNumber);

#endif
