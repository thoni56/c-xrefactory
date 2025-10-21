#include "filetable.h"

#include "commons.h"            /* For fatalError() */
#include "hash.h"
#include "memory.h"
#include "globals.h"            /* For cwd */
#include "caching.h"


/* Define the hashtab: */
#define HASH_TAB_NAME fileTable
#define HASH_TAB_TYPE FileTable
#define HASH_ELEM_TYPE FileItem

#include "hashtab.th"

/* Define hashtab functions: */
#define HASH_FUN(element) hashFun(element->name)
#define HASH_ELEM_EQUAL(e1,e2) (strcmp(e1->name,e2->name)==0)

#include "hashtab.tc"


/* Filetable allocations */
static Memory fileTableMemory;

static void ftInit(void) {
    memoryInit(&fileTableMemory, "filetable", NULL, FileTableMemorySize);
}

static void *ftAlloc(size_t size) {
    return memoryAlloc(&fileTableMemory, size);
}


int NO_FILE_NUMBER;                /* Initialized to an actual index in initNoFile() which needs
                                   to be called early */

static FileTable fileTable;

static void fillFileItem(FileItem *item, char *name) {
    memset(item, 0, sizeof(FileItem));
    item->name = name;
    item->isArgument = false;
    item->sourceFileNumber = NO_FILE_NUMBER;
}


static FileItem *newFileItem(char *normalizedFileName) {
    char *fname;
    FileItem *createdFileItem;

    fname = ftAlloc(strlen(normalizedFileName) + 1);
    strcpy(fname, normalizedFileName);

    createdFileItem = ftAlloc(sizeof(FileItem));
    fillFileItem(createdFileItem, fname);

    return createdFileItem;
}

int addFileItemToFileTable(FileItem *fileItem) {
    return fileTableAdd(&fileTable, fileItem);
}

FileItem *getFileItemWithFileNumber(int fileNumber) {
    assert(fileNumber != -1);
    assert(fileTable.tab != NULL);
    assert(fileTable.tab[fileNumber] != NULL);
    return fileTable.tab[fileNumber];
}

int getNextExistingFileNumber(int index) {
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

void initNoFileNumber(void) {
    FileItem *fileItem = newFileItem(NO_FILE_NAME);

    /* Add it to the fileTab and remember its index for future use */
    NO_FILE_NUMBER = fileTableAdd(&fileTable, fileItem);
}


/* Search for filenames as strings, not fileItems, which
   fileTabIsMember() does.

   @returns index into FileTable

   It can't be made into a hashtab macro since
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

int addFileNameToFileTable(char *fileName) {
    int fileNumber;
    char *normalizedFileName;
    struct fileItem *createdFileItem;

    /* Create a fileItem on the stack, with a static normalizedFileName, returned by normalizeFileName() */
    normalizedFileName = normalizeFileName_static(fileName, cwd);

    /* Does it already exist? */
    if (existsInFileTable(normalizedFileName))
        return fileTableLookup(&fileTable, normalizedFileName);

    /* If not, add it, but then we need a filename and a fileitem in FT-memory  */
    createdFileItem = newFileItem(normalizedFileName);

    fileNumber = fileTableAdd(&fileTable, createdFileItem);
    updateFileModificationTracking(fileNumber); // Initialize file tracking

    return fileNumber;
}

/* "Override" some hashtab functions to hide filetable variable from being global */
void mapOverFileTable(void (*fun)(FileItem *)) {
    fileTableMap(&fileTable, fun);
}

void mapOverFileTableWithIndex(void (*fun)(FileItem *, int)) {
    fileTableMapWithIndex(&fileTable, fun);
}

void mapOverFileTableWithBool(void (*fun)(FileItem *, bool), bool boolean) {
    fileTableMapWithBool(&fileTable, fun, boolean);
}

void mapOverFileTableWithPointer(void (*fun)(FileItem *, void *), void *pointer) {
    fileTableMapWithPointer(&fileTable, fun, pointer);
}

int getFileNumberFromFileName(char *fileName) {
    return fileTableLookup(&fileTable, fileName);
}

static char *getNextInputFileFromFileTable(int *indexP, FileSource wantedFileSource) {
    int         i;
    FileItem  *fileItem;

    for (i = getNextExistingFileNumber(*indexP); i != -1; i = getNextExistingFileNumber(i+1)) {
        fileItem = getFileItemWithFileNumber(i);
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

// NO-Op until we figure out how to do it...
static void recoverMemoryFromFileTableEntry(FileItem *fileItem) {
}

void recoverMemoryFromFileTable(void) {
    mapOverFileTable(recoverMemoryFromFileTableEntry);
}

void fileTableMemoryStatistics(void) {
    printMemoryStatisticsFor(&fileTableMemory);
}
