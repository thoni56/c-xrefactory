#include "options.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "argumentsvector.h"
#include "commandlogger.h"
#include "commons.h"
#include "constants.h"
#include "cxref.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "log.h"
#include "memory.h"
#include "misc.h"
#include "options_config.h"
#include "optionsets.h"
#include "parsers.h"            /* For yydebug #ifdef YYDEBUG */
#include "ppc.h"
#include "protocol.h"
#include "rebuild.h"
#include "refactorings.h"
#include "yylex.h"


#define MAX_STD_ARGS (MAX_FILES+20)


/* PUBLIC DATA: */

Options options;               // current options
Options savedOptions;
Options presetOptions = {
    /* --- SESSION --- */
    .mode = UndefinedMode,
    .exit = false,
    .cxrefProtocol = false,
    .commandlog = NULL,
    .outputFileName = NULL,
    .errors = false,
    .lexemTrace = false,
    .fileTrace = false,
    .statistics = false,
    .progressUnthrottled = false,

    /* --- PROJECT --- */
    .compiler = "gcc",
    .definitionStrings = NULL,
    .pruneNames = NULL,
    .inputFiles = NULL,
    .includeDirs = NULL,
    .cxFileLocation = NULL,
#if defined (__WIN32__)
    .cFilesSuffixes = "c;C",
#else
    .cFilesSuffixes = "c:C",
#endif
    .fileNamesCaseSensitive = true,
    .tabulator = 4,

    /* --- REQUEST --- */
    .serverOperation = OP_NONE,
    .project = NULL,
    .pushName = NULL,
    .browsedName = NULL,
    .variableToGet = "",
    .dumpSelection = "",
    .searchString = "",
    .searchKind = SEARCH_FULL,
    .completionCaseSensitive = false,
    .exactPositionResolve = false,
    .cursorLineColumn = "0:3",
    .cursorOffset = NO_CURSOR_OFFSET,
    .markOffset = NO_CURSOR_OFFSET,
    .filterValue = 0,
    .manualResolve = RESOLVE_DIALOG_DEFAULT,
    .lineNumberOfMenuSelection = 0,
    .lineLength = 79,
    .maxCompletions = MAX_COMPLETIONS,
    .index = 0,

    /* --- REFACTORING --- */
    .theRefactoring = AVR_NO_REFACTORING,
    .continueRefactoring = RC_NONE,
    .commentMovingMode = 0,
    .extractMode = 0,
    .renameTo = NULL,
    .moveTargetFile = NULL,
    .refactorParameterName = "",
    .refactorParameterValue = "",
    .refactorTargetLine = "",
    .parameterNumber = 0,
    .toParameterNumber = 0,
    .checkFileMovedFrom = NULL,
    .checkFileMovedTo = NULL,
    .checkFirstMovedLine = 0,
    .checkLinesMoved = MAXIMAL_INT,
    .checkNewLineNumber = 0,

    /* --- INTERNAL --- */
    .detectedProjectRoot = "",
    .variablesCount = 0,
    .variables = {},
    .allUsedStringOptions = NULL,
    .allUsedStringListOptions = NULL,
    .memory = {.area = NULL, .size = 0},
};

/* Auto-detected project root (survives initOptions() which overwrites options struct) */
static char autoDetectedProjectRoot[MAX_FILE_NAME_SIZE] = "";

/* memory where on-line given options are stored */
static Memory optMemory;

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
    if (options.cxrefProtocol) {
        ppcGenRecord(PPC_INFORMATION, output);
    } else {
        fprintf(stdout, "%s", output);
    }
    if (options.exit)
        exit(EXIT_SUCCESS);
}


/* *************************************************************************** */
/*                                      OPTIONS                                */

void usage(void) {
    fprintf(stdout, "c-xref - a C/Yacc refactoring tool\n\n");
#if 0
    fprintf(stdout, "Usage:\n\tc-xref <mode> <option>+ <input files>\n\n");
    fprintf(stdout, "mode (one of):\n");
    fprintf(stdout, "\t-server                   - enter edit server mode with c-xref protocol\n");
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
#if 0
    fprintf(stdout, "\t-line-length=<n>          - length of lines for on-line output\n");
    fprintf(stdout, "\t-search                   - search info about identifier\n");
    fprintf(stdout, "\t-browse-push              - generate and push references for browsing\n");
    fprintf(stdout, "\t-internal-push-for-rename - generate and push xrfs for rename\n");
    fprintf(stdout, "\t-browse-pop               - pop on-line cxrefs\n");
    fprintf(stdout, "\t-browse-next              - browse to next reference\n");
    fprintf(stdout, "\t-browse-previous          - browse to previous reference\n");
    fprintf(stdout, "\t-browse-goto=<n>          - go to the n-th on-line reference\n");
    fprintf(stdout, "\t-file <file>              - name of the file given to stdin\n");
#endif
    fprintf(stdout, "\t-o <file>                 - write output to <file>\n");
    fprintf(stdout, "\t-exactpositionresolve     - resolve symbols by def. position\n");
    fprintf(stdout, "\t-errors                   - report all error messages on the console\n");
    fprintf(stdout, "\t                            (by default only fatal errors are shown)\n");
    fprintf(stdout, "\t-warnings                 - also report warning messages on the console\n");
    fprintf(stdout, "\t-infos                    - also report informational & warning messages on the console\n");
    fprintf(stdout, "\t-log=<file>               - log all fatal/error/warnings/informational messages to <file>\n");
    fprintf(stdout, "\t-info                     - also log info messages in log\n");
    fprintf(stdout, "\t-debug                    - also log debug messages in log\n");
    fprintf(stdout, "\t-trace                    - also log trace & debug messages in log\n");
    fprintf(stdout, "\t-commandlog               - write all commands to /tmp/c-xref-command-log (experimental)\n");
    fprintf(stdout, "\t-compiler=<path>          - path to compiler to use for autodiscovered includes and defines\n");
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
    size_t len = strlen(string)+1;
    char *allocated = optAlloc(len);
    memmove(allocated, string, len);
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
    size_t len = strlen(string)+1;
    l->string = optAlloc(len);
    l->next = NULL;

    memmove(l->string, string, len);
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

/* The bare file argument of the command line being processed. Only the request's
 * own arguments are of interest, so the server clears this before processing a
 * request and reads it right after. */
static char *requestFileArgumentName = NULL;

/* Resolve each -prune entry into an absolute path anchored at the detected
 * project root. The lightweight scan compares discovered directory entries against
 * these to gate descent. Caller owns the returned list (freeStringList). */
StringList *resolvePrunePaths(void) {
    StringList *result = NULL;
    for (StringList *p = options.pruneNames; p != NULL; p = p->next) {
        char resolved[MAX_FILE_NAME_SIZE];
        snprintf(resolved, sizeof(resolved), "%s%c%s",
                 options.detectedProjectRoot, FILE_PATH_SEPARATOR, p->string);
        result = newStringList(resolved, result);
    }
    return result;
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

    /* Every expansion below is derived from the input file. A request that
       carries none - "-get <name>" from the client, say - has nothing to
       substitute, so the placeholders are left as they are rather than
       resolved against a file that is not there. */
    if (inputFilename == NULL || inputFilename[0] == '\0') {
        assert(strlen(variable) < MAX_OPTION_LEN);
        strcpy(expanded, variable);
        return expanded;
    }

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

/* Moves the position past the character, unless there is none to move */
static int readOptionChar(FILE *file, Position *position) {
    int ch = readChar(file);
    if (position != NULL && ch != EOF) {
        if (ch == '\n') {
            position->line++;
            position->col = 0;
        } else
            position->col++;
    }
    return ch;
}

/* Return EOF if at EOF, but can still return option text, return something else if not.
 * With a position, which is where the next character is read, a syntax error is reported. */
/* TODO: improve contract - return true/false instead of EOF or 'A' */
int getOptionFromFile(FILE *file, Position *position, char *foundText, int *chars_read) {
    int count, ch;
    int lastCharacter;
    bool inComment;

    ch = readOptionChar(file, position);
    do {
        *chars_read = count = 0;
        inComment = false;

        /* Skip all white space? */
        while ((ch>=0 && ch<=' ') || ch=='\n' || ch=='\t')
            ch=readOptionChar(file, position);

        if (ch==EOF) {
            lastCharacter = EOF;
            goto fini;
        }

        if (ch=='\"') {
            Position quotePosition = position != NULL ? *position : (Position){0};
            quotePosition.col--;
            /* Read a double quoted string */
            ch=readOptionChar(file, position);
            // Escaping the double quote (\") is not allowed, it creates problems
            // when someone finished a section name by \ reverse slash
            while (ch!=EOF && ch!='\"') {
                if (count < MAX_OPTION_LEN-1)
                    foundText[count++]=ch;
                ch=readOptionChar(file, position);
            }
            if (ch==EOF && position != NULL)
                parsingMessageAt(quotePosition, ERR_ST, "missing terminating \" character");
        } else if (ch=='`') {
            foundText[count++]=ch;
            ch=readOptionChar(file, position);
            while (ch!=EOF && ch!='\n' && ch!='`') {
                if (count < MAX_OPTION_LEN-1)
                    foundText[count++]=ch;
                ch=readOptionChar(file, position);
            }
            if (count < MAX_OPTION_LEN-1)
                foundText[count++]=ch;
        } else if (ch=='[') {
            foundText[count++] = ch;
            ch=readOptionChar(file, position);
            while (ch!=EOF && ch!='\n' && ch!=']') {
                if (count < MAX_OPTION_LEN-1)
                    foundText[count++]=ch;
                ch=readOptionChar(file, position);
            }
            if (ch==']')
                foundText[count++]=ch;
        } else {
            while (ch!=EOF && ch>' ') {
                if (count < MAX_OPTION_LEN-1)
                    foundText[count++]=ch;
                ch=readOptionChar(file, position);
            }
        }
        foundText[count]=0;
        /* because QNX paths can start with // */
        if (count>=2 && foundText[0]=='/' && foundText[1]=='/') {
            while (ch!=EOF && ch!='\n')
                ch=readOptionChar(file, position);
            inComment = true;
        }
    } while (inComment);
    if (strcmp(foundText, END_OF_OPTIONS_STRING)==0) {
        count = 0; lastCharacter = EOF;
    } else {
        lastCharacter = 'A';
    }

 fini:
    foundText[count] = 0;
    *chars_read = count;

    return lastCharacter;
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

bool readOptionsIntoArgs(FILE *file, Position *position, ArgumentsVector *outArgs, Memory *memory) {
    char optionText[MAX_OPTION_LEN];
    int len, ch, passNumber=0;
    bool inProjectSection, isActivePass;
    bool sectionHeaderSeen = false;
    PassMarker passMarker;

    ENTER();

    int argc = 1;
    char *argv[MAX_STD_ARGS];

    inProjectSection = isActivePass = true;

    if (memory == &optMemory)
        memoryInit(&optMemory, "argument options", OptionsMemorySize);

    bool found = false;

    ch = 'a';                    /* Something not EOF */
    while (ch!=EOF) {
        ch = getOptionFromFile(file, position, optionText, &len);
        assert(strlen(optionText) == len);
        if (ch==EOF) {
            log_debug("got option from file (@EOF): '%s'", optionText);
            break;
        } else {
            log_debug("got option from file: '%s'", optionText);
        }
        if (len>=2 && optionText[0]=='[' && optionText[len-1]==']') {
            /* A first section header is ignored, a second ends the project (ADR-0031).
             * The rest is still read, the pipe uses this too. */
            if (sectionHeaderSeen)
                inProjectSection = false;
            sectionHeaderSeen = true;
        } else if (inProjectSection && (passMarker = readPassMarker(optionText, &passNumber)) != NotAPassMarker) {
            /* An unusable marker leaves the section it introduces unread, rather
             * than silently attributing it to the pass before it. */
            isActivePass = passMarker == WellFormedPassMarker
                && (passNumber == currentPass || currentPass == ANY_PASS);
            if (passMarker == WellFormedPassMarker && passNumber > maxPasses)
                maxPasses = passNumber;
        } else if (strcmp(optionText,"-set")==0 && (inProjectSection && isActivePass) && memory != NULL) {
            // pre-evaluation of -set
            found = true;
            if (memory != NULL) {
                argc = addOptionToArgsInMemory(memory, optionText, argc, argv);
            }
            ch = getOptionFromFile(file, position, optionText, &len);
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memory != NULL) {
                argc = addOptionToArgsInMemory(memory, optionText, argc, argv);
            }
            ch = getOptionFromFile(file, position, optionText, &len);
            expandEnvironmentVariables(optionText, MAX_OPTION_LEN, &len, false);
            if (memory != NULL) {
                argc = addOptionToArgsInMemory(memory, optionText, argc, argv);
            }
            if (argc < MAX_STD_ARGS) {
                assert(argc >= 3);
                ArgumentsVector args = {.argc = argc, .argv = argv};
                handleSetOption(argc - 3, args);
            }
        } else if (ch != EOF && (inProjectSection && isActivePass)) {
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

ArgumentsVector readOptionsFromFile(char *fileName) {
    FILE *file;
    ArgumentsVector args;

    file = openFile(fileName, "r");
    if (file==NULL)
        FATAL_ERROR(ERR_CANT_OPEN, fileName, EXIT_FAILURE);
    int fileNumber = addFileNameToFileTable(fileName);
    announceAnalysing(getFileItemWithFileNumber(fileNumber)->name);
    Position position = makePosition(fileNumber, 1, 0);
    readOptionsIntoArgs(file, &position, &args, &ppmMemory);
    closeFile(file);
    return args;
}

ArgumentsVector readOptionsFromPipe(void) {
    ArgumentsVector args = {.argc = 0};

    readOptionsIntoArgs(stdin, NULL, &args, &optMemory);
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
    return args;
}

static int handleIncludeOption(int i, ArgumentsVector args) {
    ArgumentsVector includedArgs;

    ensureNextArgumentIsAFileName(&i, args);

    includedArgs = readOptionsFromFile(args.argv[i]);
    processOptions(includedArgs, COLLECT_ARGUMENTS_NO);

    return i;
}

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
    else if (strcmp(args.argv[i], "-browse-next")==0)
        options.serverOperation = OP_BROWSE_NEXT;
    else if (strcmp(args.argv[i], "-browse-previous")==0)
        options.serverOperation = OP_BROWSE_PREVIOUS;
    else if (strcmp(args.argv[i], "-browse-push")==0) {
        options.serverOperation = OP_BROWSE_PUSH;
    }
    else if (strcmp(args.argv[i], "-browse-repush")==0)
        options.serverOperation = OP_BROWSE_REPUSH;
    else if (strcmp(args.argv[i], "-browse-push-only")==0)
        options.serverOperation = OP_BROWSE_PUSH_ONLY;
    else if (strcmp(args.argv[i], "-browse-push-and-call-macro")==0)
        options.serverOperation = OP_BROWSE_PUSH_AND_CALL_MACRO;
    else if (strcmp(args.argv[i], "-browse-pop")==0)
        options.serverOperation = OP_BROWSE_POP;
    else if (strcmp(args.argv[i], "-browse-pop-only")==0)
        options.serverOperation =OP_BROWSE_POP_ONLY;
    else if (strncmp(args.argv[i], "-browse-push-name=",18)==0)   {
        options.serverOperation = OP_BROWSE_PUSH_NAME;
        options.pushName = allocateStringForOption(&options.pushName, args.argv[i]+18);
    }
    else if (strncmp(args.argv[i], "-browse-goto=",13)==0) {
        options.serverOperation = OP_BROWSE_GOTO_N;
        options.index = 0;
        sscanf(args.argv[i]+13, "%d",&options.index);
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
    else if (strncmp(args.argv[i], "-compiler=", 10)==0) {
        options.compiler = allocateStringForOption(&options.compiler, &args.argv[i][10]);
    }
    else if (strncmp(args.argv[i], "-cursor=",8)==0) {
        sscanf(args.argv[i]+8, "%d",&options.cursorOffset);
    }
    else if (strcmp(args.argv[i], "-completion")==0)
        options.serverOperation=OP_COMPLETION;
    else if (strncmp(args.argv[i], "-cursor-line-column=",20)==0) {
        // position of the cursor in line:column format
        options.cursorLineColumn = allocateStringForOption(&options.cursorLineColumn, args.argv[i]+20);
    }
    else if (strncmp(args.argv[i], "-completion-select=",19)==0) {
        options.serverOperation=OP_COMPLETION_SELECT;
        options.index = 0;
        sscanf(args.argv[i]+19, "%d",&options.index);
    }
    else if (strcmp(args.argv[i], "-completion-previous")==0) {
        options.serverOperation=OP_COMPLETION_PREVIOUS;
    }
    else if (strcmp(args.argv[i], "-completion-next")==0) {
        options.serverOperation=OP_COMPLETION_NEXT;
    }
    else if (strncmp(args.argv[i], "-completion-goto=",17)==0) {
        options.serverOperation = OP_COMPLETION_GOTO_N;
        options.index = 0;
        sscanf(args.argv[i]+17, "%d",&options.index);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processDOption(int *argi, ArgumentsVector args) {
    int i = *argi;

    if (0) {}                   /* For copy/paste/re-order convenience, all tests can start with "else if.." */
    else if (strncmp(args.argv[i], "-delay=", 7)==0) {
        /* Startup delay already handled in main() */
    } else if (strcmp(args.argv[i], "-debug")==0) {
        /* -debug already handled in initLogging() */
    } else if (strncmp(args.argv[i], "-dump=", 6)==0) {
        options.dumpSelection = allocateStringForOption(&options.dumpSelection, &args.argv[i][6]);
        options.serverOperation = OP_DUMP;
    }
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
    } else if (strcmp(args.argv[i], "-exit")==0) {
        forgetKnowledgeForHeldContent();
        forgetKnowledgeOfHeadersLeftBehind();
        saveReferences();
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
    else if (strncmp(args.argv[i], "-filter=",8)==0) {
        options.serverOperation = OP_FILTER;
        sscanf(args.argv[i]+8, "%d",&options.filterValue);
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
    else if (strcmp(args.argv[i], "-getproject")==0) {
        options.serverOperation=OP_GET_PROJECT;
    }
    else if (strcmp(args.argv[i], "-get-available-refactorings")==0)     {
        options.serverOperation = OP_GET_AVAILABLE_REFACTORINGS;
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
    else if (strcmp(args.argv[i], "-info")==0) {
        /* Log level handled in main.c initLogging() */;
    }
    else if (strcmp(args.argv[i], "-internal-parse-to-extract")==0) {
        options.serverOperation = OP_INTERNAL_PARSE_TO_EXTRACT;
    }
    else if (strcmp(args.argv[i], "-internal-push-for-rename")==0)
        options.serverOperation = OP_INTERNAL_PUSH_FOR_RENAME;
    else if (strcmp(args.argv[i], "-internal-push-for-argument-manipulation")==0)
        options.serverOperation = OP_INTERNAL_PUSH_FOR_ARGUMENT_MANIPULATION;
    else if (strcmp(args.argv[i], "-internal-safety-check")==0)
        options.serverOperation = OP_INTERNAL_SAFETY_CHECK;
    else if (strncmp(args.argv[i], "-internal-parse-to-goto-param-name=",35)==0) {
        options.serverOperation = OP_INTERNAL_PARSE_TO_GOTO_PARAM_NAME;
        options.index = 0;
        sscanf(args.argv[i]+35, "%d", &options.index);
    }
    else if (strncmp(args.argv[i], "-internal-parse-to-get-param-coordinates=",41)==0) {
        options.serverOperation = OP_INTERNAL_PARSE_TO_GET_PARAM_COORDINATES;
        options.index = 0;
        sscanf(args.argv[i]+41, "%d", &options.index);
    }
    else if (strcmp(args.argv[i], "-internal-push-for-usage-check")==0) {
        options.serverOperation = OP_INTERNAL_PUSH_FOR_USAGE_CHECK;
        options.manualResolve = RESOLVE_DIALOG_NEVER;
    }
    else if (strcmp(args.argv[i], "-internal-parse-to-set-move-target")==0)
        options.serverOperation=OP_INTERNAL_PARSE_TO_SET_MOVE_TARGET;
    else if (strcmp(args.argv[i], "-internal-get-function-bounds")==0)
        options.serverOperation=OP_INTERNAL_GET_FUNCTION_BOUNDS;
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
    else if (strncmp(args.argv[i], "-line-length=",13)==0) {
        sscanf(args.argv[i]+13, "%d",&options.lineLength);
        if (options.lineLength == 0)
            options.lineLength = 79;
        else options.lineLength--;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processMOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strncmp(args.argv[i], "-maxcompls=",11)==0)  {
        sscanf(args.argv[i]+11, "%d", &options.maxCompletions);
    }
    else if (strncmp(args.argv[i], "-movetargetfile=",16)==0) {
        options.moveTargetFile = allocateStringForOption(&options.moveTargetFile, args.argv[i]+16);
    }
    else if (strncmp(args.argv[i], "-mark=",6)==0) {
        sscanf(args.argv[i]+6, "%d",&options.markOffset);
    }
    else if (strncmp(args.argv[i], "-menu-select-only=",18)==0) {
        options.serverOperation = OP_MENU_SELECT_ONLY;
        options.lineNumberOfMenuSelection = 0;
        sscanf(args.argv[i]+18, "%d",&options.lineNumberOfMenuSelection);
    }
    else if (strncmp(args.argv[i], "-menu-toggle-select=",20)==0) {
        options.serverOperation = OP_MENU_TOGGLE_SELECT;
        options.lineNumberOfMenuSelection = 0;
        sscanf(args.argv[i]+20, "%d",&options.lineNumberOfMenuSelection);
    }
    else if (strcmp(args.argv[i], "-menu-select-all")==0) {
        options.serverOperation = OP_MENU_SELECT_ALL;
    }
    else if (strcmp(args.argv[i], "-menu-select-none")==0) {
        options.serverOperation = OP_MENU_SELECT_NONE;
    }
    else if (strncmp(args.argv[i], "-menu-filter=",13)==0) {
        options.serverOperation = OP_MENU_FILTER;
        sscanf(args.argv[i]+13, "%d",&options.filterValue);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processOOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-olexmacro")==0)
        options.extractMode=EXTRACT_MACRO;
    else if (strcmp(args.argv[i], "-olexvariable")==0)
        options.extractMode=EXTRACT_VARIABLE;
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
    else if (strcmp(args.argv[i], "-progress-unthrottled")==0) {
        options.progressUnthrottled = true;
    }
    else if (strncmp(args.argv[i], "-parameter=",11)==0) {
        options.parameterNumber = 0;
        sscanf(args.argv[i]+11, "%d", &options.parameterNumber);
    }
    else return false;
    *argi = i;
    return true;
}

/* The server's command line and the config are processed before any request, and
   again on every request, so the parser only notes these, and they are reported once,
   when the project is locked */
static bool refsWasGiven = false;
static bool refnumWasGiven = false;
static bool xrefactoryIIGiven = false;
static bool xrefrcGiven = false;

void reportUnsupportedOptions(void) {
    if (refsWasGiven)
        warningMessage(ERR_ST, "-refs is ignored, the snapshot is always .c-xref/db in the project root");
    if (refnumWasGiven)
        warningMessage(ERR_ST, "-refnum is ignored, the snapshot is always a single file");
    if (xrefactoryIIGiven)
        errorMessage(ERR_ST, "-xrefactory-II is no longer supported, -server sets the protocol");
    if (xrefrcGiven)
        errorMessage(ERR_ST, "-xrefrc is no longer supported, the .c-xrefrc is found from the file");
}

static bool processROption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strncmp(args.argv[i], "-refnum=",8)==0)  {
        refnumWasGiven = true;
    }
    else if (strncmp(args.argv[i], "-renameto=", 10)==0) {
        options.renameTo = allocateStringForOption(&options.renameTo, args.argv[i]+10);
    }
    else if (strcmp(args.argv[i], "-refs")==0)    {
        ensureNextArgumentIsAFileName(&i, args);
        refsWasGiven = true;
    }
    else if (strncmp(args.argv[i], "-refs=",6)==0)    {
        refsWasGiven = true;
    }
    else if (strcmp(args.argv[i], "-refactory")==0)   {
        FATAL_ERROR(ERR_ST, "-refactory is no longer supported. Use -server and the server protocol", -1);
    }
    else if (strcmp(args.argv[i], "-rfct-rename")==0) {
        options.theRefactoring = AVR_RENAME_SYMBOL;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-rename-module")==0) {
        options.theRefactoring = AVR_RENAME_MODULE;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-rename-included-file")==0) {
        options.theRefactoring = AVR_RENAME_INCLUDED_FILE;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-add-param")==0)  {
        options.theRefactoring = AVR_ADD_PARAMETER;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-del-param")==0)  {
        options.theRefactoring = AVR_DEL_PARAMETER;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-move-param")==0) {
        options.theRefactoring = AVR_MOVE_PARAMETER;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-move-function")==0) {
        options.theRefactoring = AVR_MOVE_FUNCTION;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-organize-includes")==0) {
        options.theRefactoring = AVR_ORGANIZE_INCLUDES;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-extract-function")==0) {
        options.theRefactoring = AVR_EXTRACT_FUNCTION;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-extract-macro")==0)  {
        options.theRefactoring = AVR_EXTRACT_MACRO;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strcmp(args.argv[i], "-rfct-extract-variable")==0)  {
        options.theRefactoring = AVR_EXTRACT_VARIABLE;
        options.serverOperation = OP_REFACTORY;
    }
    else if (strncmp(args.argv[i], "-rfct-parameter-name=", 21)==0)  {
        options.refactorParameterName = allocateStringForOption(&options.refactorParameterName, args.argv[i]+21);
    }
    else if (strncmp(args.argv[i], "-rfct-parameter-value=", 22)==0)  {
        options.refactorParameterValue = allocateStringForOption(&options.refactorParameterValue, args.argv[i]+22);
    }
    else if (strncmp(args.argv[i], "-rfct-target-line=", 18)==0)  {
        options.refactorTargetLine = allocateStringForOption(&options.refactorTargetLine, args.argv[i]+18);
    }
    else if (strcmp(args.argv[i], "-resolve-dialog=always")==0) {
        options.manualResolve = RESOLVE_DIALOG_ALWAYS;
    }
    else if (strcmp(args.argv[i], "-resolve-dialog=never")==0) {
        options.manualResolve = RESOLVE_DIALOG_NEVER;
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
    else if (strncmp(args.argv[i], "-search=",8)==0) {
        options.serverOperation=OP_SEARCH;
        options.searchString = allocateStringForOption(&options.searchString, args.argv[i]+8);
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
        options.cxrefProtocol = true;
    } else if (strcmp(args.argv[i], "-statistics")==0) {
        options.statistics = true;
    }
    else if (strcmp(args.argv[i], "-search-next")==0) {
        options.serverOperation=OP_SEARCH_NEXT;
    }
    else if (strcmp(args.argv[i], "-search-previous")==0) {
        options.serverOperation=OP_SEARCH_PREVIOUS;
    }
    else if (strncmp(args.argv[i], "-search-goto=",13)==0) {
        options.serverOperation = OP_SEARCH_GOTO_N;
        options.index = 0;
        sscanf(args.argv[i]+13, "%d",&options.index);
    }
    else if (strncmp(args.argv[i], "-search-select=",15)==0) {
        options.serverOperation = OP_SEARCH_SELECT;
        options.index = 0;
        sscanf(args.argv[i]+15, "%d",&options.index);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processTOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-trace")==0) {
        /* -trace already handled in initLogging() */
    }
    else if (strncmp(args.argv[i], "-to-parameter=",14)==0) {
        options.toParameterNumber = 0;
        sscanf(args.argv[i]+14, "%d", &options.toParameterNumber);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processUOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-unused-in-project")==0)
        options.serverOperation = OP_UNUSED_IN_PROJECT;
    else if (strcmp(args.argv[i], "-unused-in-file")==0)
        options.serverOperation = OP_UNUSED_IN_FILE;
    else return false;
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
        /* Already handled in initLogging() but we need to allow it */
    }
    else return false;
    *argi = i;
    return true;
}

static bool processXOption(int *argi, ArgumentsVector args) {
    int i = * argi;
    if (0) {}
    else if (strcmp(args.argv[i], "-xrefactory-II") == 0){
        xrefactoryIIGiven = true;
    }
    else if (strncmp(args.argv[i], "-xrefrc=",8) == 0) {
        xrefrcGiven = true;
    }
    else if (strcmp(args.argv[i], "-xrefrc") == 0) {
        ensureNextArgumentIsAFileName(&i, args);
        xrefrcGiven = true;
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
            case 'l': case 'L':
                matched = processLOption(&i, args);
                break;
            case 'm': case 'M':
                matched = processMOption(&i, args);
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
            if (doProcessFiles == COLLECT_ARGUMENTS_YES) {
                addToStringListOption(&options.inputFiles, args.argv[i]);
                requestFileArgumentName = args.argv[i];
            }
        }
        if (!matched) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "unknown option %s, (try c-xref -help)\n", args.argv[i]);
            errorMessage(ERR_ST, tmpBuff);
        }
    }
    LEAVE();
}

void clearRequestFileArgument(void) {
    requestFileArgumentName = NULL;
}

/* Enter the file the request named in the file table and return its file number,
 * or NO_FILE_NUMBER if the request named none. It is the editor's current buffer,
 * which need not exist on disk yet. */
int addRequestFileToFileTable(void) {
    if (requestFileArgumentName == NULL)
        return NO_FILE_NUMBER;

    return addFileNameToFileTable(requestFileArgumentName);
}


/* Get the project name from an options file (first section header).
 * Used in auto-detection mode where file coverage is implicit.
 * Returns true if a section was found, false otherwise.
 */
static bool getProjectNameFromConfigFile(FILE *configFile, /* out */ char *projectName) {
    int ch = ' ';

    while (ch != EOF) {
        while (ch == ' ' || ch == '\t' || ch == '\n')
            ch = readChar(configFile);
        if (ch == '[') {
            int i = 0;
            while (ch != ']' && ch != EOF) {
                ch = readChar(configFile);
                projectName[i++] = ch;
            }
            projectName[i-1] = '\0';
            return true;
        } else {
            while (ch != '\n' && ch != EOF)
                ch = readChar(configFile);
        }
    }
    return false;
}


/* LEGACY: Return a project name if found, else NULL. Only handles cases where the file
 * path is included in the project name/section. */
protected bool projectCoveringFileInConfigFile(char *fileName, FILE *configFile, /* out */ char *projectName) {
    int ch = ' ';              /* Something to get started */

    while (ch != EOF) {
        while (ch == ' ' || ch == '\t' || ch == '\n')
            ch = readChar(configFile);
        if (ch == '[') {
            char buffer[TMP_BUFF_SIZE];
            int i=0;
            while (ch != ']' && ch != EOF) {
                ch = readChar(configFile);
                buffer[i++] = ch;
            }
            buffer[i-1] = '\0';
            if (pathncmp(buffer, fileName, strlen(buffer), true)==0) {
                strcpy(projectName, buffer);
                return true;
            }
        } else
            while (ch != '\n' && ch != EOF)
                ch = readChar(configFile);
    }
    return false;
}

/* Apply convention-based database path for auto-detected projects.
 * Called after options processing to set cxFileLocation to <projectRoot>/.c-xref/db.
 * Any -refs from .c-xrefrc is ignored — the snapshot location is a convention,
 * not a configuration option.
 */
void applyConventionBasedDatabasePath(void) {
    assert(autoDetectedProjectRoot[0] != '\0')

    char dbPath[MAX_FILE_NAME_SIZE + 16];
    sprintf(dbPath, "%s/.c-xref/db", autoDetectedProjectRoot);
    options.cxFileLocation = allocateStringForOption(&options.cxFileLocation, dbPath);
    options.detectedProjectRoot = allocateStringForOption(&options.detectedProjectRoot, autoDetectedProjectRoot);
    setOptionVariable("__PROJECT_ROOT", autoDetectedProjectRoot);
    log_debug("Applied convention-based database path: %s", options.cxFileLocation);
}

/* Search upward from sourceFilename for a project-local .c-xrefrc file.
 * Returns true if found and the config covers the source file.
 * Sets foundConfigFilename to the path of the .c-xrefrc file.
 * Sets foundProjectName to the project name from the matching section.
 * Stops at HOME, so that a ~/.c-xrefrc from the old user-central config does not make
 * HOME a project root.
 */
static bool searchUpwardForProjectLocalConfig(char *sourceFilename, char *foundConfigFilename,
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
        /* Stop at HOME, a ~/.c-xrefrc there is an old user-central config */
        if (homeDir != NULL && strcmp(searchDir, homeDir) == 0) {
            log_debug("Reached HOME directory, stopping search");
            break;
        }
        snprintf(candidatePath, sizeof(candidatePath), "%s/.c-xrefrc", searchDir);
        log_trace("Checking for project-local config: %s", candidatePath);

        if (fileExists(candidatePath)) {
            FILE *configFile = openFile(candidatePath, "r");
            if (configFile != NULL) {
                char projectName[MAX_FILE_NAME_SIZE+16];
                /* In auto-detection mode, file coverage is implicit (file is under config dir).
                 * Just extract the project name from the first section header. */
                bool found = getProjectNameFromConfigFile(configFile, projectName);
                closeFile(configFile);
                if (!found) {
                    /* Empty config or no section - use directory basename as project name */
                    strcpy(projectName, simpleFileName(searchDir));
                    log_debug("Empty config, using directory basename as project name: %s", projectName);
                }
                strcpy(foundConfigFilename, candidatePath);
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

void searchForProjectConfigFileAndProjectForFile(char *sourceFilename, char *foundConfigFilename,
                                                  char *foundProjectName) {
    foundConfigFilename[0] = 0;
    foundProjectName[0] = 0;
    autoDetectedProjectRoot[0] = '\0';  /* Reset for each invocation */

    if (sourceFilename == NULL)
        return;

    searchUpwardForProjectLocalConfig(sourceFilename, foundConfigFilename, foundProjectName);
}

void printOptionsMemoryStatistics(void) {
    printMemoryStatisticsFor(&optMemory);
    printMemoryStatisticsFor(&options.memory);
}
