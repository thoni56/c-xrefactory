#define _IN_FILETAB_C_
#include "filetable.h"

#include "commons.h"            /* For fatalError() */
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


FileTable fileTable;

int noFileIndex = -1;


void fillFileItem(FileItem *item, char *name, bool fromCommandLine) {
    memset(item, 0, sizeof(FileItem));
    item->name = name;
    item->b.commandLineEntered = fromCommandLine;
    item->directEnclosingInstance = noFileIndex;
    item->b.sourceFileNumber = noFileIndex;
}

void initFileTable(FileTable *fileTable) {
    int len;
    char *fileName;
    FileItem *fileItem;

    SM_INIT(ftMemory);
    FT_ALLOCC(fileTable->tab, MAX_FILES, struct fileItem *);

    fileTableNoAllocInit(fileTable, MAX_FILES);

    /* Create a "NON_FILE" in FT memory */
    len = strlen(NO_FILE_NAME);
    FT_ALLOCC(fileName, len+1, char);
    strcpy(fileName, NO_FILE_NAME);
    FT_ALLOC(fileItem, FileItem);
    fillFileItem(fileItem, fileName, false);

    /* Add it to the fileTab and remember its index for future use */
    noFileIndex = fileTableAdd(fileTable, fileItem);
}


/* This is searching for filenames as strings, not fileItems, which
   fileTabIsMember() does. It can't be made into a hashtab macro since
   it knows about how filenames are stored and compared. So there is
   some duplication here, since this is also looking up things. */
int fileTableLookup(FileTable *table, char *fileName) {
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

bool fileTableExists(FileTable *table, char *fileName) {
    return fileTableLookup(table, fileName) != -1;
}
