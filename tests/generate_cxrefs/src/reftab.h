#ifndef REFTAB_H_INCLUDED
#define REFTAB_H_INCLUDED

#include "proto.h"

#define HASH_TAB_NAME refTab
#define HASH_TAB_TYPE ReferenceTable
#define HASH_ELEM_TYPE ReferenceItem

#include "hashlist.th"


#ifndef IN_REFTAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern void initReferenceTable(int size);
extern int  addToReferencesTable(ReferenceItem *referencesItem);
extern void pushReferenceItem(ReferenceItem *element, int position);
extern ReferenceItem *getReferenceItem(int index);
extern int getNextExistingReferenceItem(int index);
extern void setReferenceItem(int index, ReferenceItem *item);

extern bool isMemberInReferenceTable(ReferenceItem *item, int *indexP, ReferenceItem **originP);
extern void mapOverReferenceTable(void (*fun)(ReferenceItem *));
extern void mapOverReferenceTableWithIndex(void (*fun)(int));
extern void mapOverReferenceTableWithPointer(void (*fun)(ReferenceItem *, void *), void *pointer);

#endif
