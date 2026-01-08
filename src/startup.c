#include "startup.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "argumentsvector.h"
#include "characterreader.h"
#include "commons.h"
#include "constants.h"
#include "counters.h"
#include "cxfile.h"
#include "editor.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "init.h"
#include "lexem.h"
#include "list.h"
#include "log.h"
#include "macroargumenttable.h"
#include "memory.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"
#include "referenceableitemtable.h"
#include "stackmemory.h"
#include "symboltable.h"
#include "xref.h"
#include "yylex.h"

static char previousStandardOptionsFile[MAX_FILE_NAME_SIZE];
static char previousStandardOptionsSection[MAX_FILE_NAME_SIZE];
static time_t previousStandardOptionsFileModificationTime;
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
}

static void initStandardCxrefFileName(char *inputfile) {
    static char standardCxFileName[MAX_FILE_NAME_SIZE];

    extractPathInto(normalizeFileName_static(inputfile, cwd), standardCxFileName);
    strcat(standardCxFileName, DEFAULT_CXREF_FILENAME);
    assert(strlen(standardCxFileName) < MAX_FILE_NAME_SIZE);

    strcpy(standardCxFileName, getRealFileName_static(normalizeFileName_static(standardCxFileName, cwd)));
    assert(strlen(standardCxFileName) < MAX_FILE_NAME_SIZE);

    options.cxFileLocation = standardCxFileName;
}

static void initializationsPerInvocation(void) {
    parsedInfo = (ParsedInfo){0,};
    completionPositionFound = false;
    completionStringServed = false;
    completionStringInMacroBody = NULL;
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
                log_debug("Add include '%s'", line);
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
    /* Compiler builtin types - map to simple type to avoid parse issues */
    "__builtin_va_list char",
    /* Integer types */
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
    "__darwin_wint_t int",
    NULL
};

static char *gcc_defines[] = {
    /* GCC type inference extension (used in stdatomic.h macros) */
    "__auto_type int",
    NULL
};

typedef struct {char *compiler; char **defines;} CompilerDependentDefines;
static CompilerDependentDefines compiler_dependent_defines[] = {
    {"clang", clang_defines},
    {"gcc", gcc_defines}
};


static char *fallback_defines[] = {
    /* C99 */
    "_Bool int",
    "_Pragma(x)",
    /* C11 */
    "_Alignas(x)",
    "_Alignof(x) 8",
    /* C23 */
    "typeof(xxx) int",
    "_BitInt(x) int",
    "_Float128 long double",
    "_Float64 double",
    "_Float32 float",
    "_Float16 float",
    /* Clang feature-detection builtins (stub to false) */
    "__has_feature(x) 0",
    "__has_extension(x) 0",
    "__has_attribute(x) 0",
    "__has_c_attribute(x) 0",
    "__building_module(x) 0",
    /* GNUisms and compiler-specific builtin type fallbacks */
    "__attribute__(xxx)",
    "__alignof__(xxx) 8",
    "__typeof__(xxx) int",
    /* Map 128-bit builtin types to plain types for parsing */
    "__int128_t long",
    "__uint128_t unsigned long",
    /* glibc internal type placeholders (used in math.h) */
    "_Mdouble_ double",
    /* Darwin extensions that sometimes appear as macros */
    "__LEAF",
    "__leaf__",
    "__extension__",
    "__builtin_types_compatible_p(x, y) 0",
    "__builtin_offsetof(x, y) 0",
    "offsetof(x, y) 0",
    "__builtin_expect(x, y) (x)",
    "__builtin_assume(x)",
    "__builtin_va_arg(v, t) (*(t*)0)",
    "__builtin_va_list void"
};

static void addFallbackDefinitions() {
    /* Also define some faked standard base types and GNU-isms */
    for (int i = 0; i < (int)sizeof(fallback_defines) / sizeof(fallback_defines[0]); i++) {
        log_debug("Add definition '%s'", fallback_defines[i]);
        addMacroDefinedByOption(fallback_defines[i]);
    }

    for (int c = 0; c < sizeof(compiler_dependent_defines) / sizeof(CompilerDependentDefines); c++) {
        if (strstr(compiler_identification, compiler_dependent_defines[c].compiler) != NULL) {
            log_debug("Adding compiler specific defines for '%s'",
                      compiler_dependent_defines[c].compiler);
            for (char **d = compiler_dependent_defines[c].defines; *d != NULL; d++) {
                log_debug("Add definition '%s'", *d);
                addMacroDefinedByOption(*d);
            }
        }
    }
}

/* This function discovers the compiler builtin defines by making
 * a call to it and then sets those up as if they where defined on
 * the command line */
static void discoverStandardDefines(void) {
    char line[MAX_OPTION_LEN];
    int len;
    char *tempfile_name;
    FILE *tempfile;
    char command[TMP_BUFF_SIZE];

    ENTER();

    /* Start with fallback definitions so that actual definitions will have priority */
    addFallbackDefinitions();

    tempfile_name = create_temporary_filename();
    assert(strlen(tempfile_name)+1 < MAX_FILE_NAME_SIZE);

    sprintf(command, "%s -E -dM - >%s 2>&1", options.compiler, tempfile_name);

    /* Need to pipe an empty file into gcc, an alternative would be to
       create an empty file, but that seems as much work as this */
    FILE *p = popen(command, "w");
    if (p != NULL) {
        fprintf(p, "\n");  /* Write a newline to provide empty input */
        pclose(p);
    }

    tempfile = openFile(tempfile_name, "r");
    if (tempfile==NULL) {
        log_debug("Could not open tempfile");
        return;
    }
    while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF) {
        if (strncmp(line, "#define", strlen("#define")))
            log_error("Expected #define from compiler standard definitions");
        log_debug("Add definition '%s'", line);
        addMacroDefinedByOption(&line[strlen("#define")+1]);
    }

    /* Disable special Clang Blocks to avoid parsing '^' block prototypes in Clang system headers */
    undefineMacroByName("__BLOCKS__");

    LEAVE();
 }

static void getAndProcessXrefrcOptions(char *optionsFileName, char *optionsSectionName, char *project) {
    if (*optionsFileName != 0) {
        ArgumentsVector args;
        readOptionsFromFile(optionsFileName, &args, optionsSectionName, project);
        // warning, the following can overwrite variables like
        // 'cxref_file_name' allocated in ppmMemory, then when memory
        // is got back by caching, it may provoke a problem
        processOptions(args, PROCESS_FILE_ARGUMENTS_NO); /* .c-xrefrc opts*/
    } else {
        assert(0);
    }
}

/* Memory reset struct */
static struct {
    bool checkPointed;
    int firstFreeStackMemoryIndex;
    int ppmMemoryIndex;
} checkPoint = { .checkPointed = false };

void saveMemoryCheckPoint(void) {
    log_debug("Saving checkpoint: ppmMemoryIndex=%d", ppmMemory.index);

    assert(currentBlock == (CodeBlock*)stackMemory);  // Must be at outermost level
    assert(currentBlock->outerBlock == NULL);         // No nested blocks

    checkPoint.ppmMemoryIndex = ppmMemory.index;
    checkPoint.firstFreeStackMemoryIndex = currentBlock->firstFreeIndex;

    checkPoint.checkPointed = true;
}

void restoreMemoryCheckPoint(void) {
    log_debug("Restoring checkpoint: firstFreeStackMemoryIndex=%p, ppmMemoryIndex=%d",
              checkPoint.firstFreeStackMemoryIndex, checkPoint.ppmMemoryIndex);

    assert(checkPoint.checkPointed);

    initOuterCodeBlock();
    currentBlock->firstFreeIndex = checkPoint.firstFreeStackMemoryIndex;

    ppmMemory.index = checkPoint.ppmMemoryIndex;
    recoverSymbolTableMemory();
    recoverMemoryFromIncludeList();
}


/* Heavy orchestration-level initialization for legacy Server/Xref modes.
 * Handles multi-project server architecture where project settings can change per file.
 *
 * This contrasts with LSP mode's lightweight initialization (parseToCreateReferences + initInput)
 * which assumes one-time project setup.
 *
 * Major phases:
 * 1. Project discovery - find .c-xrefrc file and determine project section
 * 2. Options processing - load project settings, handle changes
 * 3. Compiler interrogation - discover system includes and compiler defines
 * 4. Memory checkpointing - cache expensive discoveries for same-project files
 * 5. Input setup - finally calls computeAndOpenInputFile() -> initInput()
 */
bool initializeFileProcessing(ArgumentsVector baseArgs, ArgumentsVector requestArgs, bool *firstPass) {
    char standardOptionsFileName[MAX_FILE_NAME_SIZE];
    char standardOptionsSectionName[MAX_FILE_NAME_SIZE];
    time_t modifiedTime;
    char *fileName;
    StringList *tmpIncludeDirs;
    bool inputOpened;

    ENTER();

    /* === PHASE 1: Project Discovery === */
    /* Find which .c-xrefrc file and project section applies to this file */
    fileName = inputFileName;
    searchStandardOptionsFileAndProjectForFile(fileName, standardOptionsFileName, standardOptionsSectionName);
    handlePathologicProjectCases(fileName, standardOptionsFileName, standardOptionsSectionName, true);

    initAllInputs();

    if (standardOptionsFileName[0] != 0 )
        modifiedTime = fileModificationTime(standardOptionsFileName);
    else
        modifiedTime = previousStandardOptionsFileModificationTime;               // !!! just for now

    /* Determine if we need full initialization or can restore from checkpoint.
     * Full init required when: first pass, different pass, different project,
     * or the current options file changed. */
    if (previousPass != currentPass                                       /* We are in a different pass */
        || strcmp(previousStandardOptionsFile, standardOptionsFileName) != 0 /* or we are using a different options file */
        || strcmp(previousStandardOptionsSection, standardOptionsSectionName) != 0 /* or a different project */
        || previousStandardOptionsFileModificationTime != modifiedTime       /* or the options file has changed */
    ) {
        /* === PHASE 2: Options File Processing === */
        /* Load and process project settings from .c-xrefrc */
        log_debug("initializeFileProcessing - if-branch with firstPass=%d", *firstPass);
        log_debug("Memories: ppmMemory.index=%d, currentBlock=%p", ppmMemory.index, currentBlock);
        if (*firstPass) {
            *firstPass = false;
        } else {
            log_debug("Restoring memory checkpoint");
            restoreMemoryCheckPoint();
        }

        initCwd();
        initOptions();
        initStandardCxrefFileName(fileName);

        /* Process options in specific order: command line, request args, .c-xrefrc */
        processOptions(baseArgs, PROCESS_FILE_ARGUMENTS_NO);   /* command line opts */
        /* piped options (no include or define options)
           must be before .xrefrc file options, but, the s_cachedOptions
           must be set after .c-xrefrc file, but s_cachedOptions can't contain
           piped options, !!! berk.
        */
        processOptions(requestArgs, PROCESS_FILE_ARGUMENTS_NO);
        reInitCwd(standardOptionsFileName, standardOptionsSectionName);

        tmpIncludeDirs = options.includeDirs;
        options.includeDirs = NULL;

        int savedPass = currentPass;
        currentPass = NO_PASS;
        getAndProcessXrefrcOptions(standardOptionsFileName, standardOptionsSectionName, standardOptionsSectionName);

        /* === PHASE 3: Compiler Interrogation === */
        /* Run compiler to discover system includes and predefined macros (expensive!) */
        discoverBuiltinIncludePaths();  /* Sets compiler_identification, must be before discoverStandardDefines */

        discoverStandardDefines();

        /* === PHASE 4: Memory Checkpoint === */
        /* Save memory state so subsequent files in same project can skip phases 2-3 */
        saveMemoryCheckPoint();

        /* Then for the particular pass */
        currentPass = savedPass;
        getAndProcessXrefrcOptions(standardOptionsFileName, standardOptionsSectionName, standardOptionsSectionName);

        LIST_APPEND(StringList, options.includeDirs, tmpIncludeDirs);

        if (options.mode != ServerMode && inputFileName == NULL) {
            /* TODO Create a test that covers this */
            inputOpened = false;
            goto fini;
        }

        deepCopyOptionsFromTo(&options, &savedOptions);
        processOptions(requestArgs, PROCESS_FILE_ARGUMENTS_NO);

        /* === PHASE 5: Input Setup === */
        inputOpened = computeAndOpenInputFile();  /* Finally calls initInput() */

        /* Save these values as previous */
        strcpy(previousStandardOptionsFile,standardOptionsFileName);
        strcpy(previousStandardOptionsSection,standardOptionsSectionName);
        previousStandardOptionsFileModificationTime = modifiedTime;
        previousPass = currentPass;

    } else {
        /* Same project as last file - restore from checkpoint (fast path) */
        log_debug("initializeFileProcessing - else-branch with firstPass=%d", *firstPass);
        log_debug("Memories: ppmMemory.index=%d, currentBlock=%p", ppmMemory.index, currentBlock);
        restoreMemoryCheckPoint();  /* Skip phases 2-3, restore cached compiler discovery */

        deepCopyOptionsFromTo(&savedOptions, &options);
        processOptions(requestArgs, PROCESS_FILE_ARGUMENTS_NO); /* no include or define options */
        inputOpened = computeAndOpenInputFile();  /* Phase 5 only */
    }

    assert(options.mode);
    if (options.mode==XrefMode) {
        if (options.xref2) {
            ppcGenRecord(PPC_INFORMATION, getRealFileName_static(inputFileName));
        } else {
            log_info("Processing file '%s'", getRealFileName_static(inputFileName));
        }
    }

 fini:
    initializationsPerInvocation();

    checkExactPositionUpdate(false);

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
    assert(MAX_TYPE < power(2,TYPE_BITS));
    assert(STORAGE_ENUMS_MAX < power(2,STORAGES_BITS));
    assert(MAX_SCOPES < power(2,SCOPES_BITS));

    // Strings
    initLexemEnumNames();

    // Memory
    initCxMemory(CX_MEMORY_INITIAL_SIZE);

    memoryInit(&presetOptions.memory, "preset options memory", NULL, OptionsMemorySize);

    // Inject error handling functions
    setFatalErrorHandlerForMemory(fatalError);
    setInternalCheckFailHandlerForMemory(internalCheckFail);

    // Start time
    // just for very beginning
    fileProcessingStartTime = time(NULL);

    // Data structures
    resetAllCounters();
    options.includeDirs = NULL;

    initFileTable(MAX_FILES);

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

void mainTaskEntryInitialisations(ArgumentsVector args) {
    ENTER();

    fileAbortEnabled = false;

    // supposing that file table is still here, but reinit it
    mapOverFileTable(clearFileItem);

    initReferenceableItemTable(MAX_REFS_HASHTABLE_ENTRIES);

    memoryInit(&ppmMemory, "pre-processor macros", NULL, PreprocessorMemorySize);
    allocateMacroArgumentTable(MAX_MACRO_ARGS);
    initOuterCodeBlock();

    // init options as soon as possible! for exampl initCwd needs them
    initOptions();

    initBuiltinTypes();
    initArchaicTypes();

    initSymbolTable(MAX_SYMBOLS_HASHTABLE_ENTRIES);

    initAllInputs();
    initCwd();

    initTypeNames();
    initStorageNames();

    previousStandardOptionsFile[0] = 0;
    previousStandardOptionsSection[0] = 0;

    /* now pre-read the option file */
    processOptions(args, PROCESS_FILE_ARGUMENTS_YES);
    processFileArguments();

    /* Ensure CX-memory has room enough for things by invoking memory resize if not */
    /* TODO Is this because CX-memory is just discarded and
     * reallocated empty when resizing is necessary? And the various
     * modes need some initial amount of memory? */
    char *tempAllocated;
    if (options.mode == RefactoryMode) {
        // some more memory for refactoring task
        assert(options.cxMemoryFactor>=1);
        tempAllocated = (char *)cxAlloc(6*options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE);
        cxFreeUntil(tempAllocated);
    } else {
        log_debug("");
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

    currentLanguage = LANG_C | LANG_YACC;
    /* Initialize token names table with all C and Yacc keywords. */
    initTokenNamesTables();

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
        ArgumentsVector dfargs;
        ProcessFileArguments inmode;

        /* Don't read pass-specific options during initialization - those are handled
         * per-pass in initializeFileProcessing. Setting NO_PASS skips all -passN sections. */
        int savedPass = currentPass;
        currentPass = NO_PASS;
        readOptionsFromFile(standardOptionsFileName, &dfargs, standardOptionsSection, standardOptionsSection);
        currentPass = savedPass;
        if (options.mode == RefactoryMode) {
            inmode = PROCESS_FILE_ARGUMENTS_NO;
        } else if (options.mode==ServerMode) {
            inmode = PROCESS_FILE_ARGUMENTS_NO;
        } else if (options.create || options.project!=NULL || options.update != UPDATE_DEFAULT) {
            inmode = PROCESS_FILE_ARGUMENTS_YES;
        } else {
            inmode = PROCESS_FILE_ARGUMENTS_NO;
        }
        // disable error reporting on xref task on this pre-reading of .c-xrefrc
        bool previousNoErrorsOption = options.noErrors;
        if (options.mode==ServerMode) {
            options.noErrors = true;
        }
        // there is a problem with INFILES_ENABLED (update for safetycheck),
        // It should first load cxref file, in order to protect file numbers.
        if (inmode==PROCESS_FILE_ARGUMENTS_YES && options.update && !options.create) { /* TODO: .update != UPDATE_CREATE?!?! */
            //&fprintf(dumpOut, "PREREADING !!!!!!!!!!!!!!!!\n");
            // this makes a problem: I need to preread cxref file before
            // reading input files in order to preserve hash numbers, but
            // I need to read options first in order to have the name
            // of cxref file.
            // I need to read fstab also to remove removed files on update
            processOptions(dfargs, PROCESS_FILE_ARGUMENTS_NO);
            loadFileNumbersFromStore();
        }
        processOptions(dfargs, inmode);
        // recover value of errors messages
        if (options.mode==ServerMode)
            options.noErrors = previousNoErrorsOption;
        checkExactPositionUpdate(false);
        if (inmode == PROCESS_FILE_ARGUMENTS_YES)
            processFileArguments();
    }

    LEAVE();
}
