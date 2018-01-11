#define _MATAB_
#include "matab.h"

#include "misc.h"
#include "commons.h"

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"

struct maTab s_maTab;
