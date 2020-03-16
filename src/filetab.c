#define _FILETAB_
#include "filetab.h"

#include "commons.h"
#include "hash.h"


#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"

bool fileTabExists(S_fileTab *table, char *fileName) {
    unsigned posid;

    posid = hashFun(fileName);
    posid = posid % table->size;
    assert(table->tab!=NULL);
    while (table->tab[posid] != NULL) {
        if (strcmp(table->tab[posid]->name, fileName) == 0) {
            return true;
        }
        posid=(posid+HASH_SHIFT) % table->size;
    }
    return false;
}
