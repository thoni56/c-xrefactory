#include "xref.h"

#include <string.h>
#include <stdlib.h>

#include "caching.h"
#include "commons.h"
#include "cxfile.h"
#include "cxref.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "log.h"
#include "main.h"
#include "memory.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "progress.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"
#include "reftab.h"


// this is necessary to put new mtimes for header files
static void setFullUpdateMtimesInFileItem(FileItem *fi) {
    if (fi->scheduledToUpdate || options.create) {
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

static ReferenceItem makeReferenceItemForIncludeFile(int fileNumber) {
    return makeReferenceItem(LINK_NAME_INCLUDE_REFS, TypeCppInclude, StorageExtern,
                             GlobalScope, GlobalVisibility, fileNumber);
}

static void makeIncludeClosureOfFilesToUpdate(void) {
    void *cxFlushPoint = cxAlloc(0);

    fullScanFor(LINK_NAME_INCLUDE_REFS);
    // iterate over scheduled files
    bool fileAddedFlag = true;
    while (fileAddedFlag) {
        fileAddedFlag = false;
        for (int i=getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i+1)) {
            FileItem *fileItem = getFileItemWithFileNumber(i);
            if (fileItem->scheduledToUpdate)
                if (!fileItem->fullUpdateIncludesProcessed) {
                    fileItem->fullUpdateIncludesProcessed = true;
                    ReferenceItem referenceItem = makeReferenceItemForIncludeFile(i);
                    ReferenceItem *foundMemberP;
                    if (isMemberInReferenceTable(&referenceItem, NULL, &foundMemberP)) {
                        for (Reference *r=foundMemberP->references; r!=NULL; r=r->next) {
                            FileItem *includerFile = getFileItemWithFileNumber(r->position.file);
                            if (!includerFile->scheduledToUpdate) {
                                includerFile->scheduledToUpdate = true;
                                fileAddedFlag = true;
                            }
                        }
                    }
                }

        }
    }
    // Does this indicate that we are allocating a lot of memory that we should flush?
    recoverMemoriesAfterOverflow(cxFlushPoint);
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
        // (missing of following if) has caused that all class hierarchy items
        // as well as all cxreferences based in .class files were lost
        // on -update, a very serious bug !!!!
        fileItem->cxLoading = true;
    } else if (options.update == UPDATE_FULL) {
        if (editorFileModificationTime(fileItem->name) != fileItem->lastFullUpdateMtime) {
            fileItem->scheduledToUpdate = true;
        }
    } else {
        if (editorFileModificationTime(fileItem->name) != fileItem->lastUpdateMtime) {
            fileItem->scheduledToUpdate = true;
        }
    }
    log_trace("Scheduling '%s' to update: %s", fileItem->name, fileItem->scheduledToUpdate?"yes":"no");
}

static void processInputFile(int argc, char **argv, bool *firstPassP, bool *atLeastOneProcessedP) {
    bool inputOpened;

    maxPasses = 1;
    for (currentPass = 1; currentPass <= maxPasses; currentPass++) {
        if (!*firstPassP)
            deepCopyOptionsFromTo(&savedOptions, &options);
        inputOpened = initializeFileProcessing(firstPassP, argc, argv, 0, NULL, &currentLanguage);
        originalFileNumber = inputFileNumber;
        originalCommandLineFileNumber = originalFileNumber;
        if (inputOpened) {
            recoverFromCache();  /* TODO Why do we need this here? */
            deactivateCaching(); /* no caching in cxref */
            if (options.fileTrace)
                fprintf(stderr, "processInputFile: '%s\n", currentFile.fileName);
            parseCurrentInputFile(currentLanguage);
            closeCharacterBuffer(&currentFile.characterBuffer);
            inputOpened = false;
            currentFile.characterBuffer.file = stdin;
            *atLeastOneProcessedP = true;
        } else {
            errorMessage(ERR_CANT_OPEN, inputFileName);
            fprintf(errOut, "\tmaybe forgotten -p option?\n");
        }
        *firstPassP                          = false;
        currentFile.characterBuffer.isAtEOF = false;
    }
}

static void getCxrefFilesListName(char **fileListFileNameP, char **suffixP) {
    if (options.referenceFileCount <= 1) {
        *suffixP           = "";
        *fileListFileNameP = options.cxrefsLocation;
    } else {
        char cxrefsFileName[MAX_FILE_NAME_SIZE];
        *suffixP = REFERENCE_FILENAME_FILES;
        sprintf(cxrefsFileName, "%s%s", options.cxrefsLocation, *suffixP);
        assert(strlen(cxrefsFileName) < MAX_FILE_NAME_SIZE - 1);
        *fileListFileNameP = cxrefsFileName;
    }
}

static void oneWholeFileProcessing(int argc, char **argv, FileItem *fileItem, bool *firstPass,
                                   bool *atLeastOneProcessed, bool isRefactoring) {
    inputFileName           = fileItem->name;
    fileProcessingStartTime = time(NULL);
    // O.K. but this is missing all header files
    fileItem->lastUpdateMtime = fileItem->lastModified;
    if (options.update == UPDATE_FULL || options.create) {
        fileItem->lastFullUpdateMtime = fileItem->lastModified;
    }
    processInputFile(argc, argv, firstPass, atLeastOneProcessed);
    // now free the buffer because it tooks too much memory,
    // but I can not free it when refactoring, nor when preloaded,
    // so be very careful about this!!!
    // TODO: WTF? All test still passes if we reverse this comparison...
    if (!isRefactoring) {
        closeEditorBufferIfCloseable(inputFileName);
        closeAllEditorBuffersIfClosable();
    }
}


void checkExactPositionUpdate(bool printMessage) {
    if (options.update == UPDATE_FAST && options.exactPositionResolve) {
        options.update = UPDATE_FULL;
        if (printMessage) {
            warningMessage(ERR_ST, "-exactpositionresolve implies full update");
        }
    }
}

static void scheduleModifiedFilesToUpdate(bool isRefactoring) {
    char        *fileListFileName;
    char        *suffix;

    checkExactPositionUpdate(true);

    getCxrefFilesListName(&fileListFileName, &suffix);

    mapOverFileTableWithBool(schedulingToUpdate, isRefactoring);

    if (options.update==UPDATE_FULL) {
        makeIncludeClosureOfFilesToUpdate();
    }
    mapOverFileTable(schedulingUpdateToProcess);
}


static void referencesOverflowed(char *cxMemFreeBase, LongjmpReason reason) {
    ENTER();
    if (reason != LONGJMP_REASON_NONE) {
        log_trace("swapping references to disk");
        if (options.xref2) {
            ppcGenRecord(PPC_INFORMATION, "swapping references to disk");
            ppcGenRecord(PPC_INFORMATION, "");
        } else {
            fprintf(errOut, "swapping references to disk (please wait)\n");
            fflush(errOut);
        }
    }
    if (options.cxrefsLocation == NULL) {
        FATAL_ERROR(ERR_ST, "sorry no file for cxrefs, use -refs option", XREF_EXIT_ERR);
    }
    for (int i=0; i < includeStack.pointer; i++) {
        log_trace("inspecting include %d, fileNumber: %d", i,
                  includeStack.stack[i].characterBuffer.fileNumber);
        if (includeStack.stack[i].characterBuffer.file != stdin) {
            int fileNumber = includeStack.stack[i].characterBuffer.fileNumber;
            getFileItemWithFileNumber(fileNumber)->cxLoading = false;
            if (includeStack.stack[i].characterBuffer.file != NULL)
                closeCharacterBuffer(&includeStack.stack[i].characterBuffer);
        }
    }
    if (currentFile.characterBuffer.file != stdin) {
        log_trace("inspecting current file, fileNumber: %d", currentFile.characterBuffer.fileNumber);
        int fileNumber                     = currentFile.characterBuffer.fileNumber;
        getFileItemWithFileNumber(fileNumber)->cxLoading = false;
        if (currentFile.characterBuffer.file != NULL)
            closeCharacterBuffer(&currentFile.characterBuffer);
    }
    if (options.mode==XrefMode)
        generateReferences();
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
            log_trace(" -># '%s'",fileItem->name);
        }
    }
    if (!savingFlag && reason!=LONGJMP_REASON_FILE_ABORT) {
        /* references overflowed, but no whole file readed */
        FATAL_ERROR(ERR_NO_MEMORY, "cxMemory", XREF_EXIT_ERR);
    }
    LEAVE();
}

void callXref(int argc, char **argv, bool isRefactoring) {
    // These are static because of the longjmp() maybe happening
    static char     *cxFreeBase;
    static bool      firstPass, atLeastOneProcessed;
    static FileItem *fileItem;
    static int       numberOfInputs;

    LongjmpReason reason = LONGJMP_REASON_NONE;

    currentPass = ANY_PASS;
    cxFreeBase = cxAlloc(0);
    cxResizingBlocked = true;
    if (options.update)
        scheduleModifiedFilesToUpdate(isRefactoring);
    atLeastOneProcessed = false;
    fileItem = createListOfInputFileItems();
    LIST_LEN(numberOfInputs, FileItem, fileItem);
    for (;;) {
        currentPass = ANY_PASS;
        firstPass   = true;
        if ((reason = setjmp(cxmemOverflow)) != 0) {
            referencesOverflowed(cxFreeBase, reason);
            if (reason == LONGJMP_REASON_FILE_ABORT) {
                if (fileItem != NULL)
                    fileItem = fileItem->next;
            }
        } else {
            int inputCounter = 0;
            fileAbortEnabled = true;

            for (; fileItem != NULL; fileItem = fileItem->next) {
                oneWholeFileProcessing(argc, argv, fileItem, &firstPass, &atLeastOneProcessed, isRefactoring);
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
            if (options.update == UPDATE_DEFAULT || options.update == UPDATE_FULL) {
                mapOverFileTable(setFullUpdateMtimesInFileItem);
            }
            if (options.xref2) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "Generating '%s'", options.cxrefsLocation);
                ppcGenRecord(PPC_INFORMATION, tmpBuff);
            } else {
                log_info("Generating '%s'", options.cxrefsLocation);
            }
            generateReferences();
        }
    } else if (options.serverOperation == OLO_ABOUT) {
        aboutMessage();
    } else if (options.update == UPDATE_DEFAULT) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "no input file");
        errorMessage(ERR_ST, tmpBuff);
    }
    if (options.xref2) {
        writeRelativeProgress(100);
    }
}

void xref(int argc, char **argv) {
    ENTER();
    openOutputFile(options.outputFileName);
    loadAllOpenedEditorBuffers();

    callXref(argc, argv, false);
    closeOutputFile();
    if (options.xref2) {
        ppcSynchronize();
    }
    //& fprintf(dumpOut, "\n\nDUMP\n\n"); fflush(dumpOut);
    //& mapOverReferenceTable(symbolRefItemDump);
    LEAVE();
}
