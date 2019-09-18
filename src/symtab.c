#define _SYMTAB_
#include "symtab.h"

#include "misc.h"

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (e1->bits.symType==e2->bits.symType && strcmp(e1->name,e2->name)==0)

#include "hashlist.tc"
