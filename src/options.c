#include "options.h"

#include "commandlogger.h"
#include "commons.h"
#include "globals.h"
#include "misc.h"
#include "cxref.h"
#include "yylex.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"
#include "protocol.h"

#include "log.h"

#include "options_config.h"

/* PUBLIC DATA: */

Options options;               // current options
Options savedOptions;
Options presetOptions = {
    /* GENERAL */
    false,                       // command log to /tmp file
    false,                       // exit
    "gcc",                       // path to compiler to use for auto-discovering compiler and defines
    MULE_DEFAULT,                // encoding
    false,                       // completeParenthesis
    IMPORT_ON_DEMAND,            // defaultAddImportStrategy
    false,                       // referenceListWithoutSource
    1,                           // completionOverloadWizardDeep
    0,                           // comment moving level
    NULL,                        // prune name
    NULL,                        // input files
    RC_NONE,                     // continue refactoring
    0,                           // completion case sensitive
    NULL,                        // xrefrc
    NO_EOL_CONVERSION,           // crlfConversion
    NULL,                        // checkVersion
    DONT_DISPLAY_NESTED_CLASSES, // nestedClassDisplaying
    NULL,                        // pushName
    0,                           // parnum2
    "",                          // refactoring parameter 1
    "",                          // refactoring parameter 2
    AVR_NO_REFACTORING,          // refactoring
    false,                       // briefoutput
    NULL,                        // renameTo
    UndefinedMode,               // refactoringMode
    false,                       // xrefactory-II
    NULL,                        // moveTargetFile
#if defined (__WIN32__)
    "c;C",                       // cFilesSuffixes
    "java;JAV",                  // javaFilesSuffixes
#else
    "c:C",                       // cFilesSuffixes
    "java",                      // javaFilesSuffixes
#endif
    true,                        // fileNamesCaseSensitive
    SEARCH_FULL,             // search Tag file specifics
    false,                       // noerrors
    0,                           // fqtNameToCompletions
    NULL,                        // moveTargetClass
    0,                           // TPC_NONE, trivial pre-check
    true,                        // urlGenTemporaryFile
    true,                        // urlautoredirect
    false,                       // exact position
    NULL,                        // -o outputFileName
    NULL,                        // -line lineFileName
    NULL,                        // -I include dirs
    DEFAULT_CXREF_FILENAME,      // -refs

    NULL,                       // file move for safety check
    NULL,
    0,                          // first moved line
    MAXIMAL_INT,                // safety check number of lines moved
    0,                          // new line number of the first line

    "",                         // getValue
    true,                       // javaSlAllowed (autoUpdateFromSrc)

    /* JAVA: */
    false,                      // allowPackagesOnCl
    NULL,                       // sourcepath

    /* MIXED THINGS... */
    false,                      // noIncludeRefs
    true,                       // allowClassFileRefs
    0,
    "",
    RESOLVE_DIALOG_DEFAULT,     // manual symbol resolution TODO: This is different from any of the RESOLVE values above, why?
    NULL,                       // browsed symbol name
    0,
    (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE), // ooChecksBits
    1,                          // cxMemoryFactor
    0,                          /* strict ansi */
    NULL,                       /* project */
    "0:3",                      // olcxlccursor
    "",                         /* olcxSearchString */
    79,                         /* olineLen */
    "*_",                       /* olExtractAddrParPrefix */
    0,                          // extractMode, must be zero TODO Why?
    MAX_COMPLETIONS,            /* maxCompletions */
    0,                          /* editor */
    0,                          /* create */
    "",                         // default classpath
    8,                          /* tabulator */
    -1,                         /* olCursorPos */
    -1,                         /* olMarkPos */
    XrefMode,                   /* operating mode */
    false,                      /* debug */
    false,                      /* trace */
    0,                          /* serverOperation */
    0,                          /* olcxGotoVal */

    /* CXREF options  */
    false,                      /* errors */
    UPDATE_DEFAULT,             /* type of update */
    false,                      // updateOnlyModifiedFiles
    0,                          /* referenceFileCount */

    0,                          // variablesCount
    {},                         // variables

    NULL,                       /* allUsedStringOptions */
    NULL,                       /* allUsedStringListOptions */

    {.area = NULL, .size = 0},  /* memory - options string storage */
};


/* memory where on-line given options are stored */
static Memory2 optMemory;

static char javaSourcePathExpanded[MAX_OPTION_LEN];

static char base[MAX_FILE_NAME_SIZE];

static char previousStandardOptionsFile[MAX_FILE_NAME_SIZE];
static char previousStandardOptionsProject[MAX_FILE_NAME_SIZE];


#define ENV_DEFAULT_VAR_FILE            "${__file}"
#define ENV_DEFAULT_VAR_PATH            "${__path}"
#define ENV_DEFAULT_VAR_NAME            "${__name}"
#define ENV_DEFAULT_VAR_SUFFIX          "${__suff}"
#define ENV_DEFAULT_VAR_THIS_CLASS      "${__this}"
#define ENV_DEFAULT_VAR_SUPER_CLASS     "${__super}"


void aboutMessage(void) {
    char output[REFACTORING_TMP_STRING_SIZE];
    sprintf(output, "C-xrefactory version %s\n", VERSION);
    sprintf(output+strlen(output), "Compiled at %s on %s\n",  __TIME__, __DATE__);
    sprintf(output+strlen(output), "from git revision %s.\n", GIT_HASH);
    strcat(output,                 "(c) 1997-2004 by Xref-Tech, http://www.xref-tech.com\n");
    strcat(output,                 "Released into GPL 2009 by Marian Vittek (SourceForge)\n");
    strcat(output,                 "Work resurrected and continued by Thomas Nilefalk 2015-\n");
    strcat(output,                 "(https://github.com/thoni56/c-xrefactory)\n");
    if (options.exit) {
        strcat(output, "Exiting!");
    }
    if (options.xref2) {
        ppcGenRecord(PPC_INFORMATION, output);
    } else {
        fprintf(stdout, "%s", output);
    }
    if (options.exit)
        exit(XREF_EXIT_BASE);
}


/* *************************************************************************** */
/*                                      OPTIONS                                */

static void usage() {
    fprintf(stdout, "c-xref - a C/Yacc/Java cross-referencer and refactoring tool\n\n");
#if 0
    fprintf(stdout, "Usage:\n\tc-xref <mode> <option>+ <input files>\n\n");
    fprintf(stdout, "mode (one of):\n");
    fprintf(stdout, "\t-xref                     - generate cross-reference data in batch mode\n");
    fprintf(stdout, "\t-server                   - enter edit server mode with c-xref protocol\n");
    fprintf(stdout, "\t-refactor                 - make an automated refactoring\n");
    fprintf(stdout, "\t-lsp                      - enter edit server mode with Language Server Protocol (LSP)\n");
    fprintf(stdout, "\n");
#else
    fprintf(stdout, "Usage:\n\t\tc-xref <option>+ <input files>\n\n");
#endif
    fprintf(stdout, "options:\n");
    fprintf(stdout, "\t-p <project>              - read options from <project> section\n");
    fprintf(stdout, "\t-I <dir>                  - search for includes in <dir>\n");
    fprintf(stdout, "\t-D<macro>[=<body>]        - define macro <macro> with body <body>\n");
    fprintf(stdout, "\t-packages                 - allow packages as input files\n");
    fprintf(stdout, "\t-sourcepath <path>        - set java sources paths\n");
    fprintf(stdout, "\t-classpath <path>         - set java class path\n");
    fprintf(stdout, "\t-filescasesensitive       - file names are case sensitive\n");
    fprintf(stdout, "\t-filescaseunsensitive     - file names are case unsensitive\n");
    fprintf(stdout, "\t-csuffixes=<suffixes>     - list of C files suffixes separated by ':' (or ';')\n");
    fprintf(stdout, "\t-javasuffixes=<suffixes>  - list of Java files suffixes separated by ':' (or ';')\n");
    fprintf(stdout, "\t-xrefrc <file>            - read options from <file> instead of ~/.c-xrefrc\n");
#if 0
    fprintf(stdout, "\t-olinelen=<n>             - length of lines for on-line output\n");
    fprintf(stdout, "\t-oocheckbits=<n>          - object-oriented resolution for completions\n");
    fprintf(stdout, "\t-olcxsearch               - search info about identifier\n");
    fprintf(stdout, "\t-olcxpush                 - generate and push on-line cxrefs\n");
    fprintf(stdout, "\t-olcxrename               - generate and push xrfs for rename\n");
    fprintf(stdout, "\t-olcxlist                 - generate, push and list on-line cxrefs\n");
    fprintf(stdout, "\t-olcxpop                  - pop on-line cxrefs\n");
    fprintf(stdout, "\t-olcxnext                 - next on-line reference\n");
    fprintf(stdout, "\t-olcxprevious             - previous on-line reference\n");
    fprintf(stdout, "\t-olcxgoto<n>              - go to the n-th on-line reference\n");
    fprintf(stdout, "\t-file <file>              - name of the file given to stdin\n");
#endif
    fprintf(stdout, "\t-o <file>                 - write output to <file>\n");
    fprintf(stdout, "\t-refs <file>              - name of file with cxrefs, or directory if refnum > 1\n");
    fprintf(stdout, "\t-refnum=<n>               - number of cxref files\n");
    fprintf(stdout, "\t-exactpositionresolve     - resolve symbols by def. position\n");
    fprintf(stdout, "\t-mf<n>                    - factor increasing cxMemory\n");
    fprintf(stdout, "\t-errors                   - report all error messages on the console\n");
    fprintf(stdout, "\t                            (by default only fatal errors are shown)\n");
    fprintf(stdout, "\t-warnings                 - also report warning messages on the console\n");
    fprintf(stdout, "\t-infos                    - also report informational & warning messages on the console\n");
    fprintf(stdout, "\t-log=<file>               - log all fatal/error/warnings/informational messages to <file>\n");
    fprintf(stdout, "\t-debug                    - also log debug messages in log\n");
    fprintf(stdout, "\t-trace                    - also log trace & debug messages in log\n");
    fprintf(stdout, "\t-commandlog               - write all commands to /tmp/c-xref-command-log (experimental)\n");
    fprintf(stdout, "\t-no-classfiles            - Don't collect references from class files\n");
    fprintf(stdout, "\t-compiler=<path>          - path to compiler to use for autodiscovered includes and defines\n");
    fprintf(stdout, "\t-update                   - update existing references database\n");
    fprintf(stdout, "\t-fastupdate               - fast update (modified files only)\n");
    fprintf(stdout, "\t-fullupdate               - full update (all files)\n");
    fprintf(stdout, "\t-version                  - print version information\n");
}


static void *optAlloc(size_t size) {
    return smAlloc(&options.memory, size);
}

/* Protected type */
typedef struct pointerLocationList {
    void **location;
    struct pointerLocationList *next;
} LocationList;

void **pointerLocationOf(LocationList *list) {
    return list->location;
}

LocationList *nextPointerLocationList(LocationList *list) {
    return list->next;
}

bool containsPointerLocation(LocationList *list, void **location) {
    for (LocationList *l = list; l != NULL; l = l->next)
        if (l->location == location)
            return true;
    return false;
}

static LocationList *concatPointerLocation(void **location, LocationList *next) {
    LocationList *list;
    list = optAlloc(sizeof(LocationList));
    list->location = location;
    list->next = next;
    return list;
}

static void addStringOptionToUsedList(char **location) {
    for (LocationList *l=options.allUsedStringOptions; l!=NULL; l=l->next) {
        // reassignement, do not keep two copies
        if (l->location == (void **)location)
            return;
    }
    options.allUsedStringOptions = concatPointerLocation((void **)location,
                                                         options.allUsedStringOptions);
}

static void addStringListOptionToUsedList(StringList **location) {
    for (LocationList *l=options.allUsedStringListOptions; l!=NULL; l=l->next) {
        // reassignement, do not keep two copies
        if (l->location == (void **)location)
            return;
    }
    options.allUsedStringListOptions = concatPointerLocation((void **)location,
                                                             options.allUsedStringListOptions);
}


char *allocateStringForOption(char **pointerToOption, char *string) {
    addStringOptionToUsedList(pointerToOption);
    char *allocated = optAlloc(strlen(string)+1);
    strcpy(allocated, string);
    *pointerToOption = allocated;
    return allocated;
}

static StringList *concatStringList(StringList *list, char *string) {
    StringList *l = list;

    /* Order is important so concat at the end */
    if (l == NULL) {
        l = optAlloc(sizeof(StringList));
    } else {
        while (l->next != NULL)
            l = l->next;
        l->next = optAlloc(sizeof(StringList));
        l = l->next;
    }
    l->string = optAlloc(strlen(string)+1);

    strcpy(l->string, string);
    return l;
}

void addToStringListOption(StringList **pointerToOption, char *string) {
    if (*pointerToOption == NULL) {
        *pointerToOption = concatStringList(*pointerToOption, string);
        addStringListOptionToUsedList(pointerToOption);
    } else
        concatStringList(*pointerToOption, string);
}

static void shiftPointer(void **location, void *src, void *dst) {
    if (location != NULL && *location != NULL) {
        size_t offset = dst - src;
        *location += offset;
    }
}

static void assertLocationIsInOptionsStructure(void *location, Options *options) {
    assert(location > (void *)options
           && location < (void *)options + ((void *)&options->memory - (void *)options));
}

static void assertLocationIsInMemory(void *l, Options *options) {
    assert(l == NULL || smIsBetween(&options->memory, l, 0, options->memory.index));
}

static void shiftLocationList(LocationList *list, Options *src, Options *dst) {
    for (LocationList *l = list; l != NULL; l = l->next) {
        assertLocationIsInMemory(l, dst);

        /* Location itself is in options structure */
        assertLocationIsInOptionsStructure(l->location, src);
        shiftPointer((void **)&l->location, src, dst);
        assertLocationIsInOptionsStructure(l->location, dst);

        /* Next node, if any, is in options memory area */
        shiftPointer((void **)&l->next, src->memory.area, dst->memory.area);
        assertLocationIsInMemory(l, dst);
    }
}

static void shiftStringOptions(LocationList *list, Options *src, Options *dst) {
    for (LocationList *l = list; l != NULL; l = l->next) {
        /* The string value is in options memory area */
        assertLocationIsInMemory(*l->location, src);
        shiftPointer(l->location, src->memory.area, dst->memory.area);
        assertLocationIsInMemory(*l->location, dst);
    }
}

static void shiftStringList(StringList *list, Options *src, Options *dst) {
    for (StringList *l = list; l != NULL; l = l->next) {
        assertLocationIsInMemory(l, dst);
        shiftPointer((void **)&l->string, src->memory.area, dst->memory.area);
        assertLocationIsInMemory(l->string, dst);

        shiftPointer((void **)&l->next, src->memory.area, dst->memory.area);
        assertLocationIsInMemory(l->next, dst);
    }
}

static void shiftStringListOptions(LocationList *list, Options *src, Options *dst) {
    for (LocationList *l = list; l != NULL; l = l->next) {
        assertLocationIsInOptionsStructure(l->location, dst);

        assertLocationIsInMemory(*l->location, src);
        shiftPointer((void **)l->location, src->memory.area, dst->memory.area);
        assertLocationIsInMemory(*l->location, dst);

        shiftStringList((StringList *)*l->location, src, dst);
    }
}


void deepCopyOptionsFromTo(Options *src, Options *dst) {
    if (dst->memory.area)
        free(dst->memory.area);

    memcpy(dst, src, sizeof(Options));

    dst->memory.area = malloc(dst->memory.size);
    memcpy(dst->memory.area, src->memory.area, dst->memory.size);

    /* Shift the pointer to the list of all String valued options to point to new memory area */
    shiftPointer((void **)&dst->allUsedStringOptions, src->memory.area, dst->memory.area);
    /* Now we can shift all such option fields and the linked list in the dst and dst.memory */
    shiftLocationList(dst->allUsedStringOptions, src, dst);
    shiftStringOptions(dst->allUsedStringOptions, src, dst);

    /* Shift the pointer to the list of all StringList valued options to point to new memory area */
    shiftPointer((void **)&dst->allUsedStringListOptions, src->memory.area, dst->memory.area);
    /* Now we can shift that list... */
    shiftLocationList(dst->allUsedStringListOptions, src, dst);
    /* ... and the StringLists */
    shiftStringListOptions(dst->allUsedStringListOptions, src, dst);
}


void setOptionVariable(char *name, char *value) {
    if (options.variablesCount+1>=MAX_SET_GET_OPTIONS) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "maximum of %d -set options reached", MAX_SET_GET_OPTIONS);
        errorMessage(ERR_ST, tmpBuff);
        options.variablesCount--;
    }

    bool found = false;
    int i;
    for (i=0; i<options.variablesCount; i++) {
        assert(options.variables[i].name);
        if (strcmp(options.variables[i].name, name)==0) {
            found = true;
            break;
        }
    }
    if (!found) {
        options.variables[i].name = allocateStringForOption(&options.variables[i].name, name);
        addStringOptionToUsedList((void *)&options.variables[i].name);
    }
    if (!found || strcmp(options.variables[i].value, value)!=0) {
        options.variables[i].value = allocateStringForOption(&options.variables[i].value, value);
        addStringOptionToUsedList((void *)&options.variables[i].value);
    }
    log_debug("setting variable '%s' to '%s'", name, value);
    if (!found)
        options.variablesCount++;
}


char *getOptionVariable(char *name) {
    char *value = NULL;
    int n = options.variablesCount;

    for (int i=0; i<n; i++) {
        //&fprintf(dumpOut,"checking (%s) %s\n",options.variables.name[i], options.variables.value[i]);
        if (strcmp(options.variables[i].name, name)==0) {
            value = options.variables[i].value;
            break;
        }
    }
    return value;
}

static void scheduleCommandLineEnteredFileToProcess(char *fn) {
    ENTER();
    int fileNumber = addFileNameToFileTable(fn);
    FileItem *fileItem = getFileItem(fileNumber);
    if (options.mode!=ServerMode) {
        // yes in edit server you process also headers, etc.
        fileItem->isArgument = true;
    }
    log_trace("recursively process command line argument file #%d '%s'", fileNumber, fileItem->name);
    if (!options.updateOnlyModifiedFiles) {
        fileItem->isScheduled = true;
    }
    LEAVE();
}

static bool fileNameShouldBePruned(char *fn) {
    for (StringList *s=options.pruneNames; s!=NULL; s=s->next) {
        MapOverPaths(s->string, {
            if (compareFileNames(currentPath, fn) == 0)
                return true;
        });
    }
    return false;
}

void dirInputFile(MAP_FUN_SIGNATURE) {
    char            *dir,*fname, *suff;
    void            *recurseFlag;
    void            *nrecurseFlag;
    char            dirName[MAX_FILE_NAME_SIZE];
    int             isTopDirectory;

    dir = a1;
    fname = file;
    recurseFlag = a4;
    isTopDirectory = *a5;

    if (isTopDirectory == 0) {
        if (strcmp(fname, ".")==0 || strcmp(fname, "..")==0)
            return;
        if (fileNameShouldBePruned(fname))
            return;
        sprintf(dirName, "%s%c%s", dir, FILE_PATH_SEPARATOR, fname);
        strcpy(dirName, normalizeFileName(dirName, cwd));
        if (fileNameShouldBePruned(dirName))
            return;
    } else {
        strcpy(dirName, normalizeFileName(fname, cwd));
    }
    if (strlen(dirName) >= MAX_FILE_NAME_SIZE) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "file name %s is too long", dirName);
        FATAL_ERROR(ERR_ST, tmpBuff, XREF_EXIT_ERR);
    }
    suff = getFileSuffix(fname);
    // Directories are never in editor buffers...
    if (isDirectory(dirName)) {
        if (recurseFlag!=NULL) {
            isTopDirectory = 0;
            nrecurseFlag = &isTopDirectory;
            mapOverDirectoryFiles(dirName, dirInputFile, DO_NOT_ALLOW_EDITOR_FILES,
                              dirName, NULL, NULL, nrecurseFlag, &isTopDirectory);
            editorMapOnNonexistantFiles(dirName, dirInputFile, DEPTH_ANY,
                                        dirName, NULL, NULL, nrecurseFlag, &isTopDirectory);
        } else {
            // no error, let it be
            //& sprintf(tmpBuff, "omitting directory %s, missing '-r' option ?",dirName);
            //& warningMessage(ERR_ST,tmpBuff);
            ;
        }
    } else if (editorFileExists(dirName)) {
        // .class can be inside a jar archive, but this makes problem on
        // recursive read of a directory, it attempts to read .class
        if (isTopDirectory==0
            && !fileNameHasOneOfSuffixes(fname, options.cFilesSuffixes)
            && !fileNameHasOneOfSuffixes(fname, options.javaFilesSuffixes)
            && compareFileNames(suff, ".y")!=0
        ) {
            return;
        }
        scheduleCommandLineEnteredFileToProcess(dirName);
    } else if (containsWildcard(dirName)) {
        char wildcardPath[MAX_OPTION_LEN];
        expandWildcardsInOnePath(dirName, wildcardPath, MAX_OPTION_LEN);
        MapOverPaths(wildcardPath, { dirInputFile(currentPath, "", NULL, NULL, recurseFlag, &isTopDirectory); });
    } else if (isTopDirectory && (!options.allowPackagesOnCommandLine || !packageOnCommandLine(fname))) {
        if (options.mode!=ServerMode) {
            errorMessage(ERR_CANT_OPEN, dirName);
        } else {
            // hacked 16.4.2003 in order to can complete in buffers
            // without existing file
            scheduleCommandLineEnteredFileToProcess(dirName);
        }
    }
}

char *expandPredefinedSpecialVariables_static(char *variable, char *inputFilename) {
    static char expanded[MAX_OPTION_LEN];
    int         i, j;
    char       *suffix;
    char        filename[MAX_FILE_NAME_SIZE];
    char        path[MAX_FILE_NAME_SIZE];
    char        name[MAX_FILE_NAME_SIZE];
    char        thisclass[MAX_FILE_NAME_SIZE];
    char        superclass[MAX_FILE_NAME_SIZE];

    strcpy(filename, getRealFileName_static(inputFilename));
    assert(strlen(filename) < MAX_FILE_NAME_SIZE - 1);
    strcpy(path, directoryName_static(filename));
    strcpy(name, simpleFileNameWithoutSuffix_st(filename));
    suffix = lastOccurenceInString(filename, '.');
    if (suffix == NULL)
        suffix = "";
    else
        suffix++;
    strcpy(thisclass, parsedInfo.currentClassAnswer);
    strcpy(superclass, parsedInfo.currentSuperClassAnswer);
    javaDotifyFileName(thisclass);
    javaDotifyFileName(superclass);

    i = j = 0;
    //& fprintf(dumpOut,"path, name, suff == %s %s %s\n", path, name, suffix);
    while (i < strlen(variable)) {
        if (strncmp(&variable[i], ENV_DEFAULT_VAR_FILE, strlen(ENV_DEFAULT_VAR_FILE))==0) {
            sprintf(&expanded[j], "%s", filename);
            i += strlen(ENV_DEFAULT_VAR_FILE);
            j += strlen(&expanded[j]);
        } else if (strncmp(&variable[i], ENV_DEFAULT_VAR_PATH, strlen(ENV_DEFAULT_VAR_PATH))==0) {
            sprintf(&expanded[j], "%s", path);
            i += strlen(ENV_DEFAULT_VAR_PATH);
            j += strlen(&expanded[j]);
        } else if (strncmp(&variable[i], ENV_DEFAULT_VAR_NAME, strlen(ENV_DEFAULT_VAR_NAME))==0) {
            sprintf(&expanded[j], "%s", name);
            i += strlen(ENV_DEFAULT_VAR_NAME);
            j += strlen(&expanded[j]);
        } else if (strncmp(&variable[i], ENV_DEFAULT_VAR_SUFFIX, strlen(ENV_DEFAULT_VAR_SUFFIX))==0) {
            sprintf(&expanded[j], "%s", suffix);
            i += strlen(ENV_DEFAULT_VAR_SUFFIX);
            j += strlen(&expanded[j]);
        } else if (strncmp(&variable[i], ENV_DEFAULT_VAR_THIS_CLASS, strlen(ENV_DEFAULT_VAR_THIS_CLASS))==0) {
            sprintf(&expanded[j], "%s", thisclass);
            i += strlen(ENV_DEFAULT_VAR_THIS_CLASS);
            j += strlen(&expanded[j]);
        } else if (strncmp(&variable[i], ENV_DEFAULT_VAR_SUPER_CLASS, strlen(ENV_DEFAULT_VAR_SUPER_CLASS))==0) {
            sprintf(&expanded[j], "%s", superclass);
            i += strlen(ENV_DEFAULT_VAR_SUPER_CLASS);
            j += strlen(&expanded[j]);
        } else {
            expanded[j++] = variable[i++];
        }
        assert(j<MAX_OPTION_LEN);
    }
    expanded[j]=0;
    return(expanded);
}

static void expandEnvironmentVariables(char *original, int availableSize, int *len,
                                       bool global_environment_only) {
    char temporary[MAX_OPTION_LEN];
    char variableName[MAX_OPTION_LEN];
    char *value;
    int i, d, j, startIndex, terminationCharacter;
    bool tilde;
    bool expanded;

    i = d = 0;
    log_trace("Expanding environment variables for option '%s'", original);
    while (original[i] && i<availableSize-2) {
        startIndex = -1;
        expanded = false;
        terminationCharacter = 0;
        tilde = false;
#if (!defined (__WIN32__))
        if (i==0 && original[i]=='~' && original[i+1]=='/') {
            startIndex = i;
            terminationCharacter = '~';
            tilde = true;
        }
#endif
        if (original[i]=='$' && original[i+1]=='{') {
            startIndex = i+2;
            terminationCharacter = '}';
        }
        if (original[i]=='%') {
            startIndex = i+1;
            terminationCharacter = '%';
        }
        if (startIndex >= 0) {
            j = startIndex;
            while (isalpha(original[j]) || isdigit(original[j]) || original[j]=='_')
                j++;
            if (j<availableSize-2 && original[j]==terminationCharacter) {
                int len = j-startIndex;
                strncpy(variableName, &original[startIndex], len);
                variableName[len]=0;
                value = NULL;
                if (!global_environment_only) {
                    value = getOptionVariable(variableName);
                }
                if (value==NULL)
                    value = getEnv(variableName);
#if (!defined (__WIN32__))
                if (tilde)
                    value = getEnv("HOME");
#endif
                if (value != NULL) {
                    strcpy(&temporary[d], value);
                    d += strlen(value);
                    expanded = true;
                }
                if (expanded)
                    i = j+1;
            }
        }
        if (!expanded) {
            temporary[d++] = original[i++];
        }
        assert(d<MAX_OPTION_LEN-2);
    }
    temporary[d] = 0;
    *len = d;
    strcpy(original, temporary);
    //&fprintf(dumpOut, "result '%s'\n", original);
}

/* Not official API, public for unittesting */
/* Return EOF if at EOF, but can still return option text, return
   something else if not.  TODO: hide this fact from caller, return
   either text or EOF (probably as a bool return value instead) */
int getOptionFromFile(FILE *file, char *text, int *chars_read) {
    int count, ch;
    int lastCharacter;
    bool inComment;
    bool quoteInOption;

    ch = readChar(file);
    do {
        quoteInOption = false;
        *chars_read = count = 0;
        inComment = false;

        /* Skip all white space? */
        while ((ch>=0 && ch<=' ') || ch=='\n' || ch=='\t')
            ch=readChar(file);

        if (ch==EOF) {
            lastCharacter = EOF;
            goto fini;
        }

        if (ch=='\"') {
            /* Read a double quoted string */
            ch=readChar(file);
            // Escaping the double quote (\") is not allowed, it creates problems
            // when someone finished a section name by \ reverse slash
            while (ch!=EOF && ch!='\"') {
                if (count < MAX_OPTION_LEN-1)
                    text[count++]=ch;
                ch=readChar(file);
            }
            if (ch!='\"' && options.mode!=ServerMode) {
                FATAL_ERROR(ERR_ST, "option string through end of file", XREF_EXIT_ERR);
            }
        } else if (ch=='`') {
            text[count++]=ch;
            ch=readChar(file);
            while (ch!=EOF && ch!='\n' && ch!='`') {
                if (count < MAX_OPTION_LEN-1)
                    text[count++]=ch;
                ch=readChar(file);
            }
            if (count < MAX_OPTION_LEN-1)
                text[count++]=ch;
            if (ch!='`'  && options.mode!=ServerMode) {
                errorMessage(ERR_ST, "option string through end of line");
            }
        } else if (ch=='[') {
            text[count++] = ch;
            ch=readChar(file);
            while (ch!=EOF && ch!='\n' && ch!=']') {
                if (count < MAX_OPTION_LEN-1)
                    text[count++]=ch;
                ch=readChar(file);
            }
            if (ch==']')
                text[count++]=ch;
        } else {
            while (ch!=EOF && ch>' ') {
                if (ch=='\"')
                    quoteInOption = true;
                if (count < MAX_OPTION_LEN-1)
                    text[count++]=ch;
                ch=readChar(file);
            }
        }
        text[count]=0;
        if (quoteInOption && options.mode!=ServerMode) {
            static bool messageWritten = false;
            if (! messageWritten) {
                char tmpBuff[TMP_BUFF_SIZE];
                messageWritten = true;
                sprintf(tmpBuff,"option '%s' contains quotes.", text);
                warningMessage(ERR_ST, tmpBuff);
            }
        }
        /* because QNX paths can start with // */
        if (count>=2 && text[0]=='/' && text[1]=='/') {
            while (ch!=EOF && ch!='\n')
                ch=readChar(file);
            inComment = true;
        }
    } while (inComment);
    if (strcmp(text, END_OF_OPTIONS_STRING)==0) {
        count = 0; lastCharacter = EOF;
    } else {
        lastCharacter = 'A';
    }

 fini:
    text[count] = 0;
    *chars_read = count;

    return lastCharacter;
}

static void processSingleProjectMarker(char *path, char *fileName,
                                       bool *projectUpdated, char *foundProjectName) {
    int length;
#if defined (__WIN32__)
    bool caseSensitivity = false;
#else
    bool caseSensitivity = true;
#endif

    length = strlen(path);
    if (pathncmp(path, fileName, length, caseSensitivity)==0
        && (fileName[length]=='/' || fileName[length]=='\\' || fileName[length]==0)) {
        strcpy(foundProjectName,path);
        assert(strlen(foundProjectName)+1 < MAX_FILE_NAME_SIZE);
        *projectUpdated = true;
    } else {
        *projectUpdated = false;
    }
}

static void processProjectMarker(char *markerText, int markerLength, char *currentProject, char *fileName,
                                 bool *projectUpdated, char *foundProjectName) {
    char *projectMarker;

    /* Remove surrounding brackets */
    markerText[markerLength - 1] = 0;
    projectMarker                = &markerText[1];
    log_debug("processing %s for file %s project==%s", projectMarker, fileName, currentProject);

    *projectUpdated = false;
    if (currentProject != NULL && strcmp(projectMarker, currentProject) == 0) {
        /* TODO This is kind a silly, if we found the project
         * marker for the current project, copy it and say it was
         * updated? So what will the caller do when "updated"? */
        strcpy(foundProjectName, projectMarker);
        assert(strlen(foundProjectName) + 1 < MAX_FILE_NAME_SIZE);
        *projectUpdated = true;
    } else {
        processSingleProjectMarker(projectMarker, fileName, projectUpdated, foundProjectName);
    }
    if (*projectUpdated) {
        strcpy(base, foundProjectName); /* Keep for debugging... */
        assert(strlen(foundProjectName) < MAX_FILE_NAME_SIZE - 1);
        setOptionVariable("__BASE", foundProjectName);
        strcpy(foundProjectName, projectMarker);
        // completely wrong, what about file names from command line ?
        //&strncpy(cwd, foundProjectName, MAX_FILE_NAME_SIZE-1);
        //&cwd[MAX_FILE_NAME_SIZE-1] = 0;
    }
}


static void *allocateSpaceForOption(MemoryKind memoryKind, int count, size_t size) {
    if (memoryKind==ALLOCATE_IN_SM) {
        return smAllocc(&optMemory, count, size);
    } else if (memoryKind==ALLOCATE_IN_PP) {
        return ppmAllocc(count, size);
    } else {
        assert(0);
        return NULL;
    }
}

static int addOptionToArgs(MemoryKind memoryKind, char optionText[], int argc, char *argv[]) {
    char *s = NULL;
    s = allocateSpaceForOption(memoryKind, strlen(optionText) + 1, sizeof(char));
    assert(s);
    strcpy(s, optionText);
    log_trace("option %s read", s);
    argv[argc] = s;
    if (argc < MAX_STD_ARGS - 1)
        argc++;

    return argc;
}

static void ensureNextArgumentIsAFileName(int *i, int argc, char *argv[]) {
    (*i)++;
    if (*i >= argc) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "file name expected after %s", argv[*i - 1]);
        errorMessage(ERR_ST, tmpBuff);
        usage();
        exit(1);
    }
}

static void ensureThereIsAnotherArgument(int *i, int argc, char *argv[]) {
    (*i)++;
    if (*i >= argc) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "further argument(s) expected after %s", argv[*i-1]);
        errorMessage(ERR_ST, tmpBuff);
        usage();
        exit(1);
    }
}


static int handleSetOption(int argc, char **argv, int i ) {
    char *name, *val;

    ensureThereIsAnotherArgument(&i, argc, argv);
    name = argv[i];
    assert(name);
    ensureThereIsAnotherArgument(&i, argc, argv);
    val = argv[i];
    setOptionVariable(name, val);
    return i;
}

bool readOptionsFromFileIntoArgs(FILE *file, int *outArgc, char ***outArgv, MemoryKind memoryKind,
                                 char *fileName, char *project, char *foundProjectName) {
    char optionText[MAX_OPTION_LEN];
    int len, argc, ch, passNumber=0;
    bool projectUpdated, isActivePass;
    bool found = false;
    char **aargv, *argv[MAX_STD_ARGS];

    ENTER();

    argc = 1;
    aargv=NULL;
    projectUpdated = isActivePass = true;
    foundProjectName[0]=0;

    if (memoryKind == ALLOCATE_IN_SM)
        smInit(&optMemory, "argument options", SIZE_optMemory);

    ch = 'a';                    /* Something not EOF */
    while (ch!=EOF) {
        ch = getOptionFromFile(file, optionText, &len);
        assert(strlen(optionText) == len);
        if (ch==EOF) {
            log_trace("got option from file (@EOF): '%s'", optionText);
            break;
        } else {
            log_trace("got option from file: '%s'", optionText);
        }
        if (len>=2 && optionText[0]=='[' && optionText[len-1]==']') {
            log_trace("checking '%s'", optionText);
            expandEnvironmentVariables(optionText+1, MAX_OPTION_LEN, &len, true);
            log_trace("expanded '%s'", optionText);
            processProjectMarker(optionText, len+1, project, fileName, &projectUpdated, foundProjectName);
        } else if (projectUpdated && strncmp(optionText, "-pass", 5) == 0) {
            sscanf(optionText+5, "%d", &passNumber);
            isActivePass = passNumber==currentPass || currentPass==ANY_PASS;
            if (passNumber > maxPasses)
                maxPasses = passNumber;
        } else if (strcmp(optionText,"-set")==0 && (projectUpdated && isActivePass) && memoryKind!=DONT_ALLOCATE) {
            // pre-evaluation of -set
            found = true;
            if (memoryKind != DONT_ALLOCATE) {
                argc = addOptionToArgs(memoryKind, optionText, argc, argv);
            }
            ch = getOptionFromFile(file, optionText, &len);
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memoryKind != DONT_ALLOCATE) {
                argc = addOptionToArgs(memoryKind, optionText, argc, argv);
            }
            ch = getOptionFromFile(file, optionText, &len);
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memoryKind != DONT_ALLOCATE) {
                argc = addOptionToArgs(memoryKind, optionText, argc, argv);
            }
            if (argc < MAX_STD_ARGS) {
                assert(argc >= 3);
                handleSetOption(argc, argv, argc - 3);
            }
        } else if (ch != EOF && (projectUpdated && isActivePass)) {
            found = true;
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memoryKind != DONT_ALLOCATE) {
                argc = addOptionToArgs(memoryKind, optionText, argc, argv);
            }
        }
    }
    if (argc >= MAX_STD_ARGS - 1)
        errorMessage(ERR_ST, "too many options");
    if (found && memoryKind!=DONT_ALLOCATE) {
        // Allocate an array of correct size to return instead of local variable argv
        aargv = allocateSpaceForOption(memoryKind, argc, sizeof(char*));
        aargv[0] = NULL;        /* Not used, ensure NULL */
        for (int i=1; i<argc; i++)
            aargv[i] = argv[i];
    }
    *outArgc = argc;
    *outArgv = aargv;

    LEAVE();

    return found;
}

void readOptionsFromFile(char *fileName, int *nargc, char ***nargv, char *section, char *project) {
    FILE *file;
    char unused[MAX_FILE_NAME_SIZE];

    file = openFile(fileName,"r");
    if (file==NULL)
        FATAL_ERROR(ERR_CANT_OPEN, fileName, XREF_EXIT_ERR);
    readOptionsFromFileIntoArgs(file, nargc, nargv, ALLOCATE_IN_PP, section, project, unused);
    closeFile(file);
}

void readOptionsFromCommand(char *command, int *outArgc, char ***outArgv, char *section) {
    FILE *file;
    char unused[MAX_FILE_NAME_SIZE];

    file = popen(command, "r");
    if (file==NULL)
        FATAL_ERROR(ERR_CANT_OPEN, command, XREF_EXIT_ERR);
    readOptionsFromFileIntoArgs(file, outArgc, outArgv, ALLOCATE_IN_PP, section, NULL, unused);
    closeFile(file);
}

void getPipedOptions(int *outNargc, char ***outNargv) {
    *outNargc = 0;
    assert(options.mode);
    if (options.mode == ServerMode) {
        char unused[MAX_FILE_NAME_SIZE];
        readOptionsFromFileIntoArgs(stdin, outNargc, outNargv, ALLOCATE_IN_SM, "", NULL, unused);
        logCommands(*outNargc, *outNargv);
        /* those options can't contain include or define options, sections neither */
        int c = getc(stdin);
        if (c == EOF) {
            /* Just log and exit since we don't know if there is someone there... */
            /* We also want a clean exit() if we are going for coverage */
            log_error("Broken pipe");
            exit(-1);
            FATAL_ERROR(ERR_INTERNAL, "broken input pipe", XREF_EXIT_ERR);
        }
    }
}

static char *getClassPath(bool defaultClassPathAllowed) {
    char *cp;
    cp = options.classpath;
    if (cp == NULL || *cp==0)
        cp = getEnv("CLASSPATH");
    if (cp == NULL || *cp==0) {
        if (defaultClassPathAllowed)
            cp = defaultClassPath;
        else
            cp = NULL;
    }
    return cp;
}

void javaSetSourcePath(bool defaultClassPathAllowed) {
    char *cp;
    cp = options.sourcePath;
    if (cp == NULL || *cp==0)
        cp = getEnv("SOURCEPATH");
    if (cp == NULL || *cp==0)
        cp = getClassPath(defaultClassPathAllowed);
    if (cp == NULL) {
        javaSourcePaths = NULL;
    } else {
        expandWildcardsInPaths(cp, javaSourcePathExpanded, MAX_OPTION_LEN);
        javaSourcePaths = javaSourcePathExpanded;
    }
}


static void convertPackageNameToPath(char *name, char *path) {
    char *np, *pp;

    for (pp=path,np=name; *np; pp++,np++) {
        if (*np == '.')
            *pp = FILE_PATH_SEPARATOR;
        else
            *pp = *np;
    }
    *pp = 0;
}


static int copyPathSegment(char *cp, char path[]) {
    int index;
    for (index=0; cp[index]!=0 && cp[index]!=CLASS_PATH_SEPARATOR; index++) {
        path[index]=cp[index];
    }
    path[index] = 0;

    return index;
}


static void pathConcat(char path[], char packagePath[]) {
    int len = strlen(path);
    if (len==0 || path[len-1]!=FILE_PATH_SEPARATOR) {
        path[len++] = FILE_PATH_SEPARATOR;
        path[len] = '\0';
    }
    strcat(path, packagePath);
}


bool packageOnCommandLine(char *packageName) {
    char *cp;
    char packagePath[MAX_FILE_NAME_SIZE];
    bool packageFound = false;

    assert(strlen(packageName)<MAX_FILE_NAME_SIZE-1);
    convertPackageNameToPath(packageName, packagePath);

    cp = javaSourcePaths;
    while (cp!=NULL && *cp!=0) {
        char path[MAX_FILE_NAME_SIZE];
        int len = copyPathSegment(cp, path);

        pathConcat(path, packagePath);
        assert(strlen(path)<MAX_FILE_NAME_SIZE-1);

        if (directoryExists(path)) {
            // it is a package name, process all source files
            packageFound = true;
            int isTopDirectory = 1;
            void *recurseFlag = &isTopDirectory;
            dirInputFile(path, "", NULL, NULL, recurseFlag, &isTopDirectory);
        }
        cp += len;
        if (*cp == CLASS_PATH_SEPARATOR)
            cp++;
    }
    return packageFound;
}

static char *canItBeJavaBinPath(char *possibleBinPath) {
    static char filename[MAX_FILE_NAME_SIZE];
    char *path;
    int len;

    path = normalizeFileName(possibleBinPath, cwd);
    len = strlen(path);
#if defined (__WIN32__)
    sprintf(filename, "%s%cjava.exe", path, FILE_PATH_SEPARATOR);
#else
    sprintf(filename, "%s%cjava", path, FILE_PATH_SEPARATOR);
#endif
    assert(len+6<MAX_FILE_NAME_SIZE);
    if (fileExists(filename)) {
        filename[len]=0;
        if (len>4 && compareFileNames(filename+len-3, "bin")==0 && filename[len-4]==FILE_PATH_SEPARATOR) {
            sprintf(filename+len-3, "jre");
            if (directoryExists(filename)) {
                /* This is a JRE or JDK < Java 9 */
                sprintf(filename+len-3, "jre%clib%crt.jar", FILE_PATH_SEPARATOR, FILE_PATH_SEPARATOR);
                assert(strlen(filename)<MAX_FILE_NAME_SIZE-1);
                if (fileExists(filename))
                    return filename;
            } else {
                sprintf(filename+len-3, "jrt-fs.jar");
                if (fileExists(filename))
                    /* In Java 9 the rt.jar was removed and replaced by "implementation dependent" files in lib... */
                    log_warn("Found Java > 8 which don't have a jar that we can read...");
            }
        }
    }
    return NULL;
}


static bool endsWithPathSeparator(char dirname[]) {
    int len = strlen(dirname);

    if (len > 0)
        return dirname[len-1]==FILE_PATH_SEPARATOR;
    else
        return false;
}


static char *getJdkClassPathFromJavaHomeOrPath(void) {
    char *path;
    char dirname[MAX_FILE_NAME_SIZE];
    int len;
    char *dir;

    path = getEnv("JAVA_HOME");
    if (path != NULL) {
        strcpy(dirname, path);
        len = strlen(dirname);
        if (endsWithPathSeparator(dirname))
            len--;
        sprintf(dirname+len, "%cbin", FILE_PATH_SEPARATOR);
        dir = canItBeJavaBinPath(dirname);
        if (dir != NULL)
            return dir;
    }
    path = getEnv("PATH");
    if (path != NULL) {
        MapOverPaths(path, {
            dir = canItBeJavaBinPath(currentPath);
            if (dir != NULL)
                return dir;
        });
    }
    return NULL;
}

static char *getJdkClassPathQuickly(void) {
    char *jdkcp;
    jdkcp = options.jdkClassPath;
    if (jdkcp == NULL || *jdkcp==0) jdkcp = getEnv("JDKCLASSPATH");
    if (jdkcp == NULL || *jdkcp==0) jdkcp = getJdkClassPathFromJavaHomeOrPath();
    return(jdkcp);
}

static char *defaultPossibleJavaBinPaths[] = {
#ifdef __WIN32__
    /* TODO: find a way to search for any j??/1.8.0_*\\bin */
    "C:\\Program Files\\Java\\jdk1.8.0_121\\bin",
    "C:\\Program Files\\Java\\jre1.8.0_251\\bin",
#else
    "/usr/lib/jvm/java-8-openjdk-amd64/bin",
#endif
    NULL
};

static char *getJdkClassPath(void) {
    char *foundPath;
    int i;

    foundPath = getJdkClassPathQuickly();
    if (foundPath != NULL && *foundPath != 0)
        return(foundPath);
    // Can't determine it with quick method, try other methods
    for(i=0; defaultPossibleJavaBinPaths[i]!=NULL; i++) {
        foundPath = canItBeJavaBinPath(defaultPossibleJavaBinPaths[i]);
        if (foundPath != NULL && *foundPath != 0)
            return(foundPath);
    }
    return NULL;
}

char *getJavaHome(void) {
    static char res[MAX_FILE_NAME_SIZE];
    char *pathString;
    int pathLength;

    pathString = getJdkClassPath();
    if (pathString!=NULL && *pathString!=0) {
        pathLength = extractPathInto(pathString, res);
        if (pathLength>0) res[pathLength-1] = 0;
        pathLength = extractPathInto(res, res);
        if (pathLength>0) res[pathLength-1] = 0;
        pathLength = extractPathInto(res, res);
        if (pathLength>0) res[pathLength-1] = 0;
        return res;
    }
    return NULL;
}

bool currentReferenceFileCountMatches(int foundReferenceFileCount) {
    bool check;

    if (options.referenceFileCount == 0)
        check = true;
    else if (options.referenceFileCount == 1)
        check = (foundReferenceFileCount <= 1);
    else
        check = (foundReferenceFileCount == options.referenceFileCount);
    options.referenceFileCount = foundReferenceFileCount;
    return check;
}

static void getXrefrcFileName(char *fileName) {
    int hlen;
    char *home;

    if (options.xrefrc!=NULL) {
        sprintf(fileName, "%s", normalizeFileName(options.xrefrc, cwd));
        return;
    }
    home = getEnv("HOME");
#ifdef __WIN32__
    if (home == NULL) home = "c:\\";
#else
    if (home == NULL) home = "";
#endif
    hlen = strlen(home);
    if (hlen>0 && (home[hlen-1]=='/' || home[hlen-1]=='\\')) {
        sprintf(fileName, "%s%cc-xrefrc", home, FILE_BEGIN_DOT);
    } else {
        sprintf(fileName, "%s%c%cc-xrefrc", home, FILE_PATH_SEPARATOR, FILE_BEGIN_DOT);
    }
    assert(strlen(fileName) < MAX_FILE_NAME_SIZE-1);
}

/* Config file handling */
char *findConfigFile(char *start) {
    char currentDir[strlen(start)+1];
    char *normalizedFileName = normalizeFileName(".c-xrefrc", start);

    strcpy(currentDir, start);
    /* Remove possible trailing file separator */
    if (cwd[strlen(cwd)] == FILE_PATH_SEPARATOR)
        cwd[strlen(cwd)] = '\0';

    while (!fileExists(normalizedFileName)) {
        char *p = strrchr(cwd, FILE_PATH_SEPARATOR);
        if (p == NULL || p == cwd)
            return NULL;
        else {
            *p = 0;
            normalizedFileName = normalizeFileName(".c-xrefrc", cwd);
        }
    }
    return normalizedFileName;
}

#if defined(__WIN32__)
static bool isAbsolutePath(char *p) {
    if (p[0]!=0 && p[1]==':' && p[2]==FILE_PATH_SEPARATOR)
        return true;
    if (p[0]==FILE_PATH_SEPARATOR)
        return true;
    return false;
}
#else
static bool isAbsolutePath(char *p) {
    return p[0] == FILE_PATH_SEPARATOR;
}
#endif


static int handleIncludeOption(int argc, char **argv, int i) {
    int nargc;
    char **nargv;
    ensureNextArgumentIsAFileName(&i, argc, argv);

    readOptionsFromFile(argv[i], &nargc, &nargv, "", NULL);
    processOptions(nargc, nargv, DONT_PROCESS_FILE_ARGUMENTS);

    return i;
}

static struct {
    bool errors;
    bool warnings;
    bool infos;
    bool debug;
    bool trace;
} logging_selected = {false, false, false, false, false};


static bool processAOption(int *argi, int argc, char **argv) {
    int i = *argi;
    if (0) {}
    else if (strncmp(argv[i], "-addimportdefault=",18)==0) {
        sscanf(argv[i]+18, "%d", (int *)&options.defaultAddImportStrategy);
    }
    else if (strcmp(argv[i], "-about")==0) {
        options.serverOperation = OLO_ABOUT;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processBOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-briefoutput")==0)
        options.briefoutput = true;
    else if (strncmp(argv[i], "-browsedsym=",12)==0)     {
        options.browsedSymName = allocateStringForOption(&options.browsedSymName, argv[i]+12);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processCOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-commandlog=", 12)==0)
        options.commandlog = allocateStringForOption(&options.commandlog, argv[i]+12);
    else if (strcmp(argv[i], "-crlfconversion")==0)
        options.eolConversion|=CR_LF_EOL_CONVERSION;
    else if (strcmp(argv[i], "-crconversion")==0)
        options.eolConversion|=CR_EOL_CONVERSION;
    else if (strcmp(argv[i], "-completioncasesensitive")==0)
        options.completionCaseSensitive = true;
    else if (strcmp(argv[i], "-completeparenthesis")==0)
        options.completeParenthesis = true;
    else if (strncmp(argv[i], "-completionoverloadwizarddepth=",27)==0)  {
        sscanf(argv[i]+27, "%d", &options.completionOverloadWizardDepth);
    }
    else if (strncmp(argv[i], "-commentmovinglevel=",20)==0) {
        sscanf(argv[i]+20, "%d", (int*)&options.commentMovingMode);
    }
    else if (strcmp(argv[i], "-continuerefactoring")==0) {
        options.continueRefactoring = RC_CONTINUE;
    }
    else if (strcmp(argv[i], "-continuerefactoring=importSingle")==0)    {
        options.continueRefactoring = RC_IMPORT_SINGLE_TYPE;
    }
    else if (strcmp(argv[i], "-continuerefactoring=importOnDemand")==0)  {
        options.continueRefactoring = RC_IMPORT_ON_DEMAND;
    }
    else if (strcmp(argv[i], "-classpath")==0) {
        ensureNextArgumentIsAFileName(&i, argc, argv);
        options.classpath = allocateStringForOption(&options.classpath, argv[i]);
    }
    else if (strncmp(argv[i], "-csuffixes=",11)==0) {
        options.cFilesSuffixes = allocateStringForOption(&options.cFilesSuffixes, argv[i]+11);
    }
    else if (strcmp(argv[i], "-create")==0)
        options.create = true;
    else if (strncmp(argv[i], "-compiler=", 10)==0) {
        options.compiler = allocateStringForOption(&options.compiler, &argv[i][10]);
    } else return false;
    *argi = i;
    return true;
}

static bool processDOption(int *argi, int argc, char **argv) {
    int i = *argi;

    if (0) {}                   /* For copy/paste/re-order convenience, all tests can start with "else if.." */
    else if (strcmp(argv[i], "-debug")==0)
        options.debug = true;
    // TODO, do this macro allocation differently!!!!!!!!!!!!!
    // just store macros in options and later add them into pp_memory
    else if (strncmp(argv[i], "-D",2)==0)
        addMacroDefinedByOption(argv[i]+2);
    else if (strcmp(argv[i], "-displaynestedwithouters")==0) {
        options.displayNestedClasses = DISPLAY_NESTED_CLASSES;
    }
    else return false;

    *argi = i;
    return true;
}

static bool processEOption(int *argi, int argc, char **argv) {
    int i = *argi;

    if (0) {}
    else if (strcmp(argv[i], "-errors")==0) {
        options.errors = true;
        logging_selected.errors = true;
    } else if (strcmp(argv[i], "-exit")==0) {
        log_debug("Exiting");
        exit(XREF_EXIT_BASE);
    }
    else if (strcmp(argv[i], "-editor=emacs")==0) {
        options.editor = EDITOR_EMACS;
    }
    else if (strcmp(argv[i], "-editor=jedit")==0) {
        options.editor = EDITOR_JEDIT;
    }
    else if (strncmp(argv[i], "-extractAddrParPrefix=",22)==0) {
        char tmpString[TMP_STRING_SIZE];
        sprintf(tmpString, "*%s", argv[i]+22);
        // TODO Not used from any editor client - it's initialized to
        // "*_", replace by constant?  This is the prefix required
        // when extracting a function which requires out parameters
        // The underscore is the prefix of the parameters so that the
        // code *inside* the new function can use the same variable names
        options.olExtractAddrParPrefix = allocateStringForOption(&options.olExtractAddrParPrefix, tmpString);
    }
    else if (strcmp(argv[i], "-exactpositionresolve")==0) {
        options.exactPositionResolve = true;
    }
    else if (strncmp(argv[i], "-encoding=", 10)==0) {
        if (options.fileEncoding == MULE_DEFAULT) {
            if (strcmp(argv[i], "-encoding=default")==0) {
                options.fileEncoding = MULE_DEFAULT;
            } else if (strcmp(argv[i], "-encoding=european")==0) {
                options.fileEncoding = MULE_EUROPEAN;
            } else if (strcmp(argv[i], "-encoding=euc")==0) {
                options.fileEncoding = MULE_EUC;
            } else if (strcmp(argv[i], "-encoding=sjis")==0) {
                options.fileEncoding = MULE_SJIS;
            } else if (strcmp(argv[i], "-encoding=utf")==0) {
                options.fileEncoding = MULE_UTF;
            } else if (strcmp(argv[i], "-encoding=utf-8")==0) {
                options.fileEncoding = MULE_UTF_8;
            } else if (strcmp(argv[i], "-encoding=utf-16")==0) {
                options.fileEncoding = MULE_UTF_16;
            } else if (strcmp(argv[i], "-encoding=utf-16le")==0) {
                options.fileEncoding = MULE_UTF_16LE;
            } else if (strcmp(argv[i], "-encoding=utf-16be")==0) {
                options.fileEncoding = MULE_UTF_16BE;
            } else {
                char tmpString[TMP_BUFF_SIZE];
                sprintf(tmpString, "unsupported encoding, available values are 'default', 'european', 'euc', 'sjis', 'utf', 'utf-8', 'utf-16', 'utf-16le' and 'utf-16be'.");
                formatOutputLine(tmpString, ERROR_MESSAGE_STARTING_OFFSET);
                errorMessage(ERR_ST, tmpString);
            }
        }
    }
    else
        return false;
    *argi = i;
    return true;
}

static bool processFOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-filescasesensitive")==0) {
        options.fileNamesCaseSensitive = true;
    }
    else if (strcmp(argv[i], "-filescaseunsensitive")==0) {
        options.fileNamesCaseSensitive = false;
    }
    else if (strcmp(argv[i], "-fastupdate")==0)  {
        options.update = UPDATE_FAST;
        options.updateOnlyModifiedFiles = true;
    }
    else if (strcmp(argv[i], "-fullupdate")==0) {
        options.update = UPDATE_FULL;
        options.updateOnlyModifiedFiles = false;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processGOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-get")==0) {
        ensureThereIsAnotherArgument(&i, argc, argv);
        options.variableToGet = allocateStringForOption(&options.variableToGet, argv[i]);
        options.serverOperation = OLO_GET_ENV_VALUE;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processHOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (strcmp(argv[i], "-help")==0) {
        usage();
        exit(0);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processIOption(int *argi, int argc, char **argv) {
    int i = * argi;

    if (0) {}
    else if (strcmp(argv[i], "-I")==0) {
        /* include dir */
        i++;
        if (i >= argc) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "directory name expected after -I");
            errorMessage(ERR_ST,tmpBuff);
            usage();
        }
        addToStringListOption(&options.includeDirs, argv[i]);
    }
    else if (strncmp(argv[i], "-I", 2)==0 && argv[i][2]!=0) {
        addToStringListOption(&options.includeDirs, argv[i]+2);
    }
    else if (strcmp(argv[i], "-infos")==0) {
        logging_selected.infos = true;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processJOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-javasuffixes=",14)==0) {
        options.javaFilesSuffixes = allocateStringForOption(&options.javaFilesSuffixes, argv[i]+14);
    }
    else if (strcmp(argv[i], "-jdkclasspath")==0 || strcmp(argv[i], "-javaruntime")==0) {
        ensureNextArgumentIsAFileName(&i, argc, argv);
        options.jdkClassPath = allocateStringForOption(&options.jdkClassPath, argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processKOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else return false;
    *argi = i;
    return true;
}

static bool processLOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-log=", 5)==0) {
        ;                       /* Already handled in initLogging() */
    }
    else return false;
    *argi = i;
    return true;
}

static bool processMOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-mf=", 4)==0) {
        int mf;
        sscanf(argv[i]+4, "%d", &mf);
        if (mf<0 || mf>=255) {
            FATAL_ERROR(ERR_ST, "memory factor out of range <1,255>", XREF_EXIT_ERR);
        }
        options.cxMemoryFactor = mf;
    }
    else if (strncmp(argv[i], "-maxcompls=",11)==0)  {
        sscanf(argv[i]+11, "%d", &options.maxCompletions);
    }
    else if (strncmp(argv[i], "-movetargetclass=",17)==0) {
        options.moveTargetClass = allocateStringForOption(&options.moveTargetClass, argv[i]+17);
    }
    else if (strncmp(argv[i], "-movetargetfile=",16)==0) {
        options.moveTargetFile = allocateStringForOption(&options.moveTargetFile, argv[i]+16);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processNOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-no-includerefs")==0)
        options.noIncludeRefs = true;
    else if (strcmp(argv[i], "-no-includerefresh")==0)
        options.noIncludeRefs=true;
    else if (strcmp(argv[i], "-no-classfiles")==0)
        options.allowClassFileRefs = false;
    else if (strcmp(argv[i], "-no-autoupdatefromsrc")==0)
        options.javaSlAllowed = false;
    else if (strcmp(argv[i], "-no-errors")==0)
        options.noErrors = true;
    else
        return false;
    *argi = i;
    return true;
}

static bool processOOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-oocheckbits=",13)==0)    {
        sscanf(argv[i]+13, "%o", &options.ooChecksBits);
    }
    else if (strncmp(argv[i], "-olinelen=",10)==0) {
        sscanf(argv[i]+10, "%d",&options.olineLen);
        if (options.olineLen == 0)
            options.olineLen = 79;
        else options.olineLen--;
    }
    else if (strncmp(argv[i], "-olcursor=",10)==0) {
        sscanf(argv[i]+10, "%d",&options.olCursorOffset);
    }
    else if (strncmp(argv[i], "-olmark=",8)==0) {
        sscanf(argv[i]+8, "%d",&options.olMarkOffset);
    }
    else if (strcmp(argv[i], "-olcxextract")==0) {
        options.serverOperation = OLO_EXTRACT;
    }
    else if (strcmp(argv[i], "-olcxtrivialprecheck")==0) {
        options.serverOperation = OLO_TRIVIAL_PRECHECK;
    }
    else if (strcmp(argv[i], "-olmanualresolve")==0) {
        options.manualResolve = RESOLVE_DIALOG_ALWAYS;
    }
    else if (strcmp(argv[i], "-olnodialog")==0) {
        options.manualResolve = RESOLVE_DIALOG_NEVER;
    }
    else if (strcmp(argv[i], "-olchecklinkage")==0) {
        options.ooChecksBits |= OOC_LINKAGE_CHECK;
    }
    else if (strcmp(argv[i], "-olcheckaccess")==0) {
        options.ooChecksBits |= OOC_ACCESS_CHECK;
    }
    else if (strcmp(argv[i], "-olnocheckaccess")==0) {
        options.ooChecksBits &= ~OOC_ACCESS_CHECK;
    }
    else if (strcmp(argv[i], "-olallchecks")==0) {
        options.ooChecksBits |= OOC_ALL_CHECKS;
    }
    else if (strncmp(argv[i], "-olfqtcompletionslevel=",23)==0) {
        options.fqtNameToCompletions = 1;
        sscanf(argv[i]+23, "%d",&options.fqtNameToCompletions);
    }
    else if (strcmp(argv[i], "-olexmacro")==0)
        options.extractMode=EXTRACT_MACRO;
    else if (strcmp(argv[i], "-olexvariable")==0)
        options.extractMode=EXTRACT_VARIABLE;
    else if (strcmp(argv[i], "-olcxrename")==0)
        options.serverOperation = OLO_RENAME;
    else if (strcmp(argv[i], "-olcxencapsulate")==0)
        options.serverOperation = OLO_ENCAPSULATE;
    else if (strcmp(argv[i], "-olcxencapsulatesc1")==0)
        options.serverOperation = OLO_PUSH_ENCAPSULATE_SAFETY_CHECK;
    else if (strcmp(argv[i], "-olcxencapsulatesc2")==0)
        options.serverOperation = OLO_ENCAPSULATE_SAFETY_CHECK;
    else if (strcmp(argv[i], "-olcxargmanip")==0)
        options.serverOperation = OLO_ARG_MANIP;
    else if (strcmp(argv[i], "-olcxdynamictostatic1")==0)
        options.serverOperation = OLO_VIRTUAL2STATIC_PUSH;
    else if (strcmp(argv[i], "-olcxsafetycheck2")==0)
        options.serverOperation = OLO_SAFETY_CHECK2;
    else if (strcmp(argv[i], "-olcxgotodef")==0)
        options.serverOperation = OLO_GOTO_DEF;
    else if (strcmp(argv[i], "-olcxgotocaller")==0)
        options.serverOperation = OLO_GOTO_CALLER;
    else if (strcmp(argv[i], "-olcxgotocurrent")==0)
        options.serverOperation = OLO_GOTO_CURRENT;
    else if (strcmp(argv[i], "-olcxgetcurrentrefn")==0)
        options.serverOperation=OLO_GET_CURRENT_REFNUM;
    else if (strcmp(argv[i], "-olcxgetlastimportline")==0)
        options.serverOperation=OLO_GET_LAST_IMPORT_LINE;
    else if (strncmp(argv[i], "-olcxgotoparname",16)==0) {
        options.serverOperation = OLO_GOTO_PARAM_NAME;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+16, "%d", &options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxgetparamcoord",18)==0) {
        options.serverOperation = OLO_GET_PARAM_COORDINATES;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+18, "%d", &options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxparnum=",12)==0) {
        options.olcxGotoVal = 0;
        sscanf(argv[i]+12, "%d", &options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxparnum2=",13)==0) {
        options.parnum2 = 0;
        sscanf(argv[i]+13, "%d", &options.parnum2);
    }
    else if (strcmp(argv[i], "-olcxtops")==0)
        options.serverOperation = OLO_SHOW_TOP;
    else if (strcmp(argv[i], "-olcxtoptype")==0)
        options.serverOperation = OLO_SHOW_TOP_TYPE;
    else if (strcmp(argv[i], "-olcxtopapplcl")==0)
        options.serverOperation = OLO_SHOW_TOP_APPL_CLASS;
    else if (strcmp(argv[i], "-olcxshowctree")==0)
        options.serverOperation = OLO_SHOW_CLASS_TREE;
    else if (strcmp(argv[i], "-olcxclasstree")==0)
        options.serverOperation = OLO_CLASS_TREE;
    else if (strcmp(argv[i], "-olcxsyntaxpass")==0)
        options.serverOperation = OLO_SYNTAX_PASS_ONLY;
    else if (strcmp(argv[i], "-olcxprimarystart")==0) {
        options.serverOperation = OLO_GET_PRIMARY_START;
    }
    else if (strcmp(argv[i], "-olcxuselesslongnames")==0)
        options.serverOperation = OLO_USELESS_LONG_NAME;
    else if (strcmp(argv[i], "-olcxuselesslongnamesinclass")==0)
        options.serverOperation = OLO_USELESS_LONG_NAME_IN_CLASS;
    else if (strcmp(argv[i], "-olcxmaybethis")==0)
        options.serverOperation = OLO_MAYBE_THIS;
    else if (strcmp(argv[i], "-olcxnotfqt")==0)
        options.serverOperation = OLO_NOT_FQT_REFS;
    else if (strcmp(argv[i], "-olcxnotfqtinclass")==0)
        options.serverOperation = OLO_NOT_FQT_REFS_IN_CLASS;
    else if (strcmp(argv[i], "-olcxgetrefactorings")==0)     {
        options.serverOperation = OLO_GET_AVAILABLE_REFACTORINGS;
    }
    else if (strcmp(argv[i], "-olcxpush")==0)
        options.serverOperation = OLO_PUSH;
    else if (strcmp(argv[i], "-olcxrepush")==0)
        options.serverOperation = OLO_REPUSH;
    else if (strcmp(argv[i], "-olcxpushonly")==0)
        options.serverOperation = OLO_PUSH_ONLY;
    else if (strcmp(argv[i], "-olcxpushandcallmacro")==0)
        options.serverOperation = OLO_PUSH_AND_CALL_MACRO;
    else if (strcmp(argv[i], "-olcxpushallinmethod")==0)
        options.serverOperation = OLO_PUSH_ALL_IN_METHOD;
    else if (strcmp(argv[i], "-olcxmmprecheck")==0)
        options.serverOperation = OLO_MM_PRE_CHECK;
    else if (strcmp(argv[i], "-olcxppprecheck")==0)
        options.serverOperation = OLO_PP_PRE_CHECK;
    else if (strcmp(argv[i], "-olcxpushforlm")==0) {
        options.serverOperation = OLO_PUSH_FOR_LOCALM;
        options.manualResolve = RESOLVE_DIALOG_NEVER;
    }
    else if (strcmp(argv[i], "-olcxpushglobalunused")==0)
        options.serverOperation = OLO_GLOBAL_UNUSED;
    else if (strcmp(argv[i], "-olcxpushfileunused")==0)
        options.serverOperation = OLO_LOCAL_UNUSED;
    else if (strcmp(argv[i], "-olcxlist")==0)
        options.serverOperation = OLO_LIST;
    else if (strcmp(argv[i], "-olcxlisttop")==0)
        options.serverOperation=OLO_LIST_TOP;
    else if (strcmp(argv[i], "-olcxpop")==0)
        options.serverOperation = OLO_POP;
    else if (strcmp(argv[i], "-olcxpoponly")==0)
        options.serverOperation =OLO_POP_ONLY;
    else if (strcmp(argv[i], "-olcxnext")==0)
        options.serverOperation = OLO_NEXT;
    else if (strcmp(argv[i], "-olcxprevious")==0)
        options.serverOperation = OLO_PREVIOUS;
    else if (strcmp(argv[i], "-olcxcomplet")==0)
        options.serverOperation=OLO_COMPLETION;
    else if (strcmp(argv[i], "-olcxmctarget")==0)
        options.serverOperation=OLO_SET_MOVE_CLASS_TARGET;
    else if (strcmp(argv[i], "-olcxmmtarget")==0)
        options.serverOperation=OLO_SET_MOVE_METHOD_TARGET;
    else if (strcmp(argv[i], "-olcxcurrentclass")==0)
        options.serverOperation=OLO_GET_CURRENT_CLASS;
    else if (strcmp(argv[i], "-olcxcurrentsuperclass")==0)
        options.serverOperation=OLO_GET_CURRENT_SUPER_CLASS;
    else if (strcmp(argv[i], "-olcxmethodlines")==0)
        options.serverOperation=OLO_GET_METHOD_COORD;
    else if (strcmp(argv[i], "-olcxclasslines")==0)
        options.serverOperation=OLO_GET_CLASS_COORD;
    else if (strcmp(argv[i], "-olcxgetsymboltype")==0)
        options.serverOperation=OLO_GET_SYMBOL_TYPE;
    else if (strcmp(argv[i], "-olcxgetprojectname")==0) {
        options.serverOperation=OLO_ACTIVE_PROJECT;
    }
    else if (strcmp(argv[i], "-olcxgetjavahome")==0) {
        options.serverOperation=OLO_JAVA_HOME;
    }
    else if (strncmp(argv[i], "-olcxlccursor=",14)==0) {
        // position of the cursor in line:column format
        options.olcxlccursor = allocateStringForOption(&options.olcxlccursor, argv[i]+14);
    }
    else if (strcmp(argv[i], "-olcxsearch")==0)
        options.serverOperation = OLO_SEARCH;
    else if (strncmp(argv[i], "-olcxcplsearch=",15)==0) {
        options.serverOperation=OLO_SEARCH;
        options.olcxSearchString = allocateStringForOption(&options.olcxSearchString, argv[i]+15);
    }
    else if (strncmp(argv[i], "-olcxtagsearch=",15)==0) {
        options.serverOperation=OLO_TAG_SEARCH;
        options.olcxSearchString = allocateStringForOption(&options.olcxSearchString, argv[i]+15);
    }
    else if (strcmp(argv[i], "-olcxtagsearchforward")==0) {
        options.serverOperation=OLO_TAG_SEARCH_FORWARD;
    }
    else if (strcmp(argv[i], "-olcxtagsearchback")==0) {
        options.serverOperation=OLO_TAG_SEARCH_BACK;
    }
    else if (strncmp(argv[i], "-olcxpushname=",14)==0)   {
        options.serverOperation = OLO_PUSH_NAME;
        options.pushName = allocateStringForOption(&options.pushName, argv[i]+14);
    }
    else if (strncmp(argv[i], "-olcxpushspecialname=",21)==0)    {
        options.serverOperation = OLO_PUSH_SPECIAL_NAME;
        options.pushName = allocateStringForOption(&options.pushName, argv[i]+21);
    }
    else if (strncmp(argv[i], "-olcomplselect",14)==0) {
        options.serverOperation=OLO_CSELECT;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+14, "%d",&options.olcxGotoVal);
    }
    else if (strcmp(argv[i], "-olcomplback")==0) {
        options.serverOperation=OLO_COMPLETION_BACK;
    }
    else if (strcmp(argv[i], "-olcomplforward")==0) {
        options.serverOperation=OLO_COMPLETION_FORWARD;
    }
    else if (strncmp(argv[i], "-olcxcgoto",10)==0) {
        options.serverOperation = OLO_CGOTO;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+10, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxtaggoto",12)==0) {
        options.serverOperation = OLO_TAGGOTO;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+12, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxtagselect",14)==0) {
        options.serverOperation = OLO_TAGSELECT;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+14, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxcbrowse",12)==0) {
        options.serverOperation = OLO_BROWSE_COMPLETION;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+12, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxgoto",9)==0) {
        options.serverOperation = OLO_GOTO;
        options.olcxGotoVal = 0;
        sscanf(argv[i]+9, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(argv[i], "-olcxfilter=",12)==0) {
        options.serverOperation = OLO_REF_FILTER_SET;
        sscanf(argv[i]+12, "%d",&options.filterValue);
    }
    else if (strncmp(argv[i], "-olcxmenusingleselect",21)==0) {
        options.serverOperation = OLO_MENU_SELECT_ONLY;
        options.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+21, "%d",&options.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i], "-olcxmenuselect",15)==0) {
        options.serverOperation = OLO_MENU_SELECT;
        options.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+15, "%d",&options.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i], "-olcxmenuinspectdef",19)==0) {
        options.serverOperation = OLO_MENU_INSPECT_DEF;
        options.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+19, "%d",&options.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i], "-olcxmenuinspectclass",21)==0) {
        options.serverOperation = OLO_MENU_INSPECT_CLASS;
        options.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+21, "%d",&options.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i], "-olcxctinspectdef",17)==0) {
        options.serverOperation = OLO_CT_INSPECT_DEF;
        options.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+17, "%d",&options.olcxMenuSelectLineNum);
    }
    else if (strcmp(argv[i], "-olcxmenuall")==0) {
        options.serverOperation = OLO_MENU_SELECT_ALL;
    }
    else if (strcmp(argv[i], "-olcxmenunone")==0) {
        options.serverOperation = OLO_MENU_SELECT_NONE;
    }
    else if (strcmp(argv[i], "-olcxmenugo")==0) {
        options.serverOperation = OLO_MENU_GO;
    }
    else if (strncmp(argv[i], "-olcxmenufilter=",16)==0) {
        options.serverOperation = OLO_MENU_FILTER_SET;
        sscanf(argv[i]+16, "%d",&options.filterValue);
    }
    else if (strcmp(argv[i], "-optinclude")==0) {
        i = handleIncludeOption(argc, argv, i);
    }
    else if (strcmp(argv[i], "-o")==0) {
        ensureNextArgumentIsAFileName(&i, argc, argv);
        options.outputFileName = allocateStringForOption(&options.outputFileName, argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processPOption(int *argi, int argc, char **argv) {
    int     i = *argi;

    if (0) {}
    else if (strncmp(argv[i], "-pause",5)==0) {
        /* Pause to be able to attach with debugger... */
        ensureThereIsAnotherArgument(&i, argc, argv);
        sleep(atoi(argv[i]));
    }
    else if (strncmp(argv[i], "-pass",5)==0) {
        errorMessage(ERR_ST, "'-pass' option can't be entered from command line");
    }
    else if (strcmp(argv[i], "-packages")==0) {
        options.allowPackagesOnCommandLine = true;
    }
    else if (strcmp(argv[i], "-p")==0) {
        ensureNextArgumentIsAFileName(&i, argc, argv);
        log_trace("Current project '%s'", argv[i]);
        options.project = allocateStringForOption(&options.project, argv[i]);
    }
    else if (strcmp(argv[i], "-preload")==0) {
        char *file, *fromFile;
        char normalizedFileName[MAX_FILE_NAME_SIZE];
        ensureNextArgumentIsAFileName(&i, argc, argv);
        file = argv[i];
        strcpy(normalizedFileName, normalizeFileName(file, cwd));
        ensureNextArgumentIsAFileName(&i, argc, argv);
        fromFile = argv[i];
        openEditorBufferNoFileLoad(normalizedFileName, fromFile);
    }
    else if (strcmp(argv[i], "-prune")==0) {
        ensureThereIsAnotherArgument(&i, argc, argv);
        addToStringListOption(&options.pruneNames, argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static void setXrefsLocation(char *argvi) {
    static bool messageWritten=false;

    if (options.mode==XrefMode && !messageWritten && !isAbsolutePath(argvi)) {
        char tmpBuff[TMP_BUFF_SIZE];
        messageWritten = true;
        sprintf(tmpBuff, "'%s' is not an absolute path, correct -refs option", argvi);
        warningMessage(ERR_ST, tmpBuff);
    }
    options.cxrefsLocation = allocateStringForOption(&options.cxrefsLocation, normalizeFileName(argvi, cwd));
}

static bool processROption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-refnum=",8)==0)  {
        sscanf(argv[i]+8, "%d", &options.referenceFileCount);
    }
    else if (strncmp(argv[i], "-renameto=", 10)==0) {
        options.renameTo = allocateStringForOption(&options.renameTo, argv[i]+10);
    }
    else if (strcmp(argv[i], "-resetIncludeDirs")==0) {
        options.includeDirs = NULL;
    }
    else if (strcmp(argv[i], "-refs")==0)    {
        ensureNextArgumentIsAFileName(&i, argc, argv);
        setXrefsLocation(argv[i]);
    }
    else if (strncmp(argv[i], "-refs=",6)==0)    {
        setXrefsLocation(argv[i]+6);
    }
    else if (strcmp(argv[i], "-rlistwithoutsrc")==0) {
        options.referenceListWithoutSource = true;
    }
    else if (strcmp(argv[i], "-refactory")==0)   {
        options.mode = RefactoryMode;
        options.refactoringMode = RefactoryMode;
    }
    else if (strcmp(argv[i], "-rfct-rename")==0) {
        options.theRefactoring = AVR_RENAME_SYMBOL;
    }
    else if (strcmp(argv[i], "-rfct-rename-class")==0)   {
        options.theRefactoring = AVR_RENAME_CLASS;
    }
    else if (strcmp(argv[i], "-rfct-rename-package")==0) {
        options.theRefactoring = AVR_RENAME_PACKAGE;
    }
    else if (strcmp(argv[i], "-rfct-expand")==0) {
        options.theRefactoring = AVR_EXPAND_NAMES;
    }
    else if (strcmp(argv[i], "-rfct-reduce")==0) {
        options.theRefactoring = AVR_REDUCE_NAMES;
    }
    else if (strcmp(argv[i], "-rfct-add-param")==0)  {
        options.theRefactoring = AVR_ADD_PARAMETER;
    }
    else if (strcmp(argv[i], "-rfct-del-param")==0)  {
        options.theRefactoring = AVR_DEL_PARAMETER;
    }
    else if (strcmp(argv[i], "-rfct-move-param")==0) {
        options.theRefactoring = AVR_MOVE_PARAMETER;
    }
    else if (strcmp(argv[i], "-rfct-move-field")==0) {
        options.theRefactoring = AVR_MOVE_FIELD;
    }
    else if (strcmp(argv[i], "-rfct-move-static-field")==0)  {
        options.theRefactoring = AVR_MOVE_STATIC_FIELD;
    }
    else if (strcmp(argv[i], "-rfct-move-static-method")==0) {
        options.theRefactoring = AVR_MOVE_STATIC_METHOD;
    }
    else if (strcmp(argv[i], "-rfct-move-class")==0) {
        options.theRefactoring = AVR_MOVE_CLASS;
    }
    else if (strcmp(argv[i], "-rfct-move-class-to-new-file")==0) {
        options.theRefactoring = AVR_MOVE_CLASS_TO_NEW_FILE;
    }
    else if (strcmp(argv[i], "-rfct-move-all-classes-to-new-file")==0)   {
        options.theRefactoring = AVR_MOVE_ALL_CLASSES_TO_NEW_FILE;
    }
    else if (strcmp(argv[i], "-rfct-static-to-dynamic")==0)  {
        options.theRefactoring = AVR_TURN_STATIC_METHOD_TO_DYNAMIC;
    }
    else if (strcmp(argv[i], "-rfct-dynamic-to-static")==0)  {
        options.theRefactoring = AVR_TURN_DYNAMIC_METHOD_TO_STATIC;
    }
    else if (strcmp(argv[i], "-rfct-extract-method")==0) {
        options.theRefactoring = AVR_EXTRACT_METHOD;
    }
    else if (strcmp(argv[i], "-rfct-extract-macro")==0)  {
        options.theRefactoring = AVR_EXTRACT_MACRO;
    }
    else if (strcmp(argv[i], "-rfct-extract-variable")==0)  {
        options.theRefactoring = AVR_EXTRACT_VARIABLE;
    }
    else if (strcmp(argv[i], "-rfct-reduce-long-names-in-the-file")==0)  {
        options.theRefactoring = AVR_ADD_ALL_POSSIBLE_IMPORTS;
    }
    else if (strcmp(argv[i], "-rfct-add-to-imports")==0) {
        options.theRefactoring = AVR_ADD_TO_IMPORT;
    }
    else if (strcmp(argv[i], "-rfct-self-encapsulate-field")==0) {
        options.theRefactoring = AVR_SELF_ENCAPSULATE_FIELD;
    }
    else if (strcmp(argv[i], "-rfct-encapsulate-field")==0)  {
        options.theRefactoring = AVR_ENCAPSULATE_FIELD;
    }
    else if (strcmp(argv[i], "-rfct-push-down-field")==0)    {
        options.theRefactoring = AVR_PUSH_DOWN_FIELD;
    }
    else if (strcmp(argv[i], "-rfct-push-down-method")==0)   {
        options.theRefactoring = AVR_PUSH_DOWN_METHOD;
    }
    else if (strcmp(argv[i], "-rfct-pull-up-field")==0)  {
        options.theRefactoring = AVR_PULL_UP_FIELD;
    }
    else if (strcmp(argv[i], "-rfct-pull-up-method")==0) {
        options.theRefactoring = AVR_PULL_UP_METHOD;
    }
    else if (strncmp(argv[i], "-rfct-param1=", 13)==0)  {
        options.refpar1 = allocateStringForOption(&options.refpar1, argv[i]+13);
    }
    else if (strncmp(argv[i], "-rfct-param2=", 13)==0)  {
        options.refpar2 = allocateStringForOption(&options.refpar2, argv[i]+13);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processSOption(int *argi, int argc, char **argv) {
    int i = *argi;
    char *name, *val;

    if (0) {}
    else if (strcmp(argv[i], "-strict")==0)
        options.strictAnsi = true;
    else if (strcmp(argv[i], "-sourcepath")==0) {
        ensureNextArgumentIsAFileName(&i, argc, argv);
        options.sourcePath = allocateStringForOption(&options.sourcePath, argv[i]);
        setOptionVariable("-sourcepath", options.sourcePath);
    }
    else if (strcmp(argv[i], "-stdop")==0) {
        i = handleIncludeOption(argc, argv, i);
    }
    else if (strcmp(argv[i], "-set")==0) {
        i = handleSetOption(argc, argv, i);
    }
    else if (strncmp(argv[i], "-set",4)==0) {
        name = argv[i]+4;
        ensureThereIsAnotherArgument(&i, argc, argv);
        val = argv[i];
        setOptionVariable(name, val);
    }
    else if (strcmp(argv[i], "-searchdef")==0) {
        options.searchKind = SEARCH_DEFINITIONS;
    }
    else if (strcmp(argv[i], "-searchshortlist")==0) {
        options.searchKind = SEARCH_FULL_SHORT;
    }
    else if (strcmp(argv[i], "-searchdefshortlist")==0) {
        options.searchKind = SEARCH_DEFINITIONS_SHORT;
    }
    else if (strcmp(argv[i], "-server")==0) {
        options.mode = ServerMode;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processTOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-trace")==0) {
        options.trace = true;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processUOption(int *argIndexP, int argc, char **argv) {
    int i = *argIndexP;
    if (0) {}
    else if (strcmp(argv[i], "-urlmanualredirect")==0)   {
        options.urlAutoRedirect = false;
    }
    else if (strcmp(argv[i], "-urldirect")==0)   {
        options.urlGenTemporaryFile = false;
    }
    else if (strcmp(argv[i], "-update")==0)  {
        options.update = UPDATE_FULL;
        options.updateOnlyModifiedFiles = true;
    }
    else
        return false;
    *argIndexP = i;
    return true;
}

static bool processVOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-version")==0) {
        options.serverOperation = OLO_ABOUT;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processWOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-warnings") == 0){
        logging_selected.warnings = true;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processXOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-xrefactory-II") == 0){
        options.xref2 = true;
    }
    else if (strncmp(argv[i], "-xrefrc=",8) == 0) {
        options.xrefrc = allocateStringForOption(&options.xrefrc, argv[i]+8);
    }
    else if (strcmp(argv[i], "-xrefrc") == 0) {
        ensureNextArgumentIsAFileName(&i, argc, argv);
        options.xrefrc = allocateStringForOption(&options.xrefrc, argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processYOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
#ifdef YYDEBUG
    else if (strcmp(argv[i], "-yydebug") == 0) {
        c_yydebug = 1;
        yacc_yydebug = 1;
        java_yydebug = 1;
    }
#endif
    else return false;
    *argi = i;
    return true;
}

static bool processDashOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "--version") == 0) {
        options.serverOperation = OLO_ABOUT;
    }
    else return false;
    *argi = i;
    return true;
}

void processOptions(int argc, char **argv, ProcessFileArguments infilesFlag) {
    int i;
    bool matched;

    ENTER();

    for (i=1; i<argc; i++) {
        log_trace("processing argument '%s'", argv[i]);

        matched = false;
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'a': case 'A':
                matched = processAOption(&i, argc, argv);
                break;
            case 'b': case 'B':
                matched = processBOption(&i, argc, argv);
                break;
            case 'c': case 'C':
                matched = processCOption(&i, argc, argv);
                break;
            case 'd': case 'D':
                matched = processDOption(&i, argc, argv);
                break;
            case 'e': case 'E':
                matched = processEOption(&i, argc, argv);
                break;
            case 'f': case 'F':
                matched = processFOption(&i, argc, argv);
                break;
            case 'g': case 'G':
                matched = processGOption(&i, argc, argv);
                break;
            case 'h': case 'H':
                matched = processHOption(&i, argc, argv);
                break;
            case 'i': case 'I':
                matched = processIOption(&i, argc, argv);
                break;
            case 'j': case 'J':
                matched = processJOption(&i, argc, argv);
                break;
            case 'k': case 'K':
                matched = processKOption(&i, argc, argv);
                break;
            case 'l': case 'L':
                matched = processLOption(&i, argc, argv);
                break;
            case 'm': case 'M':
                matched = processMOption(&i, argc, argv);
                break;
            case 'n': case 'N':
                matched = processNOption(&i, argc, argv);
                break;
            case 'o': case 'O':
                matched = processOOption(&i, argc, argv);
                break;
            case 'p': case 'P':
                matched = processPOption(&i, argc, argv);
                break;
            case 'r': case 'R':
                matched = processROption(&i, argc, argv);
                break;
            case 's': case 'S':
                matched = processSOption(&i, argc, argv);
                break;
            case 't': case 'T':
                matched = processTOption(&i, argc, argv);
                break;
            case 'u': case 'U':
                matched = processUOption(&i, argc, argv);
                break;
            case 'v': case 'V':
                matched = processVOption(&i, argc, argv);
                break;
            case 'w': case 'W':
                matched = processWOption(&i, argc, argv);
                break;
            case 'x': case 'X':
                matched = processXOption(&i, argc, argv);
                break;
            case 'y': case 'Y':
                matched = processYOption(&i, argc, argv);
                break;
            case '-':
                matched = processDashOption(&i, argc, argv);
                break;
            default:
                matched = false;
            }
        } else {
            /* input file */
            matched = true;
            if (infilesFlag == PROCESS_FILE_ARGUMENTS) {
                addToStringListOption(&options.inputFiles, argv[i]);
            }
        }
        if (!matched) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "unknown option %s, (try c-xref -help)\n", argv[i]);
            if (options.mode==XrefMode) {
                FATAL_ERROR(ERR_ST, tmpBuff, XREF_EXIT_ERR);
            } else
                errorMessage(ERR_ST, tmpBuff);
        }
    }
    LEAVE();
}

static void scheduleFileArgumentToFileTable(char *infile) {
    int   topCallFlag;
    void *recurseFlag;

    javaSetSourcePath(true); // for case of packages on command line
    topCallFlag = 1;
    recurseFlag = &topCallFlag;
    MapOverPaths(infile, { dirInputFile(currentPath, "", NULL, NULL, recurseFlag, &topCallFlag); });
}

static void processFileArgument(char *fileArgument) {
    if (fileArgument[0] == '`' && fileArgument[strlen(fileArgument) - 1] == '`') {
        // TODO: So what does backquoted filenames mean?
        // A command to be run that returns a set of files?
        int    nargc;
        char **nargv, *pp;
        char   command[MAX_OPTION_LEN];

        // Un-backtick the command
        strcpy(command, fileArgument + 1);
        pp = strchr(command, '`');
        if (pp != NULL)
            *pp = 0;

        // Run the command and get options incl. more file arguments
        readOptionsFromCommand(command, &nargc, &nargv, "");
        for (int i = 1; i < nargc; i++) {
            // Only handle file names?
            if (nargv[i][0] != '-' && nargv[i][0] != '`') {
                scheduleFileArgumentToFileTable(nargv[i]);
            }
        }

    } else {
        scheduleFileArgumentToFileTable(fileArgument);
    }
}

void processFileArguments(void) {
    for (StringList *l = options.inputFiles; l != NULL; l = l->next) {
        processFileArgument(l->string);
    }
}


/* Non-static for unittesting */
/* Return a project name if found, else NULL */
/* Only handles cases where the file path is included in the project name/section */
bool projectCoveringFileInOptionsFile(char *fileName, FILE *optionsFile, /* out */ char *projectName) {
    int ch = ' ';              /* Something to get started */

    while (ch != EOF) {
        while (ch == ' ' || ch == '\t' || ch == '\n')
            ch = readChar(optionsFile);
        if (ch == '[') {
            char buffer[TMP_BUFF_SIZE];
            int i=0;
            while (ch != ']' && ch != EOF) {
                ch = readChar(optionsFile);
                buffer[i++] = ch;
            }
            buffer[i-1] = '\0';
            if (pathncmp(buffer, fileName, strlen(buffer), true)==0) {
                strcpy(projectName, buffer);
                return true;
            }
        } else
            while (ch != '\n' && ch != EOF)
                ch = readChar(optionsFile);
    }
    return false;
}

void searchStandardOptionsFileAndProjectForFile(char *sourceFilename, char *foundOptionsFilename, char *foundProjectName) {
    int    fileno;
    bool   found = false;
    FILE  *optionsFile;

    foundOptionsFilename[0] = 0;
    foundProjectName[0] = 0;

    if (sourceFilename == NULL)
        return;

    /* Try to find section in HOME config. */
    getXrefrcFileName(foundOptionsFilename);
    optionsFile = openFile(foundOptionsFilename, "r");
    if (optionsFile != NULL) {
        // TODO: This does not work since a project (section) name does not define which sources
        // are used. This is done with the '-I' option in the project/section...
        //found = projectCoveringFileInOptionsFile(fileName, optionsFile, section);
        int    nargc;
        char **nargv;
        found = readOptionsFromFileIntoArgs(optionsFile, &nargc, &nargv, DONT_ALLOCATE, sourceFilename,
                                            options.project, foundProjectName);
        if (found) {
            log_debug("options file '%s', project '%s' found", foundOptionsFilename, foundProjectName);
        }
        closeFile(optionsFile);
    }
    if (found)
        return;

    // If automatic selection did not find project, keep previous one
    if (options.project == NULL) {
        // but do this only if file is from cxfile, would be better to
        // check if it is from active project, but nothing is perfect
        // TODO: Where else could it come from (Xref.opt is not used anymore)?

        // TODO: check whether the project still exists in the .c-xrefrc file
        // it may happen that after deletion of the project, the request for active
        // project will return non-existent project. And then return "not found"?
        fileno = getFileNumberFromName(sourceFilename);
        if (fileno != NO_FILE_NUMBER && getFileItem(fileno)->isFromCxfile) {
            strcpy(foundOptionsFilename, previousStandardOptionsFile);
            strcpy(foundProjectName, previousStandardOptionsProject);
            return;
        }
    }
    foundOptionsFilename[0] = 0;
}
