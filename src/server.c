#include "server.h"

#include "commons.h"
#include "complete.h"
#include "cxfile.h"
#include "cxref.h"
#include "editor.h"
#include "editorbuffer.h"
#include "editorbuffertable.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "misc.h"
#include "head.h"
#include "log.h"
#include "options.h"
#include "parsers.h"
#include "parsing.h"
#include "ppc.h"
#include "referenceableitemtable.h"
#include "session.h"
#include "navigation.h"
#include "startup.h"
#include "yylex.h"


const char *operationNamesTable[] = {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};


static bool needsReferenceDatabase(ServerOperation operation) {
    return operation==OP_BROWSE_PUSH
        ||  operation==OP_BROWSE_PUSH_ONLY
        ||  operation==OP_BROWSE_PUSH_AND_CALL_MACRO
        ||  operation==OP_INTERNAL_PARSE_TO_GOTO_PARAM_NAME
        ||  operation==OP_INTERNAL_PARSE_TO_GET_PARAM_COORDINATES
        ||  operation==OP_GET_AVAILABLE_REFACTORINGS
        ||  operation==OP_BROWSE_PUSH_NAME
        ||  operation==OP_INTERNAL_PUSH_FOR_USAGE_CHECK
        ||  operation==OP_UNUSED_GLOBAL
        ||  operation==OP_UNUSED_LOCAL
        ||  operation==OP_INTERNAL_LIST
        ||  operation==OP_INTERNAL_PUSH_FOR_RENAME
        ||  operation==OP_INTERNAL_PUSH_FOR_ARGUMENT_MANIPULATION
        ||  operation==OP_INTERNAL_SAFETY_CHECK
        ||  operation==OP_INTERNAL_GET_FUNCTION_BOUNDS
        ;
}

static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OP_COMPLETION
           || operation==OP_INTERNAL_PARSE_TO_EXTRACT
           || operation==OP_SEARCH
           || operation==OP_INTERNAL_PARSE_TO_SET_MOVE_TARGET
           || operation==OP_INTERNAL_GET_FUNCTION_BOUNDS
           || operation==OP_GET_ENV_VALUE
           || needsReferenceDatabase(operation)
        ;
}


static int scheduleFileUsingTheMacro(void) {
    SessionStackEntry *tmpc;

    assert(completionStringInMacroBody);
    tmpc = NULL;
    ReferenceableItem references = makeReferenceableItem(completionStringInMacroBody, TypeMacro, StorageExtern,
                                                         GlobalScope, VisibilityGlobal, NO_FILE_NUMBER);

    BrowsingMenu menu = makeBrowsingMenu(references, 1, true, 0, UsageUsed, UsageNone, NO_POSITION);
    if (sessionData.browsingStack.top==NULL) {
        pushEmptySession(&sessionData.browsingStack);
        tmpc = sessionData.browsingStack.top;
    }

    assert(sessionData.browsingStack.top);
    BrowsingMenu *oldMenu = sessionData.browsingStack.top->hkSelectedSym;
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

    if (options.serverOperation != OP_SEARCH && options.serverOperation != OP_BROWSE_PUSH_NAME) {
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

static void singlePass(ArgumentsVector args, ArgumentsVector nargs) {
    bool inputOpened = false;

    inputOpened = initializeFileProcessing(args, nargs);

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

        /* Mark file as freshly parsed so navigation doesn't think it's stale.
         * But NOT for refactoring operations (RENAME, ARGUMENT_MANIPULATION, SAFETY_CHECK)
         * because those trigger xref updates that use lastParsedMtime to decide
         * whether to re-index - and xref reads from disk, not the preload buffer.
         */
        if (buffer != NULL && buffer->preLoadedFromFile != NULL
            && options.serverOperation != OP_INTERNAL_PUSH_FOR_RENAME
            && options.serverOperation != OP_INTERNAL_PUSH_FOR_ARGUMENT_MANIPULATION
            && options.serverOperation != OP_INTERNAL_SAFETY_CHECK) {
            FileItem *fileItem = getFileItemWithFileNumber(parsingConfig.fileNumber);
            fileItem->lastParsedMtime = buffer->modificationTime;
        }
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
            inputOpened = initializeFileProcessing(args, nargs);
            if (inputOpened) {
                parseInputFile();
            }
        }
    }
}

static void processFile(ArgumentsVector baseArgs, ArgumentsVector requestArgs) {
    FileItem *fileItem = getFileItemWithFileNumber(requestFileNumber);

    assert(fileItem->isScheduled);
    maxPasses = 1;
    for (currentPass=1; currentPass<=maxPasses; currentPass++) {
        inputFileName = fileItem->name;
        assert(inputFileName!=NULL);
        singlePass(baseArgs, requestArgs);
        if (options.serverOperation==OP_INTERNAL_PARSE_TO_EXTRACT || (completionStringServed && !needsReferenceDatabase(options.serverOperation)))
            break;
    }
    fileItem->isScheduled = false;
}

#define MAX_INCLUDE_WALK_FILES 256
#define MAX_CUS_TO_REPARSE 128

/* Walk the reverse-include graph from a stale header up to compilation units,
 * using TypeCppInclude references in the reference table (populated by prior
 * parsing or loaded from disk db). Reparse those CUs so they pick up the
 * modified header content. */
static void reparseStaleHeaderIncluders(int headerFileNumber) {
    FileItem *headerItem = getFileItemWithFileNumber(headerFileNumber);
    log_debug("Looking for CUs that include stale header '%s'", headerItem->name);

    ensureReferencesAreLoadedFor(LINK_NAME_INCLUDE_REFS);

    /* Walk reverse-include graph transitively: starting from the stale header,
     * find all files that include it, then files that include those, etc.
     * Collect any CUs encountered along the way. */
    int filesToWalk[MAX_INCLUDE_WALK_FILES];
    int walkCount = 1;
    filesToWalk[0] = headerFileNumber;

    int cuFileNumbers[MAX_CUS_TO_REPARSE];
    int cuCount = 0;

    for (int i = 0; i < walkCount; i++) {
        ReferenceableItem searchItem = makeReferenceableItem(
            LINK_NAME_INCLUDE_REFS, TypeCppInclude, StorageExtern,
            GlobalScope, VisibilityGlobal, filesToWalk[i]);

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
            if (isCompilationUnit(includer->name) && cuCount < MAX_CUS_TO_REPARSE) {
                cuFileNumbers[cuCount++] = includerFileNum;
                log_debug("CU '%s' (transitively) includes stale header '%s'",
                          includer->name, headerItem->name);
            }
        }
    }

    if (cuCount == 0) {
        log_debug("No CUs found that include '%s'", headerItem->name);
        return;
    }

    removeReferenceableItemsForFile(headerFileNumber);

    for (int i = 0; i < cuCount; i++) {
        log_debug("Reparsing CU file number %d because of stale header '%s'",
                  cuFileNumbers[i], headerItem->name);
        reparseStaleFile(cuFileNumbers[i]);
        getFileItemWithFileNumber(cuFileNumbers[i])->needsBrowsingStackRefresh = true;
    }
}

void callServer(ArgumentsVector baseArgs, ArgumentsVector requestArgs) {
    static bool projectContextInitialized = false;

    ENTER();

    loadAllOpenedEditorBuffers();

    /* Reparse any stale preloaded files before dispatching the operation,
     * so all operations see fresh in-memory references.
     * Clear cursorOffset so the lexer doesn't set positionOfSelectedReference
     * during the reparse — that would trigger on-line action handling
     * (browsing stack assertions) before the operation has set things up. */
    if (projectContextInitialized) {
        int savedCursorOffset = options.cursorOffset;
        options.cursorOffset = -1;
        for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
            for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
                int fileNumber = l->buffer->fileNumber;
                FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
                if (fileNumberIsStale(fileNumber) && isCompilationUnit(fileItem->name)) {
                    reparseStaleFile(fileNumber);
                    EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);
                    if (buffer != NULL)
                        fileItem->lastParsedMtime = buffer->modificationTime;
                    fileItem->needsBrowsingStackRefresh = true;
                }
            }
        }

        /* Pass 2: For stale headers, find CUs that include them and reparse.
         * Must come after Pass 1: Pass 1 reparses stale CUs, refreshing their
         * TypeCppInclude references. Pass 2 queries those references to find
         * which CUs include the stale header (transitively). */
        for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
            for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
                int fileNumber = l->buffer->fileNumber;
                FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
                if (fileNumberIsStale(fileNumber) && !isCompilationUnit(fileItem->name)) {
                    reparseStaleHeaderIncluders(fileNumber);
                    EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);
                    if (buffer != NULL)
                        fileItem->lastParsedMtime = buffer->modificationTime;
                    fileItem->needsBrowsingStackRefresh = true;
                }
            }
        }

        options.cursorOffset = savedCursorOffset;
    }

    if (needsReferenceDatabase(options.serverOperation))
        pushEmptySession(&sessionData.browsingStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (prepareInputFileForRequest()) {
            processFile(baseArgs, requestArgs);
        } else {
            errorMessage(ERR_ST, "No input file");
        }
    } else {
        if (prepareInputFileForRequest()) {
            if (options.serverOperation == OP_ACTIVE_PROJECT && !projectContextInitialized) {
                initializeProjectContext(getFileItemWithFileNumber(requestFileNumber)->name, baseArgs, requestArgs);
                processFileArguments();
                projectContextInitialized = true;
            }
            getFileItemWithFileNumber(requestFileNumber)->isScheduled = false;
            inputFileName = NULL;
        }
    }
    LEAVE();
}

void server(ArgumentsVector args) {
    ArgumentsVector pipedOptions;

    ENTER();
    cxResizingBlocked = true;

    deepCopyOptionsFromTo(&options, &savedOptions);
    for(;;) {
        currentPass = ANY_PASS;
        deepCopyOptionsFromTo(&savedOptions, &options);

        pipedOptions = readOptionsFromPipe();
        // TODO -o option on command line should catch also file not found
        openOutputFile(options.outputFileName);
        //&dumpArguments(nargc, nargv);

        log_trace("Server: Getting request");
        initServer(pipedOptions);
        if (outputFile==stdout && options.outputFileName!=NULL) {
            openOutputFile(options.outputFileName);
        }
        callServer(args, pipedOptions);
        if (options.serverOperation == OP_ABOUT) {
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
