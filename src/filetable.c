#include "commons.h"            /* For fatalError() */
#include "hash.h"
#include "memory.h"
#include "globals.h"            /* For cwd */
#include "caching.h"            /* For checkModifiedTime() */

#define IN_FILETAB_C
#include "filetable.h"

#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"


FileTable fileTable;

int noFileIndex;                /* Initialized to an actual index in initFileTable() which needs to be called early */


/* TODO: Always called (2 locations) with FT memory allocated space, so newFileItem() is possible */
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

int addFileTabItem(char *name) {
    int fileIndex, len;
    char *fname, *normalizedFileName;
    struct fileItem *createdFileItem;

    /* Create a fileItem on the stack, with a static normalizedFileName, returned by normalizeFileName() */
    normalizedFileName = normalizeFileName(name, cwd);

    /* Does it already exist? */
    if (fileTableExists(&fileTable, normalizedFileName))
        return fileTableLookup(&fileTable, normalizedFileName);

    /* If not, add it, but then we need a filename and a fileitem in FT-memory  */
    len = strlen(normalizedFileName);
    FT_ALLOCC(fname, len+1, char);
    strcpy(fname, normalizedFileName);

    FT_ALLOC(createdFileItem, FileItem);
    fillFileItem(createdFileItem, fname, false);

    fileIndex = fileTableAdd(&fileTable, createdFileItem);
    checkFileModifiedTime(fileIndex); // it was too slow on load ?

    return fileIndex;
}
