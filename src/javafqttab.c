#define _JAVAFQTTAB_
#include "javafqttab.h"

#include "misc.h"

#define HASH_FUN(elemp) hashFun(elemp->d->linkName)
#define HASH_ELEM_EQUAL(e1,e2) (                                        \
                                e1->d->bits.symType==e2->d->bits.symType      \
                                && strcmp(e1->d->linkName,e2->d->linkName)==0 \
                                )

#include "hashlist.tc"
