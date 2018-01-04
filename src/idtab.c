#include "idtab.h"

#include "commons.h"
#include "misc.h"

#define HASH_TAB_NAME idTab
#define HASH_TAB_TYPE struct idTab
#define HASH_ELEM_TYPE S_fileItem
#define HASH_FUN_PREFIX idTab
#define HASH_FUN(elemp) fileTabHashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

HASH_TAB_TYPE s_fileTab;

static unsigned fileTabHashFun(char *ss) {
    register unsigned h = 0;
    register char *s = ss;
    register char c;
    for(c= *s; c ; c= *++s) SYM_TAB_HASH_FUN_INC(h, c);
    SYM_TAB_HASH_FUN_FINAL(h);
    return(h);
}

#include "hashtab.tc"
