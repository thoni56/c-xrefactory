#include "server.h"

#include "commons.h"
#include "complete.h"
#include "cxfile.h"
#include "cxref.h"
#include "editor.h"
#include "editorbuffer.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "parsing.h"
#include "ppc.h"
#include "referenceableitemtable.h"
#include "session.h"
#include "startup.h"
#include "yylex.h"


const char *operationNamesTable[] = {
    "OLO_NONE",
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};


static bool needsReferenceDatabase(ServerOperation operation) {
    return operation==OLO_PUSH
        ||  operation==OLO_PUSH_ONLY
        ||  operation==OLO_PUSH_AND_CALL_MACRO
        ||  operation==OLO_GOTO_PARAM_NAME
        ||  operation==OLO_GET_PARAM_COORDINATES
        ||  operation==OLO_GET_AVAILABLE_REFACTORINGS
        ||  operation==OLO_PUSH_NAME
        ||  operation==OLO_PUSH_FOR_LOCAL_MOTION
        ||  operation==OLO_GET_LAST_IMPORT_LINE
        ||  operation==OLO_GLOBAL_UNUSED
        ||  operation==OLO_LOCAL_UNUSED
        ||  operation==OLO_LIST
        ||  operation==OLO_RENAME
        ||  operation==OLO_ARGUMENT_MANIPULATION
        ||  operation==OLO_SAFETY_CHECK
        ||  operation==OLO_GET_FUNCTION_BOUNDS
        ;
}

static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OLO_COMPLETION
           || operation==OLO_EXTRACT
           || operation==OLO_TAG_SEARCH
           || operation==OLO_SET_MOVE_TARGET
           || operation==OLO_GET_FUNCTION_BOUNDS
           || operation==OLO_GET_ENV_VALUE
           || needsReferenceDatabase(operation)
        ;
}


static int scheduleFileUsingTheMacro(void) {
    SessionStackEntry *tmpc;

    assert(completionStringInMacroBody);
    tmpc = NULL;
    ReferenceableItem references = makeReferenceableItem(completionStringInMacroBody, TypeMacro, StorageExtern,
                                                         GlobalScope, VisibilityGlobal, NO_FILE_NUMBER);

    BrowserMenu menu = makeBrowserMenu(references, 1, true, 0, UsageUsed, UsageNone, NO_POSITION);
    if (sessionData.browsingStack.top==NULL) {
        pushEmptySession(&sessionData.browsingStack);
        tmpc = sessionData.browsingStack.top;
    }

    assert(sessionData.browsingStack.top);
    BrowserMenu *oldMenu = sessionData.browsingStack.top->hkSelectedSym;
    sessionData.browsingStack.top->hkSelectedSym = &menu;
    fileToParseForMacroExpansion = NO_FILE_NUMBER;
    scanForMacroUsage(completionStringInMacroBody);
    sessionData.browsingStack.top->hkSelectedSym = oldMenu;
    if (tmpc!=NULL) {
        deleteEntryFromSessionStack(tmpc);
    }
    log_debug(":scheduling file '%s'", getFileItemWithFileNumber(fileToParseForMacroExpansion)->name);
    return fileToParseForMacroExpansion;
}

/* Prepare a single input file for this request, maybe?
 *
 * TODO This function does something but the logic is seriously broken so it is
 * impossible to improve further until that mystery is sorted.
 */
static bool prepareInputFileForRequest(void) {
    fileProcessingStartTime = time(NULL);

    // Server mode: get a single scheduled file for this request
    // TODO: why is it picking the first scheduled in fileNumber order?
    int fileNumber = 0;
    if (getNextScheduledFile(&fileNumber) == NULL) { /* No more input files... */
        // No file scheduled - likely the operation doesn't need one, or error
        requestFileNumber = NO_FILE_NUMBER;
        return false;
    }

    assert(getFileItemWithFileNumber(fileNumber)->isScheduled);

    /* Ensure only this file is processed during this request */
    /* TODO: this is clearing all fileItems with a fileNumber above the one found
     * above. That just doesn't make sense... */
    for (int i=getNextExistingFileNumber(fileNumber+1); i != -1; i = getNextExistingFileNumber(i+1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (fileItem->isScheduled) {
            log_trace("Found unexpected scheduled file, '%s', unscheduling it.", fileItem->name);
        }
        fileItem->isScheduled = false;
    }

    requestFileNumber = fileNumber;
    return true;
}

static void closeInputFile(void) {
    if (currentFile.characterBuffer.file != stdin) {
        closeCharacterBuffer(&currentFile.characterBuffer);
    }
}

static void parseInputFile(void) {
    if (options.fileTrace)
        fprintf(stderr, "parseInputFile: '%s\n", currentFile.fileName);

    /* Bridge: Sync parsingConfig for all operations using this entry point */
    setupParsingConfig(requestFileNumber);

    if (options.serverOperation != OLO_TAG_SEARCH && options.serverOperation != OLO_PUSH_NAME) {
        log_debug("parse start");
        callParser(parsingConfig.fileNumber, parsingConfig.language);
        log_debug("parse end");
    } else
        log_debug("Not parsing input because of server operation TAG_SEARCH or PUSH_NAME");

    closeInputFile();
}

void initServer(ArgumentsVector args) {
    clearAvailableRefactorings();
    processOptions(args, PROCESS_FILE_ARGUMENTS_YES); /* no include or define options */
    processFileArguments();
    initCompletions(&collectedCompletions, 0, NO_POSITION);
}

static void singlePass(ArgumentsVector args, ArgumentsVector nargs, bool *firstPassP) {
    bool inputOpened = false;

    inputOpened = initializeFileProcessing(args, nargs, firstPassP);

    loadFileNumbersFromStore();
    parsingConfig.fileNumber = currentFile.characterBuffer.fileNumber;

    if (inputOpened) {
        /* If the file has preloaded content, remove old references before parsing */
        EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(inputFileName);
        if (buffer != NULL && buffer->preLoadedFromFile != NULL) {
            log_debug("file has preloaded content, removing old references for file %d",
                      parsingConfig.fileNumber);
            removeReferenceableItemsForFile(parsingConfig.fileNumber);
        }

        parseInputFile();
        *firstPassP = false;
    }
    if (options.cursorOffset==0) {
        // special case, push the file as include reference
        if (needsReferenceDatabase(options.serverOperation)) {
            Position position = makePosition(parsingConfig.fileNumber, 1, 0);
            parsingConfig.positionOfSelectedReference = position;
            addFileAsIncludeReference(parsingConfig.fileNumber);
        }
    }
    if (completionPositionFound && !completionStringServed) {
        // Cursor is on an identifier inside a macro body definition, which hasn't been
        // processed as a symbol yet. Find and parse a file where the macro is invoked
        // so the macro expansion will resolve the identifier as an actual symbol.
        int fileWithMacroExpansion = scheduleFileUsingTheMacro();
        if (fileWithMacroExpansion!=NO_FILE_NUMBER) {
            inputFileName = getFileItemWithFileNumber(fileWithMacroExpansion)->name;
            inputOpened = initializeFileProcessing(args, nargs, firstPassP);
            if (inputOpened) {
                parseInputFile();
                *firstPassP = false;
            }
        }
    }
}

static void processFile(ArgumentsVector baseArgs, ArgumentsVector requestArgs, bool *firstPassP) {
    FileItem *fileItem = getFileItemWithFileNumber(requestFileNumber);

    assert(fileItem->isScheduled);
    maxPasses = 1;
    for (currentPass=1; currentPass<=maxPasses; currentPass++) {
        inputFileName = fileItem->name;
        assert(inputFileName!=NULL);
        singlePass(baseArgs, requestArgs, firstPassP);
        if (options.serverOperation==OLO_EXTRACT || (completionStringServed && !needsReferenceDatabase(options.serverOperation)))
            break;
    }
    fileItem->isScheduled = false;
}

void callServer(ArgumentsVector baseArgs, ArgumentsVector requestArgs, bool *firstPass) {
    ENTER();

    loadAllOpenedEditorBuffers();

    if (needsReferenceDatabase(options.serverOperation))
        pushEmptySession(&sessionData.browsingStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (prepareInputFileForRequest()) {
            processFile(baseArgs, requestArgs, firstPass);
        } else {
            errorMessage(ERR_ST, "No input file");
        }
    } else {
        if (prepareInputFileForRequest()) {
            getFileItemWithFileNumber(requestFileNumber)->isScheduled = false;
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
        // TODO -o option on command line should catch also file not found
        openOutputFile(options.outputFileName);
        //&dumpArguments(nargc, nargv);

        log_trace("Server: Getting request");
        initServer(pipedOptions);
        if (outputFile==stdout && options.outputFileName!=NULL) {
            openOutputFile(options.outputFileName);
        }
        callServer(args, pipedOptions, &firstPass);
        if (options.serverOperation == OLO_ABOUT) {
            aboutMessage();
        } else {
            answerEditorAction();
        }

        closeAllEditorBuffers();
        closeOutputFile();
        if (options.xref2)
            ppcSynchronize();
        log_trace("Server: Request answered");
    }
    LEAVE();
}
