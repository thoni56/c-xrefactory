#define IN_OLCXTAB_C
#include "olcxtab.h"

#include "hash.h"

#include "memory.h"               /* For XX_ALLOCC */

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashlist.tc"

UserOlcxData *s_olcxCurrentUser;
OlcxTab s_olcxTab;
