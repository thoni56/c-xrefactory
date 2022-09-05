#include "xref.h"

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
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "progress.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"
#include "reftab.h"

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

static void fillIncludeRefItem(ReferencesItem *referencesItem, int fileIndex) {
    fillReferencesItem(referencesItem, LINK_NAME_INCLUDE_REFS,
                       cxFileHashNumber(LINK_NAME_INCLUDE_REFS),
                       fileIndex, fileIndex, TypeCppInclude, StorageExtern,
                       ScopeGlobal, AccessDefault, CategoryGlobal);
}

static void makeIncludeClosureOfFilesToUpdate(void) {
    char                *cxFreeBase;
    int                 fileAddedFlag;
    ReferencesItem referenceItem, *member;
    Reference           *rr;

    CX_ALLOCC(cxFreeBase,0,char);
    fullScanFor(LINK_NAME_INCLUDE_REFS);
    // iterate over scheduled files
    fileAddedFlag = true;
    while (fileAddedFlag) {
        fileAddedFlag = false;
        for (int i=getNextExistingFileIndex(0); i != -1; i = getNextExistingFileIndex(i+1)) {
            FileItem *fileItem = getFileItem(i);
            if (fileItem->scheduledToUpdate)
                if (!fileItem->fullUpdateIncludesProcessed) {
                    fileItem->fullUpdateIncludesProcessed = true;
                    bool isJavaFileFlag = fileNameHasOneOfSuffixes(fileItem->name, options.javaFilesSuffixes);
                    fillIncludeRefItem(&referenceItem, i);
                    if (isMemberInReferenceTable(&referenceItem, NULL, &member)) {
                        for (rr=member->references; rr!=NULL; rr=rr->next) {
                            FileItem *includer = getFileItem(rr->position.file);
                            if (!includer->scheduledToUpdate) {
                                includer->scheduledToUpdate = true;
                                fileAddedFlag = true;
                                if (isJavaFileFlag) {
                                    // no transitive closure for Java
                                    includer->fullUpdateIncludesProcessed = true;
                                }
                            }
                        }
                    }
                }

        }
    }
    recoverMemoriesAfterOverflow(cxFreeBase);
}

static void schedulingUpdateToProcess(FileItem *fileItem) {
    if (fileItem->scheduledToUpdate && fileItem->isArgument) {
        fileItem->isScheduled = true;
    }
}

/* Saved by scheduleModifiedFilesToUpdate() since we cannot send it as an argument to a Map function */
static bool calledDuringRefactoring;

/* NOTE: Map-function */
static void schedulingToUpdate(FileItem *fileItem) {
    if (fileItem == getFileItem(noFileIndex))
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
        if (fileItem->name[0] != ZIP_SEPARATOR_CHAR) {
            fileItem->cxLoading = true;     /* Hack, to remove references from file */
        }
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
            copyOptionsFromTo(&savedOptions, &options);
        inputOpened             = initializeFileProcessing(firstPassP, argc, argv, 0, NULL, &currentLanguage);
        olOriginalFileIndex     = inputFileNumber;
        olOriginalComFileNumber = olOriginalFileIndex;
        if (inputOpened) {
            recoverFromCache();
            cache.active = false; /* no caching in cxref */
            parseCurrentInputFile(currentLanguage);
            closeCharacterBuffer(currentFile.lexemBuffer.characterBuffer);
            inputOpened                       = false;
            currentFile.characterBuffer.file = stdin;
            *atLeastOneProcessedP             = true;
        } else if (LANGUAGE(LANG_JAR)) {
            jarFileParse(inputFileName);
            *atLeastOneProcessedP = true;
        } else if (LANGUAGE(LANG_CLASS)) {
            classFileParse();
            *atLeastOneProcessedP = true;
        } else {
            errorMessage(ERR_CANT_OPEN, inputFileName);
            fprintf(errOut, "\tmaybe forgotten -p option?\n");
        }
        // no multiple passes for java programs
        *firstPassP                          = false;
        currentFile.characterBuffer.isAtEOF = false;
        if (LANGUAGE(LANG_JAVA))
            break;
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
        editorCloseBufferIfClosable(inputFileName);
        editorCloseAllBuffersIfClosable();
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

void scheduleModifiedFilesToUpdate(bool isRefactoring) {
    char        *fileListFileName;
    char        *suffix;

    checkExactPositionUpdate(true);

    getCxrefFilesListName(&fileListFileName, &suffix);
    // TODO: This is probably to get modification time for the fileslist file
    // and then consider that when schedule files to update, but it never did...
    // if (editorFileStatus(fileListFileName, &stat))
    //     stat.st_mtime = 0;
    // ... but schedulingToUpdate() does not use the stat data !?!?!?
    // TODO: ... so WTF??!?!?!?
    // We should look at original sources (main.c) and try to figure out the mistake in logic
    normalScanReferenceFile(suffix);

    /* As schedulingToUpdate() is a mapped function we cannot send it as argument, so we save it here */
    calledDuringRefactoring = isRefactoring;
    mapOverFileTable(schedulingToUpdate);

    if (options.update==UPDATE_FULL /*& && !LANGUAGE(LANG_JAVA) &*/) {
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
    for (int i=0; i<includeStackPointer; i++) {
        log_trace("inspecting include %d, fileNumber: %d", i,
                  includeStack[i].characterBuffer.fileNumber);
        if (includeStack[i].characterBuffer.file != stdin) {
            int fileIndex                     = includeStack[i].characterBuffer.fileNumber;
            getFileItem(fileIndex)->cxLoading = false;
            if (includeStack[i].characterBuffer.file != NULL)
                closeCharacterBuffer(&includeStack[i].characterBuffer);
        }
    }
    if (currentFile.characterBuffer.file != stdin) {
        log_trace("inspecting current file, fileNumber: %d", currentFile.characterBuffer.fileNumber);
        int fileIndex                     = currentFile.characterBuffer.fileNumber;
        getFileItem(fileIndex)->cxLoading = false;
        if (currentFile.characterBuffer.file != NULL)
            closeCharacterBuffer(&currentFile.characterBuffer);
    }
    if (options.mode==XrefMode)
        generateReferences();
    recoverMemoriesAfterOverflow(cxMemFreeBase);

    /* ************ start with CXREFS and memories clean ************ */
    bool savingFlag = false;
    for (int i=getNextExistingFileIndex(0); i != -1; i = getNextExistingFileIndex(i+1)) {
        FileItem *fileItem = getFileItem(i);
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
    static FileItem *ffc, *pffc;
    static bool      messagePrinted = false;
    static int       numberOfInputs;

    LongjmpReason reason = LONGJMP_REASON_NONE;

    currentPass = ANY_PASS;
    CX_ALLOCC(cxFreeBase, 0, char);
    cxResizingBlocked = true;
    if (options.update)
        scheduleModifiedFilesToUpdate(isRefactoring);
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
                    oneWholeFileProcessing(argc, argv, pffc, &firstPass, &atLeastOneProcessed, isRefactoring);
                }
                writeRelativeProgress((10*inputCounter) / numberOfInputs);
                inputCounter++;
            }

            javaPreScanOnly  = false;
            fileAbortEnabled = true;

            inputCounter = 0;
            for (; ffc != NULL; ffc = ffc->next) {
                oneWholeFileProcessing(argc, argv, ffc, &firstPass, &atLeastOneProcessed, isRefactoring);
                ffc->isScheduled       = false;
                ffc->scheduledToUpdate = false;
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
    mainOpenOutputFile(options.outputFileName);
    editorLoadAllOpenedBufferFiles();

    callXref(argc, argv, false);
    closeMainOutputFile();
    if (options.xref2) {
        ppcSynchronize();
    }
    //& fprintf(dumpOut, "\n\nDUMP\n\n"); fflush(dumpOut);
    //& mapOverReferenceTable(symbolRefItemDump);
    LEAVE();
}
