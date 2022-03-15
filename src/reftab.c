#include "proto.h"
#define IN_REFTAB_C
#include "reftab.h"


#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */


ReferenceTable referenceTable;

static bool equalReferenceItems(SymbolReferenceItem *e1, SymbolReferenceItem *e2) {
    return e1->bits.symType==e2->bits.symType
        && e1->bits.storage==e2->bits.storage
        && e1->bits.category==e2->bits.category
        && e1->vApplClass==e2->vApplClass
        && strcmp(e1->name, e2->name)==0;
}


#define HASH_FUN(elemp) (hashFun(elemp->name) + (unsigned)elemp->vFunClass)
#define HASH_ELEM_EQUAL(e1,e2) equalReferenceItems(e1, e2)

#include "hashlist.tc"
