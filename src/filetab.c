#define _FILETAB_
#include "filetab.h"

#include "commons.h"
#include "hash.h"
#include "memory.h"


#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"


int s_noneFileIndex = -1;


void initFileTab(S_fileTab *s_fileTab) {
    int len;
    char *ff;
    struct fileItem *ffii;

    SM_INIT(ftMemory);
    FT_ALLOCC(s_fileTab->tab, MAX_FILES, struct fileItem *);

    fileTabNoAllocInit(s_fileTab, MAX_FILES);

    /* Create a "NON_FILE" in FT memory */
    len = strlen(NON_FILE_NAME);
    FT_ALLOCC(ff, len+1, char);
    strcpy(ff, NON_FILE_NAME);
    FT_ALLOC(ffii, S_fileItem);
    fillFileItem(ffii, ff, false);

    /* Add it to the fileTab and remember its index for future use */
    s_noneFileIndex = fileTabAdd(s_fileTab, ffii);
}


/* This is searching for filenams as strings, not fileItems, which
   fileTabIsMember() does. It can't be made into a hashtab macro since
   it knows about how filenames are stored and compared. So there is
   some duplication here, since this is also looking up things. */
int fileTabLookup(S_fileTab *table, char *fileName) {
    unsigned posid;

    posid = hashFun(fileName);
    posid = posid % table->size;
    assert(table->tab!=NULL);
    while (table->tab[posid] != NULL) {
        if (strcmp(table->tab[posid]->name, fileName) == 0) {
            return posid;
        }
        posid=(posid+HASH_SHIFT) % table->size;
    }
    return -1;                  /* Not found */
}

bool fileTabExists(S_fileTab *table, char *fileName) {
    return fileTabLookup(table, fileName) != -1;
}
