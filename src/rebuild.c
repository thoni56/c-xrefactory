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


static void setupProgress(int cuCount, int skippedCapped) {
    static char progressFormat[128];
    int totalFound = cuCount + skippedCapped;
    if (skippedCapped > 0)
        snprintf(progressFormat, sizeof(progressFormat),
                 "%d unparsed sibling compilation units, parsing %d... %%d remaining", totalFound,
                 cuCount);
    else
        snprintf(progressFormat, sizeof(progressFormat),
                 "Parsing %d sibling compilation units... %%d remaining", cuCount);
    initProgress(progressFormat);
}

/* Entry refresh Pass 3: Find CUs that share headers with the request file
 * but haven't been parsed yet. On cold start without preloads, only the
 * request file gets parsed (by processFile). This pass ensures sibling CUs
 * are also parsed so their symbol references are available for navigation.
 *
 * Uses brute-force forward-include lookup: iterates all headers in the
 * file table and checks if the request file is among their includers. */
void parseUnparsedSiblingCUs(Inputs *inputs, int requestFileNumber, ArgumentsVector baseArgs) {
    int cuFileNumbers[MAX_CUS_TO_REPARSE];
    int cuCount = 0;
    int skippedAlreadyParsed = 0;
    int skippedCapped = 0;

    log_info("Pass 3: request file '%s' (fileNumber=%d)",
             getFileItemWithFileNumber(requestFileNumber)->name, requestFileNumber);

    bool *asked = calloc(MAX_FILES, sizeof(bool)); /* A sibling shares many headers */
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
            if (!knowledgeIsOutOfDate(inputs, siblingFileNum)) {
                skippedAlreadyParsed++;
                continue;
            }

            if (cuCount < MAX_CUS_TO_REPARSE) {
                cuFileNumbers[cuCount++] = siblingFileNum;
                log_debug("Sibling CU '%s' shares header with request file", siblingItem->name);
            } else {
                skippedCapped++;
            }
        }
    }

    free(siblingCUs);
    free(asked);

    log_info("Pass 3: %d to parse, %d skipped (already parsed), %d skipped (cap %d)", cuCount,
             skippedAlreadyParsed, skippedCapped, MAX_CUS_TO_REPARSE);

    if (cuCount > 0) {
        log_info("Entry refresh pass 3: parsing %d sibling CU(s)", cuCount);
        if (options.cxrefProtocol) {
            setupProgress(cuCount, skippedCapped);
        }
        int savedCursorOffset = options.cursorOffset;
        options.cursorOffset = NO_CURSOR_OFFSET;
        for (int i = 0; i < cuCount; i++) {
            buildKnowledgeOfCU(cuFileNumbers[i], baseArgs);
            if (options.cxrefProtocol)
                writeProgressInformation(cuCount - i - 1);
        }
        options.cursorOffset = savedCursorOffset;
    }
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

static int stripChangedHeldHeaders(int cuFileNumbers[], int cuCount) {
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
                    cuCount =
                        collectCUsIncluding(fileNumber, cuFileNumbers, cuCount, MAX_CUS_TO_REPARSE);
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

    return cuCount;
}

/* Only what the client holds, as for a stale header */
static bool heldCUIsOutOfDate(Inputs *inputs, EditorBuffer *buffer) {
    if (!holdsAuthoritativeContent(buffer))
        return false;
    FileItem *fileItem = getFileItemWithFileNumber(buffer->fileNumber);
    return !fileItem->isDeleted && isCompilationUnit(fileItem->name)
           && knowledgeIsOutOfDate(inputs, buffer->fileNumber);
}

static int collectOutOfDateHeldCUs(int cuFileNumbers[], int cuCount, Inputs *inputs) {
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
            int fileNumber = l->buffer->fileNumber;
            FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
            if (heldCUIsOutOfDate(inputs, l->buffer)) {
                if (!editorFileExists(fileItem->name)) {
                    log_debug("Out-of-date CU '%s' no longer exists, marking as deleted",
                              fileItem->name);
                    markFileAsDeleted(fileNumber);
                } else if (cuCount < MAX_CUS_TO_REPARSE) {
                    cuFileNumbers[cuCount++] = fileNumber;
                }
            }
        }
    }

    return cuCount;
}

void bringHeldFilesUpToDate(ArgumentsVector baseArgs) {
    int cuFileNumbers[MAX_CUS_TO_REPARSE];
    int cuCount = 0;

    /* Held CUs whose knowledge is out of date. They go first, so the cap
     * never leaves one of them out. Inputs of its own, because the
     * request's are collected after this has parsed. */
    Inputs *inputs = collectInputs();
    cuCount = collectOutOfDateHeldCUs(cuFileNumbers, cuCount, inputs);
    freeInputs(inputs);

    /* Stale held headers are stripped before any CU is built, so each CU
     * that includes one is built once and puts its part of the header
     * back. The includers are found through the include references as
     * they were before this request. */
    cuCount = stripChangedHeldHeaders(cuFileNumbers, cuCount);

    if (cuCount > 0) {
        log_info("Building %d CU(s) to bring held files up to date", cuCount);
        if (options.cxrefProtocol) {
            static char progressFormat[128];
            snprintf(progressFormat, sizeof(progressFormat),
                     "Updating %d compilation units... %%d remaining", cuCount);
            initProgress(progressFormat);
        }
        for (int i = 0; i < cuCount; i++) {
            buildKnowledgeOfCU(cuFileNumbers[i], baseArgs);
            getFileItemWithFileNumber(cuFileNumbers[i])->needsBrowsingStackRefresh = true;
            if (options.cxrefProtocol)
                writeProgressInformation(cuCount - i - 1);
        }
    }
}
