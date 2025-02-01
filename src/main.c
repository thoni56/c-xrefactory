#include "main.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "caching.h"
#include "characterreader.h"
#include "commandlogger.h"
#include "commons.h"
#include "constants.h"
#include "cxfile.h"
#include "editor.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "init.h"
#include "lexem.h"
#include "list.h"
#include "log.h"
#include "lsp.h"
#include "macroargumenttable.h"
#include "memory.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"
#include "refactory.h"
#include "reftab.h"
#include "server.h"
#include "stackmemory.h"
#include "symboltable.h"
#include "xref.h"
#include "yylex.h"

static char previousStandardOptionsFile[MAX_FILE_NAME_SIZE];
static char previousStandardOptionsSection[MAX_FILE_NAME_SIZE];
static time_t previousStandardOptionsFileModificationTime;
static int previousLanguage;
static int previousPass;


/* *************************************************************************** */

static void writeOptionsFileMessage(char *file, char *outFName, char *outSect) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (outFName[0]==0) {
        if (options.project!=NULL) {
            sprintf(tmpBuff, "'%s' project options not found",
                    options.project);
            if (options.mode == ServerMode) {
                errorMessage(ERR_ST, tmpBuff);
            } else {
                FATAL_ERROR(ERR_ST, tmpBuff, XREF_EXIT_NO_PROJECT);
            }
        } else if (options.xref2) {
            ppcGenRecord(PPC_NO_PROJECT,file);
        } else {
            sprintf(tmpBuff, "no project name covers '%s'",file);
            warningMessage(ERR_ST, tmpBuff);
        }
    } else if (options.mode==XrefMode) {
        if (options.xref2) {
            sprintf(tmpBuff, "C-xrefactory project: %s", outSect);
            ppcGenRecord(PPC_INFORMATION, tmpBuff);
        } else {
            fprintf(errOut, "[C-xref] active project: '%s'\n", outSect);
            fflush(errOut);
        }
    }
}

static void handlePathologicProjectCases(char *fileName, char *outFName, char *section,
                                         bool showErrorMessage){
    // all this stuff should be reworked, but be very careful when refactoring it
    // WTF? Why??!?!
    assert(options.mode);
    if (options.mode == ServerMode) {
        if (showErrorMessage) {
            writeOptionsFileMessage(fileName, outFName, section);
        }
    } else {
        if (*previousStandardOptionsFile == 0) {
            static bool messageWritten = false;
            if (showErrorMessage && messageWritten == 0) {
                messageWritten = true;
                writeOptionsFileMessage(fileName, outFName, section);
            }
        } else {
            if (outFName[0]==0 || section[0]==0) {
                warningMessage(ERR_ST, "no project name covers this file");
            }
            if (outFName[0]==0 && section[0]==0) {
                strcpy(section, previousStandardOptionsSection);
            }
            if (outFName[0]==0) {
                strcpy(outFName, previousStandardOptionsFile);
            }
            if (strcmp(previousStandardOptionsFile,outFName) != 0 || strcmp(previousStandardOptionsSection,section) != 0) {
                if (options.xref2) {
                    char tmpBuff[TMP_BUFF_SIZE];                        \
                    sprintf(tmpBuff, "[C-xref] new project: '%s'", section);
                    ppcGenRecord(PPC_INFORMATION, tmpBuff);
                } else {
                    fprintf(errOut, "[C-xref] new project: '%s'\n", section);
                }
            }
        }
    }
}

static bool computeAndOpenInputFile(void) {
    FILE *inputFile;
    EditorBuffer *inputBuffer;

    assert(currentLanguage);
    inputBuffer = NULL;

    inputFile = NULL;
    inputBuffer = findOrCreateAndLoadEditorBufferForFile(inputFileName);
    if (inputBuffer == NULL) {
#if defined (__WIN32__)
        inputFile = openFile(inputFileName, "rb");
#else
        inputFile = openFile(inputFileName, "r");
#endif
        if (inputFile == NULL) {
            errorMessage(ERR_CANT_OPEN, inputFileName);
        }
    }
    initInput(inputFile, inputBuffer, "\n", inputFileName);
    if (inputFile==NULL && inputBuffer==NULL) {
        return false;
    } else {
        return true;
    }
}

static void initOptions(void) {
    deepCopyOptionsFromTo(&presetOptions, &options);

    inputFileNumber   = NO_FILE_NUMBER;
}

static void initStandardCxrefFileName(char *inputfile) {
    static char standardCxrefFileName[MAX_FILE_NAME_SIZE];

    extractPathInto(normalizeFileName_static(inputfile, cwd), standardCxrefFileName);
    strcat(standardCxrefFileName, DEFAULT_CXREF_FILENAME);
    assert(strlen(standardCxrefFileName) < MAX_FILE_NAME_SIZE);

    strcpy(standardCxrefFileName, getRealFileName_static(normalizeFileName_static(standardCxrefFileName, cwd)));
    assert(strlen(standardCxrefFileName) < MAX_FILE_NAME_SIZE);

    options.cxrefsLocation = standardCxrefFileName;
}

static void initializationsPerInvocation(void) {
    parsedInfo = (ParsedInfo){0,};
    cxRefPosition = noPosition;
    olstringFound = false;
    olstringServed = false;
    olstringInMacroBody = NULL;
}


// Read from file into *line if max length, put actual length in outLength, return EOF if at EOF
static int getLineFromFile(FILE *file, char *line, int max, int *outLength) {
    int i = 0;
    int ch;
    int result = EOF;

    ch = getc(file);
    /* Skip whitespace */
    while ((ch>=0 && ch<=' ') || ch=='\n' || ch=='\t')
        ch=getc(file);
    if (ch==EOF) {
        goto fini;
    }

    while (ch!=EOF && ch!='\n') {
        if (i < max-1)
            line[i++]=ch;
        ch=getc(file);
    }
    result = 'A';

 fini:
    line[i] = 0;
    *outLength  = i;
    return result;
}


static char compiler_identification[MAX_OPTION_LEN];

static void discoverBuiltinIncludePaths(void) {
    char line[MAX_OPTION_LEN];
    int len;
    char *tempfile_name;
    FILE *tempfile;
    char command[TMP_BUFF_SIZE];
    bool found = false;

    static bool messageWritten = false;

    if (!LANGUAGE(LANG_C) && !LANGUAGE(LANG_YACC)) {
        return;
    }
    ENTER();

    tempfile_name = create_temporary_filename();
    assert(strlen(tempfile_name)+1 < MAX_FILE_NAME_SIZE);

    /* Ensure output is in C locale */
    sprintf(command, "LANG=C %s -v -x %s -o /dev/null /dev/null >%s 2>&1", options.compiler, "c", tempfile_name);

    int rc = system(command);
    (void)rc;                   /* UNUSED */

    tempfile = openFile(tempfile_name, "r");
    if (tempfile==NULL) return;
    while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF) {
        if (strncmp(line, "#include <...> search starts here:",34)==0) {
            found = true;
            break;
        }
    }
    if (found)
        do {
            if (strncmp(line, "End of search list.", 19) == 0)
                break;
            if (directoryExists(line)) {
                log_trace("Add include '%s'", line);
                addToStringListOption(&options.includeDirs, line);
            }
        } while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF);


    /* Discover which compiler is used, for future special defines for it */
    rewind(tempfile);
    while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF) {
        if (strstr(line, " version ") != 0 && !messageWritten) {
            log_info("Compiler is '%s'", line);
            strcpy(compiler_identification, line);
            messageWritten = true;
            break;
        }
    }

    closeFile(tempfile);

    removeFile(tempfile_name);
    LEAVE();
}

/* TODO: create tables that contains specific defines needed for each
 * supported compiler. For each set the first string is the compiler
 * identification. Search will consider all sets for which the
 * identification string is a substring of the
 * compiler_identification, so you can have a set specific for "clang"
 * clang version 12", then for "Apple clang", then "clang" in the
 * order of "compiler_dependent_defines" */


// TODO: Separate out "Apple clang"...
static char *clang_defines[] = {
    "__int8_t int",
    "__int16_t int",
    "__int32_t int",
    "__int64_t int",
    "__uint8_t int",
    "__uint16_t int",
    "__uint32_t int",
    "__uint64_t int",
    "int8_t int",
    "int16_t int",
    "int32_t int",
    "int64_t int",
    "uint8_t int",
    "uint16_t int",
    "uint32_t int",
    "uint64_t int",
    "u_int8_t int",
    "u_int16_t int",
    "u_int32_t int",
    "u_int64_t int",
    "__UINTPTR_TYPE__ int",
    "__INTPTR_TYPE__ int",
    "__INTMAX_TYPE__ int",
    "__UINTMAX_TYPE__ int",

    "wchar_t int",

    "__darwin_clock_t int",
    "__darwin_ct_rune_t int",
    "__darwin_mode_t int",
    "__darwin_natural_t int",
    "__darwin_off_t int",
    "__darwin_off_t int",
    "__darwin_pid_t int",
    "__darwin_pid_t int",
    "__darwin_rune_t int",
    "__darwin_sigset_t int",
    "__darwin_size_t int",
    "__darwin_ssize_t int",
    "__darwin_suseconds_t int",
    "__darwin_time_t int",
    "__darwin_time_t int",
    "__darwin_uid_t int",
    "__darwin_useconds_t int",
    "__darwin_va_list void*",
    "__darwin_wint_t int",
    NULL
};

typedef struct {char *compiler; char **defines;} CompilerDependentDefines;
static CompilerDependentDefines compiler_dependent_defines[] = {{"clang", clang_defines}};


static char *extra_defines[] = {
    /* Standard types */
    "_Bool int",
    /* GNUisms: */
    "__attribute__(xxx)",
    "__alignof__(xxx) 8",
    "__typeof__(xxx) int",
    "__builtin_va_list void",
    "__leaf__",
    "__restrict__",
    "__extension__"
};

static void discoverStandardDefines(void) {
    char line[MAX_OPTION_LEN];
    int len;
    char *tempfile_name;
    FILE *tempfile;
    char command[TMP_BUFF_SIZE];

    ENTER();

    /* This function discovers the compiler builtin defines by making
     * a call to it and then sets those up as if they where defined on
     * the command line */

    if (!(LANGUAGE(LANG_C) || LANGUAGE(LANG_YACC))) {
        LEAVE();
        return;
    }
    tempfile_name = create_temporary_filename();
    assert(strlen(tempfile_name)+1 < MAX_FILE_NAME_SIZE);

    sprintf(command, "%s -E -dM - >%s 2>&1", options.compiler, tempfile_name);

    /* Need to pipe an empty file into gcc, an alternative would be to
       create an empty file, but that seems as much work as this */
    FILE *p = popen(command, "w");
    closeFile(p);

    tempfile = openFile(tempfile_name, "r");
    if (tempfile==NULL) {
        log_debug("Could not open tempfile");
        return;
    }
    while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF) {
        if (strncmp(line, "#define", strlen("#define")))
            log_error("Expected #define from compiler standard definitions");
        log_trace("Add definition '%s'", line);
        addMacroDefinedByOption(&line[strlen("#define")+1]);
    }

    /* Also define some faked standard base types and GNU-isms */
    for (int i=0; i<(int)sizeof(extra_defines)/sizeof(extra_defines[0]); i++) {
        log_trace("Add definition '%s'", extra_defines[i]);
        addMacroDefinedByOption(extra_defines[i]);
    }

    for (int c=0; c<sizeof(compiler_dependent_defines)/sizeof(CompilerDependentDefines); c++) {
        if (strstr(compiler_dependent_defines[c].compiler, compiler_identification) != NULL) {
            log_trace("Adding compiler specific defines for '%s'", compiler_dependent_defines[c].compiler);
            for (char **d=compiler_dependent_defines[c].defines; *d != NULL; d++) {
                log_trace("Add definition '%s'", *d);
                addMacroDefinedByOption(*d);
            }
        }
    }

    LEAVE();
 }

static void getAndProcessXrefrcOptions(char *optionsFileName, char *optionsSectionName, char *project) {
    int argc;
    char **argv;
    if (*optionsFileName != 0) {
        readOptionsFromFile(optionsFileName, &argc, &argv, optionsSectionName, project);
        // warning, the following can overwrite variables like
        // 'cxref_file_name' allocated in ppmMemory, then when memory
        // is got back by caching, it may provoke a problem
        processOptions(argc, argv, DONT_PROCESS_FILE_ARGUMENTS); /* .c-xrefrc opts*/
    }
}

bool initializeFileProcessing(bool *firstPass, int argc, char **argv, // command-line options
                              int nargc, char **nargv, Language *outLanguage) {
    char standardOptionsFileName[MAX_FILE_NAME_SIZE];
    char standardOptionsSectionName[MAX_FILE_NAME_SIZE];
    time_t modifiedTime;
    char *fileName;
    StringList *tmpIncludeDirs;
    bool inputOpened;

    ENTER();

    fileName = inputFileName;
    *outLanguage = getLanguageFor(fileName);
    searchStandardOptionsFileAndProjectForFile(fileName, standardOptionsFileName, standardOptionsSectionName);
    handlePathologicProjectCases(fileName, standardOptionsFileName, standardOptionsSectionName, true);

    initAllInputs();

    if (standardOptionsFileName[0] != 0 )
        modifiedTime = fileModificationTime(standardOptionsFileName);
    else
        modifiedTime = previousStandardOptionsFileModificationTime;               // !!! just for now

    if (*firstPass || previousPass != currentPass
        || strcmp(previousStandardOptionsFile, standardOptionsFileName) != 0       /* is not equal */
        || strcmp(previousStandardOptionsSection, standardOptionsSectionName) != 0 /* is not equal */
        || previousStandardOptionsFileModificationTime != modifiedTime || previousLanguage != *outLanguage
        || cache.index == 1                                        /* some kind of reset was made */
    ) {
        if (*firstPass) {
            initCaching();
            *firstPass = false;
        } else {
#ifndef USE_NEW_CXMEMORY
            recoverCachePointZero();
#endif
        }

        initPreCreatedTypes();
        initCwd();
        initOptions();
        initStandardCxrefFileName(fileName);

        processOptions(argc, argv, DONT_PROCESS_FILE_ARGUMENTS);   /* command line opts */
        /* piped options (no include or define options)
           must be before .xrefrc file options, but, the s_cachedOptions
           must be set after .c-xrefrc file, but s_cachedOptions can't contain
           piped options, !!! berk.
        */
        processOptions(nargc, nargv, DONT_PROCESS_FILE_ARGUMENTS);
        reInitCwd(standardOptionsFileName, standardOptionsSectionName);

        tmpIncludeDirs = options.includeDirs;
        options.includeDirs = NULL;

        getAndProcessXrefrcOptions(standardOptionsFileName, standardOptionsSectionName, standardOptionsSectionName);
        discoverStandardDefines();
        discoverBuiltinIncludePaths();

        LIST_APPEND(StringList, options.includeDirs, tmpIncludeDirs);
        if (options.mode != ServerMode && inputFileName == NULL) {
            inputOpened = false;
            goto fini;
        }
        deepCopyOptionsFromTo(&options, &savedOptions);  // before getJavaClassPath, it modifies ???
        processOptions(nargc, nargv, DONT_PROCESS_FILE_ARGUMENTS);
        inputOpened = computeAndOpenInputFile();
        strcpy(previousStandardOptionsFile,standardOptionsFileName);
        strcpy(previousStandardOptionsSection,standardOptionsSectionName);
        previousStandardOptionsFileModificationTime = modifiedTime;
        previousLanguage = *outLanguage;
        previousPass = currentPass;

        // this was before 'getAndProcessXrefrcOptions(df...' I hope it will not cause
        // troubles to move it here, because of autodetection of -javaVersion from jdkcp
        initTokenNamesTables();

        activateCaching();
        placeCachePoint(false);
        deactivateCaching();

        assert(cache.free == cache.points[0].nextLexemP);
        assert(cache.free == cache.points[1].nextLexemP);
    } else {
        deepCopyOptionsFromTo(&savedOptions, &options);
        processOptions(nargc, nargv, DONT_PROCESS_FILE_ARGUMENTS); /* no include or define options */
        inputOpened = computeAndOpenInputFile();
    }
    // reset language once knowing all language suffixes
    *outLanguage = getLanguageFor(fileName);
    inputFileNumber = currentFile.characterBuffer.fileNumber;
    assert(options.mode);
    if (options.mode==XrefMode) {
        if (options.xref2) {
            ppcGenRecord(PPC_INFORMATION, getRealFileName_static(inputFileName));
        } else {
            log_info("Processing '%s'", getRealFileName_static(inputFileName));
        }
    }
 fini:
    initializationsPerInvocation();

    checkExactPositionUpdate(false);

    // so s_input_file_number is not set if the file is not really opened!!!
    LEAVE();
    return inputOpened;
 }

static int power(int x, int y) {
    int res = 1;
    for (int i=0; i<y; i++)
        res *= x;
    return res;
}

void totalTaskEntryInitialisations(void) {
    ENTER();

    // Outputs
    errOut = stderr;

    outputFile = stdout;

    fileAbortEnabled = false;

    // Limits
    assert(MAX_TYPE < power(2,SYMTYPES_BITS));
    assert(MAX_STORAGE_NAMES < power(2,STORAGES_BITS));
    assert(MAX_SCOPES < power(2,SCOPES_BITS));

    // Memory
    initCxMemory();

    memoryInit(&presetOptions.memory, "options memory", NULL, SIZE_optMemory);

    // Inject error handling functions
    setFatalErrorHandlerForMemory(fatalError);
    setInternalCheckFailHandlerForMemory(internalCheckFail);

    // Start time
    // just for very beginning
    fileProcessingStartTime = time(NULL);

    // Data structures
    memset(&counters, 0, sizeof(Counters));
    options.includeDirs = NULL;

    initFileTable(MAX_FILES);
    initNoFileNumber();             /* Sets NO_FILE_NUMBER to something real */

    noPosition = makePosition(NO_FILE_NUMBER, 0, 0);
    inputFileNumber = NO_FILE_NUMBER;

    editorInit();

    LEAVE();
}

static void clearFileItem(FileItem *fileItem) {
    fileItem->isScheduled = false;
    fileItem->scheduledToUpdate = false;
    fileItem->fullUpdateIncludesProcessed = false;
    fileItem->cxLoaded = false;
    fileItem->cxLoading = false;
    fileItem->cxSaved = false;
}

void mainTaskEntryInitialisations(int argc, char **argv) {
    ENTER();

    initLexemEnumNames();

    fileAbortEnabled = false;

    // supposing that file table is still here, but reinit it
    mapOverFileTable(clearFileItem);

#ifndef USE_NEW_CXMEMORY
    // TODO: the following causes long jump, berk.
    // And it can't be removed because of multiple tests
    // failing with "cx_memory resizing required, see file TROUBLES"
    // This just shows how impenetrable the memory management is...
    char *tempAllocated = cxAlloc(CX_MEMORY_CHUNK_SIZE);
    cxFreeUntil(tempAllocated);
#endif

    initReferenceTable(MAX_CXREF_ENTRIES);

    memoryInit(&ppmMemory, "pre-processor macros", NULL, SIZE_ppmMemory);
    allocateMacroArgumentTable(MAX_MACRO_ARGS);
    initOuterCodeBlock();

    // init options as soon as possible! for exampl initCwd needs them
    initOptions();

    initSymbolTable(MAX_SYMBOLS);

    initAllInputs();
    initCwd();

    initTypeCharCodeTab();
    initTypeNames();
    initStorageNames();

    initArchaicTypes();
    previousStandardOptionsFile[0] = 0;
    previousStandardOptionsSection[0] = 0;

    /* now pre-read the option file */
    processOptions(argc, argv, PROCESS_FILE_ARGUMENTS);
    processFileArguments();

#ifndef USE_NEW_CXMEMORY
    /* Ensure CX-memory has room enough for things by invoking memory resize if not */
    /* TODO Is this because CX-memory is just discarded and
     * reallocated empty when resizing is necessary? And the various
     * modes need some initial amount of memory? */
    if (options.mode == RefactoryMode) {
        // some more memory for refactoring task
        assert(options.cxMemoryFactor>=1);
        tempAllocated = (char *)cxAlloc(6*options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE);
        cxFreeUntil(tempAllocated);
    } else {
        log_trace("");
    }
    if (options.mode==XrefMode) {
        // get some memory if cross referencing
        assert(options.cxMemoryFactor>=1);
        tempAllocated = (char *)cxAlloc(3*options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE);
        cxFreeUntil(tempAllocated);
    }
    if (options.cxMemoryFactor > 1) {
        // reinit cxmemory taking into account -mf
        // just make an allocation provoking resizing
        tempAllocated = (char *)cxAlloc(options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE);
        cxFreeUntil(tempAllocated);
    }
#endif

    // must be after processing command line options
    initCaching();

    // enclosed in cache point, because of persistent #define in XrefEdit. WTF?
    int argcount = 0;
    inputFileName = getNextArgumentFile(&argcount);
    char fileName[MAX_FILE_NAME_SIZE];
    if (inputFileName==NULL) {
        char *ss = strmcpy(fileName, cwd);
        if (ss!=fileName && ss[-1] == FILE_PATH_SEPARATOR)
            ss[-1]=0;
        assert(strlen(fileName)+1<MAX_FILE_NAME_SIZE);
        inputFileName=fileName;
    } else {
        strcpy(fileName, inputFileName);
    }

    char standardOptionsFileName[MAX_FILE_NAME_SIZE];
    char standardOptionsSection[MAX_FILE_NAME_SIZE];
    searchStandardOptionsFileAndProjectForFile(fileName, standardOptionsFileName, standardOptionsSection);
    handlePathologicProjectCases(fileName, standardOptionsFileName, standardOptionsSection, false);

    reInitCwd(standardOptionsFileName, standardOptionsSection);

    if (standardOptionsFileName[0]!=0) {
        int dfargc;
        char **dfargv;
        ProcessFileArguments inmode;

        readOptionsFromFile(standardOptionsFileName, &dfargc, &dfargv, standardOptionsSection, standardOptionsSection);
        if (options.mode == RefactoryMode) {
            inmode = DONT_PROCESS_FILE_ARGUMENTS;
        } else if (options.mode==ServerMode) {
            inmode = DONT_PROCESS_FILE_ARGUMENTS;
        } else if (options.create || options.project!=NULL || options.update != UPDATE_DEFAULT) {
            inmode = PROCESS_FILE_ARGUMENTS;
        } else {
            inmode = DONT_PROCESS_FILE_ARGUMENTS;
        }
        // disable error reporting on xref task on this pre-reading of .c-xrefrc
        bool previousNoErrorsOption = options.noErrors;
        if (options.mode==ServerMode) {
            options.noErrors = true;
        }
        // there is a problem with INFILES_ENABLED (update for safetycheck),
        // It should first load cxref file, in order to protect file numbers.
        if (inmode==PROCESS_FILE_ARGUMENTS && options.update && !options.create) { /* TODO: .update != UPDATE_CREATE?!?! */
            //&fprintf(dumpOut, "PREREADING !!!!!!!!!!!!!!!!\n");
            // this makes a problem: I need to preread cxref file before
            // reading input files in order to preserve hash numbers, but
            // I need to read options first in order to have the name
            // of cxref file.
            // I need to read fstab also to remove removed files on update
            processOptions(dfargc, dfargv, DONT_PROCESS_FILE_ARGUMENTS);
            smartReadReferences();
        }
        processOptions(dfargc, dfargv, inmode);
        // recover value of errors messages
        if (options.mode==ServerMode)
            options.noErrors = previousNoErrorsOption;
        checkExactPositionUpdate(false);
        if (inmode == PROCESS_FILE_ARGUMENTS)
            processFileArguments();
    }
    recoverCachePointZero();

    initCaching();

    LEAVE();
}

/* initLogging() is called as the first thing in main() so we look for log command line options here */
static void initLogging(int argc, char *argv[]) {
    char fileName[MAX_FILE_NAME_SIZE+1] = "";
    LogLevel log_level = LOG_ERROR;
    LogLevel console_level = LOG_FATAL;

    for (int i=0; i<argc; i++) {
        /* Levels in the log file, if enabled */
        if (strncmp(argv[i], "-log=", 5)==0)
            strcpy(fileName, &argv[i][5]);
        if (strcmp(argv[i], "-debug") == 0)
            log_level = LOG_DEBUG;
        if (strcmp(argv[i], "-trace") == 0)
            log_level = LOG_TRACE;
        /* Levels on the console */
        if (strcmp(argv[i], "-errors") == 0)
            console_level = LOG_ERROR;
        if (strcmp(argv[i], "-warnings") == 0)
            console_level = LOG_WARN;
        if (strcmp(argv[i], "-infos") == 0)
            console_level = LOG_INFO;
    }

    /* Was there a filename, -log given? */
    if (fileName[0] != '\0') {
        FILE *logFile = openFile(fileName, "w");
        if (logFile != NULL)
            log_add_fp(logFile, log_level);
    }

    /* Always log errors and above to console */
    log_set_level(console_level);

    /* Should we log the arguments? */
    if (true)
        logCommands(argc, argv);
}

static void checkForStartupDelay(int argc, char *argv[]) {
    for (int i=0; i<argc; i++) {
        if (strncmp(argv[i], "-delay=", 7)==0) {
            sleep(atoi(&argv[i][7]));
            return;
        }
    }
}

/* *********************************************************************** */
/* **************************       MAIN      **************************** */
/* *********************************************************************** */

int main(int argc, char *argv[]) {
    checkForStartupDelay(argc, argv);

    /* Options are read very late down below, so we need to setup logging before then */
    initLogging(argc, argv);
    ENTER();

    /* And if we want to run the experimental LSP server, ignore anything else */
    if (want_lsp_server(argc, argv))
        return lsp_server(stdin);

    /* else continue with legacy implementation */
    if (setjmp(memoryResizeJumpTarget) != 0) {
        /* CX_ALLOCC always makes one longjmp back to here before we can
           start processing for real ... Allocating initial CX memory */
        if (cxResizingBlocked) {
            FATAL_ERROR(ERR_ST, "cx_memory resizing required, see file TROUBLES",
                       XREF_EXIT_ERR);
        }
    }

    currentPass = ANY_PASS;
    totalTaskEntryInitialisations();
    mainTaskEntryInitialisations(argc, argv);

    if (options.mode == RefactoryMode)
        refactory();
    if (options.mode == XrefMode)
        xref(argc, argv);
    if (options.mode == ServerMode)
        server(argc, argv);

    LEAVE();
    return 0;
}
