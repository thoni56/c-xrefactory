#include "matab.h"

#include "misc.h"
#include "commons.h"

#define HASH_TAB_TYPE struct maTab
#define HASH_ELEM_TYPE S_macroArgTabElem
#define HASH_FUN_PREFIX maTab
#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX
#undef HASH_FUN
#undef HASH_ELEM_EQUAL

struct maTab s_maTab;
