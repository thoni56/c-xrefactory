#define _FILETAB_
#include "filetab.h"

#include "commons.h"
#include "hash.h"

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"
