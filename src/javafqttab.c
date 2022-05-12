#define IN_JAVAFQTTAB_C
#include "javafqttab.h"

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->d->linkName)
#define HASH_ELEM_EQUAL(e1,e2) (                                        \
        e1->d->bits.type==e2->d->bits.type                        \
        && strcmp(e1->d->linkName,e2->d->linkName)==0                   \
    )

JavaFqtTable javaFqtTable;

#include "hashlist.tc"
