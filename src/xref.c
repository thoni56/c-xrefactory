#include "xref.h"

#include "caching.h"
#include "commons.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "log.h"
#include "main.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"

static void printPrescanningMessage(void) {
    if (options.xref2) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Prescanning classes, please wait.");
        ppcGenRecord(PPC_INFORMATION, tmpBuff);
    } else {
        log_info("Prescanning classes, please wait.");
    }
}

// this is necessary to put new mtimes for header files
static void setFullUpdateMtimesInFileItem(FileItem *fi) {
    if (fi->scheduledToUpdate || options.create) {
        fi->lastFullUpdateMtime = fi->lastModified;
    }
}

static bool inputFileItemLess(FileItem *fileItem1, FileItem *fileItem2) {
    int  comparison;
    char directoryName1[MAX_FILE_NAME_SIZE];
    char directoryName2[MAX_FILE_NAME_SIZE];

    // first compare directory
    strcpy(directoryName1, directoryName_st(fileItem1->name));
    strcpy(directoryName2, directoryName_st(fileItem2->name));
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

static FileItem *createListOfInputFileItems(void) {
    FileItem *fileItem;
    int       fileIndex;

    fileItem  = NULL;
    fileIndex = 0;
    for (char *fileName = getNextScheduledFile(&fileIndex); fileName != NULL;
         fileIndex++, fileName = getNextScheduledFile(&fileIndex)) {
        FileItem *current = getFileItem(fileIndex);
        current->next     = fileItem;
        fileItem          = current;
    }
    LIST_MERGE_SORT(FileItem, fileItem, inputFileItemLess);
    return fileItem;
}

static void processInputFile(int argc, char **argv, bool *firstPassP, bool *atLeastOneProcessedP) {
    bool inputOpened;

    maxPasses = 1;
    for (currentPass = 1; currentPass <= maxPasses; currentPass++) {
        if (!*firstPassP)
            copyOptionsFromTo(&savedOptions, &options);
        inputOpened             = fileProcessingInitialisations(firstPassP, argc, argv, 0, NULL, &currentLanguage);
        olOriginalFileIndex     = inputFileNumber;
        olOriginalComFileNumber = olOriginalFileIndex;
        if (inputOpened) {
            recoverFromCache();
            cache.active = false; /* no caching in cxref */
            parseInputFile();
            closeCharacterBuffer(&currentFile.lexBuffer.buffer);
            inputOpened                       = false;
            currentFile.lexBuffer.buffer.file = stdin;
            *atLeastOneProcessedP             = true;
        } else if (LANGUAGE(LANG_JAR)) {
            jarFileParse(inputFilename);
            *atLeastOneProcessedP = true;
        } else if (LANGUAGE(LANG_CLASS)) {
            classFileParse();
            *atLeastOneProcessedP = true;
        } else {
            errorMessage(ERR_CANT_OPEN, inputFilename);
            fprintf(dumpOut, "\tmaybe forgotten -p option?\n");
        }
        // no multiple passes for java programs
        *firstPassP                          = false;
        currentFile.lexBuffer.buffer.isAtEOF = false;
        if (LANGUAGE(LANG_JAVA))
            break;
    }
}

static void oneWholeFileProcessing(int argc, char **argv, FileItem *fileItem, bool *firstPass,
                                bool *atLeastOneProcessed) {
    inputFilename           = fileItem->name;
    fileProcessingStartTime = time(NULL);
    // O.K. but this is missing all header files
    fileItem->lastUpdateMtime = fileItem->lastModified;
    if (options.update == UPDATE_FULL || options.create) {
        fileItem->lastFullUpdateMtime = fileItem->lastModified;
    }
    processInputFile(argc, argv, firstPass, atLeastOneProcessed);
    // now free the buffer because it tooks too much memory,
    // but I can not free it when refactoring, nor when preloaded,
    // so be very carefull about this!!!
    if (refactoringOptions.refactoringMode != RefactoryMode) {
        editorCloseBufferIfClosable(inputFilename);
        editorCloseAllBuffersIfClosable();
    }
}

void mainCallXref(int argc, char **argv) {
    // These are static because of the longjmp() maybe happening
    static char     *cxFreeBase;
    static bool      firstPass, atLeastOneProcessed;
    static FileItem *ffc, *pffc;
    static bool      messagePrinted = false;
    static int       numberOfInputs;

    LongjmpReason reason = LONGJMP_REASON_NONE;

    currentPass = ANY_PASS;
    CX_ALLOCC(cxFreeBase, 0, char);
    cxResizingBlocked = true;
    if (options.update)
        scheduleModifiedFilesToUpdate();
    atLeastOneProcessed = false;
    ffc = pffc = createListOfInputFileItems();
    LIST_LEN(numberOfInputs, FileItem, ffc);
    for (;;) {
        currentPass = ANY_PASS;
        firstPass   = true;
        if ((reason = setjmp(cxmemOverflow)) != 0) {
            referencesOverflowed(cxFreeBase, reason);
            if (reason == LONGJMP_REASON_FILE_ABORT) {
                if (pffc != NULL)
                    pffc = pffc->next;
                else if (ffc != NULL)
                    ffc = ffc->next;
            }
        } else {
            int inputCounter = 0;

            javaPreScanOnly = true;
            for (; pffc != NULL; pffc = pffc->next) {
                if (!messagePrinted) {
                    printPrescanningMessage();
                    messagePrinted = true;
                }
                currentLanguage = getLanguageFor(pffc->name);
                if (LANGUAGE(LANG_JAVA)) {
                    /* TODO: problematic if a single file generates overflow, e.g. a JAR
                       Can we just reread from the last class file? */
                    oneWholeFileProcessing(argc, argv, pffc, &firstPass, &atLeastOneProcessed);
                }
                if (options.xref2)
                    writeRelativeProgress(10 * inputCounter / numberOfInputs);
                inputCounter++;
            }

            javaPreScanOnly  = false;
            fileAbortEnabled = true;

            inputCounter = 0;
            for (; ffc != NULL; ffc = ffc->next) {
                oneWholeFileProcessing(argc, argv, ffc, &firstPass, &atLeastOneProcessed);
                ffc->isScheduled       = false;
                ffc->scheduledToUpdate = false;
                if (options.xref2)
                    writeRelativeProgress(10 + 90 * inputCounter / numberOfInputs);
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
            generateReferenceFile();
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
    mainOpenOutputFile(options.outputFileName);
    editorLoadAllOpenedBufferFiles();

    mainCallXref(argc, argv);
    closeMainOutputFile();
    if (options.xref2) {
        ppcSynchronize();
    }
    //& fprintf(dumpOut, "\n\nDUMP\n\n"); fflush(dumpOut);
    //& mapOverReferenceTable(symbolRefItemDump);
    LEAVE();
}
