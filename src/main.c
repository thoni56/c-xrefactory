#include "main.h"

#include <stdbool.h>

#include "c_parser.h"
#include "caching.h"
#include "characterreader.h"
#include "commons.h"
#include "complete.h"
#include "constants.h"
#include "cxfile.h"
#include "cxref.h"
#include "editor.h"
#include "extract.h"
#include "filedescriptor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "init.h"
#include "java_parser.h"
#include "javafqttab.h"
#include "jsemact.h"
#include "jslsemact.h"
#include "lexer.h"
#include "list.h"
#include "log.h"
#include "macroargumenttable.h"
#include "memory.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "proto.h"
#include "protocol.h"
#include "recyacc.h"
#include "refactory.h"
#include "reftab.h"
#include "server.h"
#include "symboltable.h"
#include "xref.h"
#include "yacc_parser.h"
#include "yylex.h"

static char previousStandardOptionsFile[MAX_FILE_NAME_SIZE];
static char previousStandardOptionsSection[MAX_FILE_NAME_SIZE];
static char previousOnLineClassPath[MAX_OPTION_LEN];
static time_t previousStdopTime;
static int previousLanguage;
static int previousPass;


#define NEXT_ARG(i) {                                                   \
    char tmpBuff[TMP_BUFF_SIZE];                                        \
    i++;                                                                \
    if (i >= argc) {                                                    \
        sprintf(tmpBuff, "further argument(s) expected after %s", argv[i-1]); \
        errorMessage(ERR_ST,tmpBuff);                                   \
        exit(1);                                                        \
    }                                                                   \
}

int mainHandleSetOption(int argc, char **argv, int i ) {
    char *name, *val;

    NEXT_ARG(i);
    name = argv[i];
    assert(name);
    NEXT_ARG(i);
    val = argv[i];
    xrefSetenv(name, val);
    return i;
}

/* *************************************************************************** */


void searchStandardOptionsFileFor(char *filename, char *optionsFilename, char *section) {
    int fileno;
    bool found=false;
    FILE *options_file;
    int nargc;
    char **nargv;

    optionsFilename[0] = 0;
    section[0]=0;

    if (filename == NULL || options.no_stdoptions)
        return;

    /* Try to find section in HOME config. */
    getXrefrcFileName(optionsFilename);
    options_file = openFile(optionsFilename, "r");
    if (options_file != NULL) {
        // TODO: This reads all arguments, when we only want to know if there is a matching project there?
        found = readOptionsFromFileIntoArgs(options_file, &nargc, &nargv, DONT_ALLOCATE, filename, options.project, section);
        if (found) {
            log_debug("options file '%s' section '%s' found", optionsFilename, section);
        }
        closeFile(options_file);
    }
    if (found)
        return;

    // If automatic selection did not find project, keep previous one
    if (options.project==NULL) {
        // but do this only if file is from cxfile, would be better to
        // check if it is from active project, but nothing is perfect
        // TODO: Where else could it come from (Xref.opt is not used anymore)?

        // TODO: check whether the project still exists in the .c-xrefrc file
        // it may happen that after deletion of the project, the request for active
        // project will return non-existent project. And then return "not found"?
        fileno = getFileNumberFromName(filename);
        if (fileno != noFileIndex && getFileItem(fileno)->isFromCxfile) {
            strcpy(optionsFilename, previousStandardOptionsFile);
            strcpy(section, previousStandardOptionsSection);
            return;
        }
    }
    optionsFilename[0]=0;
}

static void writeOptionsFileMessage(char *file, char *outFName, char *outSect) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (options.refactoringMode==RefactoryMode)
        return;
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
            fprintf(dumpOut, "[C-xref] active project: '%s'\n", outSect);
            fflush(dumpOut);
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
                    fprintf(dumpOut, "[C-xref] new project: '%s'\n", section);
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
    //!!!! hack for .jar files !!!
    if (LANGUAGE(LANG_JAR) || LANGUAGE(LANG_CLASS))
        return false;

    inputFile = NULL;
    inputBuffer = editorFindFile(inputFilename);
    if (inputBuffer == NULL) {
#if defined (__WIN32__)
        inputFile = openFile(inputFilename, "rb");
#else
        inputFile = openFile(inputFilename, "r");
#endif
        if (inputFile == NULL) {
            errorMessage(ERR_CANT_OPEN, inputFilename);
        }
    }
    initInput(inputFile, inputBuffer, "\n", inputFilename);
    if (inputFile==NULL && inputBuffer==NULL) {
        return false;
    } else {
        return true;
    }
}

static void initOptions(void) {
    copyOptionsFromTo(&presetOptions, &options);

    inputFileNumber   = noFileIndex;
}

static void initStandardCxrefFileName(char *inputfile) {
    static char standardCxrefFileName[MAX_FILE_NAME_SIZE];

    extractPathInto(normalizeFileName(inputfile, cwd), standardCxrefFileName);
    strcat(standardCxrefFileName, DEFAULT_CXREF_FILENAME);
    assert(strlen(standardCxrefFileName) < MAX_FILE_NAME_SIZE);

    strcpy(standardCxrefFileName, getRealFileName_static(normalizeFileName(standardCxrefFileName, cwd)));
    assert(strlen(standardCxrefFileName) < MAX_FILE_NAME_SIZE);

    options.cxrefsLocation = standardCxrefFileName;
}

static void initializationsPerInvocation(void) {
    int i;
    parsedClassInfo = parsedClassInfoInit;
    parsedInfo = (CurrentlyParsedInfo){0,};
    for(i=0; i<SPP_MAX; i++) parsedPositions[i] = noPosition;
    s_cxRefPos = noPosition;
    s_olstring[0]=0;
    s_olstringFound = false;
    s_olstringServed = false;
    s_olstringInMbody = NULL;
    s_yygstate = s_initYygstate;
    s_jsl = NULL;
    s_javaObjectSymbol = NULL;
}


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
                addStringListOption(&options.includeDirs, line);
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
    "__restrict=",
    "__restrict__=",
    "__extension__="
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
    for (int i=0; i<sizeof(extra_defines)/sizeof(extra_defines[0]); i++) {
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

static void getAndProcessXrefrcOptions(char *dffname, char *dffsect, char *project) {
    int dfargc;
    char **dfargv;
    if (*dffname != 0 && !options.no_stdoptions) {
        readOptionsFromFile(dffname, &dfargc, &dfargv, dffsect, project);
        // warning, the following can overwrite variables like
        // 's_cxref_file_name' allocated in PPM_MEMORY, then when memory
        // is got back by caching, it may provoke a problem
        processOptions(dfargc, dfargv, DONT_PROCESS_FILE_ARGUMENTS); /* .c-xrefrc opts*/
    }
}

// extern for xref.c extraction
void checkExactPositionUpdate(bool printMessage) {
    if (options.update == UPDATE_FAST && options.exactPositionResolve) {
        options.update = UPDATE_FULL;
        if (printMessage) {
            warningMessage(ERR_ST, "-exactpositionresolve implies full update");
        }
    }
}

static void writeProgressInformation(int progress) {
    static int      lastprogress;
    static time_t   timeZero;
    static bool     dialogDisplayed = false;
    static bool     initialCall = true;
    time_t          ct;

    if (progress == 0 || initialCall) {
        initialCall = false;
        dialogDisplayed = false;
        lastprogress = 0;
        timeZero = time(NULL);
    } else {
        if (progress <= lastprogress) return;
    }
    ct = time(NULL);
    // write progress only if it seems to be longer than 3 sec
    if (dialogDisplayed
        || (progress == 0 && ct-timeZero > 1)
        || (progress != 0 && ct-timeZero >= 1 && 100*((double)ct-timeZero)/progress > 3)
        ) {
        if (!dialogDisplayed) {
            // display progress bar
            fprintf(stdout, "<%s>0 \n", PPC_PROGRESS);
            dialogDisplayed = true;
        }
        fprintf(stdout, "<%s>%d \n", PPC_PROGRESS, progress);
        fflush(stdout);
        lastprogress = progress;
    }
}

void writeRelativeProgress(int progress) {
    writeProgressInformation((100*progressOffset + progress)/progressFactor);
    if (progress==100)
        progressOffset++;
}

bool fileProcessingInitialisations(bool *firstPass,
                                   int argc, char **argv,      // command-line options
                                   int nargc, char **nargv,
                                   Language *outLanguage
) {
    char standardOptionsFileName[MAX_FILE_NAME_SIZE];
    char standardOptionsSectionName[MAX_FILE_NAME_SIZE];
    time_t modifiedTime;
    char *fileName;
    StringList *tmpIncludeDirs;
    bool inputOpened;

    ENTER();

    fileName = inputFilename;
    *outLanguage = getLanguageFor(fileName);
    searchStandardOptionsFileFor(fileName, standardOptionsFileName, standardOptionsSectionName);
    handlePathologicProjectCases(fileName, standardOptionsFileName, standardOptionsSectionName, true);

    initAllInputs();

    if (standardOptionsFileName[0] != 0 )
        modifiedTime = fileModificationTime(standardOptionsFileName);
    else
        modifiedTime = previousStdopTime;               // !!! just for now

    log_trace("Checking previous cp==%s", previousOnLineClassPath);
    log_trace("Checking newcp==%s", options.classpath);
    if (*firstPass
        || previousPass != currentPass
        || strcmp(previousStandardOptionsFile, standardOptionsFileName)
        || strcmp(previousStandardOptionsSection,standardOptionsSectionName)
        || previousStdopTime != modifiedTime
        || previousLanguage!= *outLanguage
        || strcmp(previousOnLineClassPath, options.classpath)
        || cache.cpIndex == 1     /* some kind of reset was made */
    ) {
        if (*firstPass) {
            initCaching();
            *firstPass = false;
        } else {
            recoverCachePointZero();
        }
        strcpy(previousOnLineClassPath, options.classpath);
        assert(strlen(previousOnLineClassPath)<MAX_OPTION_LEN-1);

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
        if (options.mode != ServerMode && inputFilename == NULL) {
            inputOpened = false;
            goto fini;
        }
        copyOptionsFromTo(&options, &savedOptions);  // before getJavaClassPath, it modifies ???
        processOptions(nargc, nargv, DONT_PROCESS_FILE_ARGUMENTS);
        getJavaClassAndSourcePath();
        inputOpened = computeAndOpenInputFile();
        strcpy(previousStandardOptionsFile,standardOptionsFileName);
        strcpy(previousStandardOptionsSection,standardOptionsSectionName);
        previousStdopTime = modifiedTime;
        previousLanguage = *outLanguage;
        previousPass = currentPass;

        // this was before 'getAndProcessXrefrcOptions(df...' I hope it will not cause
        // troubles to move it here, because of autodetection of -javaVersion from jdkcp
        initTokenNamesTables();

        cache.active = true;
        placeCachePoint(false);
        cache.active = false;
        assert(cache.lbcc == cache.cp[0].lbcc);
        assert(cache.lbcc == cache.cp[1].lbcc);
    } else {
        copyOptionsFromTo(&savedOptions, &options);
        processOptions(nargc, nargv, DONT_PROCESS_FILE_ARGUMENTS); /* no include or define options */
        inputOpened = computeAndOpenInputFile();
    }
    // reset language once knowing all language suffixes
    *outLanguage = getLanguageFor(fileName);
    inputFileNumber = currentFile.lexBuffer.buffer.fileNumber;
    assert(options.mode);
    if (options.mode==XrefMode && !javaPreScanOnly) {
        if (options.xref2) {
            ppcGenRecord(PPC_INFORMATION, getRealFileName_static(inputFilename));
        } else {
            log_info("Processing '%s'", getRealFileName_static(inputFilename));
        }
    }
 fini:
    initializationsPerInvocation();
    // some final touch to options
    if (options.debug)
        errOut = dumpOut;
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

static bool optionsOverflowHandler(int n) {
    FATAL_ERROR(ERR_NO_MEMORY, "opiMemory", XREF_EXIT_ERR);
    return true;
}

static void totalTaskEntryInitialisations() {
    ENTER();

    // Outputs
    errOut = stderr;
    dumpOut = stdout;

    communicationChannel = stdout;
    // TODO: how come it is not always Xref, as set in options?
    if (options.mode == ServerMode)
        errOut = stdout;

    fileAbortEnabled = false;

    // Limits
    assert(MAX_TYPE < power(2,SYMTYPES_LN));
    assert(MAX_STORAGE_NAMES < power(2,STORAGES_LN));
    assert(MAX_SCOPES < power(2,SCOPES_LN));

    assert(PPC_MAX_AVAILABLE_REFACTORINGS < MAX_AVAILABLE_REFACTORINGS);

    // Memory

    // TODO: this initialize cxMemory by simulating overflow for one
    // byte(?)
    // NOTE: this is called multiple times at start up since
    // we get more overflows later which cases longjmp's
    int mm = cxMemoryOverflowHandler(1);
    assert(mm);

    // init options memory
    initMemory(((Memory*)&presetOptions.memory), "",
               optionsOverflowHandler, SIZE_optMemory);

    // Inject error handling functions
    memoryUseFunctionForFatalError(fatalError);
    memoryUseFunctionForInternalCheckFail(internalCheckFail);

    // Start time
    // just for very beginning
    fileProcessingStartTime = time(NULL);

    // Data structures
    memset(&counters, 0, sizeof(Counters));
    options.includeDirs = NULL;
    SM_INIT(ftMemory);

    initFileTable(MAX_FILES);
    initNoFileIndex();             /* Sets noFileIndex to something real */

    noPosition = makePosition(noFileIndex, 0, 0);
    inputFileNumber = noFileIndex;
    javaAnonymousClassName.position = noPosition;

    olcxInit();
    editorInit();

    LEAVE();
}

static void clearFileItem(FileItem *fileItem) {
    fileItem->inferiorClasses = fileItem->superClasses = NULL;
    fileItem->directEnclosingInstance = noFileIndex;
    fileItem->isScheduled = false;
    fileItem->scheduledToUpdate = false;
    fileItem->fullUpdateIncludesProcessed = false;
    fileItem->cxLoaded = false;
    fileItem->cxLoading = false;
    fileItem->cxSaved = false;
}

void mainTaskEntryInitialisations(int argc, char **argv) {
    char fileName[MAX_FILE_NAME_SIZE];
    char standardOptionsFileName[MAX_FILE_NAME_SIZE];
    char standardOptionsSection[MAX_FILE_NAME_SIZE];
    char *ss;
    int dfargc;
    char **dfargv;
    int argcount;
    char *sss,*cmdlnInputFile;
    int inmode;
    bool previousNoErrorsOption;

    ENTER();

    fileAbortEnabled = false;

    // supposing that file table is still here, but reinit it
    mapOverFileTable(clearFileItem);

    initReferenceTable(MAX_CXREF_ENTRIES);

    SM_INIT(ppmMemory);
    allocateMacroArgumentTable();
    initOuterCodeBlock();

    // init options as soon as possible! for exampl initCwd needs them
    initOptions();

    initSymbolTable(MAX_SYMBOLS);

    fillJavaStat(&s_initJavaStat,NULL,NULL,NULL,0, NULL, NULL, NULL,
                  symbolTable,NULL,AccessDefault,parsedClassInfoInit,noFileIndex,NULL);
    s_javaStat = StackMemoryAlloc(S_javaStat);
    *s_javaStat = s_initJavaStat;
    javaFqtTableInit(&javaFqtTable, FQT_CLASS_TAB_SIZE);

    // initialize recursive java parsing
    s_yygstate = StackMemoryAlloc(struct yyGlobalState);
    memset(s_yygstate, 0, sizeof(struct yyGlobalState));
    s_initYygstate = s_yygstate;

    initAllInputs();
    initCwd();

    initTypeCharCodeTab();
    initJavaTypePCTIConvertIniTab();
    initTypeNames();
    initStorageNames();

    setupCaching();
    initArchaicTypes();
    previousStandardOptionsFile[0] = 0;
    previousStandardOptionsSection[0] = 0;

    /* now pre-read the option file */
    processOptions(argc, argv, PROCESS_FILE_ARGUMENTS);
    processFileArguments();

    if (options.refactoringMode == RefactoryMode) {
        // some more memory for refactoring task
        assert(options.cxMemoryFactor>=1);
        CX_ALLOCC(sss, 6*options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE, char);
        CX_FREE_UNTIL(sss);
    }
    if (options.mode==XrefMode) {
        // get some memory if cross referencing
        assert(options.cxMemoryFactor>=1);
        CX_ALLOCC(sss, 3*options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE, char);
        CX_FREE_UNTIL(sss);
    }
    if (options.cxMemoryFactor > 1) {
        // reinit cxmemory taking into account -mf
        // just make an allocation provoking resizing
        CX_ALLOCC(sss, options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE, char);
        CX_FREE_UNTIL(sss);
    }

    // must be after processing command line options
    initCaching();

    // enclosed in cache point, because of persistent #define in XrefEdit. WTF?
    argcount = 0;
    inputFilename = cmdlnInputFile = getNextArgumentFile(&argcount);
    if (inputFilename==NULL) {
        ss = strmcpy(fileName, cwd);
        if (ss!=fileName && ss[-1] == FILE_PATH_SEPARATOR)
            ss[-1]=0;
        assert(strlen(fileName)+1<MAX_FILE_NAME_SIZE);
        inputFilename=fileName;
    } else {
        strcpy(fileName, inputFilename);
    }

    searchStandardOptionsFileFor(fileName, standardOptionsFileName, standardOptionsSection);
    handlePathologicProjectCases(fileName, standardOptionsFileName, standardOptionsSection, false);

    reInitCwd(standardOptionsFileName, standardOptionsSection);

    if (standardOptionsFileName[0]!=0) {
        readOptionsFromFile(standardOptionsFileName, &dfargc, &dfargv, standardOptionsSection, standardOptionsSection);
        if (options.refactoringMode == RefactoryMode) {
            inmode = DONT_PROCESS_FILE_ARGUMENTS;
        } else if (options.mode==ServerMode) {
            inmode = DONT_PROCESS_FILE_ARGUMENTS;
        } else if (options.create || options.project!=NULL || options.update != UPDATE_DEFAULT) {
            inmode = PROCESS_FILE_ARGUMENTS;
        } else {
            inmode = DONT_PROCESS_FILE_ARGUMENTS;
        }
        // disable error reporting on xref task on this pre-reading of .c-xrefrc
        previousNoErrorsOption = options.noErrors;
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

// extern required by xref.c extraction
void referencesOverflowed(char *cxMemFreeBase, LongjmpReason mess) {
    ENTER();
    if (mess != LONGJMP_REASON_NONE) {
        log_trace("swapping references to disk");
        if (options.xref2) {
            ppcGenRecord(PPC_INFORMATION, "swapping references to disk");
            ppcGenRecord(PPC_INFORMATION, "");
        } else {
            fprintf(dumpOut, "swapping references to disk (please wait)\n");
            fflush(dumpOut);
        }
    }
    if (options.cxrefsLocation == NULL) {
        FATAL_ERROR(ERR_ST, "sorry no file for cxrefs, use -refs option", XREF_EXIT_ERR);
    }
    for (int i=0; i<includeStackPointer; i++) {
        log_trace("inspecting include %d, fileNumber: %d", i, includeStack[i].lexBuffer.buffer.fileNumber);
        if (includeStack[i].lexBuffer.buffer.file != stdin) {
            int fileIndex = includeStack[i].lexBuffer.buffer.fileNumber;
            getFileItem(fileIndex)->cxLoading = false;
            if (includeStack[i].lexBuffer.buffer.file!=NULL)
                closeCharacterBuffer(&includeStack[i].lexBuffer.buffer);
        }
    }
    if (currentFile.lexBuffer.buffer.file != stdin) {
        log_trace("inspecting current file, fileNumber: %d", currentFile.lexBuffer.buffer.fileNumber);
        int fileIndex = currentFile.lexBuffer.buffer.fileNumber;
        getFileItem(fileIndex)->cxLoading = false;
        if (currentFile.lexBuffer.buffer.file!=NULL)
            closeCharacterBuffer(&currentFile.lexBuffer.buffer);
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
    if (!savingFlag && mess!=LONGJMP_REASON_FILE_ABORT) {
        /* references overflowed, but no whole file readed */
        FATAL_ERROR(ERR_NO_MEMORY, "cxMemory", XREF_EXIT_ERR);
    }
    LEAVE();
}

void getPipedOptions(int *outNargc,char ***outNargv){
    char nsect[MAX_FILE_NAME_SIZE];
    int c;
    *outNargc = 0;
    assert(options.mode);
    if (options.mode == ServerMode) {
        readOptionsFromFileIntoArgs(stdin, outNargc, outNargv, ALLOCATE_IN_SM,
                           "", NULL, nsect);
        /* those options can't contain include or define options, */
        /* sections neither */
        c = getc(stdin);
        if (c==EOF) {
            /* Just log and exit since we don't know if there is someone there... */
            /* We also want a clean exit() if we are going for coverage */
            log_error("Broken pipe");
            exit(-1);
            FATAL_ERROR(ERR_INTERNAL, "broken input pipe", XREF_EXIT_ERR);
        }
    }
}

void mainOpenOutputFile(char *outfile) {
    closeMainOutputFile();
    if (outfile!=NULL) {
        log_trace("Opening output file '%s'", options.outputFileName);
#if defined (__WIN32__)
        // open it as binary file, so that record lengths will be correct
        communicationChannel = openFile(outfile, "wb");
#else
        communicationChannel = openFile(outfile, "w");
#endif
    } else {
        communicationChannel = stdout;
    }
    if (communicationChannel == NULL) {
        errorMessage(ERR_CANT_OPEN, outfile);
        communicationChannel = stdout;
    }
    errOut = communicationChannel;
    dumpOut = communicationChannel;
}

/* initLogging() is called as the first thing in main() so we look for log command line options here */
static void initLogging(int argc, char *argv[]) {
    char fileName[MAX_FILE_NAME_SIZE+1] = "";
    int log_level = LOG_ERROR;
    int console_level = LOG_FATAL;

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
}

/* *********************************************************************** */
/* **************************       MAIN      **************************** */
/* *********************************************************************** */

int main(int argc, char **argv) {
    /* Options are read very late down below, so we need to setup logging before then */
    initLogging(argc, argv);
    ENTER();

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

    // If there is no configuration file given auto-find it
    // TODO: This should probably be done where all other config file
    // discovery is done...
    /* if (options.xrefrc == NULL) { */
    /*     char *configFileName = findConfigFile(cwd); */
    /*     createOptionString(&options.xrefrc, configFileName); */
    /*     // And then the storage will be parallel to that */
    /*     strcpy(&configFileName[strlen(configFileName)-2], "db"); */
    /*     setXrefsLocation(configFileName); */
    /* } */

    // Ok, so there were these five, now four, no three, main operating modes
    if (options.mode == RefactoryMode)
        refactory();
    if (options.mode == XrefMode)
        xref(argc, argv);
    if (options.mode == ServerMode)
        server(argc, argv);

    LEAVE();
    return 0;
}
