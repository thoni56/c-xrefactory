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


int noFileIndex;                /* Initialized to an actual index in initNoFile() which needs
                                   to be called early */

static FileTable fileTable;

static void fillFileItem(FileItem *item, char *name) {
    memset(item, 0, sizeof(FileItem));
    item->name = name;
    item->b.commandLineEntered = false;
    item->directEnclosingInstance = noFileIndex;
    item->b.sourceFileNumber = noFileIndex;
}


static struct fileItem *newFileItem(char *normalizedFileName) {
    int              len;
    char *           fname;
    struct fileItem *createdFileItem;
    len = strlen(normalizedFileName);
    FT_ALLOCC(fname, len + 1, char);
    strcpy(fname, normalizedFileName);

    FT_ALLOC(createdFileItem, FileItem);
    fillFileItem(createdFileItem, fname);

    return createdFileItem;
}

int addToFileTable(FileItem *fileItem) {
    return fileTableAdd(&fileTable, fileItem);
}

FileItem *getFileItem(int fileIndex) {
    assert(fileIndex != -1);
    assert(fileTable.tab != NULL);
    assert(fileTable.tab[fileIndex] != NULL);
    return fileTable.tab[fileIndex];
}

int getNextExistingFileIndex(int index) {
    for (int i=index; i < fileTable.size; i++)
        if (fileTable.tab[i] != NULL)
            return i;
    return -1;
}


void initFileTable(void) {
    SM_INIT(ftMemory);
    FT_ALLOCC(fileTable.tab, MAX_FILES, struct fileItem *);

    fileTableNoAllocInit(&fileTable, MAX_FILES);
}

void initNoFile(void) {
    FileItem *fileItem = newFileItem(NO_FILE_NAME);

    /* Add it to the fileTab and remember its index for future use */
    noFileIndex = fileTableAdd(&fileTable, fileItem);
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

bool existsInFileTable(char *fileName) {
    return fileTableLookup(&fileTable, fileName) != -1;
}

bool isMemberInFileTable(FileItem *fileItem, int *index) {
    return fileTableIsMember(&fileTable, fileItem, index);
}


int addFileNameToFileTable(char *name) {
    int fileIndex;
    char *normalizedFileName;
    struct fileItem *createdFileItem;

    /* Create a fileItem on the stack, with a static normalizedFileName, returned by normalizeFileName() */
    normalizedFileName = normalizeFileName(name, cwd);

    /* Does it already exist? */
    if (existsInFileTable(normalizedFileName))
        return fileTableLookup(&fileTable, normalizedFileName);

    /* If not, add it, but then we need a filename and a fileitem in FT-memory  */
    createdFileItem = newFileItem(normalizedFileName);

    fileIndex = fileTableAdd(&fileTable, createdFileItem);
    checkFileModifiedTime(fileIndex); // it was too slow on load ?

    return fileIndex;
}

/* "Override" some hashtab functions to hide filetable variable from being global */
void mapOverFileTable(void (*fun)(FileItem *)) {
    fileTableMap(&fileTable, fun);
}

void mapOverFileTableWithIndex(void (*fun)(FileItem *, int)) {
    fileTableMapWithIndex(&fileTable, fun);
}

void mapOverFileTableWithPointer(void (*fun)(FileItem *, void *), void *pointer) {
    fileTableMap2(&fileTable, fun, pointer);
}

int lookupFileTable(char *fileName) {
    return fileTableLookup(&fileTable, fileName);
}
