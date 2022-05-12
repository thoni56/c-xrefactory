#define IN_JSLTYPETAB_C
#include "jsltypetab.h"

#define HASH_FUN(elemp) hashFun(elemp->d->name)
#define HASH_ELEM_EQUAL(e1,e2) (                                        \
        e1->d->bits.type==e2->d->bits.type                        \
        && strcmp(e1->d->name,e2->d->name)==0                           \
    )

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#include "hashlist.tc"
