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

extern void initReferenceTable(void);


#endif
