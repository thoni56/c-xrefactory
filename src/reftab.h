#ifndef REFTAB_H_INCLUDED
#define REFTAB_H_INCLUDED

#include "proto.h"

#define HASH_TAB_NAME refTab
#define HASH_TAB_TYPE ReferenceTable
#define HASH_ELEM_TYPE ReferencesItem

#include "hashlist.th"


extern ReferenceTable referenceTable;

#ifndef IN_REFTAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

extern void initReferenceTable(int size);
extern int addToReferencesTable(ReferencesItem *referencesItem);
extern ReferencesItem *getReferencesItem(int index);
extern int getNextExistingReferencesItem(int index);
extern void setReferencesItem(int index, ReferencesItem *item);

extern bool isMemberInReferenceTable(ReferencesItem *item, int *indexP, ReferencesItem **originP);
extern void mapOverReferenceTable(void (*fun)(ReferencesItem *));
extern void mapOverReferenceTableWithIndex(void (*fun)(int));
extern void mapOverReferenceTableWithPointer(void (*fun)(ReferencesItem *, void *), void *pointer);

#endif
