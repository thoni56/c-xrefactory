#include "commons.h"            /* For fatalError() */
#include "hash.h"
#include "memory.h"
#include "globals.h"            /* For cwd */
#include "caching.h"            /* For checkFileModifiedTime() */

#include "filetable.h"

/* Define the hashtab: */
#define HASH_TAB_NAME fileTable
#define HASH_TAB_TYPE FileTable
#define HASH_ELEM_TYPE FileItem

#include "hashtab.th"

/* Define hashtab functions: */
#define HASH_FUN(elemp) hashFun(elemp->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"


/* Filetable allocations */
static Memory2 fileTableMemory;

static void ftInit(void) {
    smInit(&fileTableMemory, "filetable", FileTableMemorySize);
}

static void *ftAlloc(size_t size) {
    return smAlloc(&fileTableMemory, size);
}


int noFileIndex;                /* Initialized to an actual index in initNoFile() which needs
                                   to be called early */

static FileTable fileTable;

static void fillFileItem(FileItem *item, char *name) {
    memset(item, 0, sizeof(FileItem));
    item->name = name;
    item->isArgument = false;
    item->directEnclosingInstance = noFileIndex;
    item->sourceFileNumber = noFileIndex;
}


static struct fileItem *newFileItem(char *normalizedFileName) {
    int              len;
    char            *fname;
    FileItem *createdFileItem;

    len = strlen(normalizedFileName);
    fname = ftAlloc(len + 1);
    strcpy(fname, normalizedFileName);

    createdFileItem = ftAlloc(sizeof(FileItem));
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


void initFileTable(int count) {
    ftInit();
    fileTable.tab = ftAlloc(count*sizeof(FileItem *));
    fileTableNoAllocInit(&fileTable, count);
}

void initNoFileIndex(void) {
    FileItem *fileItem = newFileItem(NO_FILE_NAME);

    /* Add it to the fileTab and remember its index for future use */
    noFileIndex = fileTableAdd(&fileTable, fileItem);
}


/* This is searching for filenames as strings, not fileItems, which
   fileTabIsMember() does. It can't be made into a hashtab macro since
   it knows about how filenames are stored and compared. So there is
   some duplication here, since this is also looking up things. */
static int fileTableLookup(FileTable *table, char *fileName) {
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
    fileTableMapWithPointer(&fileTable, fun, pointer);
}

int lookupFileTable(char *fileName) {
    return fileTableLookup(&fileTable, fileName);
}

static char *getNextInputFileFromFileTable(int *indexP, FileSource wantedFileSource) {
    int         i;
    FileItem  *fileItem;

    for (i = getNextExistingFileIndex(*indexP); i != -1; i = getNextExistingFileIndex(i+1)) {
        fileItem = getFileItem(i);
        assert(fileItem!=NULL);
        if (wantedFileSource==FILE_IS_SCHEDULED && fileItem->isScheduled)
            break;
        if (wantedFileSource==FILE_IS_ARGUMENT && fileItem->isArgument)
            break;
    }
    *indexP = i;
    if (i != -1)
        return fileItem->name;
    else
        return NULL;
}

char *getNextScheduledFile(int *indexP) {
    return getNextInputFileFromFileTable(indexP, FILE_IS_SCHEDULED);
}

char *getNextArgumentFile(int *indexP) {
    return getNextInputFileFromFileTable(indexP, FILE_IS_ARGUMENT);
}
