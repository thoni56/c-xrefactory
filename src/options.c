#include "options.h"

#include "commons.h"
#include "globals.h"
#include "misc.h"
#include "cxref.h"
#include "yylex.h"
#include "classfilereader.h"
#include "editor.h"
#include "fileio.h"
#include "filetable.h"

#include "main.h" /* For mainHandleSetOption() */

#include "protocol.h"

#include "log.h"


/* PUBLIC DATA: */

Options options;               // current options
Options refactoringOptions;    // xref -refactory command line options
Options savedOptions;
Options presetOptions = {
                                /* GENERAL */
    false,                      // exit
    "gcc",                      // path to compiler to use for auto-discovering compiler and defines
    MULE_DEFAULT,               // encoding
    false,                      // completeParenthesis
    NID_IMPORT_ON_DEMAND,       // defaultAddImportStrategy
    0,                          // referenceListWithoutSource
    1,                          // completionOverloadWizardDeep
    0,                          // comment moving level
    NULL,                       // prune name
    NULL,                       // input files
    RC_NONE,                    // continue refactoring
    0,                          // completion case sensitive
    NULL,                       // xrefrc
    NO_EOL_CONVERSION,          // crlfConversion
    NULL,                       // checkVersion
    DONT_DISPLAY_NESTED_CLASSES,                 // nestedClassDisplaying
    NULL,                       // pushName
    0,                          // parnum2
    "",                         // refactoring parameter 1
    "",                         // refactoring parameter 2
    AVR_NO_REFACTORING,         // refactoring
    false,                      // briefoutput
    NULL,                       // renameTo
    UndefinedMode,              // refactoringMode
    false,                      // xrefactory-II
    NULL,                       // moveTargetFile
#if defined (__WIN32__)
    "c;C",                      // cFilesSuffixes
    "java;JAV",                 // javaFilesSuffixes
#else
    "c:C",                      // cFilesSuffixes
    "java",                     // javaFilesSuffixes
#endif
    true,                       // fileNamesCaseSensitive
    TSS_FULL_SEARCH,            // search Tag file specifics
    "",                         // windel file
    0,                          // following is windel line:col x line-col
    0,
    0,
    0,
    false,                      // noerrors
    0,                          // fqtNameToCompletions
    NULL,                       // moveTargetClass
    0,                          // TPC_NONE, trivial pre-check
    true,                       // urlGenTemporaryFile
    true,                       // urlautoredirect
    false,                      // javafilesonly
    false,                      // exact position
    NULL,                       // -o outputFileName
    NULL,                       // -line lineFileName
    NULL,                       // -I include dirs
    DEFAULT_CXREF_FILENAME,         // -refs

    NULL,                       // file move for safety check
    NULL,
    0,                          // first moved line
    MAXIMAL_INT,                // safety check number of lines moved
    0,                          // new line number of the first line

    "",                         // getValue
    true,                       // javaSlAllowed (autoUpdateFromSrc)

    /* JAVADOC: */
    "java.applet:java.awt:java.beans:java.io:java.lang:java.math:java.net:java.rmi:java.security:java.sql:java.text:java.util:javax.accessibility:javax.swing:org.omg.CORBA:org.omg.CosNaming",     // -htmljavadocavailable
    NULL,                       // htmlJdkDocUrl - "http://java.sun.com/j2se/1.3/docs/api",

    /* JAVA: */
    "",                         // javaDocPath
    false,                      // allowPackagesOnCl
    NULL,                       // sourcepath
    "/tmp",                     // jdocTmpDir

    /* MIXED THINGS... */
    false,                      // noIncludeRefs
    true,                       // allowClassFileRefs
    0,
    "",
    RESOLVE_DIALOG_DEFAULT,     // manual symbol resolution TODO: This is different from any of the RESOLVE values above, why?
    NULL,                       // browsed symbol name
    true,                       // modifiedFlag
    0,
    (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE), // ooChecksBits
    1,                          // cxMemoryFactor
    0,                          /* strict ansi */
    NULL,                       /* project */
    "0:3",                      // olcxlccursor
    "",                         /* olcxSearchString */
    79,                         /* olineLen */
    "*_",                       /* olExtractAddrParPrefix */
    0,                          // extractMode, must be zero
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

    false,                      /* no_stdoptions */

    /* CXREF options  */
    false,                      /* errors */
    UPDATE_DEFAULT,             /* type of update */
    false,                      // updateOnlyModifiedFiles
    0,                          /* referenceFileCount */

    // all the rest is initialized to zeros
    {0, },                      // get/set end

    // pending memory for string values
    NULL,                       /* allAllocatedStrings */
    {0, },                      /* pendingMemory */
    {0, },                      /* pendingFreeSpace[] */
};


/* memory where on-line given options are stored */
static char optMemory[SIZE_optMemory];
static int optMemoryIndex;

static char javaSourcePathExpanded[MAX_OPTION_LEN];
static char javaClassPathExpanded[MAX_OPTION_LEN];

static char base[MAX_FILE_NAME_SIZE];

static char previousStandardOptionsFile[MAX_FILE_NAME_SIZE];
static char previousStandardOptionsSection[MAX_FILE_NAME_SIZE];


#define ENV_DEFAULT_VAR_FILE            "${__file}"
#define ENV_DEFAULT_VAR_PATH            "${__path}"
#define ENV_DEFAULT_VAR_NAME            "${__name}"
#define ENV_DEFAULT_VAR_SUFFIX          "${__suff}"
#define ENV_DEFAULT_VAR_THIS_CLASS      "${__this}"
#define ENV_DEFAULT_VAR_SUPER_CLASS     "${__super}"


void aboutMessage(void) {
    char output[REFACTORING_TMP_STRING_SIZE];
    sprintf(output, "C-xrefactory version %s\n", C_XREF_VERSION_NUMBER);
    sprintf(output+strlen(output), "Compiled at %s on %s\n",  __TIME__, __DATE__);
    sprintf(output+strlen(output), "from git revision %s.\n", GIT_HASH);
    sprintf(output+strlen(output), "(c) 1997-2004 by Xref-Tech, http://www.xref-tech.com\n");
    sprintf(output+strlen(output), "Released into GPL 2009 by Marian Vittek (SourceForge)\n");
    sprintf(output+strlen(output), "Work resurrected and continued by Thomas Nilefalk 2015-\n");
    sprintf(output+strlen(output), "(https://github.com/thoni56/c-xrefactory)\n");
    if (options.exit) {
        sprintf(output+strlen(output), "Exiting!");
    }
    if (options.xref2) {
        ppcGenRecord(PPC_INFORMATION, output);
    } else {
        fprintf(stdout, "%s", output);
    }
    if (options.exit)
        exit(XREF_EXIT_BASE);
}


void xrefSetenv(char *name, char *val) {
    SetGetEnv *sge;
    int j, n;

    sge = &options.setGetEnv;
    n = sge->num;
    if (n+1>=MAX_SET_GET_OPTIONS) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "maximum of %d -set options reached", MAX_SET_GET_OPTIONS);
        errorMessage(ERR_ST, tmpBuff);
        sge->num--; n--;
    }

    for(j=0; j<n; j++) {
        assert(sge->name[j]);
        if (strcmp(sge->name[j], name)==0)
            break;
    }
    if (j==n)
        createOptionString(&(sge->name[j]), name);
    if (j==n || strcmp(sge->value[j], val)!=0) {
        createOptionString(&(sge->value[j]), val);
    }
    log_debug("setting xrefEnvVar '%s' to '%s'\n", name, val);
    if (j==n)
        sge->num++;
}


typedef struct stringPointerList {
    char **destination;
    struct stringPointerList *next;
} StringPointerList;

static StringPointerList *newStringPointerList(char **destination, StringPointerList *next) {
    StringPointerList *list;
    OPT_ALLOC(list, StringPointerList);
    list->destination = destination;
    list->next = next;
    return list;
}

static void optionAddStringToAllocatedList(char **destination) {
    StringPointerList *ll;
    for (ll=options.allAllocatedStrings; ll!=NULL; ll=ll->next) {
        // reassignement, do not keep two copies
        if (ll->destination == destination) break;
    }
    if (ll==NULL) {
        ll = newStringPointerList(destination, options.allAllocatedStrings);
        options.allAllocatedStrings = ll;
    }
}

static void allocOptionString(void **optAddress, int size) {
    char **res;
    res = (char**)optAddress;
    OPT_ALLOCC((*res), size, char); /* TODO: WTF what side effects does this have?! */
    optionAddStringToAllocatedList(res);
}

/* TODO: Memory management is a mystery, e.g. this can't be turned into a function returning the address... */
void createOptionString(char **optAddress, char *text) {
    allocOptionString((void**)optAddress, strlen(text)+1);
    strcpy(*optAddress, text);
}

static void copyOptionShiftPointer(char **lld, Options *dest, Options *src) {
    char    **dlld;
    int     offset, localOffset;
    offset = ((char*)dest) - ((char*)src);
    localOffset = ((char*)lld) - ((char*)src);
    dlld = ((char**) (((char*)dest) + localOffset));
    // dlld is dest equivalent of *lld from src
    //&fprintf(dumpOut, "shifting (%x->%x) [%x]==%x ([%x]==%x), offsets == %d, %d, size==%d\n", src, dest, lld, *lld, dlld, *dlld, offset, localOffset, sizeof(Options));
    if (*dlld != *lld) {
        fprintf(errOut, "problem %s\n", *lld);
    }
    assert(*dlld == *lld);
    *dlld = *lld + offset;
}

void copyOptionsFromTo(Options *src, Options *dest) {
    memcpy(dest, src, sizeof(Options));
    for (StringPointerList **l= &src->allAllocatedStrings; *l!=NULL; l = &(*l)->next) {
        copyOptionShiftPointer((*l)->destination, dest, src);
        copyOptionShiftPointer(((char**)&(*l)->destination), dest, src);
        copyOptionShiftPointer(((char**)l), dest, src);
    }
}

void addStringListOption(StringList **optlist, char *string) {
    StringList **list;
    for (list=optlist; *list!=NULL; list= &(*list)->next)
        ;

    /* TODO refactor out to newOptionString()? */
    allocOptionString((void**)list, sizeof(StringList));
    createOptionString(&(*list)->string, string);
    (*list)->next = NULL;
}

static void scheduleCommandLineEnteredFileToProcess(char *fn) {
    ENTER();
    int fileIndex = addFileNameToFileTable(fn);
    FileItem *fileItem = getFileItem(fileIndex);
    if (options.mode!=ServerMode) {
        // yes in edit server you process also headers, etc.
        fileItem->isArgument = true;
    }
    log_trace("recursively process command line argument file #%d '%s'", fileIndex, fileItem->name);
    if (!options.updateOnlyModifiedFiles) {
        fileItem->isScheduled = true;
    }
    LEAVE();
}

static bool fileNameShouldBePruned(char *fn) {
    for (StringList *s=options.pruneNames; s!=NULL; s=s->next) {
        MapOverPaths(s->string, {
                if (compareFileNames(currentPath, fn)==0)
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
            mapDirectoryFiles(dirName, dirInputFile, DO_NOT_ALLOW_EDITOR_FILES,
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
        if (options.javaFilesOnly && options.mode != ServerMode
            && !fileNameHasOneOfSuffixes(fname, options.javaFilesSuffixes)
            && !fileNameHasOneOfSuffixes(fname, "jar:class")
        ) {
            return;
        }
        scheduleCommandLineEnteredFileToProcess(dirName);
    } else if (containsWildcard(dirName)) {
        char wildcardPath[MAX_OPTION_LEN];
        expandWildcardsInOnePath(dirName, wildcardPath, MAX_OPTION_LEN);
        MapOverPaths(wildcardPath, {
                dirInputFile(currentPath, "", NULL, NULL, recurseFlag, &isTopDirectory);
            });
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
    strcpy(path, directoryName_st(filename));
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

static void expandEnvironmentVariables(char *original, int availableSize, int *len, bool global_environment_only) {
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
                    value = getXrefEnvironmentValue(variableName);
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

static void processSingleSectionMarker(char *path, char *section,
                                       bool *writeFlagP, char *resultingSection) {
    int length;
    bool casesensitivity=true;

    length = strlen(path);
#if defined (__WIN32__)
    casesensitivity = false;
#endif
    if (pathncmp(path, section, length, casesensitivity)==0
        && (section[length]=='/' || section[length]=='\\' || section[length]==0)) {
        if (length > strlen(resultingSection)) {
            strcpy(resultingSection,path);
            assert(strlen(resultingSection)+1 < MAX_FILE_NAME_SIZE);
        }
        *writeFlagP = true;
    } else {
        *writeFlagP = false;
    }
}

static void processSectionMarker(char *markerText, int markerLength, char *project, char *section,
                                 bool *writeFlagP, char *resultingSection) {
    char *projectName;
    char firstPath[MAX_FILE_NAME_SIZE] = "";

    /* Remove surrounding brackets */
    markerText[markerLength-1]=0;
    projectName = &markerText[1];
    log_debug("processing %s for file %s project==%s", projectName, section, project);

    *writeFlagP = false;
    MapOverPaths(projectName, {
        if (firstPath[0] == 0)
            strcpy(firstPath, currentPath);
        if (project != NULL) {
            if (strcmp(currentPath, project) == 0) {
                strcpy(resultingSection, currentPath);
                assert(strlen(resultingSection) + 1 < MAX_FILE_NAME_SIZE);
                *writeFlagP = true;
                goto fini;
            } else {
                *writeFlagP = false;
            }
        } else {
            processSingleSectionMarker(currentPath, section, writeFlagP, resultingSection);
            if (*writeFlagP)
                goto fini;
        }
    });
fini:;
    if (*writeFlagP) {
        // TODO!!! YOU NEED TO ALLOCATE SPACE FOR THIS!!!
        strcpy(base, resultingSection);
        assert(strlen(resultingSection) < MAX_FILE_NAME_SIZE-1);
        xrefSetenv("__BASE", base);
        strcpy(resultingSection, firstPath);
        // completely wrong, what about file names from command line ?
        //&strncpy(cwd, resultingSection, MAX_FILE_NAME_SIZE-1);
        //&cwd[MAX_FILE_NAME_SIZE-1] = 0;
    }
}


#define ALLOCATE_OPTION_SPACE(memFl, cc, num, type) { \
        if (memFl==ALLOCATE_IN_SM) {                  \
            SM_ALLOCC(optMemory, cc, num, type);      \
        } else if (memFl==ALLOCATE_IN_PP) {           \
            PPM_ALLOCC(cc, num, type);                \
        } else {                                      \
            assert(0);                                \
        }                                             \
    }

static int addOptionToArgs(MemoryKind memoryKind, char optionText[], int argc, char *argv[]) {
    char *s = NULL;
    ALLOCATE_OPTION_SPACE(memoryKind, s, strlen(optionText) + 1, char);
    assert(s);
    strcpy(s, optionText);
    log_trace("option %s read", s);
    argv[argc] = s;
    if (argc < MAX_STD_ARGS - 1)
        argc++;

    return argc;
}

bool readOptionsFromFileIntoArgs(FILE *file, int *outArgc, char ***outArgv, MemoryKind memoryKind,
                                 char *section, char *project, char *resultingSection) {
    char optionText[MAX_OPTION_LEN];
    int len, argc, ch, passNumber=0;
    bool isActiveSection, isActivePass;
    bool found = false;
    char **aargv, *argv[MAX_STD_ARGS];

    ENTER();

    argc = 1;
    aargv=NULL;
    isActiveSection = isActivePass = true;
    resultingSection[0]=0;

    if (memoryKind == ALLOCATE_IN_SM)
        SM_INIT(optMemory);

    ch = 'a';                    /* Something not EOF */
    while (ch!=EOF) {
        ch = getOptionFromFile(file, optionText, &len);
        assert(strlen(optionText) == len);
        if (ch==EOF) {
            log_trace("got option from file (@EOF): '%s'", optionText);
        } else {
            log_trace("got option from file: '%s'", optionText);
        }
        if (len>=2 && optionText[0]=='[' && optionText[len-1]==']') {
            log_trace("checking '%s'", optionText);
            expandEnvironmentVariables(optionText+1, MAX_OPTION_LEN, &len, true);
            log_trace("expanded '%s'", optionText);
            processSectionMarker(optionText, len+1, project, section, &isActiveSection, resultingSection);
        } else if (isActiveSection && strncmp(optionText, "-pass", 5) == 0) {
            sscanf(optionText+5, "%d", &passNumber);
            isActivePass = passNumber==currentPass || currentPass==ANY_PASS;
            if (passNumber > maxPasses)
                maxPasses = passNumber;
        } else if (strcmp(optionText,"-set")==0 && (isActiveSection && isActivePass) && memoryKind!=DONT_ALLOCATE) {
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
                mainHandleSetOption(argc, argv, argc - 3);
            }
        } else if (ch != EOF && (isActiveSection && isActivePass)) {
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
        ALLOCATE_OPTION_SPACE(memoryKind, aargv, argc, char*);
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
    char realSection[MAX_FILE_NAME_SIZE];

    file = openFile(fileName,"r");
    if (file==NULL)
        FATAL_ERROR(ERR_CANT_OPEN, fileName, XREF_EXIT_ERR);
    readOptionsFromFileIntoArgs(file, nargc, nargv, ALLOCATE_IN_PP, section, project, realSection);
    closeFile(file);
}

void readOptionsFromCommand(char *command, int *outArgc, char ***outArgv, char *section) {
    FILE *file;
    char realSection[MAX_FILE_NAME_SIZE];

    file = popen(command, "r");
    if (file==NULL)
        FATAL_ERROR(ERR_CANT_OPEN, command, XREF_EXIT_ERR);
    readOptionsFromFileIntoArgs(file, outArgc, outArgv, ALLOCATE_IN_PP, section, NULL, realSection);
    closeFile(file);
}

void getPipedOptions(int *outNargc, char ***outNargv) {
    *outNargc = 0;
    assert(options.mode);
    if (options.mode == ServerMode) {
        char nsect[MAX_FILE_NAME_SIZE];
        readOptionsFromFileIntoArgs(stdin, outNargc, outNargv, ALLOCATE_IN_SM, "", NULL, nsect);
        /* those options can't contain include or define options, */
        /* sections neither */
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

static char *getClassPath(bool defaultCpAllowed) {
    char *cp;
    cp = options.classpath;
    if (cp == NULL || *cp==0)
        cp = getEnv("CLASSPATH");
    if (cp == NULL || *cp==0) {
        if (defaultCpAllowed)
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


static void processClassPathString(char *cp) {
    char            *nn, *np, *sfp;
    char            ttt[MAX_FILE_NAME_SIZE];
    StringList    **ll;
    int             ind, nlen;

    for (ll = &javaClassPaths; *ll != NULL; ll = &(*ll)->next) ;

    while (*cp!=0) {
        for(ind=0; cp[ind]!=0 && cp[ind]!=CLASS_PATH_SEPARATOR; ind++) {
            ttt[ind]=cp[ind];
        }
        ttt[ind]=0;
        np = normalizeFileName(ttt,cwd);
        nlen = strlen(np); sfp = np+nlen-4;
        if (nlen>=4 && (strcmp(sfp,".zip")==0 || strcmp(sfp,".jar")==0)){
            // probably zip archive
            log_debug("Indexing '%s'", np);
            zipIndexArchive(np);
            log_debug("Done.");
        } else {
            // just path
            PPM_ALLOCC(nn, strlen(np)+1, char);
            strcpy(nn,np);
            PPM_ALLOC(*ll, StringList);
            **ll = (StringList){.string = nn, .next = NULL};
            ll = &(*ll)->next;
        }
        cp += ind;
        if (*cp == CLASS_PATH_SEPARATOR) cp++;
    }
}


static void convertPackageNameToPath(char *name, char *path) {
    char *np, *pp;

    for(pp=path,np=name; *np; pp++,np++) {
        if (*np == '.') *pp = FILE_PATH_SEPARATOR;
        else *pp = *np;
    }
    *pp = 0;
}


static int copyPathSegment(char *cp, char path[]) {
    int ind;
    for(ind=0; cp[ind]!=0 && cp[ind]!=CLASS_PATH_SEPARATOR; ind++) {
        path[ind]=cp[ind];
    }
    path[ind] = 0;

    return ind;
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

void getJavaClassAndSourcePath(void) {
    char *cp, *jdkcp;
    int i;

    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES; i++) {
        if (zipArchiveTable[i].fn[0] == 0) break;
        zipArchiveTable[i].fn[0]=0;
    }

    // Keeping this comment as a historical artefact:
    // optimize wild char expand and getEnv [5.2.2003]
    javaClassPaths = NULL;

    if (LANGUAGE(LANG_JAVA)) {
        javaSetSourcePath(0);

        if (javaSourcePaths==NULL) {
            if (LANGUAGE(LANG_JAVA)) {
                errorMessage(ERR_ST, "no classpath or sourcepath specified");
            }
            javaSourcePaths = defaultClassPath;
        }

        cp = getClassPath(true);
        expandWildcardsInPaths(cp, javaClassPathExpanded, MAX_OPTION_LEN);
        cp = javaClassPathExpanded;

        createOptionString(&options.classpath, cp);  //??? why is this, only optimisation of getEnv?
        processClassPathString(cp);
        jdkcp = getJdkClassPathQuickly();
        if (jdkcp != NULL && *jdkcp!=0) {
            createOptionString(&options.jdkClassPath, jdkcp);  //only optimisation of getEnv?
            processClassPathString( jdkcp);
        }

        if (LANGUAGE(LANG_JAVA) && options.mode != ServerMode) {
            static bool messageFlag=false;
            if (messageFlag && ! options.briefoutput) {
                if (options.xref2) {
                    char tmpBuff[TMP_BUFF_SIZE];
                    if (jdkcp!=NULL && *jdkcp!=0) {
                        sprintf(tmpBuff,"java runtime == %s", jdkcp);
                        ppcGenRecord(PPC_INFORMATION, tmpBuff);
                    }
                    sprintf(tmpBuff,"classpath == %s", cp);
                    ppcGenRecord(PPC_INFORMATION, tmpBuff);
                    sprintf(tmpBuff,"sourcepath == %s", javaSourcePaths);
                    ppcGenRecord(PPC_INFORMATION, tmpBuff);
                } else {
                    if (jdkcp!=NULL && *jdkcp!=0) {
                        log_debug("java runtime == %s", jdkcp);
                    }
                    log_debug("classpath == %s", cp);
                    log_debug("sourcepath == %s", javaSourcePaths);
                }
                messageFlag = true;
            }
        }
    }
}

bool referenceFileCountMatches(int newReferenceFileCount) {
    bool check;

    if (options.referenceFileCount == 0)
        check = true;
    else if (options.referenceFileCount == 1)
        check = (newReferenceFileCount <= 1);
    else
        check = (newReferenceFileCount == options.referenceFileCount);
    options.referenceFileCount = newReferenceFileCount;
    return check;
}

void getXrefrcFileName(char *fileName) {
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
    fprintf(stdout, "\t-javadocurl=<http>        - url to existing Java API docs\n");
    fprintf(stdout, "\t-javadocpath=<path>       - paths to existing Java API docs\n");
    fprintf(stdout, "\t-javadocavailable=<packs> - packages for which javadoc is available\n");
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
    fprintf(stdout, "\t-no-stdoptions            - don't read the '~/.c-xrefrc' option file\n");
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
    fprintf(stdout, "\t-o <file>                 - write output to <file>\n");
    fprintf(stdout, "\t-file <file>              - name of the file given to stdin\n");
#endif
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
    fprintf(stdout, "\t-no-classfiles            - Don't collect references from class files\n");
    fprintf(stdout, "\t-compiler=<path>          - path to compiler to use for autodiscovered includes and defines\n");
    fprintf(stdout, "\t-update                   - update existing references database\n");
    fprintf(stdout, "\t-fastupdate               - fast update (modified files only)\n");
    fprintf(stdout, "\t-fullupdate               - full update (all files)\n");
    fprintf(stdout, "\t-version                  - print version information\n");
}


#define NEXT_FILE_ARG(i) {                                              \
    char tmpBuff[TMP_BUFF_SIZE];                                        \
    i++;                                                                \
    if (i >= argc) {                                                    \
        sprintf(tmpBuff, "file name expected after %s", argv[i-1]);     \
        errorMessage(ERR_ST,tmpBuff);                                   \
        usage();                                                        \
        exit(1);                                                        \
    }                                                                   \
}

#define NEXT_ARG(i) {                                                   \
    char tmpBuff[TMP_BUFF_SIZE];                                        \
    i++;                                                                \
    if (i >= argc) {                                                    \
        sprintf(tmpBuff, "further argument(s) expected after %s", argv[i-1]); \
        errorMessage(ERR_ST,tmpBuff);                                   \
        usage();                                                        \
        exit(1);                                                        \
    }                                                                   \
}


static int handleIncludeOption(int argc, char **argv, int i) {
    int nargc;
    char **nargv;
    NEXT_FILE_ARG(i);

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
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-addimportdefault=",18)==0) {
        sscanf(argv[i]+18, "%d", &options.defaultAddImportStrategy);
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
        createOptionString(&options.browsedSymName, argv[i]+12);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processCOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strcmp(argv[i], "-crlfconversion")==0)
        options.eolConversion|=CR_LF_EOL_CONVERSION;
    else if (strcmp(argv[i], "-crconversion")==0)
        options.eolConversion|=CR_EOL_CONVERSION;
    else if (strcmp(argv[i], "-completioncasesensitive")==0)
        options.completionCaseSensitive = true;
    else if (strcmp(argv[i], "-completeparenthesis")==0)
        options.completeParenthesis = true;
    else if (strncmp(argv[i], "-completionoverloadwizdeep=",27)==0)  {
        sscanf(argv[i]+27, "%d", &options.completionOverloadWizardDeep);
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
        NEXT_FILE_ARG(i);
        createOptionString(&options.classpath, argv[i]);
    }
    else if (strncmp(argv[i], "-csuffixes=",11)==0) {
        createOptionString(&options.cFilesSuffixes, argv[i]+11);
    }
    else if (strcmp(argv[i], "-create")==0)
        options.create = true;
    else if (strncmp(argv[i], "-compiler=", 10)==0) {
        options.compiler = &argv[i][10];
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
        createOptionString(&options.olExtractAddrParPrefix, tmpString);
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
        NEXT_ARG(i);
        createOptionString(&options.getValue, argv[i]);
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
        addStringListOption(&options.includeDirs, argv[i]);
    }
    else if (strncmp(argv[i], "-I", 2)==0 && argv[i][2]!=0) {
        addStringListOption(&options.includeDirs, argv[i]+2);
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
    else if (strncmp(argv[i], "-jdoctmpdir=",12)==0) {
        int len = strlen(argv[i]);
        if (len>13 && argv[i][len-1] == FILE_PATH_SEPARATOR) {
            warningMessage(ERR_ST, "slash at the end of -jdoctmpdir path");
        }
        createOptionString(&options.jdocTmpDir, argv[i]+12);
    }
    else if (strncmp(argv[i], "-javadocavailable=",18)==0)   {
        createOptionString(&options.htmlJdkDocAvailable, argv[i]+18);
    }
    else if (strncmp(argv[i], "-javadocurl=",12)==0) {
        createOptionString(&options.htmlJdkDocUrl, argv[i]+12);
    }
    else if (strncmp(argv[i], "-javadocpath=",13)==0)    {
        createOptionString(&options.javaDocPath, argv[i]+13);
    } else if (strcmp(argv[i], "-javadocpath")==0)   {
        NEXT_FILE_ARG(i);
        createOptionString(&options.javaDocPath, argv[i]);
    }
    else if (strncmp(argv[i], "-javasuffixes=",14)==0) {
        createOptionString(&options.javaFilesSuffixes, argv[i]+14);
    }
    else if (strcmp(argv[i], "-javafilesonly")==0) {
        options.javaFilesOnly = true;
    }
    else if (strcmp(argv[i], "-jdkclasspath")==0 || strcmp(argv[i], "-javaruntime")==0) {
        NEXT_FILE_ARG(i);
        createOptionString(&options.jdkClassPath, argv[i]);
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
    else if (strncmp(argv[i], "-maxCompls=",11)==0 || strncmp(argv[i], "-maxcompls=",11)==0)  {
        sscanf(argv[i]+11, "%d", &options.maxCompletions);
    }
    else if (strncmp(argv[i], "-movetargetclass=",17)==0) {
        createOptionString(&options.moveTargetClass, argv[i]+17);
    }
    else if (strncmp(argv[i], "-movetargetfile=",16)==0) {
        createOptionString(&options.moveTargetFile, argv[i]+16);
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
    else if (strcmp(argv[i], "-no-stdoptions")==0)
        options.no_stdoptions = true;
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
        sscanf(argv[i]+10, "%d",&options.olCursorPos);
    }
    else if (strncmp(argv[i], "-olmark=",8)==0) {
        sscanf(argv[i]+8, "%d",&options.olMarkPos);
    }
    else if (strncmp(argv[i], "-olcheckversion=",16)==0) {
        createOptionString(&options.checkVersion, argv[i]+16);
        options.serverOperation = OLO_CHECK_VERSION;
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
    else if (strcmp(argv[i], "-olexaddress")==0) {
        options.extractMode = EXTRACT_FUNCTION_ADDRESS_ARGS;
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
    else if (strcmp(argv[i], "-olcxunmodified")==0)  {
        options.modifiedFlag = false;
    }
    else if (strcmp(argv[i], "-olcxmodified")==0)    {
        options.modifiedFlag = true;
    }
    else if (strcmp(argv[i], "-olcxrename")==0)
        options.serverOperation = OLO_RENAME;
    else if (strcmp(argv[i], "-olcxencapsulate")==0)
        options.serverOperation = OLO_ENCAPSULATE;
    else if (strcmp(argv[i], "-olcxargmanip")==0)
        options.serverOperation = OLO_ARG_MANIP;
    else if (strcmp(argv[i], "-olcxdynamictostatic1")==0)
        options.serverOperation = OLO_VIRTUAL2STATIC_PUSH;
    else if (strcmp(argv[i], "-olcxsafetycheckinit")==0)
        options.serverOperation = OLO_SAFETY_CHECK_INIT;
    else if (strcmp(argv[i], "-olcxsafetycheck1")==0)
        options.serverOperation = OLO_SAFETY_CHECK1;
    else if (strcmp(argv[i], "-olcxsafetycheck2")==0)
        options.serverOperation = OLO_SAFETY_CHECK2;
    else if (strcmp(argv[i], "-olcxintersection")==0)
        options.serverOperation = OLO_INTERSECTION;
    else if (strcmp(argv[i], "-olcxsafetycheckmovedfile")==0) {
        NEXT_ARG(i);
        createOptionString(&options.checkFileMovedFrom, argv[i]);
        NEXT_ARG(i);
        createOptionString(&options.checkFileMovedTo, argv[i]);
    }
    else if (strcmp(argv[i], "-olcxwindel")==0) {
        options.serverOperation = OLO_REMOVE_WIN;
    }
    else if (strcmp(argv[i], "-olcxwindelfile")==0) {
        NEXT_ARG(i);
        createOptionString(&options.olcxWinDelFile, argv[i]);
    }
    else if (strncmp(argv[i], "-olcxwindelwin=",15)==0) {
        options.olcxWinDelFromLine = options.olcxWinDelToLine = 0;
        options.olcxWinDelFromCol = options.olcxWinDelToCol = 0;
        sscanf(argv[i]+15, "%d:%dx%d:%d",
               &options.olcxWinDelFromLine, &options.olcxWinDelFromCol,
               &options.olcxWinDelToLine, &options.olcxWinDelToCol);
        log_trace("; delete refs %d:%d-%d:%d", options.olcxWinDelFromLine, options.olcxWinDelFromCol,
                  options.olcxWinDelToLine, options.olcxWinDelToCol);
    }
    else if (strncmp(argv[i], "-olcxsafetycheckmovedblock=",27)==0) {
        sscanf(argv[i]+27, "%d:%d:%d", &options.checkFirstMovedLine,
               &options.checkLinesMoved, &options.checkNewLineNumber);
        log_trace("safety check block moved == %d:%d:%d", options.checkFirstMovedLine, options.checkLinesMoved,
                  options.checkNewLineNumber);
    }
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
    else if (strcmp(argv[i], "-olcxedittop")==0)
        options.serverOperation = OLO_TOP_SYMBOL_RES;
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
    else if (strcmp(argv[i], "-olcxencapsulatesc1")==0)
        options.serverOperation = OLO_PUSH_ENCAPSULATE_SAFETY_CHECK;
    else if (strcmp(argv[i], "-olcxencapsulatesc2")==0)
        options.serverOperation = OLO_ENCAPSULATE_SAFETY_CHECK;
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
    else if (strcmp(argv[i], "-olcxtarget")==0)
        options.serverOperation=OLO_SET_MOVE_TARGET;
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
        createOptionString(&options.olcxlccursor, argv[i]+14);
    }
    else if (strcmp(argv[i], "-olcxsearch")==0)
        options.serverOperation = OLO_SEARCH;
    else if (strncmp(argv[i], "-olcxcplsearch=",15)==0) {
        options.serverOperation=OLO_SEARCH;
        createOptionString(&options.olcxSearchString, argv[i]+15);
    }
    else if (strncmp(argv[i], "-olcxtagsearch=",15)==0) {
        options.serverOperation=OLO_TAG_SEARCH;
        createOptionString(&options.olcxSearchString, argv[i]+15);
    }
    else if (strcmp(argv[i], "-olcxtagsearchforward")==0) {
        options.serverOperation=OLO_TAG_SEARCH_FORWARD;
    }
    else if (strcmp(argv[i], "-olcxtagsearchback")==0) {
        options.serverOperation=OLO_TAG_SEARCH_BACK;
    }
    else if (strncmp(argv[i], "-olcxpushname=",14)==0)   {
        options.serverOperation = OLO_PUSH_NAME;
        createOptionString(&options.pushName, argv[i]+14);
    }
    else if (strncmp(argv[i], "-olcxpushspecialname=",21)==0)    {
        options.serverOperation = OLO_PUSH_SPECIAL_NAME;
        createOptionString(&options.pushName, argv[i]+21);
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
        options.serverOperation = OLO_CBROWSE;
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
        NEXT_FILE_ARG(i);
        createOptionString(&options.outputFileName, argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processPOption(int *argi, int argc, char **argv) {
    char    ttt[MAX_FILE_NAME_SIZE];
    int     i = *argi;

    if (0) {}
    else if (strncmp(argv[i], "-pause",5)==0) {
        /* Pause to be able to attach with debugger... */
        NEXT_ARG(i);
        sleep(atoi(argv[i]));
    }
    else if (strncmp(argv[i], "-pass",5)==0) {
        errorMessage(ERR_ST, "'-pass' option can't be entered from command line");
    }
    else if (strcmp(argv[i], "-packages")==0) {
        options.allowPackagesOnCommandLine = true;
    }
    else if (strcmp(argv[i], "-p")==0) {
        NEXT_FILE_ARG(i);
        log_trace("Current project '%s'", argv[i]);
        createOptionString(&options.project, argv[i]);
    }
    else if (strcmp(argv[i], "-preload")==0) {
        char *file, *fromFile;
        NEXT_FILE_ARG(i);
        file = argv[i];
        strcpy(ttt, normalizeFileName(file, cwd));
        NEXT_FILE_ARG(i);
        fromFile = argv[i];
        // TODO, maybe do this also through allocated list of options
        // and serve them later ?
        //&sprintf(tmpBuff, "-preload %s %s\n", ttt, fromFile); ppcGenRecord(PPC_IGNORE, tmpBuff);
        editorOpenBufferNoFileLoad(ttt, fromFile);
    }
    else if (strcmp(argv[i], "-prune")==0) {
        NEXT_ARG(i);
        addStringListOption(&options.pruneNames, argv[i]);
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
    createOptionString(&options.cxrefsLocation, normalizeFileName(argvi, cwd));
}

static bool processROption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
    else if (strncmp(argv[i], "-refnum=",8)==0)  {
        sscanf(argv[i]+8, "%d", &options.referenceFileCount);
    }
    else if (strncmp(argv[i], "-renameto=", 10)==0) {
        createOptionString(&options.renameTo, argv[i]+10);
    }
    else if (strcmp(argv[i], "-resetIncludeDirs")==0) {
        options.includeDirs = NULL;
    }
    else if (strcmp(argv[i], "-refs")==0)    {
        NEXT_FILE_ARG(i);
        setXrefsLocation(argv[i]);
    }
    else if (strncmp(argv[i], "-refs=",6)==0)    {
        setXrefsLocation(argv[i]+6);
    }
    else if (strcmp(argv[i], "-rlistwithoutsrc")==0) {
        options.referenceListWithoutSource = 1;
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
        createOptionString(&options.refpar1, argv[i]+13);
    }
    else if (strncmp(argv[i], "-rfct-param2=", 13)==0)  {
        createOptionString(&options.refpar2, argv[i]+13);
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
        NEXT_FILE_ARG(i);
        createOptionString(&options.sourcePath, argv[i]);
        xrefSetenv("-sourcepath", options.sourcePath);
    }
    else if (strcmp(argv[i], "-stdop")==0) {
        i = handleIncludeOption(argc, argv, i);
    }
    else if (strcmp(argv[i], "-set")==0) {
        i = mainHandleSetOption(argc, argv, i);
    }
    else if (strncmp(argv[i], "-set",4)==0) {
        name = argv[i]+4;
        NEXT_ARG(i);
        val = argv[i];
        xrefSetenv(name, val);
    }
    else if (strcmp(argv[i], "-searchdef")==0) {
        options.tagSearchSpecif = TSS_SEARCH_DEFS_ONLY;
    }
    else if (strcmp(argv[i], "-searchshortlist")==0) {
        options.tagSearchSpecif = TSS_FULL_SEARCH_SHORT;
    }
    else if (strcmp(argv[i], "-searchdefshortlist")==0) {
        options.tagSearchSpecif = TSS_SEARCH_DEFS_ONLY_SHORT;
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
        createOptionString(&options.xrefrc, argv[i]+8);
    }
    else if (strcmp(argv[i], "-xrefrc") == 0) {
        NEXT_FILE_ARG(i);
        createOptionString(&options.xrefrc, argv[i]);
    }
    else return false;
    *argi = i;
    return true;
}

static bool processYOption(int *argi, int argc, char **argv) {
    int i = * argi;
    if (0) {}
#ifdef YYDEBUG
    else if (strcmp(argv[i], "-yydebug") == 0){
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
                addStringListOption(&options.inputFiles, argv[i]);
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

void searchStandardOptionsFileAndSectionForFile(char *fileName, char *optionsFileName, char *section) {
    int    fileno;
    bool   found = false;
    FILE  *optionsFile;
    int    nargc;
    char **nargv;

    optionsFileName[0] = 0;
    section[0]         = 0;

    if (fileName == NULL || options.no_stdoptions)
        return;

    /* Try to find section in HOME config. */
    getXrefrcFileName(optionsFileName);
    optionsFile = openFile(optionsFileName, "r");
    if (optionsFile != NULL) {
        // TODO: This reads all arguments, when we only want to know if there is a matching project there?
        found = readOptionsFromFileIntoArgs(optionsFile, &nargc, &nargv, DONT_ALLOCATE, fileName, options.project,
                                            section);
        if (found) {
            log_debug("options file '%s' section '%s' found", optionsFileName, section);
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
        fileno = getFileNumberFromName(fileName);
        if (fileno != noFileIndex && getFileItem(fileno)->isFromCxfile) {
            strcpy(optionsFileName, previousStandardOptionsFile);
            strcpy(section, previousStandardOptionsSection);
            return;
        }
    }
    optionsFileName[0] = 0;
}
