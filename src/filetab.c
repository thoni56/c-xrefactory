#define _FILETAB_
#include "filetab.h"

#include "commons.h"
#include "hash.h"

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

static unsigned fileTabHashFun(char *ss) {
    register unsigned h = 0;
    register char *s = ss;
    register char c;
    for(c= *s; c ; c= *++s) SYMTAB_HASH_FUN_INC(h, c);
    SYMTAB_HASH_FUN_FINAL(h);
    return(h);
}

#include "hashtab.tc"
