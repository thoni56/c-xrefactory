#include "xref.h"

#include <stdlib.h>
#include <string.h>

#include "argumentsvector.h"
#include "commons.h"
#include "cxfile.h"
#include "cxref.h"
#include "editor.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "log.h"
#include "memory.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "parsing.h"
#include "ppc.h"
#include "progress.h"
#include "proto.h"
#include "protocol.h"
#include "referenceableitemtable.h"
#include "startup.h"
#include "yylex.h"


// this is necessary to put new mtimes for header files
static void setFullUpdateMtimesInFileItem(FileItem *fi) {
    if (fi->scheduledToUpdate || options.update == UPDATE_CREATE) {
        fi->lastFullUpdateMtime = fi->lastModified;
    }
}

static bool isFileItemLess(FileItem *fileItem1, FileItem *fileItem2) {
    int  comparison;
    char directoryName1[MAX_FILE_NAME_SIZE];
    char directoryName2[MAX_FILE_NAME_SIZE];

    // first compare directory
    strcpy(directoryName1, directoryName_static(fileItem1->name));
    strcpy(directoryName2, directoryName_static(fileItem2->name));
    comparison = strcmp(directoryName1, directoryName2);
    if (comparison < 0)
        return true;
    if (comparison > 0)
        return false;
    // then full file name
    comparison = strcmp(fileItem1->name, fileItem2->name);
    if (comparison < 0)
        return true;
    if (comparison > 0)
        return false;
    return false;
}

static void sortFileItemList(FileItem **fileItemsP,
                             bool (*compareFunction)(FileItem *fileItem1, FileItem *fileItem2)) {
    LIST_MERGE_SORT(FileItem, *fileItemsP, compareFunction);
}

static FileItem *createListOfInputFileItems(void) {
    FileItem *fileItems  = NULL;
    int fileNumber = 0;

    for (char *fileName = getNextScheduledFile(&fileNumber); fileName != NULL;
         fileNumber++, fileName = getNextScheduledFile(&fileNumber)) {
        FileItem *current = getFileItemWithFileNumber(fileNumber);
        current->next     = fileItems;
        fileItems          = current;
    }
    sortFileItemList(&fileItems, isFileItemLess);
    return fileItems;
}

static ReferenceableItem makeReferenceableItemForIncludeFile(int fileNumber) {
    return makeReferenceableItem(LINK_NAME_INCLUDE_REFS, TypeCppInclude, StorageExtern,
                                 GlobalScope, VisibilityGlobal, fileNumber);
}

static void makeIncludeClosureOfFilesToUpdate(void) {
    ensureReferencesAreLoadedFor(LINK_NAME_INCLUDE_REFS);
    // iterate over scheduled files
    bool fileAddedFlag = true;
    while (fileAddedFlag) {
        fileAddedFlag = false;
        for (int i=getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i+1)) {
            FileItem *fileItem = getFileItemWithFileNumber(i);
            if (fileItem->scheduledToUpdate)
                if (!fileItem->fullUpdateIncludesProcessed) {
                    fileItem->fullUpdateIncludesProcessed = true;
                    ReferenceableItem referenceableItem = makeReferenceableItemForIncludeFile(i);
                    ReferenceableItem *foundMemberP;
                    if (isMemberInReferenceableItemTable(&referenceableItem, NULL, &foundMemberP)) {
                        for (Reference *r=foundMemberP->references; r!=NULL; r=r->next) {
                            FileItem *includingFile = getFileItemWithFileNumber(r->position.file);
                            if (!includingFile->scheduledToUpdate) {
                                includingFile->scheduledToUpdate = true;
                                fileAddedFlag = true;
                            }
                        }
                    }
                }

        }
    }
    initAllInputs();
}

static void schedulingUpdateToProcess(FileItem *fileItem) {
    if (fileItem->scheduledToUpdate && fileItem->isArgument) {
        fileItem->isScheduled = true;
    }
}

/* NOTE: Map-function */
static void schedulingToUpdate(FileItem *fileItem, bool calledDuringRefactoring) {
    if (fileItem == getFileItemWithFileNumber(NO_FILE_NUMBER))
        return;

    if (!editorFileExists(fileItem->name)) {
        // removed file, remove it from watched updates, load no reference
        if (fileItem->isArgument) {
            if (!calledDuringRefactoring) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "file %s not accessible", fileItem->name);
                warningMessage(ERR_ST, tmpBuff);
            }
        }
        fileItem->isArgument = false;
        fileItem->isScheduled = false;
        fileItem->scheduledToUpdate = false;
        fileItem->cxLoading = true;
    } else if (options.update == UPDATE_FULL) {
        if (editorFileModificationTime(fileItem->name) != fileItem->lastFullUpdateMtime) {
            fileItem->scheduledToUpdate = true;
        }
    } else {
        if (editorFileModificationTime(fileItem->name) != fileItem->lastParsedMtime) {
            fileItem->scheduledToUpdate = true;
        }
    }
    log_trace("Scheduling '%s' to update: %s", fileItem->name, fileItem->scheduledToUpdate?"yes":"no");
}

static void processInputFile(ArgumentsVector args, bool *atLeastOneProcessedP) {
    bool inputOpened;

    maxPasses = 1;
    for (currentPass = 1; currentPass <= maxPasses; currentPass++) {
        ArgumentsVector nargs = {.argc = 0, .argv = NULL};

        inputOpened = initializeFileProcessing(args, nargs);
        parsingConfig.fileNumber = currentFileNumber;

        if (inputOpened) {
            if (options.fileTrace)
                fprintf(stderr, "Processing input file: '%s\n", currentFile.fileName);
            parseToCreateReferences(inputFileName);
            closeCharacterBuffer(&currentFile.characterBuffer);
            inputOpened = false;
            currentFile.characterBuffer.file = stdin;
            *atLeastOneProcessedP = true;
        } else {
            errorMessage(ERR_CANT_OPEN, inputFileName);
            fprintf(errOut, "\tmaybe forgotten -p option?\n");
        }
        currentFile.characterBuffer.isAtEOF = false;
    }
}

static void oneWholeFileProcessing(ArgumentsVector args, FileItem *fileItem,
                                   bool *atLeastOneProcessed, XrefConfig *config) {
    inputFileName           = fileItem->name;
    fileProcessingStartTime = time(NULL);
    // O.K. but this is missing all header files
    fileItem->lastParsedMtime = fileItem->lastModified;
    if (config->updateType == UPDATE_FULL || config->updateType == UPDATE_CREATE) {
        fileItem->lastFullUpdateMtime = fileItem->lastModified;
    }
    processInputFile(args, atLeastOneProcessed);
    // now free the buffer because it tooks too much memory,
    // but I can not free it when refactoring, nor when preloaded,
    // so be very careful about this!!!
    if (!config->isRefactoring) {
        closeEditorBufferIfCloseable(inputFileName);
        closeAllEditorBuffersIfClosable();
    }
}

void checkExactPositionUpdate(UpdateType *updateType, bool printMessage) {
    if (*updateType == UPDATE_FAST && options.exactPositionResolve) {
        *updateType = UPDATE_FULL;
        if (printMessage) {
            warningMessage(ERR_ST, "-exactpositionresolve implies full update");
        }
    }
}

static void scheduleModifiedFilesToUpdate(XrefConfig *config) {
    checkExactPositionUpdate(&config->updateType, true);

    mapOverFileTableWithBool(schedulingToUpdate, config->isRefactoring);

    if (config->updateType == UPDATE_FULL) {
        makeIncludeClosureOfFilesToUpdate();
    }
    mapOverFileTable(schedulingUpdateToProcess);
}


static void recoverMemoriesAfterOverflow(char *cxMemFreeBase) {
    // This was some intricate recovery of various memories mixed with the caching.
    // When the caching was removed piece by piece those functions were also removed.
    // If "out of cxMemory" is ever to be handled they can be recovered from history
    // beyond this point.
    // static void deleteReferencesOutOfMemory(Reference **referenceP) { ...
    // static void recoverMemoryFromReferenceTableEntry(int index) { ...
    // static void recoverMemoryFromReferenceTable(void) { ...
    // static void recoverCxMemory(void *cxMemoryFlushPoint) { ...
}

static void referencesOverflowed(char *cxMemFreeBase, LongjmpReason reason) {
    ENTER();
    log_trace("swapping references to disk");
    if (options.xref2) {
        ppcGenRecord(PPC_INFORMATION, "swapping references to disk");
        ppcGenRecord(PPC_INFORMATION, "");
    } else {
        fprintf(errOut, "swapping references to disk (please wait)\n");
        fflush(errOut);
    }
    if (options.cxFileLocation == NULL) {
        FATAL_ERROR(ERR_ST, "sorry no file for cxrefs, use -refs option", XREF_EXIT_ERR);
    }
    for (int i=0; i < includeStack.pointer; i++) {
        log_debug("inspecting include %d, fileNumber: %d", i,
                  includeStack.stack[i].characterBuffer.fileNumber);
        if (includeStack.stack[i].characterBuffer.file != stdin) {
            int fileNumber = includeStack.stack[i].characterBuffer.fileNumber;
            getFileItemWithFileNumber(fileNumber)->cxLoading = false;
            if (includeStack.stack[i].characterBuffer.file != NULL)
                closeCharacterBuffer(&includeStack.stack[i].characterBuffer);
        }
    }
    if (currentFile.characterBuffer.file != stdin) {
        log_debug("inspecting current file, fileNumber: %d", currentFile.characterBuffer.fileNumber);
        int fileNumber                     = currentFile.characterBuffer.fileNumber;
        getFileItemWithFileNumber(fileNumber)->cxLoading = false;
        if (currentFile.characterBuffer.file != NULL)
            closeCharacterBuffer(&currentFile.characterBuffer);
    }
    if (options.mode==XrefMode)
        saveReferences();
    recoverMemoriesAfterOverflow(cxMemFreeBase);

    /* ************ start with CXREFS and memories clean ************ */
    bool savingFlag = false;
    for (int i=getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i+1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (fileItem->cxLoading) {
            fileItem->cxLoading = false;
            fileItem->cxSaved = true;
            if (fileItem->isArgument)
                savingFlag = true;
            log_debug(" -># '%s'",fileItem->name);
        }
    }
    if (!savingFlag && reason!=LONGJMP_REASON_FILE_ABORT) {
        /* references overflowed, but no whole file readed */
        FATAL_ERROR(ERR_NO_MEMORY, "cxMemory", XREF_EXIT_ERR);
    }
    LEAVE();
}

void callXref(ArgumentsVector args, XrefConfig *config) {
    // These are static because of the longjmp() maybe happening
    static char     *cxFreeBase;
    static bool      atLeastOneProcessed;
    static FileItem *fileItem;
    static int       numberOfInputs;

    LongjmpReason reason = LONGJMP_REASON_NONE;

    currentPass = ANY_PASS;
    cxFreeBase = cxAlloc(0);
    cxResizingBlocked = true;
    if (config->updateType)
        scheduleModifiedFilesToUpdate(config);
    atLeastOneProcessed = false;
    fileItem = createListOfInputFileItems();
    LIST_LEN(numberOfInputs, FileItem, fileItem);
    for (;;) {
        currentPass = ANY_PASS;
        if ((reason = setjmp(errorLongJumpBuffer)) != 0) {
            if (reason == LONGJMP_REASON_FILE_ABORT) {
                if (fileItem != NULL)
                    fileItem = fileItem->next;
            } else if (reason == LONGJMP_REASON_REFERENCES_OVERFLOW) {
                referencesOverflowed(cxFreeBase, reason);
            }
        } else {
            int inputCounter = 0;
            fileAbortEnabled = true;

            for (; fileItem != NULL; fileItem = fileItem->next) {
                oneWholeFileProcessing(args, fileItem, &atLeastOneProcessed,
                                       config);
                fileItem->isScheduled       = false;
                fileItem->scheduledToUpdate = false;
                if (options.xref2)
                    writeRelativeProgress(10 + (90 * inputCounter) / numberOfInputs);
                inputCounter++;
            }
            goto finish;
        }
    }

finish:
    fileAbortEnabled = false;
    if (atLeastOneProcessed) {
        if (options.mode == XrefMode) {
            if (config->updateType == UPDATE_DEFAULT || config->updateType == UPDATE_FULL
                || config->updateType == UPDATE_CREATE) {
                mapOverFileTable(setFullUpdateMtimesInFileItem);
            }
            if (options.xref2) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "Generating '%s'", options.cxFileLocation);
                ppcGenRecord(PPC_INFORMATION, tmpBuff);
            } else {
                log_info("Generating '%s'", options.cxFileLocation);
            }
            saveReferences();
        }
    } else if (options.serverOperation == OLO_ABOUT) {
        aboutMessage();
    } else if (config->updateType == UPDATE_DEFAULT) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "no input file");
        errorMessage(ERR_ST, tmpBuff);
    }
    if (options.xref2) {
        writeRelativeProgress(100);
    }
}

void xref(ArgumentsVector args) {
    ENTER();
    openOutputFile(options.outputFileName);
    loadAllOpenedEditorBuffers();

    XrefConfig config = {
        .projectName = options.project,
        .updateType = options.update,
        .isRefactoring = false
    };
    callXref(args, &config);
    closeOutputFile();
    if (options.xref2) {
        ppcSynchronize();
    }
    //& fprintf(dumpOut, "\n\nDUMP\n\n"); fflush(dumpOut);
    //& mapOverReferenceableItemTable(dumpReferenceableItem);
    LEAVE();
}
