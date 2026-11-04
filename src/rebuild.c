#include "rebuild.h"

#include <stdlib.h>
#include <string.h>          /* strdup */

#include "characterreader.h" /* closeCharacterBuffer */
#include "commons.h"         /* silenceMessages, resumeMessages */
#include "cxref.h"
#include "editor.h"
#include "editorbuffertable.h"
#include "filedescriptor.h"  /* currentFile */
#include "filetable.h"
#include "globals.h"         /* inputFileName, currentPass, maxPasses */
#include "log.h"
#include "misc.h"
#include "navigation.h"
#include "options.h"
#include "parsing.h"         /* parseToCreateReferences */
#include "progress.h"
#include "referenceableitem.h"
#include "referenceableitemtable.h"
#include "startup.h"         /* initializeFileProcessing */
#include "timestamp.h"       /* fileTimestampNow, if it doesn't come via another header */


/* The CUs to build, at most capacity of them */
struct buildList {
    int  *fileNumbers;
    int   capacity;
    int   count;
    int   skippedCapped; /* Out of date, but did not fit */
    bool *built;         /* By file number, on the list or parsed by the operation */
    bool  stripRequest;  /* The operation parses the request CU, from scratch */
};

BuildList *newBuildList(int capacity) {
    BuildList *list = calloc(1, sizeof(BuildList));
    list->fileNumbers = malloc(capacity * sizeof(int));
    list->capacity = capacity;
    list->built = calloc(MAX_FILES, sizeof(bool));
    return list;
}

void freeBuildList(BuildList *list) {
    free(list->fileNumbers);
    free(list->built);
    free(list);
}

bool addToBuildList(BuildList *list, int fileNumber) {
    if (list->built[fileNumber])
        return true;
    if (list->count < list->capacity) {
        list->fileNumbers[list->count++] = fileNumber;
        list->built[fileNumber] = true;
        return true;
    }
    list->skippedCapped++;
    return false;
}

static void setupProgress(BuildList *list) {
    static char progressFormat[128];
    int totalFound = list->count + list->skippedCapped;
    if (list->skippedCapped > 0)
        snprintf(progressFormat, sizeof(progressFormat),
                 "Analysing %d of %d files... %%d remaining", list->count, totalFound);
    else
        snprintf(progressFormat, sizeof(progressFormat),
                 "Analysing %d files... %%d remaining", list->count);
    initProgress(progressFormat);
}

/* The request is parsed by the operation itself, not here */
void collectRequestCUForStripping(BuildList *list, Inputs *inputs, int requestFileNumber) {
    if (isCompilationUnit(getFileItemWithFileNumber(requestFileNumber)->name)
        && knowledgeIsOutOfDate(inputs, requestFileNumber)) {
        list->built[requestFileNumber] = true;
        list->stripRequest = true;
    }
}

/* An operation that parses nothing has its request built like any other CU (#91) */
void collectRequestCUToBuild(BuildList *list, Inputs *inputs, int requestFileNumber) {
    if (isCompilationUnit(getFileItemWithFileNumber(requestFileNumber)->name)
        && knowledgeIsOutOfDate(inputs, requestFileNumber))
        addToBuildList(list, requestFileNumber);
}

/* Only after the collecting, since the request's include references lead to its
 * siblings. Never for a header: its references were put there by its includers,
 * and parsing it on its own cannot put them all back. */
void stripRequestCU(BuildList *list, int requestFileNumber) {
    if (list->stripRequest) {
        log_debug("file has changed content, removing old references for file %d",
                  requestFileNumber);
        removeReferenceableItemsForFile(requestFileNumber);
    }
}

/* The siblings of the request: the CUs that reach the request file or a header it
 * includes, at any depth, also through a file that is not a CU, such as a mock or
 * another header. Those out of date go on the build list. */
void collectSiblingCUs(BuildList *list, Inputs *inputs, int requestFileNumber) {
    log_info("Siblings of '%s' (fileNumber=%d)",
             getFileItemWithFileNumber(requestFileNumber)->name, requestFileNumber);

    bool *asked = calloc(MAX_FILES, sizeof(bool)); /* A sibling shares many headers */

    /* The request file first, since browsing can start in a header, and then the
     * headers it includes */
    int *headers = malloc((MAX_FILES + 1) * sizeof(int));
    headers[0] = requestFileNumber;
    int headerCount = 1 + collectIncludeClosureOf(inputs, requestFileNumber, &headers[1], MAX_FILES);
    int skippedCurrent = 0;

    int *siblingCUs = malloc(MAX_FILES * sizeof(int));
    for (int h = 0; h < headerCount; h++) {
        int headerIndex = headers[h];
        FileItem *headerFileItem = getFileItemWithFileNumber(headerIndex);
        if (isCompilationUnit(headerFileItem->name))
            continue; /* Skip CUs, only look at headers */

        int siblingCount = collectCUsIncluding(headerIndex, siblingCUs, 0, MAX_FILES);
        for (int j = 0; j < siblingCount; j++) {
            int siblingFileNum = siblingCUs[j];
            if (siblingFileNum == requestFileNumber)
                continue;

            FileItem *siblingItem = getFileItemWithFileNumber(siblingFileNum);
            if (asked[siblingFileNum])
                continue;
            asked[siblingFileNum] = true;
            if (list->built[siblingFileNum])
                continue; /* Held, and on the list already */
            if (!knowledgeIsOutOfDate(inputs, siblingFileNum)) {
                skippedCurrent++;
                continue;
            }

            if (addToBuildList(list, siblingFileNum))
                log_debug("Sibling CU '%s' shares header with request file", siblingItem->name);
        }
    }

    free(siblingCUs);
    free(headers);
    free(asked);

    log_info("Siblings: %d to build, %d skipped (current), %d skipped (cap %d)", list->count,
             skippedCurrent, list->skippedCapped, list->capacity);
}

/* For an operation on a name: every CU in the project that is out of date */
void collectOutOfDateCUs(BuildList *list, Inputs *inputs) {
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (isCompilationUnit(fileItem->name) && !fileItem->isDeleted && knowledgeIsOutOfDate(inputs, i))
            addToBuildList(list, i);
    }
}

void markAllCompilationUnitsStale(void) {
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (isCompilationUnit(fileItem->name)) {
            fileItem->knowledgeTime = NULL_TIMESTAMP;
        }
    }
}

/* A header that includes a changed one changes with it, since what it emits
 * depends on what it includes. Every CU that reaches it also reaches the
 * changed one, so no other CU becomes out of date (ADR-0035). The graph was
 * collected before anything was removed. */
static void removeReferencesOfIncludersOfChanged(Inputs *inputs, int headers[], FileItem *items[],
                                                 int headerCount, FileTimestamp changedAt[]) {
    int *closure = malloc(MAX_FILES * sizeof(int));
    for (int h = 0; h < headerCount; h++) {
        if (!fileTimestampIsZero(changedAt[headers[h]]))
            continue;
        int closureCount = collectIncludeClosureOf(inputs, headers[h], closure, MAX_FILES);
        for (int c = 0; c < closureCount && c < MAX_FILES; c++) {
            if (!fileTimestampIsZero(changedAt[closure[c]])) {
                log_debug("'%s' includes a changed header, removing its references", items[h]->name);
                removeReferenceableItemsForFile(headers[h]);
                if (fileTimestampIsLessThan(items[h]->changeTime, changedAt[closure[c]]))
                    items[h]->changeTime = changedAt[closure[c]];
                break;
            }
        }
    }
    free(closure);
}

/* A CU's own references are removed when it is built, and its include
 * references lead to its siblings until then, so only headers here. A header
 * seen for the first time has its change time recorded, and keeps what the
 * table has. */
void removeReferencesOfChangedFiles(Inputs *inputs) {
    int *headers = malloc(MAX_FILES * sizeof(int));
    FileItem **items = malloc(MAX_FILES * sizeof(FileItem *));
    FileTimestamp *changedAt = calloc(MAX_FILES, sizeof(FileTimestamp)); /* Zero if not */
    int headerCount = 0;
    bool anyChanged = false;

    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (isCompilationUnit(fileItem->name))
            continue;
        headers[headerCount] = i;
        items[headerCount++] = fileItem;
        FileTimestamp contentTime = editorFileModificationTime(fileItem->name);
        if (fileTimestampIsZero(fileItem->changeTime)) {
            fileItem->changeTime = contentTime;
        } else if (fileTimestampIsLessThan(fileItem->changeTime, contentTime)) {
            log_debug("'%s' changed, removing its references", fileItem->name);
            removeReferenceableItemsForFile(i);
            fileItem->needsBrowsingStackRefresh = true;
            fileItem->changeTime = contentTime;
            changedAt[i] = contentTime;
            anyChanged = true;
        }
    }

    if (anyChanged)
        removeReferencesOfIncludersOfChanged(inputs, headers, items, headerCount, changedAt);

    free(changedAt);
    free(items);
    free(headers);
}

/* The server reads the disk again, so the content changes now, although the
 * file on disk is older than the knowledge built from the buffer (ADR-0035) */
void letGoOfHeldHeader(int fileNumber) {
    removeReferenceableItemsForFile(fileNumber);
    getFileItemWithFileNumber(fileNumber)->changeTime = changeTimeNow();
}

int buildListCount(BuildList *list) {
    return list->count;
}

#define MAX_INCLUDE_WALK_FILES 256
/* Walk the reverse-include graph from a file up to compilation units,
 * using TypeCppInclude references in the reference table (recorded by parsing
 * or by the lightweight scan). Collect CU file numbers into the provided
 * array, deduplicating against entries already present. */
int collectCUsIncluding(int fileNumber, int cuFileNumbers[], int cuCount, int maxCUs) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    log_debug("Looking for CUs that include '%s'", fileItem->name);

    /* Walk reverse-include graph transitively: starting from the file,
     * find all files that include it, then files that include those, etc.
     * Collect any CUs encountered along the way. */
    int filesToWalk[MAX_INCLUDE_WALK_FILES];
    int walkCount = 1;
    filesToWalk[0] = fileNumber;

    for (int i = 0; i < walkCount; i++) {
        ReferenceableItem searchItem = makeReferenceableItem(
            LINK_NAME_INCLUDE_REFS, TypeCppInclude, StorageExtern, VisibilityGlobal, filesToWalk[i]);

        ReferenceableItem *found;
        if (!isMemberInReferenceableItemTable(&searchItem, NULL, &found))
            continue;

        for (Reference *r = found->references; r != NULL; r = r->next) {
            int includerFileNum = r->position.file;

            /* Already in our walk list? */
            bool alreadySeen = false;
            for (int j = 0; j < walkCount; j++) {
                if (filesToWalk[j] == includerFileNum) {
                    alreadySeen = true;
                    break;
                }
            }
            if (alreadySeen)
                continue;

            if (walkCount < MAX_INCLUDE_WALK_FILES)
                filesToWalk[walkCount++] = includerFileNum;

            FileItem *includer = getFileItemWithFileNumber(includerFileNum);
            if (isCompilationUnit(includer->name)) {
                /* Deduplicate against CUs already collected (from other files) */
                bool alreadyCollected = false;
                for (int j = 0; j < cuCount; j++) {
                    if (cuFileNumbers[j] == includerFileNum) {
                        alreadyCollected = true;
                        break;
                    }
                }
                if (!alreadyCollected && cuCount < maxCUs) {
                    cuFileNumbers[cuCount++] = includerFileNum;
                    log_debug("CU '%s' (transitively) includes '%s'", includer->name, fileItem->name);
                }
            }
        }
    }

    if (cuCount == 0)
        log_debug("No CUs found that include '%s'", fileItem->name);

    return cuCount;
}

/* Only a CU whose content the client holds */
static bool heldCUIsOutOfDate(Inputs *inputs, EditorBuffer *buffer) {
    if (!holdsAuthoritativeContent(buffer))
        return false;
    FileItem *fileItem = getFileItemWithFileNumber(buffer->fileNumber);
    return !fileItem->isDeleted && isCompilationUnit(fileItem->name)
           && knowledgeIsOutOfDate(inputs, buffer->fileNumber);
}

/* Held CUs whose knowledge is out of date. They go first, so the cap never
 * leaves one of them out. */
void collectOutOfDateHeldCUs(BuildList *list, Inputs *inputs) {
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
            int fileNumber = l->buffer->fileNumber;
            FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
            if (heldCUIsOutOfDate(inputs, l->buffer)) {
                if (!editorFileExists(fileItem->name)) {
                    log_debug("Out-of-date CU '%s' no longer exists, marking as deleted",
                              fileItem->name);
                    markFileAsDeleted(fileNumber);
                } else {
                    addToBuildList(list, fileNumber);
                }
            }
        }
    }
}

/* Parse a file using the full initializeFileProcessing machinery (project
 * discovery, options, checkpoint) with multi-pass support.  Saves and
 * restores REQUEST-level option fields that initOptions() inside
 * initializeFileProcessing would otherwise wipe. */
static bool parseFileWithFullInit(char *fileName, ArgumentsVector baseArgs) {
    bool parsed = false;
    int savedCursorOffset = options.cursorOffset;
    int savedIndex = options.index;
    ServerOperation savedServerOperation = options.serverOperation;
    /* A string option lives in the options' own memory, which the reset frees, so it is
     * copied out and allocated again. A stopgap until the options partition: every other
     * REQUEST option is reset here too. */
    char *savedDumpSelection = options.dumpSelection != NULL ? strdup(options.dumpSelection) : NULL;

    inputFileName = fileName;
    ArgumentsVector emptyArgs = {.argc = 0, .argv = NULL};
    maxPasses = 1;
    silenceMessages();
    for (currentPass = 1; currentPass <= maxPasses; currentPass++) {
        if (initializeFileProcessing(baseArgs, emptyArgs)) {
            options.cursorOffset = NO_CURSOR_OFFSET;
            parseToCreateReferences(inputFileName);
            parsed = true;
            closeCharacterBuffer(&currentFile.characterBuffer);
            currentFile.characterBuffer.file = stdin;
        }
        currentFile.characterBuffer.isAtEOF = false;
    }
    resumeMessages();

    options.cursorOffset = savedCursorOffset;
    options.index = savedIndex;
    options.serverOperation = savedServerOperation;
    if (savedDumpSelection != NULL) {
        allocateStringForOption(&options.dumpSelection, savedDumpSelection);
        free(savedDumpSelection);
    }
    return parsed;
}

static void buildKnowledgeOfCU(int fileNumber, ArgumentsVector baseArgs) {
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    char *fileName = fileItem->name;
    if (!editorFileExists(fileName)) {
        markFileAsDeleted(fileNumber);
        return;
    }
    removeReferenceableItemsForFile(fileNumber);
    /* Taken before the content is read, so an edit during the parse makes the
     * knowledge out of date. */
    FileTimestamp knowledgeTime = fileTimestampNow();
    bool parsed = parseFileWithFullInit(fileName, baseArgs);
    if (parsed)
        fileItem->knowledgeTime = knowledgeTime;
}

void buildKnowledgeOfAll(BuildList *list, ArgumentsVector baseArgs) {
    if (list->count == 0)
        return;

    log_info("Building %d CU(s)", list->count);
    if (options.cxrefProtocol)
        setupProgress(list);

    for (int i = 0; i < list->count; i++) {
        buildKnowledgeOfCU(list->fileNumbers[i], baseArgs);
        getFileItemWithFileNumber(list->fileNumbers[i])->needsBrowsingStackRefresh = true;
        if (options.cxrefProtocol)
            writeProgressInformation(list->count - i - 1);
        /* Save the snapshot every 100 CUs, so a long build survives Ctrl-g or a crash */
        if ((i + 1) % 100 == 0)
            saveReferences();
    }
}
