#include "server.h"

#include "argumentsvector.h"
#include "browsermenu.h"
#include "commons.h"
#include "complete.h"
#include "cxfile.h"
#include "cxref.h"
#include "editorbuffer.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "lexer.h"
#include "log.h"
#include "startup.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "ppc.h"
#include "referenceableitemtable.h"
#include "session.h"
#include "yylex.h"


const char *operationNamesTable[] = {
    "OLO_NONE",
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};



static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OLO_COMPLETION
           || operation==OLO_EXTRACT
           || operation==OLO_TAG_SEARCH
           || operation==OLO_SET_MOVE_FUNCTION_TARGET
           || operation==OLO_GET_METHOD_COORD
           || operation==OLO_GET_ENV_VALUE
           || requiresCreatingRefs(operation)
        ;
}


static int scheduleFileUsingTheMacro(void) {
    SessionStackEntry *tmpc;

    assert(completionStringInMacroBody);
    tmpc = NULL;
    ReferenceableItem references = makeReferenceableItem(completionStringInMacroBody, TypeMacro, StorageExtern,
                                                         GlobalScope, GlobalVisibility, NO_FILE_NUMBER);

    BrowserMenu menu = makeBrowserMenu(references, 1, true, 0, UsageUsed, UsageNone, noPosition);
    if (sessionData.browsingStack.top==NULL) {
        pushEmptySession(&sessionData.browsingStack);
        tmpc = sessionData.browsingStack.top;
    }

    assert(sessionData.browsingStack.top);
    BrowserMenu *oldMenu = sessionData.browsingStack.top->menu;
    sessionData.browsingStack.top->menu = &menu;
    olMacro2PassFile = NO_FILE_NUMBER;
    scanForMacroUsage(completionStringInMacroBody);
    sessionData.browsingStack.top->menu = oldMenu;
    if (tmpc!=NULL) {
        deleteEntryFromSessionStack(tmpc);
    }
    log_debug(":scheduling file '%s'", getFileItemWithFileNumber(olMacro2PassFile)->name);
    return olMacro2PassFile;
}

// WTF does "DependingStatics" mean?
static char *presetEditServerFileDependingStatics(void) {
    fileProcessingStartTime = time(NULL);

    primaryStartPosition = noPosition;
    staticPrefixStartPosition = noPosition;

    // This is pretty stupid, there is always only one input file
    // in edit server, otherwise it is an error
    int fileNumber = 0;
    inputFileName = getNextScheduledFile(&fileNumber);
    if (inputFileName == NULL) { /* No more input files... */
        // conservative message, probably macro invoked on nonsaved file, TODO: WTF?
        originalCommandLineFileNumber = NO_FILE_NUMBER;
        return NULL;
    }

    /* TODO: This seems strange, we only assert that the first file is scheduled to process.
       Then reset all other files, why? */
    assert(getFileItemWithFileNumber(fileNumber)->isScheduled);
    for (int i=getNextExistingFileNumber(fileNumber+1); i != -1; i = getNextExistingFileNumber(i+1)) {
        getFileItemWithFileNumber(i)->isScheduled = false;
    }

    originalCommandLineFileNumber = fileNumber;

    char *fileName = inputFileName;
    currentLanguage = getLanguageFor(fileName);

    // O.K. just to be sure, there is no other input file
    return fileName;
}

static void closeInputFile(void) {
    if (currentFile.characterBuffer.file != stdin) {
        closeCharacterBuffer(&currentFile.characterBuffer);
    }
}

static void parseInputFile(void) {
    if (options.fileTrace)
        fprintf(stderr, "parseInputFile: '%s\n", currentFile.fileName);
    if (options.serverOperation != OLO_TAG_SEARCH && options.serverOperation != OLO_PUSH_NAME) {
        log_debug("parse start");
        parseCurrentInputFile(currentLanguage);
        log_debug("parse end");
    } else
        log_debug("Not parsing input because of server operation TAG_SEARCH or PUSH_NAME");
    closeInputFile();
}

void initServer(ArgumentsVector args, int nargc, char **nargv) {
    clearAvailableRefactorings();
    processOptions(args, nargc, nargv, PROCESS_FILE_ARGUMENTS); /* no include or define options */
    processFileArguments();
    initCompletions(&collectedCompletions, 0, noPosition);
}

static void singlePass(int argc, char **argv,
                       int nargc, char **nargv,
                       bool *firstPassP
) {
    bool inputOpened = false;

    inputOpened = initializeFileProcessing(firstPassP, argc, argv, nargc, nargv, &currentLanguage);

    smartReadReferences();
    originalFileNumber = inputFileNumber;

    if (inputOpened) {
        /* If the file has preloaded content, remove old references before parsing */
        EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(inputFileName);
        if (buffer != NULL && buffer->preLoadedFromFile != NULL) {
            log_debug("file has preloaded content, removing old references for file %d", inputFileNumber);
            removeReferenceableItemsForFile(inputFileNumber);
        }

        parseInputFile();
        *firstPassP = false;
    }
    if (options.olCursorOffset==0) {
        // special case, push the file as include reference
        if (requiresCreatingRefs(options.serverOperation)) {
            Position position = makePosition(inputFileNumber, 1, 0);
            gotOnLineCxRefs(position);
        }
        addFileAsIncludeReference(inputFileNumber);
    }
    if (completionPositionFound && !completionStringServed) {
        // on-line action with cursor in an un-used macro body ???
        int ol2procfile = scheduleFileUsingTheMacro();
        if (ol2procfile!=NO_FILE_NUMBER) {
            inputFileName = getFileItemWithFileNumber(ol2procfile)->name;
            inputOpened = initializeFileProcessing(firstPassP, argc, argv, nargc, nargv, &currentLanguage);
            if (inputOpened) {
                parseInputFile();
                *firstPassP = false;
            }
        }
    }
}

static void processFile(int argc, char **argv,
                        int nargc, char **nargv,
                        bool *firstPassP
) {
    FileItem *fileItem = getFileItemWithFileNumber(originalCommandLineFileNumber);

    assert(fileItem->isScheduled);
    maxPasses = 1;
    for (currentPass=1; currentPass<=maxPasses; currentPass++) {
        inputFileName = fileItem->name;
        assert(inputFileName!=NULL);
        singlePass(argc, argv, nargc, nargv, firstPassP);
        if (options.serverOperation==OLO_EXTRACT || (completionStringServed && !requiresCreatingRefs(options.serverOperation)))
            break;
    }
    fileItem->isScheduled = false;
}

static void reparseFile(int argc, char **argv, int nargc, char **nargv, bool *firstPassP,
                        FileItem *fileItem) {
    inputFileName = fileItem->name;
    currentLanguage = getLanguageFor(inputFileName);
    bool inputOpened = initializeFileProcessing(firstPassP, argc, argv, nargc, nargv, &currentLanguage);
    if (inputOpened) {
        parseInputFile();
        fileItem->lastParsedMtime = editorFileModificationTime(fileItem->name);
        *firstPassP = false;
    }
}

static bool fileModifiedSinceLastParse(FileItem *fileItem) {
    return editorFileModificationTime(fileItem->name) != fileItem->lastParsedMtime;
}

static void processModifiedFilesForNavigation(int argc, char **argv,
                                              int nargc, char **nargv,
                                              bool *firstPassP) {
    /* Check which files with buffers have been modified.
     * For each modified file, reparse it to update references in the referenceableItemTable.
     * Then rebuild the current session's reference list. */

    ENTER();

    /* Iterate through all open editor buffers and check if they're modified */
    bool anyModified = false;
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i+1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);

        if (buffer != NULL) {
            if (fileModifiedSinceLastParse(fileItem)) {
                log_debug("File %s has been modified, reparsing...", fileItem->name);

                removeReferenceableItemsForFile(i);

                /* Save current state */
                char *savedInputFileName = inputFileName;
                int savedInputFileNumber = inputFileNumber;
                int savedOriginalFileNumber = originalFileNumber;
                Language savedLanguage = currentLanguage;

                reparseFile(argc, argv, nargc, nargv, firstPassP, fileItem);

                /* Restore state */
                inputFileName = savedInputFileName;
                inputFileNumber = savedInputFileNumber;
                originalFileNumber = savedOriginalFileNumber;
                currentLanguage = savedLanguage;

                anyModified = true;
            }
        }
    }

    /* If any files were modified, mark all sessions in the stack as needing refresh */
    if (anyModified) {
        for (SessionStackEntry *entry = sessionData.browsingStack.root; entry != NULL; entry = entry->previous) {
            entry->needsRefresh = true;
        }
    }

    /* Check if the current session needs refresh (either because of modifications or the flag) */
    if (sessionData.browsingStack.top != NULL &&
        sessionData.browsingStack.top->needsRefresh &&
        sessionData.browsingStack.top->menu != NULL) {
        log_debug("Updating menu referenceables and recomputing session references");

        /* Find the current reference's index in the list before recomputing */
        int currentIndex = 0;
        if (sessionData.browsingStack.top->current != NULL) {
            Reference *ref = sessionData.browsingStack.top->references;
            while (ref != NULL && ref != sessionData.browsingStack.top->current) {
                currentIndex++;
                ref = ref->next;
            }
        }

        /* Find which reference (by index) matches the callerPosition, so we can update it */
        int callerIndex = -1;
        if (sessionData.browsingStack.top->callerPosition.file != NO_FILE_NUMBER) {
            Reference *ref = sessionData.browsingStack.top->references;
            int index = 0;
            while (ref != NULL) {
                if (positionsAreEqual(ref->position, sessionData.browsingStack.top->callerPosition)) {
                    callerIndex = index;
                    break;
                }
                index++;
                ref = ref->next;
            }
        }

        /* Free the old session reference list since it has stale positions */
        freeReferences(sessionData.browsingStack.top->references);
        sessionData.browsingStack.top->references = NULL;

        /* Rebuild the reference list directly from the updated referenceableItemTable */
        for (BrowserMenu *menu = sessionData.browsingStack.top->menu; menu != NULL; menu = menu->next) {
            if (menu->selected) {
                ReferenceableItem *updatedItem = NULL;
                if (isMemberInReferenceableItemTable(&menu->referenceable, NULL, &updatedItem)) {
                    if (updatedItem != NULL) {
                        /* Add references from the updated item in the table (this copies them) */
                        addReferencesFromFileToList(updatedItem->references, ANY_FILE,
                                                   &sessionData.browsingStack.top->references);
                    }
                }
            }
        }

        /* Restore current to the same index in the new list (or the last reference if list is shorter) */
        if (sessionData.browsingStack.top->references != NULL) {
            Reference *ref = sessionData.browsingStack.top->references;
            int index = 0;
            while (ref->next != NULL && index < currentIndex) {
                ref = ref->next;
                index++;
            }
            sessionData.browsingStack.top->current = ref;
        }

        /* Update callerPosition to the same index in the new list if it was found in the old list */
        if (callerIndex >= 0 && sessionData.browsingStack.top->references != NULL) {
            Reference *ref = sessionData.browsingStack.top->references;
            int index = 0;
            while (ref != NULL && index < callerIndex) {
                ref = ref->next;
                index++;
            }
            if (ref != NULL) {
                log_debug("Updating callerPosition from %d:%d to %d:%d",
                         sessionData.browsingStack.top->callerPosition.line,
                         sessionData.browsingStack.top->callerPosition.col,
                         ref->position.line, ref->position.col);
                sessionData.browsingStack.top->callerPosition = ref->position;
            }
        }

        /* Clear the needsRefresh flag now that we've rebuilt this session */
        sessionData.browsingStack.top->needsRefresh = false;
    }
    LEAVE();
}

void callServer(int argc, char **argv, int nargc, char **nargv, bool *firstPass) {
    ENTER();

    loadAllOpenedEditorBuffers();

    /* For navigation operations (NEXT/PREVIOUS/POP), reparse any modified files
     * and update the current session's references before navigating */
    if (options.serverOperation == OLO_NEXT || options.serverOperation == OLO_PREVIOUS ||
        options.serverOperation == OLO_POP || options.serverOperation == OLO_POP_ONLY) {
        processModifiedFilesForNavigation(argc, argv, nargc, nargv, firstPass);
    }

    if (requiresCreatingRefs(options.serverOperation))
        pushEmptySession(&sessionData.browsingStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (presetEditServerFileDependingStatics() == NULL) {
            errorMessage(ERR_ST, "No input file");
        } else {
            processFile(argc, argv, nargc, nargv, firstPass);
        }
    } else {
        if (presetEditServerFileDependingStatics() != NULL) {
            getFileItemWithFileNumber(originalCommandLineFileNumber)->isScheduled = false;
            // added [26.12.2002] because of loading options without input file
            inputFileName = NULL;
        }
    }
    LEAVE();
}

void server(ArgumentsVector args) {
    ArgumentsVector pipedOptions;
    bool firstPass;

    ENTER();
    cxResizingBlocked = true;
    firstPass = true;
    deepCopyOptionsFromTo(&options, &savedOptions);
    for(;;) {
        currentPass = ANY_PASS;
        deepCopyOptionsFromTo(&savedOptions, &options);

        pipedOptions = getPipedOptions();
        // -o option on command line should catch also file not found
        openOutputFile(options.outputFileName);
        //&dumpArguments(nargc, nargv);
        log_trace("Server: Getting request");
        initServer(pipedOptions, pipedOptions.argc, pipedOptions.argv);
        if (outputFile==stdout && options.outputFileName!=NULL) {
            openOutputFile(options.outputFileName);
        }
        callServer(args.argc, args.argv, pipedOptions.argc, pipedOptions.argv, &firstPass);
        if (options.serverOperation == OLO_ABOUT) {
            aboutMessage();
        } else {
            answerEditAction();
        }
        //& options.outputFileName = NULL;  // why this was here ???
        //editorCloseBufferIfNotUsedElsewhere(s_input_file_name);
        closeAllEditorBuffers();
        closeOutputFile();
        if (options.xref2)
            ppcSynchronize();
        log_trace("Server: Request answered");
    }
    LEAVE();
}
