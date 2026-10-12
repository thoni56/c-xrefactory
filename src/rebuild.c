#include "rebuild.h"

#include <stdlib.h>

#include "editor.h"
#include "editorbuffertable.h"
#include "filetable.h"
#include "log.h"
#include "misc.h"
#include "navigation.h"
#include "options.h"
#include "progress.h"
#include "referenceableitem.h"
#include "referenceableitemtable.h"
#include "referencerefresh.h"


/* The CUs to build, at most MAX_CUS_TO_REPARSE of them */
struct buildList {
    int   fileNumbers[MAX_CUS_TO_REPARSE];
    int   count;
    int   skippedCapped; /* Out of date, but did not fit */
    bool *built;         /* By file number, on the list or parsed by the operation */
};

BuildList *newBuildList(void) {
    BuildList *list = calloc(1, sizeof(BuildList));
    list->built = calloc(MAX_FILES, sizeof(bool));
    return list;
}

void freeBuildList(BuildList *list) {
    free(list->built);
    free(list);
}

static bool addToBuildList(BuildList *list, int fileNumber) {
    if (list->built[fileNumber])
        return true;
    if (list->count < MAX_CUS_TO_REPARSE) {
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
                 "%d unparsed sibling compilation units, parsing %d... %%d remaining", totalFound,
                 list->count);
    else
        snprintf(progressFormat, sizeof(progressFormat),
                 "Parsing %d sibling compilation units... %%d remaining", list->count);
    initProgress(progressFormat);
}

static bool everyIncluderIsRebuilt(int includers[], int includerCount, bool rebuilt[]) {
    for (int i = 0; i < includerCount; i++) {
        if (!rebuilt[includers[i]])
            return false;
    }

    return includerCount > 0;
}

/* Entry refresh Pass 3: Find CUs that share headers with the request file
 * but haven't been parsed yet. On cold start without preloads, only the
 * request file gets parsed (by processFile). This pass ensures sibling CUs
 * are also parsed so their symbol references are available for navigation.
 *
 * Uses brute-force forward-include lookup: iterates all headers in the
 * file table and checks if the request file is among their includers. */
void collectSiblingCUs(BuildList *list, Inputs *inputs, int requestFileNumber, bool requestIsParsed) {
    int skippedAlreadyParsed = 0;

    log_info("Pass 3: request file '%s' (fileNumber=%d)",
             getFileItemWithFileNumber(requestFileNumber)->name, requestFileNumber);

    bool *asked = calloc(MAX_FILES, sizeof(bool)); /* A sibling shares many headers */

    /* The request is parsed by the operation itself, not here */
    if (requestIsParsed && isCompilationUnit(getFileItemWithFileNumber(requestFileNumber)->name)
        && knowledgeIsOutOfDate(inputs, requestFileNumber))
        list->built[requestFileNumber] = true;

    int *siblingCUs = malloc(MAX_FILES * sizeof(int));
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fi = getFileItemWithFileNumber(i);
        if (isCompilationUnit(fi->name))
            continue; /* Skip CUs, only look at headers */

        ReferenceableItem searchItem = makeReferenceableItem(LINK_NAME_INCLUDE_REFS, TypeCppInclude,
                                                             StorageExtern, VisibilityGlobal, i);

        ReferenceableItem *found;
        if (!isMemberInReferenceableItemTable(&searchItem, NULL, &found))
            continue;

        /* Is this header the request file, or included by it? Browsing can start
         * in a header, and then the CUs that include it are the siblings that
         * need parsing - on a cold start none of them has been parsed. */
        bool requestIncludesThis = (i == requestFileNumber);
        for (Reference *r = found->references; r != NULL; r = r->next) {
            if (r->position.file == requestFileNumber) {
                requestIncludesThis = true;
                break;
            }
        }

        if (!requestIncludesThis)
            continue;

        /* Collect sibling CUs that include this header and need parsing, also
         * through a file that is not a CU, such as a mock or another header */
        int siblingCount = collectCUsIncluding(i, siblingCUs, 0, MAX_FILES);
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
                skippedAlreadyParsed++;
                continue;
            }

            if (addToBuildList(list, siblingFileNum))
                log_debug("Sibling CU '%s' shares header with request file", siblingItem->name);
        }

        /* The references in a header are all or nothing (ADR-0032): strip them
         * only when every CU that includes it is built again, before any is */
        if (everyIncluderIsRebuilt(siblingCUs, siblingCount, list->built)) {
            log_debug("Every includer of '%s' is built again, stripping it", fi->name);
            removeReferenceableItemsForFile(i);
        }
    }

    free(siblingCUs);
    free(asked);

    log_info("Pass 3: %d to parse, %d skipped (already parsed), %d skipped (cap %d)", list->count,
             skippedAlreadyParsed, list->skippedCapped, MAX_CUS_TO_REPARSE);
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

static void stripChangedHeldHeaders(BuildList *list) {
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
            int fileNumber = l->buffer->fileNumber;
            FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
            if (fileNumberIsStale(fileNumber) && !isCompilationUnit(fileItem->name)) {
                if (!editorFileExists(fileItem->name)) {
                    log_debug("Changed header '%s' no longer exists, marking as deleted",
                              fileItem->name);
                    markFileAsDeleted(fileNumber);
                } else {
                    int count = collectCUsIncluding(fileNumber, list->fileNumbers, list->count,
                                                    MAX_CUS_TO_REPARSE);
                    for (; list->count < count; list->count++)
                        list->built[list->fileNumbers[list->count]] = true;
                    /* The stale header's own references go now; building the CUs
                     * above re-emits them. */
                    removeReferenceableItemsForFile(fileNumber);
                    EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);
                    if (buffer != NULL)
                        fileItem->lastParsedMtime = buffer->modificationTime;
                    fileItem->needsBrowsingStackRefresh = true;
                }
            }
        }
    }
}

/* Only what the client holds, as for a stale header */
static bool heldCUIsOutOfDate(Inputs *inputs, EditorBuffer *buffer) {
    if (!holdsAuthoritativeContent(buffer))
        return false;
    FileItem *fileItem = getFileItemWithFileNumber(buffer->fileNumber);
    return !fileItem->isDeleted && isCompilationUnit(fileItem->name)
           && knowledgeIsOutOfDate(inputs, buffer->fileNumber);
}

static void collectOutOfDateHeldCUs(BuildList *list, Inputs *inputs) {
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

void collectOutOfDateHeldFiles(BuildList *list, Inputs *inputs) {
    /* Held CUs whose knowledge is out of date. They go first, so the cap
     * never leaves one of them out. */
    collectOutOfDateHeldCUs(list, inputs);

    /* Stale held headers are stripped before any CU is built, so each CU
     * that includes one is built once and puts its part of the header
     * back. The includers are found through the include references as
     * they were before this request. */
    stripChangedHeldHeaders(list);
}

void buildKnowledgeOfAll(BuildList *list, ArgumentsVector baseArgs) {
    if (list->count == 0)
        return;

    log_info("Building %d CU(s)", list->count);
    if (options.cxrefProtocol)
        setupProgress(list);

    /* Clear cursorOffset so the lexer doesn't set positionOfSelectedReference
     * during the reparse — that would trigger on-line action handling (browsing stack
     * assertions) before the operation has set things up. */
    int savedCursorOffset = options.cursorOffset;
    options.cursorOffset = NO_CURSOR_OFFSET;
    for (int i = 0; i < list->count; i++) {
        buildKnowledgeOfCU(list->fileNumbers[i], baseArgs);
        getFileItemWithFileNumber(list->fileNumbers[i])->needsBrowsingStackRefresh = true;
        if (options.cxrefProtocol)
            writeProgressInformation(list->count - i - 1);
    }
    options.cursorOffset = savedCursorOffset;
}
