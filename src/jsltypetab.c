#define _JSLTYPETAB_
#include "jsltypetab.h"

#define HASH_FUN(elemp) hashFun(elemp->d->name)
#define HASH_ELEM_EQUAL(e1,e2) (                                        \
                                e1->d->b.symType==e2->d->b.symType      \
                                && strcmp(e1->d->name,e2->d->name)==0   \
                                )

#include "misc.h"

#include "hashlist.tc"
