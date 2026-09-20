#include "server.h"

#include <string.h>

#include "commons.h"
#include "complete.h"
#include "cxfile.h"
#include "cxref.h"
#include "editor.h"
#include "editorbuffer.h"
#include "editorbuffertable.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "log.h"
#include "misc.h"
#include "navigation.h"
#include "options.h"
#include "parsers.h"
#include "parsing.h"
#include "ppc.h"
#include "progress.h"
#include "projectstructure.h"
#include "referenceableitemtable.h"
#include "referencerefresh.h"
#include "session.h"
#include "startup.h"
#include "timestamp.h"
#include "yylex.h"


const char *operationNamesTable[] = {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};


/* Once-per-process flag: true after the first request's project context
 * has been fully initialized (savedOptions synced, snapshot loaded).
 * Promoted from function-local static to module scope so cxref.c's
 * handleProject can mark it true after locking via the GetProject path
 * — otherwise the next request's gate would re-detect from a different
 * file and clobber the lock. See markProjectContextInitialized() below. */
static bool projectContextInitialized = false;

void markProjectContextInitialized(void) {
    projectContextInitialized = true;
}


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
        ||  operation==OP_REFACTORY
        ;
}

/* Name-based operations: the request names a symbol, not a position, so the
   name can live in any compilation unit and all of them must be parsed. */
static bool needsWholeProjectParsed(ServerOperation operation) {
    return operation==OP_SEARCH
        ||  operation==OP_UNUSED_GLOBAL
        ||  operation==OP_BROWSE_PUSH_NAME
        ;
}

static const char *parseAllPromptFor(ServerOperation operation) {
    switch (operation) {
    case OP_SEARCH:           return "searching";
    case OP_UNUSED_GLOBAL:    return "checking for unused";
    case OP_BROWSE_PUSH_NAME: return "browsing";
    default:                  assert(0); return "";
    }
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


/* The identifiers in a macro body are only code once some compilation unit
 * expands the macro. A CU can only expand it if it includes the file defining
 * it, so the includers of the request file bound the candidates - and the
 * include graph knows them without parsing anything (ADR-0027). */
#define MAX_CUS_TO_REPARSE 128

static int collectIncludersOfStaleHeader(int headerFileNumber, int cuFileNumbers[], int cuCount,
                                         int maxCUs);

static int findCompilationUnitExpandingTheMacro(void) {
    int cuFileNumbers[MAX_CUS_TO_REPARSE];

    int cuCount = collectIncludersOfStaleHeader(requestFileNumber, cuFileNumbers, 0, MAX_CUS_TO_REPARSE);
    if (cuCount == 0) {
        log_debug(":no compilation unit includes '%s', so nothing expands the macro",
                  getFileItemWithFileNumber(requestFileNumber)->name);
        return NO_FILE_NUMBER;
    }
    log_debug(":scheduling file '%s'", getFileItemWithFileNumber(cuFileNumbers[0])->name);
    return cuFileNumbers[0];
}

/* The file this request named, as a bare argument on the request's own command
 * line, or NO_FILE_NUMBER when the request named none (menu and filter
 * operations). Other files can be scheduled alongside it - a legacy project
 * config expands its source directories into every file it finds - so the
 * request has to say which one it is about. */
static int requestFileArgument = -1;

protected void setRequestFileArgument(int fileNumber) {
    requestFileArgument = fileNumber;
}

/* Prepare the single input file this request is about.
 *
 * The request names it, but other files can be scheduled alongside: a legacy
 * project config expands its source directories into every file it finds. So
 * prepare the file the request named, and leave it as the only scheduled one.
 * When the request names no file - menu and filter operations work on the
 * session, not on a file - fall back to whatever happens to be scheduled.
 */
protected bool prepareInputFileForRequest(void) {
    fileProcessingStartTime = fileTimestampNow();

    int fileNumber = requestFileArgument;
    if (fileNumber == NO_FILE_NUMBER || !getFileItemWithFileNumber(fileNumber)->isScheduled) {
        fileNumber = 0;
        if (getNextScheduledFile(&fileNumber) == NULL) {
            /* Nothing scheduled - the operation does not need a file, or the
             * one it named does not exist. */
            requestFileNumber = NO_FILE_NUMBER;
            return false;
        }
    }

    /* Ensure only this file is processed during this request */
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (i != fileNumber && fileItem->isScheduled) {
            log_trace("Unscheduling '%s', it is not the file this request named.", fileItem->name);
            fileItem->isScheduled = false;
        }
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
    clearRequestFileArgument();
    processOptions(args, PROCESS_FILE_ARGUMENTS_YES); /* no include or define options */
    processFileArguments();
    setRequestFileArgument(scheduleRequestFileArgument());
    initCompletions(&collectedCompletions, 0, NO_POSITION);
}


static void singlePass(ArgumentsVector args, ArgumentsVector nargs) {
    bool inputOpened = false;

    inputOpened = initializeFileProcessing(args, nargs);

    parsingConfig.fileNumber = currentFile.characterBuffer.fileNumber;

    if (inputOpened) {
        /* If the file has preloaded content, remove old references before parsing */
        EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(inputFileName);
        if (isPreloaded(buffer)) {
            log_debug("file has preloaded content, removing old references for file %d",
                      parsingConfig.fileNumber);
            removeReferenceableItemsForFile(parsingConfig.fileNumber);
        }

        parseInputFile();

    }
    if (options.cursorOffset == 0) {
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
        int fileWithMacroExpansion = findCompilationUnitExpandingTheMacro();
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
/* Walk the reverse-include graph from a stale header up to compilation units,
 * using TypeCppInclude references in the reference table (populated by prior
 * parsing or loaded from disk db). Collect CU file numbers into the provided
 * array, deduplicating against entries already present. */
static int collectIncludersOfStaleHeader(int headerFileNumber,
                                         int cuFileNumbers[], int cuCount, int maxCUs) {
    FileItem *headerItem = getFileItemWithFileNumber(headerFileNumber);
    log_debug("Looking for CUs that include stale header '%s'", headerItem->name);

    /* Walk reverse-include graph transitively: starting from the stale header,
     * find all files that include it, then files that include those, etc.
     * Collect any CUs encountered along the way. */
    int filesToWalk[MAX_INCLUDE_WALK_FILES];
    int walkCount = 1;
    filesToWalk[0] = headerFileNumber;

    for (int i = 0; i < walkCount; i++) {
        ReferenceableItem searchItem = makeReferenceableItem(
            LINK_NAME_INCLUDE_REFS, TypeCppInclude, StorageExtern,
            VisibilityGlobal, filesToWalk[i]);

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
            if (isCompilationUnit(includer->name)) {
                /* Deduplicate against CUs already collected (from other stale headers) */
                bool alreadyCollected = false;
                for (int j = 0; j < cuCount; j++) {
                    if (cuFileNumbers[j] == includerFileNum) {
                        alreadyCollected = true;
                        break;
                    }
                }
                if (!alreadyCollected && cuCount < maxCUs) {
                    cuFileNumbers[cuCount++] = includerFileNum;
                    log_debug("CU '%s' (transitively) includes stale header '%s'",
                              includer->name, headerItem->name);
                }
            }
        }
    }

    if (cuCount == 0)
        log_debug("No CUs found that include '%s'", headerItem->name);

    return cuCount;
}

static bool fileNeedsParsing(FileItem *fileItem);

static void setupProgress(int cuCount, int skippedCapped) {
    static char progressFormat[128];
    int totalFound = cuCount + skippedCapped;
    if (skippedCapped > 0)
        snprintf(progressFormat, sizeof(progressFormat),
                 "%d unparsed sibling compilation units, parsing %d... %%d remaining", totalFound, cuCount);
    else
        snprintf(progressFormat, sizeof(progressFormat), "Parsing %d sibling compilation units... %%d remaining",
                 cuCount);
    initProgress(progressFormat);
}

/* Entry refresh Pass 3: Find CUs that share headers with the request file
 * but haven't been parsed yet. On cold start without preloads, only the
 * request file gets parsed (by processFile). This pass ensures sibling CUs
 * are also parsed so their symbol references are available for navigation.
 *
 * Uses brute-force forward-include lookup: iterates all headers in the
 * file table and checks if the request file is among their includers. */
static void parseUnparsedSiblingCUs(int requestFileNumber, ArgumentsVector baseArgs) {
    int cuFileNumbers[MAX_CUS_TO_REPARSE];
    int cuCount = 0;
    int skippedAlreadyParsed = 0;
    int skippedCapped = 0;

    log_info("Pass 3: request file '%s' (fileNumber=%d)",
             getFileItemWithFileNumber(requestFileNumber)->name, requestFileNumber);

    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fi = getFileItemWithFileNumber(i);
        if (isCompilationUnit(fi->name))
            continue;  /* Skip CUs, only look at headers */

        ReferenceableItem searchItem = makeReferenceableItem(
            LINK_NAME_INCLUDE_REFS, TypeCppInclude, StorageExtern,
            VisibilityGlobal, i);

        ReferenceableItem *found;
        if (!isMemberInReferenceableItemTable(&searchItem, NULL, &found))
            continue;

        /* Is this header the request file, or included by it? Browsing can start
         * in a header, and then the CUs that include it are the siblings that
         * need parsing - on a cold start none of them has been parsed. */
        bool requestIncludesThis = (i == requestFileNumber);
        for (Reference *r = found->references; r != NULL; r = r->next) {
            if (r->position.file == requestFileNumber) {
                requestIncludesThis = true;
                break;
            }
        }

        if (!requestIncludesThis)
            continue;

        /* Collect sibling CUs that include this header and need parsing */
        for (Reference *r = found->references; r != NULL; r = r->next) {
            int siblingFileNum = r->position.file;
            if (siblingFileNum == requestFileNumber)
                continue;

            FileItem *siblingItem = getFileItemWithFileNumber(siblingFileNum);
            if (!isCompilationUnit(siblingItem->name))
                continue;
            if (!fileNeedsParsing(siblingItem)) {
                skippedAlreadyParsed++;
                continue;
            }

            /* Deduplicate */
            bool alreadyCollected = false;
            for (int j = 0; j < cuCount; j++) {
                if (cuFileNumbers[j] == siblingFileNum) {
                    alreadyCollected = true;
                    break;
                }
            }
            if (!alreadyCollected) {
                if (cuCount < MAX_CUS_TO_REPARSE) {
                    cuFileNumbers[cuCount++] = siblingFileNum;
                    log_debug("Sibling CU '%s' shares header with request file",
                              siblingItem->name);
                } else {
                    skippedCapped++;
                }
            }
        }
    }

    log_info("Pass 3: %d to parse, %d skipped (already parsed), %d skipped (cap %d)",
             cuCount, skippedAlreadyParsed, skippedCapped, MAX_CUS_TO_REPARSE);

    if (cuCount > 0) {
        log_info("Entry refresh pass 3: parsing %d sibling CU(s)", cuCount);
        if (options.xref2) {
            setupProgress(cuCount, skippedCapped);
        }
        int savedCursorOffset = options.cursorOffset;
        options.cursorOffset = NO_CURSOR_OFFSET;
        for (int i = 0; i < cuCount; i++) {
            reparseStaleFile(cuFileNumbers[i], baseArgs);
            FileItem *fi = getFileItemWithFileNumber(cuFileNumbers[i]);
            fi->lastParsedMtime = editorFileModificationTime(fi->name);
            if (options.xref2)
                writeProgressInformation(cuCount - i - 1);
        }
        options.cursorOffset = savedCursorOffset;
    }
}

static int countStalePreloadedFiles(void) {
    int count = 0;
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1))
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next)
            if (fileNumberIsStale(l->buffer->fileNumber))
                count++;
    return count;
}

static void reparseStalePreloadedFiles(ArgumentsVector baseArgs) {
    int staleCount = countStalePreloadedFiles();
    if (staleCount == 0)
        return;

    log_info("Refreshing %d stale preloaded file(s)", staleCount);

    /* Pass 1: Reparse stale CUs directly. This also refreshes their
     * TypeCppInclude references, which Pass 2 depends on.
     * No progress reporting here — Pass 1 is fast (only directly
     * preloaded CUs, typically 1-2 files). */
    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
            int fileNumber = l->buffer->fileNumber;
            FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
            if (fileNumberIsStale(fileNumber) && isCompilationUnit(fileItem->name)) {
                if (!editorFileExists(fileItem->name)) {
                    log_debug("Stale CU '%s' no longer exists, marking as deleted", fileItem->name);
                    markFileAsDeleted(fileNumber);
                } else {
                    log_debug("Reparsing stale CU '%s'", fileItem->name);
                    reparseStaleFile(fileNumber, baseArgs);
                    EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);
                    if (buffer != NULL)
                        fileItem->lastParsedMtime = buffer->modificationTime;
                    fileItem->needsBrowsingStackRefresh = true;
                }
            }
        }
    }

    /* Pass 2: For stale headers, find CUs that include them and reparse.
     * Must come after Pass 1: Pass 1 reparses stale CUs, refreshing their
     * TypeCppInclude references. Pass 2 queries those references to find
     * which CUs include the stale header (transitively).
     *
     * First collect all CUs across all stale headers (deduplicated),
     * then reparse with per-CU progress reporting. */
    int cuFileNumbers[MAX_CUS_TO_REPARSE];
    int cuCount = 0;

    for (int i = 0; i != -1; i = getNextExistingEditorBufferIndex(i + 1)) {
        for (EditorBufferList *l = getEditorBufferListElementAt(i); l != NULL; l = l->next) {
            int fileNumber = l->buffer->fileNumber;
            FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
            if (fileNumberIsStale(fileNumber) && !isCompilationUnit(fileItem->name)) {
                if (!editorFileExists(fileItem->name)) {
                    log_debug("Stale header '%s' no longer exists, marking as deleted", fileItem->name);
                    markFileAsDeleted(fileNumber);
                } else {
                    cuCount = collectIncludersOfStaleHeader(fileNumber, cuFileNumbers, cuCount,
                                                           MAX_CUS_TO_REPARSE);
                    /* The stale header's own references go now; reparsing the CUs
                     * above re-emits them. */
                    removeReferenceableItemsForFile(fileNumber);
                    EditorBuffer *buffer = getOpenedAndLoadedEditorBuffer(fileItem->name);
                    if (buffer != NULL)
                        fileItem->lastParsedMtime = buffer->modificationTime;
                    fileItem->needsBrowsingStackRefresh = true;
                }
            }
        }
    }

    if (cuCount > 0) {
        log_info("Reparsing %d CU(s) for stale header includers", cuCount);
        if (options.xref2) {
            static char progressFormat[128];
            snprintf(progressFormat, sizeof(progressFormat),
                     "Updating %d header includers... %%d remaining", cuCount);
            initProgress(progressFormat);
        }
        for (int i = 0; i < cuCount; i++) {
            reparseStaleFile(cuFileNumbers[i], baseArgs);
            getFileItemWithFileNumber(cuFileNumbers[i])->needsBrowsingStackRefresh = true;
            if (options.xref2)
                writeProgressInformation(cuCount - i - 1);
        }
    }
}

static bool fileNeedsParsing(FileItem *fileItem) {
    return fileTimestampIsZero(fileItem->lastParsedMtime)
        || !fileTimestampsEqual(editorFileModificationTime(fileItem->name), fileItem->lastParsedMtime);
}

static void parseDiscoveredCompilationUnits(ArgumentsVector baseArgs) {
    /* Parse all discovered CUs to populate in-memory references.
     * Skip the request file — it will be handled by the dispatch below
     * (processFile for input-processing operations, or just unscheduled). */
    int cuCount = 0;
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (fileItem->isScheduled && isCompilationUnit(fileItem->name) && i != requestFileNumber
            && fileNeedsParsing(fileItem))
            cuCount++;
    }

    int parsed = 0;
    for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
        FileItem *fileItem = getFileItemWithFileNumber(i);
        if (fileItem->isScheduled && isCompilationUnit(fileItem->name) && i != requestFileNumber) {
            if (fileNeedsParsing(fileItem)) {
                reparseStaleFile(i, baseArgs);
                parsed++;
                if (options.xref2)
                    writeRelativeProgress((100 * parsed) / cuCount);
            }
            fileItem->isScheduled = false;
        }
    }
    log_info("Startup: parsed %d compilation units", parsed);
}

static bool waitForUserConfirmation(char *message) {
    ppcWaitConfirmation(message);
    closeOutputFile();
    ppcSynchronize();

    /* Read the response directly from stdin without using readOptionsFromPipe,
     * which would overwrite the static optMemory buffer and clobber requestArgs. */
    char line[MAX_OPTION_LEN];
    bool confirmed = false;

    while (fgets(line, sizeof(line), stdin) != NULL) {
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (strcmp(line, END_OF_OPTIONS_STRING) == 0)
            break;
        if (strcmp(line, "-continue") == 0)
            confirmed = true;
        if (strcmp(line, "-cancel") == 0)
            confirmed = false;
    }

    /* Consume the trailing empty line (protocol separator) */
    int c = getc(stdin);
    if (c == EOF) {
        log_error("Broken pipe in waitForUserConfirmation");
        exit(EXIT_FAILURE);
    }

    openOutputFile(options.outputFileName);
    return confirmed;
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

void callServer(ArgumentsVector baseArgs, ArgumentsVector requestArgs) {
    static bool scanDone = false;

    ENTER();

    loadAllOpenedEditorBuffers();

    /* Close preloaded buffers that the client no longer sends — the user
     * has saved and/or closed the file in the editor. Without this, the
     * server would keep parsing from the stale preloaded content instead
     * of reading the current disk file. */
    closeEditorBuffersNoLongerPreloaded(baseArgs);

    /* Reparse any stale preloaded files before dispatching the operation,
     * so all operations see fresh in-memory references. */
    if (projectContextInitialized) {
        /* Clear cursorOffset so the lexer doesn't set positionOfSelectedReference
         * during the reparse — that would trigger on-line action handling (browsing stack
         * assertions) before the operation has set things up. */
        int savedCursorOffset = options.cursorOffset;
        options.cursorOffset = NO_CURSOR_OFFSET;
        reparseStalePreloadedFiles(baseArgs);
        options.cursorOffset = savedCursorOffset;
    }

    bool hasInputFile = prepareInputFileForRequest();

    /* ONE-TIME: project identity + snapshot load */
    if (!projectContextInitialized && hasInputFile) {
        bool initialized = initializeProjectContext(
            getFileItemWithFileNumber(requestFileNumber)->name, baseArgs, requestArgs);

        if (initialized) {
            loadSnapshotFromStore();

            if (options.serverOperation == OP_GET_PROJECT
                && (options.detectedProjectRoot == NULL || options.detectedProjectRoot[0] == '\0')) {
                /* Legacy path: no detected project root, fall back to old flow */
                if (options.inputFiles == NULL)
                    addToStringListOption(&options.inputFiles, ".");
                processFileArguments();
                parseDiscoveredCompilationUnits(baseArgs);
            }

            projectContextInitialized = true;
        } else {
            /* No project configuration found — c-xrefactory cannot operate.
             * For OP_GET_PROJECT the discovery code (handlePathologicProjectCases)
             * already reported the specifics to the client; other operations need
             * an explicit message. */
            if (options.serverOperation != OP_GET_PROJECT)
                errorMessage(ERR_ST, "No project configuration (.c-xrefrc) found for this file");
            goto done;
        }
    }

    /* CONFIG-CHANGE-AWARE SCAN (auto-detect path only).
     * First request: !scanDone triggers the scan.
     * Config change: re-reads .c-xrefrc (updates options.includeDirs and
     * savedOptions), then re-runs scan with new include dirs. */
    if (projectContextInitialized
        && options.detectedProjectRoot != NULL
        && options.detectedProjectRoot[0] != '\0') {
        bool configChanged = isProjectConfigChanged();
        if (configChanged) {
            reloadProjectConfig(baseArgs, requestArgs);
            markAllCompilationUnitsStale();   /* config change = logical cold restart */
        }
        if (!scanDone || configChanged) {
            scanProjectStructure();
            scanDone = true;
        }
    }

    /* Entry refresh pass 3: parse sibling CUs that share headers with
     * the request file but haven't been parsed yet (e.g. cold start). */
    if (projectContextInitialized && hasInputFile
        && options.detectedProjectRoot != NULL && options.detectedProjectRoot[0] != '\0') {
        parseUnparsedSiblingCUs(requestFileNumber, baseArgs);
    }

    /* Completeness for name-based operations: if stale CUs exist, ask user before proceeding */
    if (projectContextInitialized && needsWholeProjectParsed(options.serverOperation)) {
        int totalCUs = 0, staleCUs = 0;
        for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
            FileItem *fi = getFileItemWithFileNumber(i);
            if (isCompilationUnit(fi->name)) {
                totalCUs++;
                if (fileTimestampIsZero(fi->lastParsedMtime))
                    staleCUs++;
            }
        }
        if (staleCUs > 0) {
            char msg[TMP_STRING_SIZE];
            const char *operationName = parseAllPromptFor(options.serverOperation);
            sprintf(msg, "%d of %d compilation units need reparsing. Parse all before %s?",
                    staleCUs, totalCUs, operationName);
            if (waitForUserConfirmation(msg)) {
                int parsed = 0;
                char progressFormat[128];
                snprintf(progressFormat, sizeof(progressFormat),
                         "Parsing %d compilation units... %%d remaining", staleCUs);
                initProgress(progressFormat);
                for (int i = getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i + 1)) {
                    FileItem *fi = getFileItemWithFileNumber(i);
                    if (isCompilationUnit(fi->name) && fileTimestampIsZero(fi->lastParsedMtime)) {
                        reparseStaleFile(i, baseArgs);
                        fi->lastParsedMtime = editorFileModificationTime(fi->name);
                        parsed++;
                        writeProgressInformation(staleCUs - parsed);
                        /* Save snapshot periodically so progress survives Ctrl-g/crash */
                        if (parsed % 100 == 0)
                            saveReferences();
                    }
                }
                log_info("Completeness parse: parsed %d CUs", parsed);
            }
        }
    }

    if (needsReferenceDatabase(options.serverOperation))
        pushEmptySession(&browsingStack);

    if (requiresProcessingInputFile(options.serverOperation)) {
        if (hasInputFile) {
            processFile(baseArgs, requestArgs);
            projectContextInitialized = true;
        } else {
            errorMessage(ERR_ST, "No input file");
        }
    } else {
        if (hasInputFile) {
            getFileItemWithFileNumber(requestFileNumber)->isScheduled = false;
            inputFileName = NULL;
        }
    }
done:
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

        progressOffset = 0;
        progressFactor = 1;

        log_trace("Server: Getting request");
        clearPreloadedThisRequestFlags();
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
        if (options.xref2)
            ppcSynchronize();
        log_trace("Server: Request answered");
    }
    LEAVE();
}
