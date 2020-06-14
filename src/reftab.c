#define _REFTAB_
#include "reftab.h"

// following can't depend on vApplClass, because of finding def in html.c
#define HASH_FUN(elemp) (hashFun(elemp->name) + (unsigned)elemp->vFunClass)
#define HASH_ELEM_EQUAL(e1,e2) REF_ELEM_EQUAL(e1,e2)

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#include "hashlist-new.tc"

ReferenceTable s_cxrefTab;
