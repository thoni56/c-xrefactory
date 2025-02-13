#include "server.h"

#include "caching.h"
#include "commons.h"
#include "complete.h"
#include "cxfile.h"
#include "cxref.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "lexer.h"
#include "log.h"
#include "main.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "ppc.h"
#include "session.h"
#include "yylex.h"


const char *operationNamesTable[] = {
    "OLO_NONE",
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};



static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OLO_COMPLETION
           || operation==OLO_SEARCH
           || operation==OLO_EXTRACT
           || operation==OLO_TAG_SEARCH
           || operation==OLO_SET_MOVE_FUNCTION_TARGET
           || operation==OLO_GET_METHOD_COORD
           || operation==OLO_GET_ENV_VALUE
           || requiresCreatingRefs(operation)
        ;
}


static bool symbolCanBeIdentifiedByPosition(int fileNumber) {
    int line,col;

    // there is a serious problem with options memory for options got from
    // the .c-xrefrc file. so for the moment this will not work.
    // which problem ??????
    // seems that those options are somewhere in ppmMemory overwritten?
    //&return 0;
    if (!requiresCreatingRefs(options.serverOperation))
        return false;
    if (options.browsedSymName == NULL)
        return false;
    log_trace("looking for sym %s on %s", options.browsedSymName, options.olcxlccursor);

    // here I will need also the symbol name
    // do not bypass commanline entered files, because of local symbols
    // and because references from currently procesed file would
    // be not loaded from the TAG file (it expects they are loaded
    // by parsing).
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    log_trace("commandLineEntered %s == %d", fileItem->name, fileItem->isArgument);
    if (fileItem->isArgument)
        return false;

    // if references are not updated do not search it here
    // there were fullUpdate time? why?
    log_trace("checking last modified: %d, update: %d", fileItem->lastModified, fileItem->lastUpdateMtime);
    if (fileItem->lastModified != fileItem->lastUpdateMtime)
        return false;

    // here read one reference file looking for the refs
    // assume s_opt.olcxlccursor is correctly set;
    getLineAndColumnCursorPositionFromCommandLineOptions(&line, &col);
    olcxByPassPos = makePosition(fileNumber, line, col);
    olSetCallerPosition(olcxByPassPos);
    scanForBypass(options.browsedSymName);

    // if no symbol found, it may be a local symbol, try by parsing
    log_trace("checking that %d != NULL", sessionData.browserStack.top->hkSelectedSym);
    if (sessionData.browserStack.top->hkSelectedSym == NULL)
        return false;

    // here I should set caching to 1 and recover the cachePoint ???
    // yes, because last file references are still stored, even if I
    // update the cxref file, so do it only if switching file?
    // but how to ensure that next pass will start parsing?
    // By recovering of point 0 handled as such a special case.
    recoverCachePointZero();
    log_trace("yes, it can be identified by position");

    return true;
}

static int scheduleFileUsingTheMacro(void) {
    OlcxReferences *tmpc;

    assert(completionStringInMacroBody);
    tmpc = NULL;
    ReferenceItem references = makeReferenceItem(completionStringInMacroBody, TypeMacro, StorageExtern,
                                                 GlobalScope, GlobalVisibility, NO_FILE_NUMBER);

    SymbolsMenu menu = makeSymbolsMenu(references, 1, true, 0, UsageUsed, 0, UsageNone, noPosition);
    if (sessionData.browserStack.top==NULL) {
        pushEmptySession(&sessionData.browserStack);
        tmpc = sessionData.browserStack.top;
    }

    assert(sessionData.browserStack.top);
    SymbolsMenu *oldMenu = sessionData.browserStack.top->symbolsMenu;
    sessionData.browserStack.top->symbolsMenu = &menu;
    olMacro2PassFile = NO_FILE_NUMBER;
    scanForMacroUsage(completionStringInMacroBody);
    sessionData.browserStack.top->symbolsMenu = oldMenu;
    if (tmpc!=NULL) {
        olStackDeleteSymbol(tmpc);
    }
    log_trace(":scheduling file '%s'", getFileItemWithFileNumber(olMacro2PassFile)->name);
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
        olOriginalComFileNumber = NO_FILE_NUMBER;
        return NULL;
    }

    /* TODO: This seems strange, we only assert that the first file is scheduled to process.
       Then reset all other files, why? */
    assert(getFileItemWithFileNumber(fileNumber)->isScheduled);
    for (int i=getNextExistingFileNumber(fileNumber+1); i != -1; i = getNextExistingFileNumber(i+1)) {
        getFileItemWithFileNumber(i)->isScheduled = false;
    }

    olOriginalComFileNumber = fileNumber;

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
    if (options.serverOperation != OLO_TAG_SEARCH && options.serverOperation != OLO_PUSH_NAME) {
        log_trace("parse start");
        parseCurrentInputFile(currentLanguage);
        log_trace("parse end");
    } else
        log_trace("Not parsing input because of server operation TAG_SEARCH or PUSH_NAME");
    closeInputFile();
}

void initServer(int nargc, char **nargv) {
    clearAvailableRefactorings();
    processOptions(nargc, nargv, PROCESS_FILE_ARGUMENTS); /* no include or define options */
    processFileArguments();
    if (options.serverOperation == OLO_EXTRACT)
        cache.index = 2; // !!!! no cache, TODO why is 2 = no cache?
    initCompletions(&collectedCompletions, 0, noPosition);
}

static void singlePass(int argc, char **argv,
                       int nargc, char **nargv,
                       bool *firstPassP
) {
    bool inputOpened = false;

    inputOpened = initializeFileProcessing(firstPassP, argc, argv, nargc, nargv, &currentLanguage);

    smartReadReferences();
    olOriginalFileNumber = inputFileNumber;
    if (symbolCanBeIdentifiedByPosition(inputFileNumber)) {
        if (inputOpened)
            closeInputFile();
        return;
    }
    if (inputOpened) {
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
    FileItem *fileItem = getFileItemWithFileNumber(olOriginalComFileNumber);

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

void callServer(int argc, char **argv, int nargc, char **nargv, bool *firstPass) {
    ENTER();

    loadAllOpenedEditorBuffers();

    if (requiresCreatingRefs(options.serverOperation))
        pushEmptySession(&sessionData.browserStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (presetEditServerFileDependingStatics() == NULL) {
            errorMessage(ERR_ST, "No input file");
        } else {
            processFile(argc, argv, nargc, nargv, firstPass);
        }
    } else {
        if (presetEditServerFileDependingStatics() != NULL) {
            getFileItemWithFileNumber(olOriginalComFileNumber)->isScheduled = false;
            // added [26.12.2002] because of loading options without input file
            inputFileName = NULL;
        }
    }
    LEAVE();
}

void server(int argc, char **argv) {
    int nargc;  char **nargv;
    bool firstPass;

    ENTER();
    cxResizingBlocked = true;
    firstPass = true;
    deepCopyOptionsFromTo(&options, &savedOptions);
    for(;;) {
        currentPass = ANY_PASS;
        deepCopyOptionsFromTo(&savedOptions, &options);
        getPipedOptions(&nargc, &nargv);
        // O.K. -o option given on command line should catch also file not found
        // message
        openOutputFile(options.outputFileName);
        //&dumpArguments(nargc, nargv);
        log_trace("Server: Getting request");
        initServer(nargc, nargv);
        if (outputFile==stdout && options.outputFileName!=NULL) {
            openOutputFile(options.outputFileName);
        }
        callServer(argc, argv, nargc, nargv, &firstPass);
        if (options.serverOperation == OLO_ABOUT) {
            aboutMessage();
        } else {
            answerEditAction();
        }
        //& options.outputFileName = NULL;  // why this was here ???
        //editorCloseBufferIfNotUsedElsewhere(s_input_file_name);
        closeAllEditorBuffers();
        closeOutputFile();
        if (options.serverOperation == OLO_EXTRACT)
            cache.index = 2; // !!!! no cache
        if (options.xref2)
            ppcSynchronize();
        log_trace("Server: Request answered");
    }
    LEAVE();
}
