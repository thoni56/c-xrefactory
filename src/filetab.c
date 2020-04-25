#define _FILETAB_
#include "filetab.h"

#include "commons.h"
#include "hash.h"
#include "memory.h"

/* FILETAB:

   The .tab field is an array of file items. A file "number" is an
   index into this array. Since the file items have the filename as a
   field, all you need to find a filename from a "number" is

   <filetab>.tab[<fileno>]->name

 */

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"


int s_noneFileIndex = -1;


void initFileTab(S_fileTab *fileTable) {
    int len;
    char *fileName;
    struct fileItem *fileItem;

    SM_INIT(ftMemory);
    FT_ALLOCC(fileTable->tab, MAX_FILES, struct fileItem *);

    fileTabNoAllocInit(fileTable, MAX_FILES);

    /* Create a "NON_FILE" in FT memory */
    len = strlen(NON_FILE_NAME);
    FT_ALLOCC(fileName, len+1, char);
    strcpy(fileName, NON_FILE_NAME);
    FT_ALLOC(fileItem, S_fileItem);
    fillFileItem(fileItem, fileName, false);

    /* Add it to the fileTab and remember its index for future use */
    s_noneFileIndex = fileTabAdd(fileTable, fileItem);
}


/* This is searching for filenames as strings, not fileItems, which
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
