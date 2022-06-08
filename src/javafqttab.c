#define IN_JAVAFQTTAB_C
#include "javafqttab.h"

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->element->linkName)
#define HASH_ELEM_EQUAL(e1,e2) (                                        \
        e1->element->type==e2->element->type                                        \
        && strcmp(e1->element->linkName,e2->element->linkName)==0                   \
    )

JavaFqtTable javaFqtTable;

#include "hashlist.tc"
