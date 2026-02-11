#include "options.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "argumentsvector.h"
#include "commandlogger.h"
#include "commons.h"
#include "globals.h"
#include "head.h"
#include "memory.h"
#include "misc.h"
#include "cxref.h"
#include "refactorings.h"
#include "yylex.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"
#ifdef YYDEBUG
#include "parsers.h"
#endif
#include "ppc.h"
#include "protocol.h"

#include "log.h"

#include "options_config.h"


#define END_OF_OPTIONS_STRING "end-of-options"
#define MAX_STD_ARGS (MAX_FILES+20)


/* PUBLIC DATA: */

Options options;               // current options
Options savedOptions;
Options presetOptions = {
    /* GENERAL */
    false,                       // command log to /tmp file
    false,                       // exit
    "gcc",                       // path to compiler to use for auto-discovering compiler and defines
    NULL,                        // strings for commandline entered definitions (-D)
    0,                           // comment moving level
    NULL,                        // prune name
    NULL,                        // input files
    RC_NONE,                     // continue refactoring
    0,                           // completion case sensitive
    NULL,                        // xrefrc
    NO_EOL_CONVERSION,           // crlfConversion
    NULL,                        // pushName
    0,                           // parnum
    0,                           // parnum2
    "",                          // refactoring parameter name
    "",                          // refactoring parameter value
    "",                          // refactoring target line
    AVR_NO_REFACTORING,          // refactoring
    NULL,                        // renameTo
    false,                       // xrefactory-II
    NULL,                        // moveTargetFile
#if defined (__WIN32__)
    "c;C",                       // cFilesSuffixes
#else
    "c:C",                       // cFilesSuffixes
#endif
    true,                        // fileNamesCaseSensitive
    SEARCH_FULL,                 // search Tag file specifics
    false,                       // noerrors
    false,                       // exact position
    NULL,                        // -o outputFileName
    NULL,                        // -I include dirs
    DEFAULT_CXREF_FILENAME,      // -refs

    NULL,                       // file move for safety check
    NULL,
    0,                          // first moved line
    MAXIMAL_INT,                // safety check number of lines moved
    0,                          // new line number of the first line

    "",                         // getValue

    /* MIXED THINGS... */
    0,
    RESOLVE_DIALOG_DEFAULT,     // manual symbol resolution TODO: This is different from any of the RESOLVE values above, why?
    NULL,                       // browsed symbol name
    0,
    1,                          // cxMemoryFactor
    NULL,                       /* project */
    "",                         /* detected project root */
    "0:3",                      // olcxlccursor
    "",                         /* olcxSearchString */
    79,                         /* olineLen */
    0,                          // extractMode, must be zero TODO Why?
    MAX_COMPLETIONS,            /* maxCompletions */
    4,                          /* tabulator */
    -1,                         /* cursorOffset */
    -1,                         /* markOffset */
    XrefMode,                   /* operating mode */
    false,                      /* debug */
    false,                      /* trace */
    false,                      /* lexemTrace */
    false,                      /* fileTrace */
    OP_NONE,                   /* serverOperation */
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
    false,                      /* memory statistics print out on exit */
};

/* Auto-detected project root (survives initOptions() which overwrites options struct) */
static char autoDetectedProjectRoot[MAX_FILE_NAME_SIZE] = "";

/* memory where on-line given options are stored */
static Memory optMemory;

static char base[MAX_FILE_NAME_SIZE];

#define ENV_DEFAULT_VAR_FILE            "${__file}"
#define ENV_DEFAULT_VAR_PATH            "${__path}"
#define ENV_DEFAULT_VAR_NAME            "${__name}"
#define ENV_DEFAULT_VAR_SUFFIX          "${__suff}"


void aboutMessage(void) {
    char output[REFACTORING_TMP_STRING_SIZE];
    sprintf(output, "C-xrefactory version %s\n", VERSION);
    sprintf(output+strlen(output), "Compiled at %s on %s\n",  __TIME__, __DATE__);
    sprintf(output+strlen(output), "from git revision %s.\n", GIT_HASH);
    strcat(output,                 "(c) 1997-2004 by Xref-Tech, http://www.xref-tech.com\n");
    strcat(output,                 "Released into GPL 2009 by Marián Vittek (SourceForge)\n");
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
        exit(EXIT_SUCCESS);
}


/* *************************************************************************** */
/*                                      OPTIONS                                */

static void usage() {
    fprintf(stdout, "c-xref - a C/Yacc refactoring tool\n\n");
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
    fprintf(stdout, "\t-filescasesensitive       - file names are case sensitive\n");
    fprintf(stdout, "\t-filescaseunsensitive     - file names are case unsensitive\n");
    fprintf(stdout, "\t-csuffixes=<suffixes>     - list of C files suffixes separated by ':' (or ';')\n");
    fprintf(stdout, "\t-xrefrc <file>            - read options from <file> instead of ~/.c-xrefrc\n");
#if 0
    fprintf(stdout, "\t-olinelen=<n>             - length of lines for on-line output\n");
    fprintf(stdout, "\t-olcxsearch               - search info about identifier\n");
    fprintf(stdout, "\t-olcxpush                 - generate and push on-line cxrefs\n");
    fprintf(stdout, "\t-olcxrename               - generate and push xrfs for rename\n");
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
    fprintf(stdout, "\t-compiler=<path>          - path to compiler to use for autodiscovered includes and defines\n");
    fprintf(stdout, "\t-update                   - update existing references database\n");
    fprintf(stdout, "\t-fastupdate               - fast update (modified files only)\n");
    fprintf(stdout, "\t-fullupdate               - full update (all files)\n");
    fprintf(stdout, "\t-version                  - print version information\n");
}


static void *optAlloc(size_t size) {
    return memoryAlloc(&options.memory, size);
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
        l->next = NULL;
    } else {
        while (l->next != NULL)
            l = l->next;
        l->next = optAlloc(sizeof(StringList));
        l = l->next;
    }
    l->string = optAlloc(strlen(string)+1);
    l->next = NULL;

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
    assert(l == NULL || memoryIsBetween(&options->memory, l, 0, options->memory.index));
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
    FileItem *fileItem = getFileItemWithFileNumber(fileNumber);
    if (options.mode!=ServerMode) {
        // yes in edit server you process also headers, etc.
        fileItem->isArgument = true;
    }
    log_debug("recursively process command line argument file #%d '%s'", fileNumber, fileItem->name);
    if (!options.updateOnlyModifiedFiles) {
        fileItem->isScheduled = true;
    }
    LEAVE();
}

static bool fileNameShouldBePruned(char *fn) {
    for (StringList *s=options.pruneNames; s!=NULL; s=s->next) {
        MAP_OVER_PATHS(s->string, {
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
        strcpy(dirName, normalizeFileName_static(dirName, cwd));
        if (fileNameShouldBePruned(dirName))
            return;
    } else {
        strcpy(dirName, normalizeFileName_static(fname, cwd));
    }
    if (strlen(dirName) >= MAX_FILE_NAME_SIZE) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "file name %s is too long", dirName);
        FATAL_ERROR(ERR_ST, tmpBuff, EXIT_FAILURE);
    }
    suff = getFileSuffix(fname);
    // Directories are never in editor buffers...
    if (isDirectory(dirName)) {
        if (recurseFlag!=NULL) {
            isTopDirectory = 0;
            nrecurseFlag = &isTopDirectory;
            mapOverDirectoryFiles(dirName, dirInputFile, DO_NOT_ALLOW_EDITOR_FILES,
                              dirName, NULL, NULL, nrecurseFlag, &isTopDirectory);
            editorMapOnNonExistantFiles(dirName, dirInputFile, DEPTH_ANY,
                                        dirName, NULL, NULL, nrecurseFlag, &isTopDirectory);
        }
    } else if (editorFileExists(dirName)) {
        if (isTopDirectory==0
            && !fileNameHasOneOfSuffixes(fname, options.cFilesSuffixes)
            && compareFileNames(suff, ".y")!=0
        ) {
            return;
        }
        scheduleCommandLineEnteredFileToProcess(dirName);
    } else if (containsWildcard(dirName)) {
        char wildcardPath[MAX_OPTION_LEN];
        expandWildcardsInOnePath(dirName, wildcardPath, MAX_OPTION_LEN);
        MAP_OVER_PATHS(wildcardPath, { dirInputFile(currentPath, "", NULL, NULL, recurseFlag, &isTopDirectory); });
    } else if (isTopDirectory) {
        if (options.mode!=ServerMode) {
            errorMessage(ERR_CANT_OPEN, dirName);
        } else {
            // Needed to be able to complete in buffer without existing file
            scheduleCommandLineEnteredFileToProcess(dirName);
        }
    }
}

void recoverMemoryFromIncludeList(void) {
    StringList **pp;
    pp = & options.includeDirs;
    while (*pp!=NULL) {
        if (ppmIsFreedPointer(*pp)) {
            *pp = (*pp)->next;
            continue;
        }
        pp= &(*pp)->next;
    }
}

char *expandPredefinedSpecialVariables_static(char *variable, char *inputFilename) {
    static char expanded[MAX_OPTION_LEN];
    int         i, j;
    char       *suffix;
    char        filename[MAX_FILE_NAME_SIZE];
    char        path[MAX_FILE_NAME_SIZE];
    char        name[MAX_FILE_NAME_SIZE];

    strcpy(filename, getRealFileName_static(inputFilename));
    assert(strlen(filename) < MAX_FILE_NAME_SIZE - 1);
    strcpy(path, directoryName_static(filename));
    strcpy(name, simpleFileNameWithoutSuffix_static(filename));
    suffix = lastOccurenceInString(filename, '.');
    if (suffix == NULL)
        suffix = "";
    else
        suffix++;

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
        } else {
            expanded[j++] = variable[i++];
        }
        assert(j<MAX_OPTION_LEN);
    }
    expanded[j]=0;
    return expanded;
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
    log_debug("Expanding environment variables for option '%s'", original);
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
protected int getOptionFromFile(FILE *file, char *text, int *chars_read) {
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
                FATAL_ERROR(ERR_ST, "option string through end of file", EXIT_FAILURE);
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
#if defined (__WIN32__)
    bool caseSensitivity = false;
#else
    bool caseSensitivity = true;
#endif

    int length = strlen(path);
    if (pathncmp(path, fileName, length, caseSensitivity)==0
        && (fileName[length]=='/' || fileName[length]=='\\' || fileName[length]==0))
    {
        strcpy(foundProjectName,path);
        assert(strlen(foundProjectName)+1 < MAX_FILE_NAME_SIZE);
        *projectUpdated = true;
    } else {
        *projectUpdated = false;
    }
}

static void processProjectMarker(char *markerText, char *currentProject, char *fileName,
                                 bool *projectUpdated, char *foundProjectName) {
    /* First remove surrounding brackets */
    char *projectMarker = &markerText[1];
    markerText[strlen(markerText) - 1] = 0;

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


static int addOptionToArgsInMemory(Memory *memory, char optionText[], int argCount, char *arguments[]) {
    char *space = memoryAllocc(memory, strlen(optionText) + 1, sizeof(char));
    assert(space);
    strcpy(space, optionText);
    log_debug("option %s read", space);
    arguments[argCount] = space;
    if (argCount < MAX_STD_ARGS - 1)
        argCount++;

    return argCount;
}

static void ensureNextArgumentIsAFileName(int *i, ArgumentsVector args) {
    /* TODO this seems to only check that there is a next argument, not that it is a filename (non-option) */
    (*i)++;
    if (*i >= args.argc) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "file name expected after %s", args.argv[*i - 1]);
        errorMessage(ERR_ST, tmpBuff);
        usage();
        exit(EXIT_FAILURE);
    }
}

static void ensureThereIsAnotherArgument(int *i, ArgumentsVector args) {
    (*i)++;
    if (*i >= args.argc) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "further argument(s) expected after %s", args.argv[*i-1]);
        errorMessage(ERR_ST, tmpBuff);
        usage();
        exit(EXIT_FAILURE);
    }
}


static int handleSetOption(int i, ArgumentsVector args) {
    char *name, *val;

    ensureThereIsAnotherArgument(&i, args);
    name = args.argv[i];
    assert(name);
    ensureThereIsAnotherArgument(&i, args);
    val = args.argv[i];
    setOptionVariable(name, val);
    return i;
}

bool readOptionsIntoArgs(FILE *file, ArgumentsVector *outArgs, Memory *memory, char *fileName, char *project,
                         char *foundProjectName) {
    char optionText[MAX_OPTION_LEN];
    int len, ch, passNumber=0;
    bool projectUpdated, isActivePass;

    ENTER();

    int argc = 1;
    char *argv[MAX_STD_ARGS];

    projectUpdated = isActivePass = true;
    foundProjectName[0]=0;

    if (memory == &optMemory)
        memoryInit(&optMemory, "argument options", NULL, OptionsMemorySize);

    bool found = false;

    ch = 'a';                    /* Something not EOF */
    while (ch!=EOF) {
        ch = getOptionFromFile(file, optionText, &len);
        assert(strlen(optionText) == len);
        if (ch==EOF) {
            log_debug("got option from file (@EOF): '%s'", optionText);
            break;
        } else {
            log_debug("got option from file: '%s'", optionText);
        }
        if (len>=2 && optionText[0]=='[' && optionText[len-1]==']') {
            log_debug("checking '%s'", optionText);
            expandEnvironmentVariables(optionText+1, MAX_OPTION_LEN, &len, true);
            log_debug("expanded '%s'", optionText);
            processProjectMarker(optionText, project, fileName, &projectUpdated, foundProjectName);
        } else if (projectUpdated && strncmp(optionText, "-pass", 5) == 0) {
            sscanf(optionText+5, "%d", &passNumber);
            isActivePass = passNumber==currentPass || currentPass==ANY_PASS;
            if (passNumber > maxPasses)
                maxPasses = passNumber;
        } else if (strcmp(optionText,"-set")==0 && (projectUpdated && isActivePass) && memory != NULL) {
            // pre-evaluation of -set
            found = true;
            if (memory != NULL) {
                argc = addOptionToArgsInMemory(memory, optionText, argc, argv);
            }
            ch = getOptionFromFile(file, optionText, &len);
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memory != NULL) {
                argc = addOptionToArgsInMemory(memory, optionText, argc, argv);
            }
            ch = getOptionFromFile(file, optionText, &len);
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memory != NULL) {
                argc = addOptionToArgsInMemory(memory, optionText, argc, argv);
            }
            if (argc < MAX_STD_ARGS) {
                assert(argc >= 3);
                ArgumentsVector args = {.argc = argc, .argv = argv};
                handleSetOption(argc - 3, args);
            }
        } else if (ch != EOF && (projectUpdated && isActivePass)) {
            found = true;
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memory != NULL) {
                argc = addOptionToArgsInMemory(memory, optionText, argc, argv);
            }
        }
    }
    if (argc >= MAX_STD_ARGS - 1)
        errorMessage(ERR_ST, "too many options");

    char **allocatedVector = NULL;
    if (found && memory != NULL) {
        // Allocate an array of correct size to return instead of local variable argv
        allocatedVector = memoryAllocc(memory, argc, sizeof(char*));
        allocatedVector[0] = NULL;        /* Not used, ensure NULL */
        for (int i=1; i<argc; i++)
            allocatedVector[i] = argv[i];
    }
    outArgs->argc = argc;
    outArgs->argv = allocatedVector;

    LEAVE();

    return found;
}

ArgumentsVector readOptionsFromFile(char *fileName, char *section, char *project) {
    FILE *file;
    char unused[MAX_FILE_NAME_SIZE];
    ArgumentsVector args;

    file = openFile(fileName, "r");
    if (file==NULL)
        FATAL_ERROR(ERR_CANT_OPEN, fileName, EXIT_FAILURE);
    readOptionsIntoArgs(file, &args, &ppmMemory, section, project, unused);
    closeFile(file);
    return args;
}

void readOptionsFromCommand(char *command, ArgumentsVector *outArgs, char *section) {
    FILE *file;
    char unused[MAX_FILE_NAME_SIZE];

    file = popen(command, "r");
    if (file==NULL)
        FATAL_ERROR(ERR_CANT_OPEN, command, EXIT_FAILURE);
    readOptionsIntoArgs(file, outArgs, &ppmMemory, section, NULL, unused);
    closeFile(file);
}

ArgumentsVector readOptionsFromPipe(void) {
    ArgumentsVector args = {.argc = 0};

    assert(options.mode);
    if (options.mode == ServerMode) {
        char unused[MAX_FILE_NAME_SIZE];
        readOptionsIntoArgs(stdin, &args, &optMemory, "", NULL, unused);
        logCommands(args);
        /* those options can't contain include or define options, sections neither */
        int c = getc(stdin);
        if (c == EOF) {
            /* Just log and exit since we don't know if there is someone there... */
            /* We also want a clean exit() if we are going for coverage */
            log_error("Broken pipe");
            exit(EXIT_FAILURE);
            FATAL_ERROR(ERR_INTERNAL, "broken input pipe", EXIT_FAILURE);
        }
    }
    return args;
}

bool currentCxFileCountMatches(int foundCxFileCount) {
    bool check;

    if (options.cxFileCount == 0)
        check = true;
    else if (options.cxFileCount == 1)
        check = (foundCxFileCount <= 1);
    else
        check = (foundCxFileCount == options.cxFileCount);
    options.cxFileCount = foundCxFileCount;
    return check;
}

static void putXrefrcFileNameInto(char *fileName) {
    int hlen;
    char *home;

    if (options.xrefrc!=NULL) {
        sprintf(fileName, "%s", normalizeFileName_static(options.xrefrc, cwd));
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


static int handleIncludeOption(int i, ArgumentsVector args) {
    ArgumentsVector includedArgs;

    ensureNextArgumentIsAFileName(&i, args);

    includedArgs = readOptionsFromFile(args.argv[i], "", NULL);
    processOptions(includedArgs, PROCESS_FILE_ARGUMENTS_NO);

    return i;
}

static struct {
    bool errors;
    bool warnings;
    bool infos;
    bool debug;
    bool trace;
} logging_selected = {false, false, false, false, false};


static bool processAOption(int *argi, ArgumentsVector args) {
    int i = *argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-about")==0) {
        options.serverOperation = OP_ABOUT;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processBOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strncmp(args.argv[i], "-browsedsym=",12)==0)     {
        options.browsedName = allocateStringForOption(&options.browsedName, args.argv[i]+12);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processCOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strncmp(args.argv[i], "-commandlog=", 12)==0)
        options.commandlog = allocateStringForOption(&options.commandlog, args.argv[i]+12);
    else if (strcmp(args.argv[i], "-crlfconversion")==0)
        options.eolConversion|=CR_LF_EOL_CONVERSION;
    else if (strcmp(args.argv[i], "-crconversion")==0)
        options.eolConversion|=CR_EOL_CONVERSION;
    else if (strcmp(args.argv[i], "-completioncasesensitive")==0)
        options.completionCaseSensitive = true;
    else if (strncmp(args.argv[i], "-commentmovinglevel=",20)==0) {
        sscanf(args.argv[i]+20, "%d", (int*)&options.commentMovingMode);
    }
    else if (strcmp(args.argv[i], "-continuerefactoring")==0) {
        options.continueRefactoring = RC_CONTINUE;
    }
    else if (strncmp(args.argv[i], "-csuffixes=",11)==0) {
        options.cFilesSuffixes = allocateStringForOption(&options.cFilesSuffixes, args.argv[i]+11);
    }
    else if (strcmp(args.argv[i], "-create")==0)
        options.update = UPDATE_CREATE;
    else if (strncmp(args.argv[i], "-compiler=", 10)==0) {
        options.compiler = allocateStringForOption(&options.compiler, &args.argv[i][10]);
    } else
        return false;
    *argi = i;
    return true;
}

static bool processDOption(int *argi, ArgumentsVector args) {
    int i = *argi;

    if (0) {}                   /* For copy/paste/re-order convenience, all tests can start with "else if.." */
    else if (strncmp(args.argv[i], "-delay=", 7)==0)
        /* Startup delay already handled in main() */ ;
    else if (strcmp(args.argv[i], "-debug")==0)
        options.debug = true;
    else if (strncmp(args.argv[i], "-D",2)==0) {
        // Save this definition so that it can be turned into a pre-processor definition
        // at file processing start, which we will implement later
        options.definitionStrings = allocateStringForOption(&options.definitionStrings, &args.argv[i][2]);
        // For now do it also the old way by directly turning it into a macro definition
        addMacroDefinedByOption(args.argv[i]+2);
    } else return false;

    *argi = i;
    return true;
}

static bool processEOption(int *argi, ArgumentsVector args) {
    int i = *argi;

    if (0) {}
    else if (strcmp(args.argv[i], "-errors")==0) {
        options.errors = true;
        logging_selected.errors = true;
    } else if (strcmp(args.argv[i], "-exit")==0) {
        log_debug("Exiting");
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(args.argv[i], "-exactpositionresolve")==0) {
        options.exactPositionResolve = true;
    }
    else if (strncmp(args.argv[i], "-encoding=", 10)==0) {
        char tmpString[TMP_BUFF_SIZE];
        sprintf(tmpString, "'-encoding' is not supported anymore, files are expected to be 'utf-8'.");
        formatOutputLine(tmpString, ERROR_MESSAGE_STARTING_OFFSET);
        warningMessage(ERR_ST, tmpString);
    }
    else
        return false;
    *argi = i;
    return true;
}

static bool processFOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-filetrace")==0) {
        options.fileTrace = true;
    }
    else if (strcmp(args.argv[i], "-filescasesensitive")==0) {
        options.fileNamesCaseSensitive = true;
    }
    else if (strcmp(args.argv[i], "-filescaseunsensitive")==0) {
        options.fileNamesCaseSensitive = false;
    }
    else if (strcmp(args.argv[i], "-fastupdate")==0)  {
        options.update = UPDATE_FAST;
        options.updateOnlyModifiedFiles = true;
    }
    else if (strcmp(args.argv[i], "-fullupdate")==0) {
        options.update = UPDATE_FULL;
        options.updateOnlyModifiedFiles = false;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processGOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-get")==0) {
        ensureThereIsAnotherArgument(&i, args);
        options.variableToGet = allocateStringForOption(&options.variableToGet, args.argv[i]);
        options.serverOperation = OP_GET_ENV_VALUE;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processHOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (strcmp(args.argv[i], "-help")==0) {
        usage();
        exit(EXIT_SUCCESS);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processIOption(int *argi, ArgumentsVector args) {
    int i = * argi;

    if (0) {}
    else if (strcmp(args.argv[i], "-I")==0) {
        /* include dir */
        i++;
        if (i >= args.argc) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "directory name expected after -I");
            errorMessage(ERR_ST,tmpBuff);
            usage();
        }
        addToStringListOption(&options.includeDirs, args.argv[i]);
    }
    else if (strncmp(args.argv[i], "-I", 2)==0 && args.argv[i][2]!=0) {
        addToStringListOption(&options.includeDirs, args.argv[i]+2);
    }
    else if (strcmp(args.argv[i], "-infos")==0) {
        logging_selected.infos = true;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processJOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else return false;
    *argi = i;
    return true;
}

static bool processKOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else return false;
    *argi = i;
    return true;
}

static bool processLOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-lexemtrace")==0) {
        options.lexemTrace = true;
    }
    else if (strncmp(args.argv[i], "-log=", 5)==0) {
        ;                       /* Already handled in initLogging() */
    }
    else return false;
    *argi = i;
    return true;
}

static bool processMOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strncmp(args.argv[i], "-mf=", 4)==0) {
        int mf;
        sscanf(args.argv[i]+4, "%d", &mf);
        if (mf<0 || mf>=255) {
            FATAL_ERROR(ERR_ST, "memory factor out of range <1,255>", EXIT_FAILURE);
        }
        options.cxMemoryFactor = mf;
    }
    else if (strncmp(args.argv[i], "-maxcompls=",11)==0)  {
        sscanf(args.argv[i]+11, "%d", &options.maxCompletions);
    }
    else if (strncmp(args.argv[i], "-movetargetfile=",16)==0) {
        options.moveTargetFile = allocateStringForOption(&options.moveTargetFile, args.argv[i]+16);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processNOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-no-errors")==0)
        options.noErrors = true;
    else
        return false;
    *argi = i;
    return true;
}

static bool processOOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strncmp(args.argv[i], "-olinelen=",10)==0) {
        sscanf(args.argv[i]+10, "%d",&options.olineLen);
        if (options.olineLen == 0)
            options.olineLen = 79;
        else options.olineLen--;
    }
    else if (strncmp(args.argv[i], "-olcursor=",10)==0) {
        sscanf(args.argv[i]+10, "%d",&options.cursorOffset);
    }
    else if (strncmp(args.argv[i], "-olmark=",8)==0) {
        sscanf(args.argv[i]+8, "%d",&options.markOffset);
    }
    else if (strcmp(args.argv[i], "-olcxextract")==0) {
        options.serverOperation = OP_INTERNAL_PARSE_TO_EXTRACT;
    }
    else if (strcmp(args.argv[i], "-olmanualresolve")==0) {
        options.manualResolve = RESOLVE_DIALOG_ALWAYS;
    }
    else if (strcmp(args.argv[i], "-olnodialog")==0) {
        options.manualResolve = RESOLVE_DIALOG_NEVER;
    }
    else if (strcmp(args.argv[i], "-olexmacro")==0)
        options.extractMode=EXTRACT_MACRO;
    else if (strcmp(args.argv[i], "-olexvariable")==0)
        options.extractMode=EXTRACT_VARIABLE;
    else if (strcmp(args.argv[i], "-olcxrename")==0)
        options.serverOperation = OP_INTERNAL_PUSH_FOR_RENAME;
    else if (strcmp(args.argv[i], "-olcxargmanip")==0)
        options.serverOperation = OP_INTERNAL_PUSH_FOR_ARGUMENT_MANIPULATION;
    else if (strcmp(args.argv[i], "-olcxsafetycheck")==0)
        options.serverOperation = OP_INTERNAL_SAFETY_CHECK;
    else if (strncmp(args.argv[i], "-olcxgotoparname",16)==0) {
        options.serverOperation = OP_INTERNAL_PARSE_TO_GOTO_PARAM_NAME;
        options.olcxGotoVal = 0;
        sscanf(args.argv[i]+16, "%d", &options.olcxGotoVal);
    }
    else if (strncmp(args.argv[i], "-olcxgetparamcoord",18)==0) {
        options.serverOperation = OP_INTERNAL_PARSE_TO_GET_PARAM_COORDINATES;
        options.olcxGotoVal = 0;
        sscanf(args.argv[i]+18, "%d", &options.olcxGotoVal);
    }
    else if (strncmp(args.argv[i], "-olcxparnum=",12)==0) {
        options.parnum = 0;
        sscanf(args.argv[i]+12, "%d", &options.parnum);
    }
    else if (strncmp(args.argv[i], "-olcxparnum2=",13)==0) {
        options.parnum2 = 0;
        sscanf(args.argv[i]+13, "%d", &options.parnum2);
    }
    else if (strcmp(args.argv[i], "-olcxgetrefactorings")==0)     {
        options.serverOperation = OP_GET_AVAILABLE_REFACTORINGS;
    }
    else if (strcmp(args.argv[i], "-olcxpush")==0)
        options.serverOperation = OP_BROWSE_PUSH;
    else if (strcmp(args.argv[i], "-olcxrepush")==0)
        options.serverOperation = OP_BROWSE_REPUSH;
    else if (strcmp(args.argv[i], "-olcxpushonly")==0)
        options.serverOperation = OP_BROWSE_PUSH_ONLY;
    else if (strcmp(args.argv[i], "-olcxpushandcallmacro")==0)
        options.serverOperation = OP_BROWSE_PUSH_AND_CALL_MACRO;
    else if (strcmp(args.argv[i], "-olcxpushforlm")==0) {
        options.serverOperation = OP_INTERNAL_PUSH_FOR_LOCAL_MOTION;
        options.manualResolve = RESOLVE_DIALOG_NEVER;
    }
    else if (strcmp(args.argv[i], "-olcxpushglobalunused")==0)
        options.serverOperation = OP_UNUSED_GLOBAL;
    else if (strcmp(args.argv[i], "-olcxpushfileunused")==0)
        options.serverOperation = OP_UNUSED_LOCAL;
    else if (strcmp(args.argv[i], "-olcxpop")==0)
        options.serverOperation = OP_BROWSE_POP;
    else if (strcmp(args.argv[i], "-olcxpoponly")==0)
        options.serverOperation =OP_BROWSE_POP_ONLY;
    else if (strcmp(args.argv[i], "-olcxnext")==0)
        options.serverOperation = OP_BROWSE_NEXT;
    else if (strcmp(args.argv[i], "-olcxprevious")==0)
        options.serverOperation = OP_BROWSE_PREVIOUS;
    else if (strcmp(args.argv[i], "-olcxcomplet")==0)
        options.serverOperation=OP_COMPLETION;
    else if (strcmp(args.argv[i], "-olcxmovetarget")==0)
        options.serverOperation=OP_INTERNAL_PARSE_TO_SET_MOVE_TARGET;
    else if (strcmp(args.argv[i], "-olcxgetfunctionbounds")==0)
        options.serverOperation=OP_INTERNAL_GET_FUNCTION_BOUNDS;
    else if (strcmp(args.argv[i], "-olcxgetprojectname")==0) {
        options.serverOperation=OP_ACTIVE_PROJECT;
    }
    else if (strncmp(args.argv[i], "-olcxlccursor=",14)==0) {
        // position of the cursor in line:column format
        options.olcxlccursor = allocateStringForOption(&options.olcxlccursor, args.argv[i]+14);
    }
    else if (strncmp(args.argv[i], "-olcxtagsearch=",15)==0) {
        options.serverOperation=OP_SEARCH;
        options.olcxSearchString = allocateStringForOption(&options.olcxSearchString, args.argv[i]+15);
    }
    else if (strcmp(args.argv[i], "-olcxtagsearchforward")==0) {
        options.serverOperation=OP_SEARCH_NEXT;
    }
    else if (strcmp(args.argv[i], "-olcxtagsearchback")==0) {
        options.serverOperation=OP_SEARCH_PREVIOUS;
    }
    else if (strncmp(args.argv[i], "-olcxpushname=",14)==0)   {
        options.serverOperation = OP_BROWSE_PUSH_NAME;
        options.pushName = allocateStringForOption(&options.pushName, args.argv[i]+14);
    }
    else if (strncmp(args.argv[i], "-olcomplselect",14)==0) {
        options.serverOperation=OP_COMPLETION_SELECT;
        options.olcxGotoVal = 0;
        sscanf(args.argv[i]+14, "%d",&options.olcxGotoVal);
    }
    else if (strcmp(args.argv[i], "-olcomplback")==0) {
        options.serverOperation=OP_COMPLETION_PREVIOUS;
    }
    else if (strcmp(args.argv[i], "-olcomplforward")==0) {
        options.serverOperation=OP_COMPLETION_NEXT;
    }
    else if (strncmp(args.argv[i], "-olcxcgoto",10)==0) {
        options.serverOperation = OP_COMPLETION_GOTO_N;
        options.olcxGotoVal = 0;
        sscanf(args.argv[i]+10, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(args.argv[i], "-olcxtaggoto",12)==0) {
        options.serverOperation = OP_SEARCH_GOTO_N;
        options.olcxGotoVal = 0;
        sscanf(args.argv[i]+12, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(args.argv[i], "-olcxtagselect",14)==0) {
        options.serverOperation = OP_SEARCH_SELECT;
        options.olcxGotoVal = 0;
        sscanf(args.argv[i]+14, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(args.argv[i], "-olcxgoto",9)==0) {
        options.serverOperation = OP_BROWSE_GOTO_N;
        options.olcxGotoVal = 0;
        sscanf(args.argv[i]+9, "%d",&options.olcxGotoVal);
    }
    else if (strncmp(args.argv[i], "-olcxfilter=",12)==0) {
        options.serverOperation = OP_FILTER_SET;
        sscanf(args.argv[i]+12, "%d",&options.filterValue);
    }
    else if (strncmp(args.argv[i], "-olcxmenusingleselect",21)==0) {
        options.serverOperation = OP_MENU_SELECT_ONLY;
        options.lineNumberOfMenuSelection = 0;
        sscanf(args.argv[i]+21, "%d",&options.lineNumberOfMenuSelection);
    }
    else if (strncmp(args.argv[i], "-olcxmenuselect",15)==0) {
        options.serverOperation = OP_MENU_TOGGLE_SELECT;
        options.lineNumberOfMenuSelection = 0;
        sscanf(args.argv[i]+15, "%d",&options.lineNumberOfMenuSelection);
    }
    else if (strcmp(args.argv[i], "-olcxmenuall")==0) {
        options.serverOperation = OP_MENU_SELECT_ALL;
    }
    else if (strcmp(args.argv[i], "-olcxmenunone")==0) {
        options.serverOperation = OP_MENU_SELECT_NONE;
    }
    else if (strncmp(args.argv[i], "-olcxmenufilter=",16)==0) {
        options.serverOperation = OP_MENU_FILTER_SET;
        sscanf(args.argv[i]+16, "%d",&options.filterValue);
    }
    else if (strcmp(args.argv[i], "-optinclude")==0) {
        i = handleIncludeOption(i, args);
    }
    else if (strcmp(args.argv[i], "-o")==0) {
        ensureNextArgumentIsAFileName(&i, args);
        options.outputFileName = allocateStringForOption(&options.outputFileName, args.argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processPOption(int *argi, ArgumentsVector args) {
    int i = *argi;

    if (0) {}
    else if (strncmp(args.argv[i], "-pass",5)==0) {
        errorMessage(ERR_ST, "'-pass' option can't be entered from command line");
    }
    else if (strcmp(args.argv[i], "-p")==0) {
        ensureNextArgumentIsAFileName(&i, args);
        log_debug("Current project '%s'", args.argv[i]);
        options.project = allocateStringForOption(&options.project, args.argv[i]);
    }
    else if (strcmp(args.argv[i], "-preload")==0) {
        ensureNextArgumentIsAFileName(&i, args);

        char normalizedFileName[MAX_FILE_NAME_SIZE];
        char *file = args.argv[i];
        strcpy(normalizedFileName, normalizeFileName_static(file, cwd));

        ensureNextArgumentIsAFileName(&i, args);
        char *preloadFile = args.argv[i];
        openEditorBufferFromPreload(normalizedFileName, preloadFile);
    }
    else if (strcmp(args.argv[i], "-prune")==0) {
        ensureThereIsAnotherArgument(&i, args);
        addToStringListOption(&options.pruneNames, args.argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static void setXrefsLocation(char *arg) {
    static bool messageWritten=false;

    /* In auto-detection mode, -refs is ignored - we use convention-based path */
    if (options.detectedProjectRoot != NULL && options.detectedProjectRoot[0] != '\0') {
        log_warn("-refs is ignored in project auto-detection mode (using %s/.c-xref/db)",
                 options.detectedProjectRoot);
        return;
    }

    if (options.mode==XrefMode && !messageWritten && !isAbsolutePath(arg)) {
        char tmpBuff[TMP_BUFF_SIZE];
        messageWritten = true;
        sprintf(tmpBuff, "'%s' is not an absolute path, correct -refs option", arg);
        warningMessage(ERR_ST, tmpBuff);
    }
    options.cxFileLocation = allocateStringForOption(&options.cxFileLocation, normalizeFileName_static(arg, cwd));
}

static bool processROption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strncmp(args.argv[i], "-refnum=",8)==0)  {
        sscanf(args.argv[i]+8, "%d", &options.cxFileCount);
    }
    else if (strncmp(args.argv[i], "-renameto=", 10)==0) {
        options.renameTo = allocateStringForOption(&options.renameTo, args.argv[i]+10);
    }
    else if (strcmp(args.argv[i], "-refs")==0)    {
        ensureNextArgumentIsAFileName(&i, args);
        setXrefsLocation(args.argv[i]);
    }
    else if (strncmp(args.argv[i], "-refs=",6)==0)    {
        setXrefsLocation(args.argv[i]+6);
    }
    else if (strcmp(args.argv[i], "-refactory")==0)   {
        options.mode = RefactoryMode;
    }
    else if (strcmp(args.argv[i], "-rfct-rename")==0) {
        options.theRefactoring = AVR_RENAME_SYMBOL;
    }
    else if (strcmp(args.argv[i], "-rfct-rename-module")==0) {
        options.theRefactoring = AVR_RENAME_MODULE;
    }
    else if (strcmp(args.argv[i], "-rfct-rename-included-file")==0) {
        options.theRefactoring = AVR_RENAME_INCLUDED_FILE;
    }
    else if (strcmp(args.argv[i], "-rfct-add-param")==0)  {
        options.theRefactoring = AVR_ADD_PARAMETER;
    }
    else if (strcmp(args.argv[i], "-rfct-del-param")==0)  {
        options.theRefactoring = AVR_DEL_PARAMETER;
    }
    else if (strcmp(args.argv[i], "-rfct-move-param")==0) {
        options.theRefactoring = AVR_MOVE_PARAMETER;
    }
    else if (strcmp(args.argv[i], "-rfct-move-function")==0) {
        options.theRefactoring = AVR_MOVE_FUNCTION;
    }
    else if (strcmp(args.argv[i], "-rfct-organize-includes")==0) {
        options.theRefactoring = AVR_ORGANIZE_INCLUDES;
    }
    else if (strcmp(args.argv[i], "-rfct-extract-function")==0) {
        options.theRefactoring = AVR_EXTRACT_FUNCTION;
    }
    else if (strcmp(args.argv[i], "-rfct-extract-macro")==0)  {
        options.theRefactoring = AVR_EXTRACT_MACRO;
    }
    else if (strcmp(args.argv[i], "-rfct-extract-variable")==0)  {
        options.theRefactoring = AVR_EXTRACT_VARIABLE;
    }
    else if (strncmp(args.argv[i], "-rfct-parameter-name=", 21)==0)  {
        options.refactor_parameter_name = allocateStringForOption(&options.refactor_parameter_name, args.argv[i]+21);
    }
    else if (strncmp(args.argv[i], "-rfct-parameter-value=", 22)==0)  {
        options.refactor_parameter_value = allocateStringForOption(&options.refactor_parameter_value, args.argv[i]+22);
    }
    else if (strncmp(args.argv[i], "-rfct-target-line=", 18)==0)  {
        options.refactor_target_line = allocateStringForOption(&options.refactor_target_line, args.argv[i]+18);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processSOption(int *argi, ArgumentsVector args) {
    int i = *argi;

    if (0) {}
    else if (strcmp(args.argv[i], "-set")==0) {
        i = handleSetOption(i, args);
    }
    else if (strcmp(args.argv[i], "-searchdef")==0) {
        options.searchKind = SEARCH_DEFINITIONS;
    }
    else if (strcmp(args.argv[i], "-searchshortlist")==0) {
        options.searchKind = SEARCH_FULL_SHORT;
    }
    else if (strcmp(args.argv[i], "-searchdefshortlist")==0) {
        options.searchKind = SEARCH_DEFINITIONS_SHORT;
    }
    else if (strcmp(args.argv[i], "-server")==0) {
        options.mode = ServerMode;
        options.xref2 = true;
    } else if (strcmp(args.argv[i], "-statistics")==0) {
        options.statistics = true;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processTOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-trace")==0) {
        options.trace = true;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processUOption(int *argi, ArgumentsVector args) {
    int i = *argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-update")==0)  {
        options.update = UPDATE_FULL;
        options.updateOnlyModifiedFiles = true;
    }
    else
        return false;
    *argi = i;
    return true;
}

static bool processVOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-version")==0) {
        options.serverOperation = OP_ABOUT;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processWOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-warnings") == 0){
        logging_selected.warnings = true;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processXOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-xrefactory-II") == 0){
        options.xref2 = true;
    }
    else if (strncmp(args.argv[i], "-xrefrc=",8) == 0) {
        options.xrefrc = allocateStringForOption(&options.xrefrc, args.argv[i]+8);
    }
    else if (strcmp(args.argv[i], "-xrefrc") == 0) {
        ensureNextArgumentIsAFileName(&i, args);
        options.xrefrc = allocateStringForOption(&options.xrefrc, args.argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processYOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
#ifdef YYDEBUG
    else if (strcmp(args.argv[i], "-yydebug") == 0) {
        c_yydebug = 1;
        yacc_yydebug = 1;
    }
#endif
    else return false;
    *argi = i;
    return true;
}

static bool processDoubleDashOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "--version") == 0) {
        options.serverOperation = OP_ABOUT;
    }
    else return false;
    *argi = i;
    return true;
}

void processOptions(ArgumentsVector args, ProcessFileArguments doProcessFiles) {
    int i;
    bool matched;

    ENTER();

    for (i=1; i<args.argc; i++) {
        log_debug("processing argument '%s'", args.argv[i]);

        matched = false;
        if (args.argv[i][0] == '-') {
            switch (args.argv[i][1]) {
            case 'a': case 'A':
                matched = processAOption(&i, args);
                break;
            case 'b': case 'B':
                matched = processBOption(&i, args);
                break;
            case 'c': case 'C':
                matched = processCOption(&i, args);
                break;
            case 'd': case 'D':
                matched = processDOption(&i, args);
                break;
            case 'e': case 'E':
                matched = processEOption(&i, args);
                break;
            case 'f': case 'F':
                matched = processFOption(&i, args);
                break;
            case 'g': case 'G':
                matched = processGOption(&i, args);
                break;
            case 'h': case 'H':
                matched = processHOption(&i, args);
                break;
            case 'i': case 'I':
                matched = processIOption(&i, args);
                break;
            case 'j': case 'J':
                matched = processJOption(&i, args);
                break;
            case 'k': case 'K':
                matched = processKOption(&i, args);
                break;
            case 'l': case 'L':
                matched = processLOption(&i, args);
                break;
            case 'm': case 'M':
                matched = processMOption(&i, args);
                break;
            case 'n': case 'N':
                matched = processNOption(&i, args);
                break;
            case 'o': case 'O':
                matched = processOOption(&i, args);
                break;
            case 'p': case 'P':
                matched = processPOption(&i, args);
                break;
            case 'r': case 'R':
                matched = processROption(&i, args);
                break;
            case 's': case 'S':
                matched = processSOption(&i, args);
                break;
            case 't': case 'T':
                matched = processTOption(&i, args);
                break;
            case 'u': case 'U':
                matched = processUOption(&i, args);
                break;
            case 'v': case 'V':
                matched = processVOption(&i, args);
                break;
            case 'w': case 'W':
                matched = processWOption(&i, args);
                break;
            case 'x': case 'X':
                matched = processXOption(&i, args);
                break;
            case 'y': case 'Y':
                matched = processYOption(&i, args);
                break;
            case '-':
                matched = processDoubleDashOption(&i, args);
                break;
            default:
                matched = false;
            }
        } else {
            /* input file */
            matched = true;
            if (doProcessFiles == PROCESS_FILE_ARGUMENTS_YES) {
                addToStringListOption(&options.inputFiles, args.argv[i]);
            }
        }
        if (!matched) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "unknown option %s, (try c-xref -help)\n", args.argv[i]);
            if (options.mode==XrefMode) {
                FATAL_ERROR(ERR_ST, tmpBuff, EXIT_FAILURE);
            } else
                errorMessage(ERR_ST, tmpBuff);
        }
    }
    LEAVE();
}

static void scheduleFileArgumentToFileTable(char *infile) {
    int   topCallFlag;
    void *recurseFlag;

    topCallFlag = 1;
    recurseFlag = &topCallFlag;
    MAP_OVER_PATHS(infile, { dirInputFile(currentPath, "", NULL, NULL, recurseFlag, &topCallFlag); });
}

static void processFileArgument(char *fileArgument) {
    if (fileArgument[0] == '`' && fileArgument[strlen(fileArgument) - 1] == '`') {
        // TODO: So what does backquoted filenames mean?
        // A command to be run that returns a set of files?
        char command[MAX_OPTION_LEN];

        // Un-backtick the command
        strcpy(command, fileArgument + 1);
        char *pp = strchr(command, '`');
        if (pp != NULL)
            *pp = 0;

        ArgumentsVector args;
        // Run the command and get options incl. more file arguments
        readOptionsFromCommand(command, &args, "");
        for (int i = 1; i < args.argc; i++) {
            // Only handle file names?
            if (args.argv[i][0] != '-' && args.argv[i][0] != '`') {
                scheduleFileArgumentToFileTable(args.argv[i]);
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


/* Get the project name from an options file (first section header).
 * Used in auto-detection mode where file coverage is implicit.
 * Returns true if a section was found, false otherwise.
 */
static bool getProjectNameFromOptionsFile(FILE *optionsFile, /* out */ char *projectName) {
    int ch = ' ';

    while (ch != EOF) {
        while (ch == ' ' || ch == '\t' || ch == '\n')
            ch = readChar(optionsFile);
        if (ch == '[') {
            int i = 0;
            while (ch != ']' && ch != EOF) {
                ch = readChar(optionsFile);
                projectName[i++] = ch;
            }
            projectName[i-1] = '\0';
            return true;
        } else {
            while (ch != '\n' && ch != EOF)
                ch = readChar(optionsFile);
        }
    }
    return false;
}


/* LEGACY: Return a project name if found, else NULL. Only handles cases where the file
 * path is included in the project name/section. */
protected bool projectCoveringFileInOptionsFile(char *fileName, FILE *optionsFile, /* out */ char *projectName) {
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

/* Apply convention-based database path for auto-detected projects.
 * Called after options processing to set cxFileLocation to <projectRoot>/.c-xref/db
 * if auto-detection was used and no explicit -refs was specified in the config.
 */
void applyConventionBasedDatabasePath(void) {
    if (autoDetectedProjectRoot[0] == '\0') {
        /* Not in auto-detect mode */
        return;
    }

    /* Only apply if no explicit -xrefrc was specified (true auto-detect mode) */
    if (options.xrefrc != NULL) {
        return;
    }

    /* Check if an explicit -refs was set in the config by comparing with the default.
     * If cxFileLocation still contains the default "CXrefs" pattern, replace it. */
    if (options.cxFileLocation != NULL &&
        strstr(options.cxFileLocation, "CXrefs") != NULL) {
        char dbPath[MAX_FILE_NAME_SIZE + 16];
        sprintf(dbPath, "%s/.c-xref/db", autoDetectedProjectRoot);
        options.cxFileLocation = allocateStringForOption(&options.cxFileLocation, dbPath);
        options.detectedProjectRoot = allocateStringForOption(&options.detectedProjectRoot, autoDetectedProjectRoot);
        log_debug("Applied convention-based database path: %s", options.cxFileLocation);
    }
}

/* Search upward from sourceFilename for a project-local .c-xrefrc file.
 * Returns true if found and the config covers the source file.
 * Sets foundOptionsFilename to the path of the .c-xrefrc file.
 * Sets foundProjectName to the project name from the matching section.
 * Stops at HOME directory to avoid finding ~/.c-xrefrc (which is the global config).
 */
static bool searchUpwardForProjectLocalConfig(char *sourceFilename, char *foundOptionsFilename,
                                               char *foundProjectName) {
    char searchDir[MAX_FILE_NAME_SIZE];
    char candidatePath[MAX_FILE_NAME_SIZE + 16];  /* Extra space for "/.c-xrefrc" suffix */
    char *homeDir = getEnv("HOME");

    ENTER();

    /* Start from the directory containing the source file, or from the directory itself */
    if (isDirectory(sourceFilename)) {
        strcpy(searchDir, sourceFilename);
    } else {
        strcpy(searchDir, directoryName_static(sourceFilename));
    }
    log_debug("sourceFilename='%s', searchDir='%s'", sourceFilename, searchDir);

    while (searchDir[0] != 0 && strlen(searchDir) > 1) {
        /* Stop at HOME to avoid finding ~/.c-xrefrc (global config, not project-local) */
        if (homeDir != NULL && strcmp(searchDir, homeDir) == 0) {
            log_debug("Reached HOME directory, stopping search");
            break;
        }
        snprintf(candidatePath, sizeof(candidatePath), "%s/.c-xrefrc", searchDir);
        log_trace("Checking for project-local config: %s", candidatePath);

        if (fileExists(candidatePath)) {
            FILE *optionsFile = openFile(candidatePath, "r");
            if (optionsFile != NULL) {
                char projectName[MAX_FILE_NAME_SIZE+16];
                /* In auto-detection mode, file coverage is implicit (file is under config dir).
                 * Just extract the project name from the first section header. */
                bool found = getProjectNameFromOptionsFile(optionsFile, projectName);
                closeFile(optionsFile);
                if (!found) {
                    /* Empty config or no section - use directory basename as project name */
                    strcpy(projectName, simpleFileName(searchDir));
                    log_debug("Empty config, using directory basename as project name: %s", projectName);
                }
                strcpy(foundOptionsFilename, candidatePath);
                strcpy(foundProjectName, projectName);
                /* Store project root in module-static variable (survives initOptions()) */
                strcpy(autoDetectedProjectRoot, searchDir);
                log_debug("Detected project-local config '%s' covering '%s', project '%s', root '%s'",
                          candidatePath, sourceFilename, foundProjectName, autoDetectedProjectRoot);
                LEAVE();
                return true;
            }
        }

        /* Move up one directory */
        char *lastSlash = strrchr(searchDir, '/');
        if (lastSlash == NULL || lastSlash == searchDir) {
            break;  /* Reached root or no more slashes */
        }
        *lastSlash = 0;
    }

    log_debug("Could not detect project-local config covering '%s'", sourceFilename);
    LEAVE();
    return false;
}

void searchForProjectOptionsFileAndProjectForFile(char *sourceFilename, char *foundOptionsFilename,
                                                  char *foundProjectName) {
    int    fileno;
    bool   found = false;
    FILE  *optionsFile;

    foundOptionsFilename[0] = 0;
    foundProjectName[0] = 0;
    autoDetectedProjectRoot[0] = '\0';  /* Reset for each invocation */

    if (sourceFilename == NULL)
        return;

    /* If no explicit -xrefrc was provided, try to find a project-local .c-xrefrc by searching upward */
    if (options.xrefrc == NULL) {
        if (searchUpwardForProjectLocalConfig(sourceFilename, foundOptionsFilename, foundProjectName)) {
            return;
        }
    }

    /* Fall back: try to find section in explicit -xrefrc or HOME config. */
    putXrefrcFileNameInto(foundOptionsFilename);
    optionsFile = openFile(foundOptionsFilename, "r");
    if (optionsFile != NULL) {
        ArgumentsVector nargs;
        found = readOptionsIntoArgs(optionsFile, &nargs, NULL, sourceFilename,
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
        if (fileno != NO_FILE_NUMBER && getFileItemWithFileNumber(fileno)->isFromCxfile) {
            return;
        }
    }
    foundOptionsFilename[0] = 0;
}

void printOptionsMemoryStatistics(void) {
    printMemoryStatisticsFor(&optMemory);
    printMemoryStatisticsFor(&options.memory);
}
