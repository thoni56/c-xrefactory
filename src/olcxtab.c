#define _OLCXTAB_
#include "olcxtab.h"

#include "misc.h"

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashlist.tc"

S_olcxTab s_olcxTab;
