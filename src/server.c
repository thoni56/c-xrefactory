#include "server.h"

#include <string.h>

#include "commons.h"
#include "complete.h"
#include "cxfile.h"
#include "cxref.h"
#include "editor.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "freshness.h"
#include "globals.h"
#include "head.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "parsing.h"
#include "ppc.h"
#include "progress.h"
#include "projectstructure.h"
#include "session.h"
#include "startup.h"
#include "timestamp.h"
#include "yylex.h"
#include "rebuild.h"
#include "referenceableitemtable.h"


const char *operationNamesTable[] = {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};


/* Once-per-process flag: true after the first request's project context
 * has been fully initialized (savedOptions synced, snapshot loaded).
 * Promoted from function-local static to module scope so cxref.c's
 * handleProject can mark it true after locking via the GetProject path
 * - otherwise the next request's gate would re-detect from a different
 * file and clobber the lock. See markProjectContextInitialized() below. */
static bool projectContextInitialized = false;

void markProjectContextInitialized(void) {
    projectContextInitialized = true;
}


static bool opensBrowsingSession(ServerOperation operation) {
    return operation==OP_BROWSE_PUSH
        ||  operation==OP_BROWSE_PUSH_ONLY
        ||  operation==OP_BROWSE_PUSH_AND_CALL_MACRO
        ||  operation==OP_GET_AVAILABLE_REFACTORINGS
        ||  operation==OP_BROWSE_PUSH_NAME
        ||  operation==OP_UNUSED_IN_PROJECT
        ||  operation==OP_UNUSED_IN_FILE
        ||  operation==OP_INTERNAL_LIST
        ||  operation==OP_REFACTORY
        ;
}

/* Name-based operations: the request names a symbol, not a position, so the
   name can live in any compilation unit and all of them must be parsed. */
static bool needsWholeProjectParsed(ServerOperation operation) {
    return operation==OP_SEARCH
        ||  operation==OP_UNUSED_IN_PROJECT
        ||  operation==OP_BROWSE_PUSH_NAME
        ;
}

static const char *parseAllPromptFor(ServerOperation operation) {
    switch (operation) {
    case OP_SEARCH:           return "searching";
    case OP_UNUSED_IN_PROJECT:    return "checking for unused";
    case OP_BROWSE_PUSH_NAME: return "browsing";
    default:                  assert(0); return "";
    }
}

static bool requiresProcessingInputFile(ServerOperation operation) {
    return operation==OP_COMPLETION
           || operation==OP_SEARCH
           || opensBrowsingSession(operation)
        ;
}

/* Requests about the project rather than its code. They are answered from the config and
   the scan, so entry refresh has nothing to do for them (Setup Ladder: opening a project
   parses nothing). */
static bool answeredWithoutReferences(ServerOperation operation) {
    return operation == OP_GET_PROJECT
        || operation == OP_GET_ENV_VALUE
        || operation == OP_DUMP;     /* shows the table as it is, refreshing would change it */
}

/* -get answers from the locked project's variables. An input file only adds the
   ${__file} family of expansions to the value, so a request without one is
   answerable rather than an error. A search is over the whole project. */
static bool toleratesMissingInputFile(ServerOperation operation) {
    return operation == OP_GET_ENV_VALUE
        || operation == OP_DUMP
        || operation == OP_SEARCH;
}


/* The identifiers in a macro body are only code once some compilation unit
 * expands the macro. A CU can only expand it if it includes the file defining
 * it, so the includers of the request file bound the candidates - and the
 * include graph knows them without parsing anything (ADR-0027). */
static int collectCompilationUnitsExpandingTheMacro(Prerequisites *prerequisites, int cuFileNumbers[],
                                                    int maxCUs) {
    int cuCount = collectCUsIncluding(prerequisites, requestFileNumber, cuFileNumbers, 0, maxCUs);

    if (cuCount == 0)
        log_debug(":no compilation unit includes '%s', so nothing expands the macro",
                  getFileItemWithFileNumber(requestFileNumber)->name);
    return cuCount;
}

/* The file this request named, as a bare argument on the request's own command
 * line, or NO_FILE_NUMBER when the request named none (menu and filter
 * operations). */
static int requestFileArgument = -1;

protected void setRequestFileArgument(int fileNumber) {
    requestFileArgument = fileNumber;
}

/* Prepare the single input file this request is about, the one it named. Menu
 * and filter operations name none: they work on the session, not on a file. */
protected bool prepareInputFileForRequest(void) {
    requestFileNumber = requestFileArgument;
    return requestFileNumber != NO_FILE_NUMBER;
}

static void closeInputFile(void) {
    if (currentFile.characterBuffer.file != stdin) {
        closeCharacterBuffer(&currentFile.characterBuffer);
    }
}

/* Search and push by name answer from the table; the request file is not parsed */
static bool operationParsesRequestFile(ServerOperation operation) {
    return operation != OP_SEARCH && operation != OP_BROWSE_PUSH_NAME;
}

static void parseInputFile(void) {
    if (options.fileTrace)
        fprintf(stderr, "parseInputFile: '%s\n", currentFile.fileName);

    /* Bridge: Sync parsingConfig for all operations using this entry point */
    setupParsingConfig(requestFileNumber);

    if (operationParsesRequestFile(options.serverOperation)) {
        log_debug("parse start");
        if (currentPass == 1)
            announceAnalysing(currentFile.fileName);
        callParser();
        log_debug("parse end");
    } else
        log_debug("Not parsing input because of server operation TAG_SEARCH or PUSH_NAME");

    closeInputFile();
}

void initServer(ArgumentsVector args) {
    clearAvailableRefactorings();
    StringList *operands = NULL;
    processOptions(args, &operands); /* no include or define options */
    setRequestFileArgument(addRequestFileToFileTable(operands));
    freeStringList(operands);
    initCompletions(&collectedCompletions, 0, NO_POSITION);
}


protected void singlePass(ArgumentsVector args, ArgumentsVector nargs, Prerequisites *prerequisites, char *fileName) {
    bool inputOpened = false;

    inputOpened = initializeFileProcessing(args, nargs, fileName);

    parsingConfig.fileNumber = currentFile.characterBuffer.fileNumber;

    if (inputOpened) {
        bool parses = operationParsesRequestFile(options.serverOperation);
        FileItem *fileItem = getFileItemWithFileNumber(parsingConfig.fileNumber);

        /* Taken before the content is read, as in buildKnowledgeOfCU() */
        FileTimestamp knowledgeTime = fileTimestampNow();
        parseInputFile();
        if (parses) {
            fileItem->knowledgeTime = knowledgeTime;
        }
    }
    if (options.cursorOffset == 0) {
        // special case, push the file as include reference
        if (opensBrowsingSession(options.serverOperation)) {
            Position position = makePosition(parsingConfig.fileNumber, 1, 0);
            parsingConfig.positionOfSelectedReference = position;
            addFileAsIncludeReference(parsingConfig.fileNumber);
        }
    }
    if (completionPositionFound && !completionStringServed) {
        /* Cursor is on an identifier inside a macro body, which is not a symbol until
         * some compilation unit expands the macro. Parse every CU that can, so each
         * meaning the identifier has becomes a referent of its own and the answer is
         * the whole set (ADR-0027) - one of them is a goto, several are the menu. */
        int cuFileNumbers[MAX_CUS_TO_REPARSE];
        int cuCount = collectCompilationUnitsExpandingTheMacro(prerequisites, cuFileNumbers,
                                                               MAX_CUS_TO_REPARSE);
        for (int i = 0; i < cuCount; i++) {
            log_debug(":parsing '%s' to expand the macro",
                      getFileItemWithFileNumber(cuFileNumbers[i])->name);
            inputOpened = initializeFileProcessing(args, nargs,
                                                   getFileItemWithFileNumber(cuFileNumbers[i])->name);
            if (inputOpened) {
                parseInputFile();
            }
        }
    }
}

static void processFile(ArgumentsVector baseArgs, ArgumentsVector requestArgs,
                        Prerequisites *prerequisites) {
    FileItem *fileItem = getFileItemWithFileNumber(requestFileNumber);

    for (currentPass=1; currentPass <= numberOfPasses(); currentPass++) {
        assert(fileItem->name!=NULL);
        singlePass(baseArgs, requestArgs, prerequisites, fileItem->name);
        if (completionStringServed && !opensBrowsingSession(options.serverOperation))
            break;
    }
}


static void scanProjectStructure() {
    StringList *prunePaths = resolvePrunePaths();
    StringList *discoveredCUs =
        scanProjectForFilesAndIncludes(options.detectedProjectRoot, options.includeDirs, prunePaths);
    /* Also scan extra source directories from the project config */
    for (StringList *dir = getProjectConfig()->sourceDirs; dir != NULL; dir = dir->next) {
        if (isDirectory(dir->string) && strcmp(dir->string, options.detectedProjectRoot) != 0) {
            StringList *extraCUs =
                scanProjectForFilesAndIncludes(dir->string, options.includeDirs, prunePaths);
            /* Prepend to discoveredCUs */
            if (extraCUs != NULL) {
                StringList *tail = extraCUs;
                while (tail->next != NULL)
                    tail = tail->next;
                tail->next = discoveredCUs;
                discoveredCUs = extraCUs;
            }
        }
    }
    freeStringList(prunePaths);
    markMissingFilesAsDeleted(discoveredCUs);
    freeStringList(discoveredCUs);
}

/* Analysing within the cap is not worth a question (ADR-0032, time budget) */
protected bool needsConfirmationToAnalyseThatMany(int outOfDateCUs) {
    return outOfDateCUs > MAX_CUS_TO_REPARSE;
}

static int countCompilationUnits(void) {
    int count = 0;
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fi = getFileItemWithFileNumber(i);
        if (isCompilationUnit(fi->name) && !fi->isDeleted)
            count++;
    }
    return count;
}

static bool setUpProjectIfNew(ArgumentsVector baseArgs, ArgumentsVector requestArgs) {
    if (projectContextInitialized)
        return true;

    bool initialized = initializeProjectContext(getFileItemWithFileNumber(requestFileNumber)->name,
                                                baseArgs, requestArgs);
    if (!initialized) {
        /* No project configuration found - c-xrefactory cannot operate.
         * OP_GET_PROJECT answers that itself; other operations need an
         * explicit message. */
        if (options.serverOperation != OP_GET_PROJECT)
            errorMessage(ERR_ST, "No project configuration (.c-xrefrc) found for this file");
        return false;
    }
    loadSnapshotFromStore();
    projectContextInitialized = true;
    return true;
}

static bool findChangedFiles(BuildList *buildList, ArgumentsVector baseArgs, ArgumentsVector requestArgs) {
    loadAllOpenedEditorBuffers();

    /* Close preloaded buffers that the client no longer sends - the user has saved
     * and/or closed the file in the editor. Without this, the server would keep parsing
     * from preloaded content the client has dropped. */
    closeEditorBuffersNoLongerHeld(buildList);

    bool configChanged = false;
    if (projectContextInitialized) {
        /* A project is only found with its root, see applyConventionBasedDatabasePath() */
        assert(options.detectedProjectRoot[0] != '\0');
        if (isProjectConfigChanged()) {
            reloadProjectConfig(baseArgs, requestArgs);
            configChanged = true;
        }
    }

    return configChanged;
}

static Prerequisites *scanIncludesOfChangedFiles(bool configChanged) {
    /* The first request, or a config change, scans
       everything, otherwise the changed files are rescanned */
    static bool scanDone = false;
    if (projectContextInitialized) {
        if (!scanDone || configChanged) {
            scanProjectStructure();
            scanDone = true;
        } else {
            rescanChangedFiles(options.includeDirs);
        }
    }
    return collectPrerequisites();
}

static void removeReferences(bool configChanged, Prerequisites *prerequisites) {
    if (configChanged) {
        /* A config change is a change of every file (ADR-0035) */
        removeAllReferencesExceptIncludes();
        markAllCompilationUnitsStale();
    }
    removeReferencesOfChangedFiles(prerequisites);
}

static void collectGoal(BuildList *buildList, bool projectWasOpen, bool hasInputFile,
                        Prerequisites *prerequisites) {
    /* The held CUs that are out of date are in every operation's goal, so all
     * operations see what is in the editor, but only in a project that was open already */
    if (projectWasOpen)
        collectOutOfDateHeldCUs(buildList, prerequisites);

    if (hasInputFile && requiresProcessingInputFile(options.serverOperation)
        && operationParsesRequestFile(options.serverOperation))
        collectRequestCUForStripping(buildList, prerequisites, requestFileNumber);

    if (hasInputFile && !requiresProcessingInputFile(options.serverOperation)
        && !answeredWithoutReferences(options.serverOperation))
        collectRequestCUToBuild(buildList, prerequisites, requestFileNumber);

    /* Sibling CUs that share headers with the request file but are out of date,
     * e.g. on a cold start */
    if (projectContextInitialized && hasInputFile && !answeredWithoutReferences(options.serverOperation)
        && !needsWholeProjectParsed(options.serverOperation))
        collectSiblingCUs(buildList, prerequisites, requestFileNumber);
}

/* Above the budget the user is asked */
static bool okToAnalyse(int outOfDateCUs) {
    if (!needsConfirmationToAnalyseThatMany(outOfDateCUs))
        return true;
    char msg[TMP_STRING_SIZE];
    sprintf(msg, "%d of %d files need analysing. Analyse all before %s?", outOfDateCUs,
            countCompilationUnits(), parseAllPromptFor(options.serverOperation));
    return ppcWaitConfirmation(msg);
}

static void buildAfterAsking(ArgumentsVector baseArgs, BuildList *list) {
    int outOfDateCUs = buildListCount(list);
    if (okToAnalyse(outOfDateCUs))
        buildKnowledgeOfAll(list, baseArgs);
}

void callServer(ArgumentsVector baseArgs, ArgumentsVector requestArgs) {

    ENTER();

    bool projectWasOpen = projectContextInitialized;
    bool hasInputFile = prepareInputFileForRequest();

    /* SETUP */
    if (hasInputFile && !setUpProjectIfNew(baseArgs, requestArgs)) {
        LEAVE();
        return;
    }

    /* CHANGED */
    BuildList *buildList = newBuildList(MAX_FILES);
    bool configChanged = findChangedFiles(buildList, baseArgs, requestArgs);

    /* DISCOVER + SCAN */
    Prerequisites *prerequisites = scanIncludesOfChangedFiles(configChanged);

    /* REMOVE */
    removeReferences(configChanged, prerequisites);

    /* GOAL */
    collectGoal(buildList, projectWasOpen, hasInputFile, prerequisites);

    /* Another REMOVE... */
    stripRequestCU(buildList, requestFileNumber);

    buildKnowledgeOfAll(buildList, baseArgs);

    /* Completeness for name-based operations: if CUs are out of date, ask user
     * before proceeding */
    if (projectContextInitialized && needsWholeProjectParsed(options.serverOperation)) {
        BuildList *list = newBuildList(MAX_FILES);
        collectOutOfDateCUs(list, prerequisites);
        buildAfterAsking(baseArgs, list);
        freeBuildList(list);
    }

    /* A build starts each CU from the project's options without the request's
     * (initializeFileProcessing()), so the request gets its own back */
    deepCopyOptionsFromTo(&savedOptions, &options);
    processOptions(requestArgs, NULL);

    if (opensBrowsingSession(options.serverOperation))
        pushEmptySession(&browsingStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (hasInputFile) {
            processFile(baseArgs, requestArgs, prerequisites);
            projectContextInitialized = true;
        } else if (!toleratesMissingInputFile(options.serverOperation)) {
            errorMessage(ERR_ST, "No input file");
        }
    }
    freePrerequisites(prerequisites);
    freeBuildList(buildList);
    LEAVE();
}

static long millisecondsSince(struct timespec start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
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
        struct timespec requestStart;
        clock_gettime(CLOCK_MONOTONIC, &requestStart);
        // TODO -o option on command line should catch also file not found
        openOutputFile(options.outputFileName);
        //&dumpArguments(nargc, nargv);

        progressOffset = 0;
        progressFactor = 1;

        log_trace("Server: Getting request");
        clearHeldThisRequestFlags();
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

        closeAllEditorBuffersIfClosable();
        closeOutputFile();
        if (options.cxrefProtocol)
            ppcSynchronize();
        log_info("Request %s answered in %ld ms", operationNamesTable[options.serverOperation],
                 millisecondsSince(requestStart));
        log_trace("Server: Request answered");
    }
    LEAVE();
}
