#define _MACROARGUMENTTABLE_
#include "macroargumenttable.h"

#include "hash.h"
#include "commons.h"

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"

S_macroArgumentTable s_macroArgumentTable;
