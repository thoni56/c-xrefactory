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
#include "yylex.h"

const char *operationNamesTable[] = {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};



static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OLO_COMPLETION
           || operation==OLO_SEARCH
           || operation==OLO_EXTRACT
           || operation==OLO_TAG_SEARCH
           || operation==OLO_SET_MOVE_TARGET
           || operation==OLO_SET_MOVE_CLASS_TARGET
           || operation==OLO_SET_MOVE_METHOD_TARGET
           || operation==OLO_GET_CURRENT_CLASS
           || operation==OLO_GET_CURRENT_SUPER_CLASS
           || operation==OLO_GET_METHOD_COORD
           || operation==OLO_GET_CLASS_COORD
           || operation==OLO_GET_ENV_VALUE
           || isCreatingRefs(operation)
        ;
}


static bool symbolCanBeIdentifiedByPosition(int fileIndex) {
    int line,col;

    // there is a serious problem with options memory for options got from
    // the .c-xrefrc file. so for the moment this will not work.
    // which problem ??????
    // seems that those options are somewhere in ppmMemory overwritten?
    //&return 0;
    if (!isCreatingRefs(options.serverOperation))
        return false;
    if (options.browsedSymName == NULL)
        return false;
    log_trace("looking for sym %s on %s", options.browsedSymName, options.olcxlccursor);

    // modified file, can't identify the reference
    log_trace(":modif flag == %d", options.modifiedFlag);
    if (options.modifiedFlag)
        return false;

    // here I will need also the symbol name
    // do not bypass commanline entered files, because of local symbols
    // and because references from currently procesed file would
    // be not loaded from the TAG file (it expects they are loaded
    // by parsing).
    FileItem *fileItem = getFileItem(fileIndex);
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
    s_olcxByPassPos = makePosition(fileIndex, line, col);
    olSetCallerPosition(&s_olcxByPassPos);
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
    ReferencesItem  references;
    SymbolsMenu     menu, *oldMenu;
    OlcxReferences *tmpc;

    assert(s_olstringInMbody);
    tmpc = NULL;
    fillReferencesItem(&references, s_olstringInMbody,
                       cxFileHashNumber(s_olstringInMbody),
                       noFileIndex, noFileIndex, TypeMacro, StorageExtern,
                       ScopeGlobal, AccessDefault, CategoryGlobal);

    fillSymbolsMenu(&menu, references, 1, true, 0, UsageUsed, 0, 0, 0, UsageNone, noPosition, 0, NULL, NULL);
    if (sessionData.browserStack.top==NULL) {
        olcxPushEmptyStackItem(&sessionData.browserStack);
        tmpc = sessionData.browserStack.top;
    }
    assert(sessionData.browserStack.top);
    oldMenu = sessionData.browserStack.top->menuSym;
    sessionData.browserStack.top->menuSym = &menu;
    s_olMacro2PassFile = noFileIndex;
    scanForMacroUsage(s_olstringInMbody);
    sessionData.browserStack.top->menuSym = oldMenu;
    if (tmpc!=NULL) {
        olStackDeleteSymbol(tmpc);
    }
    log_trace(":scheduling file '%s'", getFileItem(s_olMacro2PassFile)->name);
    if (s_olMacro2PassFile == noFileIndex)
        return noFileIndex;
    return s_olMacro2PassFile;
}

// WTF does "DependingStatics" mean?
static char *presetEditServerFileDependingStatics(void) {
    fileProcessingStartTime = time(NULL);

    s_primaryStartPosition = noPosition;
    s_staticPrefixStartPosition = noPosition;

    // This is pretty stupid, there is always only one input file
    // in edit server, otherwise it is an error
    int fileIndex = 0;
    inputFileName = getNextScheduledFile(&fileIndex);
    if (fileIndex == -1) { /* No more input files... */
        // conservative message, probably macro invoked on nonsaved file, TODO: WTF?
        olOriginalComFileNumber = noFileIndex;
        return NULL;
    }

    /* TODO: This seems strange, we only assert that the first file is scheduled to process.
       Then reset all other files, why? */
    assert(getFileItem(fileIndex)->isScheduled);
    for (int i=getNextExistingFileIndex(fileIndex+1); i != -1; i = getNextExistingFileIndex(i+1)) {
        getFileItem(i)->isScheduled = false;
    }

    olOriginalComFileNumber = fileIndex;

    char *fileName = inputFileName;
    currentLanguage = getLanguageFor(fileName);

    // O.K. just to be sure, there is no other input file
    return fileName;
}

static void closeInputFile(void) {
    if (currentFile.lexBuffer.buffer.file!=stdin) {
        closeCharacterBuffer(&currentFile.lexBuffer.buffer);
    }
}

static void parseInputFile(bool *firstPassP) {
    if (options.serverOperation != OLO_TAG_SEARCH && options.serverOperation != OLO_PUSH_NAME) {
        log_trace("parse start");
        recoverFromCache();
        parseCurrentInputFile(currentLanguage);
        log_trace("parse end");
    } else
        log_trace("Not parsing input because of server operation TAG_SEARCH or PUSH_NAME");
    currentFile.lexBuffer.buffer.isAtEOF = false; /* Why? */
    closeInputFile();
}

void initServer(int nargc, char **nargv) {
    clearAvailableRefactorings();
    options.classpath = "";
    processOptions(nargc, nargv, PROCESS_FILE_ARGUMENTS); /* no include or define options */
    processFileArguments();
    if (options.serverOperation == OLO_EXTRACT)
        cache.cpIndex = 2; // !!!! no cache, TODO why is 2 = no cache?
    initCompletions(&collectedCompletions, 0, noPosition);
}

void singlePass(int argc, char **argv,
                int nargc, char **nargv,
                bool *firstPassP
) {
    bool inputOpened = false;

    inputOpened = initializeFileProcessing(firstPassP, argc, argv, nargc, nargv, &currentLanguage);

    smartReadReferences();
    olOriginalFileIndex = inputFileNumber;
    if (symbolCanBeIdentifiedByPosition(inputFileNumber)) {
        if (inputOpened)
            closeInputFile();
        return;
    }
    if (inputOpened) {
        parseInputFile(firstPassP);
        *firstPassP = false;
    }
    if (options.olCursorPos==0 && !LANGUAGE(LANG_JAVA)) {
        // special case, push the file as include reference
        if (isCreatingRefs(options.serverOperation)) {
            Position position = makePosition(inputFileNumber, 1, 0);
            gotOnLineCxRefs(&position);
        }
        addThisFileDefineIncludeReference(inputFileNumber);
    }
    if (s_olstringFound && !s_olstringServed) {
        // on-line action with cursor in an un-used macro body ???
        int ol2procfile = scheduleFileUsingTheMacro();
        if (ol2procfile!=noFileIndex) {
            inputFileName = getFileItem(ol2procfile)->name;
            inputOpened = false;
            inputOpened = initializeFileProcessing(firstPassP, argc, argv, nargc, nargv, &currentLanguage);
            if (inputOpened) {
                parseInputFile(firstPassP);
                *firstPassP = false;
            }
        }
    }
}

static void processFile(int argc, char **argv,
                        int nargc, char **nargv,
                        bool *firstPassP
) {
    FileItem *fileItem = getFileItem(olOriginalComFileNumber);

    assert(fileItem->isScheduled);
    maxPasses = 1;
    for (currentPass=1; currentPass<=maxPasses; currentPass++) {
        inputFileName = fileItem->name;
        assert(inputFileName!=NULL);
        singlePass(argc, argv, nargc, nargv, firstPassP);
        if (options.serverOperation==OLO_EXTRACT || (s_olstringServed && !isCreatingRefs(options.serverOperation)))
            break;
        if (LANGUAGE(LANG_JAVA))
            break;
    }
    fileItem->isScheduled = false;
}

void callServer(int argc, char **argv, int nargc, char **nargv, bool *firstPass) {
    ENTER();

    editorLoadAllOpenedBufferFiles();

    if (isCreatingRefs(options.serverOperation))
        olcxPushEmptyStackItem(&sessionData.browserStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (presetEditServerFileDependingStatics() == NULL) {
            errorMessage(ERR_ST, "No input file");
        } else {
            processFile(argc, argv, nargc, nargv, firstPass);
        }
    } else {
        if (presetEditServerFileDependingStatics() != NULL) {
            getFileItem(olOriginalComFileNumber)->isScheduled = false;
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
    copyOptionsFromTo(&options, &savedOptions);
    for(;;) {
        currentPass = ANY_PASS;
        copyOptionsFromTo(&savedOptions, &options);
        getPipedOptions(&nargc, &nargv);
        // O.K. -o option given on command line should catch also file not found
        // message
        mainOpenOutputFile(options.outputFileName);
        //&dumpOptions(nargc, nargv);
        log_trace("Server: Getting request");
        initServer(nargc, nargv);
        if (communicationChannel==stdout && options.outputFileName!=NULL) {
            mainOpenOutputFile(options.outputFileName);
        }
        callServer(argc, argv, nargc, nargv, &firstPass);
        if (options.serverOperation == OLO_ABOUT) {
            aboutMessage();
        } else {
            mainAnswerEditAction();
        }
        //& options.outputFileName = NULL;  // why this was here ???
        //editorCloseBufferIfNotUsedElsewhere(s_input_file_name);
        editorCloseAllBuffers();
        closeMainOutputFile();
        if (options.serverOperation == OLO_EXTRACT)
            cache.cpIndex = 2; // !!!! no cache
        if (options.xref2)
            ppcSynchronize();
        log_trace("Server: Request answered");
    }
    LEAVE();
}
