#include "options.h"

#include "commons.h"
#include "globals.h"
#include "misc.h"
#include "cxref.h"
#include "html.h"
#include "yylex.h"
#include "classfilereader.h"
#include "editor.h"
#include "fileio.h"

/* The following are currently needed from main:
   mainHandleSetOption
 */
#include "main.h"

#include "protocol.h"

#include "log.h"
#include "utils.h"

/* PUBLIC DATA: */

Options options;               // current options
Options refactoringOptions;    // xref -refactory command line options
Options s_cachedOptions;
Options s_initOpt = {
                                /* GENERAL */
    0,                          // exit
    "gcc",                      // path to compiler to use for auto-discovering compiler and defines
    MULE_DEFAULT,               // encoding
    0,                          // completeParenthesis
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
    CUT_OUTERS,                 // nestedClassDisplaying
    NULL,                       // pushName
    0,                          // parnum2
    "",                         // refactoring parameter 1
    "",                         // refactoring parameter 2
    AVR_NO_REFACTORING,         // refactoring
    false,                      // briefoutput
    0,                          // cacheIncludes
    0,                          // stdopFlag
    NULL,                       // renameTo
    RegimeUndefined,            // refactoringRegime
    0,                          // xrefactory-II
    NULL,                       // moveTargetFile
#if defined (__WIN32__)
    "c;C",                      // cFilesSuffixes
    "java;JAV",                 // javaFilesSuffixes
    "C;cpp;CC;cc",              // c++FilesSuffixes
#else
    "c:C",                      // cFilesSuffixes
    "java",                     // javaFilesSuffixes
    "C:cpp:CC:cc",              // c++FilesSuffixes
#endif
    1,                      // fileNamesCaseSensitive
    TSS_FULL_SEARCH,        // search Tag file specifics
    JAVA_VERSION_AUTO,      // java version
    "",                     // windel file
    0,                      // following is windel line:col x line-col
    0,
    0,
    0,
    "nouser",                   // moveToUser
    0,                          // noerrors
    0,                          // fqtNameToCompletions
    NULL,                       // moveTargetClass
    0,                          // TPC_NON, trivial pre-check
    1,                          // urlGenTemporaryFile
    1,                          // urlautoredirect
    0,                          // javafilesonly
    0,                          // exact position
    NULL,                       // -o outputFileName
    NULL,                       // -line lineFileName
    NULL,                       // -I include dirs
    DEFAULT_CXREF_FILE,         // -refs

    NULL,                       // file move for safety check
    NULL,
    0,                          // first moved line
    MAXIMAL_INT,                // safety check number of lines moved
    0,                          // new line number of the first line

    "",                         // getValue
    1,                          // javaSlAllowed (autoUpdateFromSrc)
    XFILE_HASH_DEFAULT,

    /* HTML */
    "",                         // htmlroot
    NULL,                       // htmlLineNumColor
    80,                         // htmlcxlinelen
    "java.applet:java.awt:java.beans:java.io:java.lang:java.math:java.net:java.rmi:java.security:java.sql:java.text:java.util:javax.accessibility:javax.swing:org.omg.CORBA:org.omg.CosNaming",     // -htmljavadocavailable
    ":",                        // htmlLineNumLabel
    0,                          // html cut suffix
    0,                          // htmlGenJdkDocLinks
    NULL,                       // htmlJdkDocUrl - "http://java.sun.com/j2se/1.3/docs/api",
    false,                      // htmlDirectX
    "",                         // htmlLinkSuffix
    false,                      // htmlNoUnderline
    "navy",                     // htmlLinkColor
    "",                         // htmlCutPath
    0,                          // htmlCutPathLen
    0,                          /* htmlLineNums */
    false,                      /* htmlNoColors */
    NULL,                       /* htmlZipCommand */
    false,                      /* htmlFunSeparate */
    false,                      /* htmlglobalx */
    false,                      /* htmllocalx */
    {0, },                      // -cutpaths

    /* JAVA: */
    "",                         // javaDocPath
    0,                          // allowPackagesOnCl
    NULL,                       // sourcepath
    "/tmp",                     // jdocTmpDir

    /* MIXED THINGS... */
    0,                          // noCxFile
    false,                      // javaDoc
    false,                      // noIncludeRefs
    true,                       // allowClassFileRefs
    0,
    "",
    DEFAULT_VALUE,              // manual symbol resolution
    NULL,                       // browsed symbol name
    true,                       // modifiedFlag
    0,
    (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE), // ooChecksBits
    1,                          // cxMemoryFactor
    1,
    0,
    NULL,
    "0:3",                      // olcxlccursor
    "",                         /* olcxSearchString */
    79,                         /* olineLen */
    "*_",                       /* olExtractAddrParPrefix */
    0,                          // extractMode, must be zero
    MAX_COMPLETIONS,
    0,
    0,
    "",
    true,                       // recurse directories
    "",                         // default classpath
    8,                          /* tabulator */
    -1,
    -1,
    RegimeXref,
    "nouser",                   // -user
    false,                      /* debug */
    false,                      /* trace */
    true,                       /* cpp_comment allowed? */
    0,
    0,
    "",

    false,                      /* no locals */
    false,                      /* no structs/records */
    false,                      /* no enums */
    false,                      /* no typedefs */
    false,                      /* no macros */
    false,                      /* no stdoptions */

    /* CXREF options  */
    0,                          /* show err */
    UPDATE_CREATE,              /* create, update */
    false,                      // updateOnlyModifiedFiles
    NULL,                       /* last_message */
    0,                          /* refnum */

    // all the rest is initialized to zeros
    {0, },                      // get/set end

    // pending memory for string values
    NULL,                       /* allAllocatedStrings */
    {0, },                      /* pendingMemory */
    {0, },                      /* pendingFreeSpace[] */
};


/* memory where on-line given options are stored */
#define SIZE_optMemory SIZE_opiMemory
static char optMemory[SIZE_optMemory];
static int optMemoryIndex;

static char javaSourcePathExpanded[MAX_OPTION_LEN];
static char javaClassPathExpanded[MAX_OPTION_LEN];


#define ENV_DEFAULT_VAR_FILE            "${__file}"
#define ENV_DEFAULT_VAR_PATH            "${__path}"
#define ENV_DEFAULT_VAR_NAME            "${__name}"
#define ENV_DEFAULT_VAR_SUFFIX          "${__suff}"
#define ENV_DEFAULT_VAR_THIS_CLASS      "${__this}"
#define ENV_DEFAULT_VAR_SUPER_CLASS     "${__super}"


void xrefSetenv(char *name, char *val) {
    S_setGetEnv *sge;
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
    if (j==n) createOptionString(&(sge->name[j]), name);
    if (j==n || strcmp(sge->value[j], val)!=0) {
        createOptionString(&(sge->value[j]), val);
    }
    //&fprintf(dumpOut, "setting '%s' to '%s'\n", name, val);
    if (j==n)
        sge->num ++;
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

static void optionAddToAllocatedList(char **destination) {
    StringPointerList *ll;
    for(ll=options.allAllocatedStrings; ll!=NULL; ll=ll->next) {
        // reassignement, do not keep two copies
        if (ll->destination == destination) break;
    }
    if (ll==NULL) {
        ll = newStringPointerList(destination, options.allAllocatedStrings);
        options.allAllocatedStrings = ll;
    }
}

static void allocOptionSpace(void **optAddress, int size) {
    char **res;
    res = (char**)optAddress;
    OPT_ALLOCC((*res), size, char); /* TODO: WTF what side effects does this have?! */
    optionAddToAllocatedList(res);
}

/* TODO: Memory management is a mystery, e.g. this can't be turned into a function returning the address... */
void createOptionString(char **optAddress, char *text) {
    allocOptionSpace((void**)optAddress, strlen(text)+1);
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
        fprintf(dumpOut, "problem %s\n", *lld);
    }
    assert(*dlld == *lld);
    *dlld = *lld + offset;
}

void copyOptions(Options *dest, Options *src) {
    StringPointerList    **ll;
    memcpy(dest, src, sizeof(Options));
    for(ll= &src->allAllocatedStrings; *ll!=NULL; ll = &(*ll)->next) {
        copyOptionShiftPointer((*ll)->destination, dest, src);
        copyOptionShiftPointer(((char**)&(*ll)->destination), dest, src);
        copyOptionShiftPointer(((char**)ll), dest, src);
    }
    //&fprintf(dumpOut, "options copied\n");
}

void addStringListOption(StringList **optlist, char *string) {
    StringList **list;
    for(list=optlist; *list!=NULL; list= &(*list)->next)
        ;

    /* TODO refactor out to newOptionString()? */
    allocOptionSpace((void**)list, sizeof(StringList));
    createOptionString(&(*list)->string, string);
    (*list)->next = NULL;
}

int addHtmlCutPath(char *ss) {
    int i,ln,len, res;

    res = 0;
    ss = htmlNormalizedPath(ss);
    ln = strlen(ss);
    if (ln>=1 && ss[ln-1] == FILE_PATH_SEPARATOR) {
        warningMessage(ERR_ST, "slash at the end of -htmlcutpath path, ignoring it");
        return(res);
    }
    for(i=0; i<options.htmlCut.pathsNum; i++) {
        // if yet in cutpaths, do nothing
        if (strcmp(options.htmlCut.path[i], ss)==0) return(res);
    }
    createOptionString(&(options.htmlCut.path[options.htmlCut.pathsNum]), ss);
    ss = options.htmlCut.path[options.htmlCut.pathsNum];
    options.htmlCut.plen[options.htmlCut.pathsNum] = ln;
    //&fprintf(dumpOut, "adding cutpath %d %s\n",options.htmlCut.pathsNum,ss);
    for(i=0; i<options.htmlCut.pathsNum; i++) {
        // a more specialized path after a more general, exchange them
        len = options.htmlCut.plen[i];
        if (fnnCmp(options.htmlCut.path[i], ss, len)==0) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,
                    "htmlcutpath: '%s' supersede \n\t\t'%s', exchanging them",
                    options.htmlCut.path[i], ss);
            warningMessage(ERR_ST, tmpBuff);
            res = 1;
            options.htmlCut.path[options.htmlCut.pathsNum]=options.htmlCut.path[i];
            options.htmlCut.plen[options.htmlCut.pathsNum]=options.htmlCut.plen[i];
            options.htmlCut.path[i] = ss;
            options.htmlCut.plen[i] = ln;
        }
    }
    if (options.htmlCut.pathsNum+2 >= MAX_HTML_CUT_PATHS) {
        errorMessage(ERR_ST, "# of htmlcutpaths overflow over MAX_HTML_CUT_PATHES");
    } else {
        options.htmlCut.pathsNum++;
    }
    return(res);
}

static void scheduleCommandLineEnteredFileToProcess(char *fn) {
    int fileIndex;

    ENTER();
    fileIndex = addFileTabItem(fn);
    if (options.taskRegime!=RegimeEditServer) {
        // yes in edit server you process also headers, etc.
        fileTable.tab[fileIndex]->b.commandLineEntered = true;
    }
    log_trace("recursively process command line argument file #%d '%s'", fileIndex, fileTable.tab[fileIndex]->name);
    if (!options.updateOnlyModifiedFiles) {
        fileTable.tab[fileIndex]->b.scheduledToProcess = true;
    }
    LEAVE();
}

static bool fileNameShouldBePruned(char *fn) {
    StringList    *pp;
    for(pp=options.pruneNames; pp!=NULL; pp=pp->next) {
        JavaMapOnPaths(pp->string, {
                if (compareFileNames(currentPath, fn)==0) return true;
            });
    }
    return false;
}

void dirInputFile(MAP_FUN_SIGNATURE) {
    char            *dir,*fname, *suff;
    void            *recurseFlag;
    void            *nrecurseFlag;
    struct stat     st;
    char            fn[MAX_FILE_NAME_SIZE];
    char            wcPaths[MAX_OPTION_LEN];
    int             topCallFlag, stt;

    dir = a1; fname = file; recurseFlag = a4; topCallFlag = *a5;
    if (topCallFlag == 0) {
        if (strcmp(fname, ".")==0) return;
        if (strcmp(fname, "..")==0) return;
        if (fileNameShouldBePruned(fname)) return;
        sprintf(fn, "%s%c%s",dir,FILE_PATH_SEPARATOR,fname);
        strcpy(fn, normalizeFileName(fn, cwd));
        if (fileNameShouldBePruned(fn)) return;
    } else {
        strcpy(fn, normalizeFileName(fname, cwd));
    }
    if (strlen(fn) >= MAX_FILE_NAME_SIZE) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "file name %s is too long", fn);
        fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
    }
    suff = getFileSuffix(fname);
    stt = editorFileStatus(fn, &st);
    if (stt==0  && (st.st_mode & S_IFMT)==S_IFDIR) {
        if (recurseFlag!=NULL) {
            topCallFlag = 0;
            if (options.recurseDirectories) nrecurseFlag = &topCallFlag;
            else nrecurseFlag = NULL;
            mapDirectoryFiles(fn, dirInputFile, DO_NOT_ALLOW_EDITOR_FILES,
                              fn, NULL, NULL, nrecurseFlag, &topCallFlag);
            editorMapOnNonexistantFiles(fn, dirInputFile, DEPTH_ANY,
                                        fn, NULL, NULL, nrecurseFlag, &topCallFlag);
        } else {
            // no error, let it be
            //& sprintf(tmpBuff, "omitting directory %s, missing '-r' option ?",fn);
            //& warningMessage(ERR_ST,tmpBuff);
        }
    } else if (stt==0) {
        // .class can be inside a jar archive, but this makes problem on
        // recursive read of a directory, it attempts to read .class
        if (topCallFlag==0
            &&  (! fileNameHasOneOfSuffixes(fname, options.cFilesSuffixes))
            &&  (! fileNameHasOneOfSuffixes(fname, options.javaFilesSuffixes))
            &&  compareFileNames(suff, ".y")!=0
        ) {
            return;
        }
        if (options.javaFilesOnly && options.taskRegime != RegimeEditServer
            && (! fileNameHasOneOfSuffixes(fname, options.javaFilesSuffixes))
            && (! fileNameHasOneOfSuffixes(fname, "jar:class"))
        ) {
            return;
        }
        scheduleCommandLineEnteredFileToProcess(fn);
    } else if (containsWildcard(fn)) {
        expandWildcardsInOnePath(fn, wcPaths, MAX_OPTION_LEN);
        //&fprintf(dumpOut, "wildcard path %s expanded to %s\n", fn, wcPaths);
        JavaMapOnPaths(wcPaths, {
                dirInputFile(currentPath, "", NULL, NULL, recurseFlag, &topCallFlag);
            });
    } else if (topCallFlag
               && ((!options.allowPackagesOnCommandLine)
                   || !packageOnCommandLine(fname))) {
        if (options.taskRegime!=RegimeEditServer) {
            errorMessage(ERR_CANT_OPEN, fn);
        } else {
            // hacked 16.4.2003 in order to can complete in buffers
            // without existing file
            scheduleCommandLineEnteredFileToProcess(fn);
        }
    }
}

char *expandSpecialFilePredefinedVariables_st(char *variable, char *inputFilename) {
    static char expanded[MAX_OPTION_LEN];
    int i, j;
    char *suffix;
    char filename[MAX_FILE_NAME_SIZE];
    char path[MAX_FILE_NAME_SIZE];
    char name[MAX_FILE_NAME_SIZE];
    char thisclass[MAX_FILE_NAME_SIZE];
    char superclass[MAX_FILE_NAME_SIZE];

    strcpy(filename, getRealFileNameStatic(inputFilename));
    assert(strlen(filename) < MAX_FILE_NAME_SIZE-1);
    strcpy(path, directoryName_st(filename));
    strcpy(name, simpleFileNameWithoutSuffix_st(filename));
    suffix = lastOccurenceInString(filename, '.');
    if (suffix==NULL) suffix="";
    else suffix++;
    strcpy(thisclass, s_cps.currentClassAnswer);
    strcpy(superclass, s_cps.currentSuperClassAnswer);
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

static void expandEnvironmentVariables(char *tt, int ttsize, int *len,
                                       int envFlag) {
    char ttt[MAX_OPTION_LEN];
    char vname[MAX_OPTION_LEN];
    char *vval;
    int i, d, j, starti, termc, expanded, vlen, tilda;
    i = d = 0;
    //&fprintf(dumpOut, "expanding option '%s'\n", tt);
    while(tt[i] && i<ttsize-2) {
        starti = -1;
        expanded = 0;
        termc = 0;
        tilda = 0;
#if (!defined (__WIN32__))
        if (i==0 && tt[i]=='~' && tt[i+1]=='/') {starti = i; termc='~'; tilda=1;}
#endif
        if (tt[i]=='$' && tt[i+1]=='{') {starti = i+2; termc='}';}
        if (tt[i]=='%') {starti = i+1; termc='%';}
        if (starti >= 0) {
            j = starti;
            while (isalpha(tt[j]) || isdigit(tt[j]) || tt[j]=='_') j++;
            if (j<ttsize-2 && tt[j]==termc) {
                vlen = j-starti;
                strncpy(vname, &tt[starti], vlen);
                vname[vlen]=0;
                vval = NULL;
                if (envFlag != GLOBAL_ENV_ONLY) {
                    vval = getXrefEnvironmentValue(vname);
                }
                if (vval==NULL) vval = getenv(vname);
#if (!defined (__WIN32__))
                if (tilda) vval = getenv("HOME");
#endif
                if (vval != NULL) {
                    strcpy(&ttt[d], vval);
                    d += strlen(vval);
                    expanded = 1;
                }
                //& expanded = 1;
                if (expanded) i = j+1;
            }
        }
        if (expanded==0) {
            ttt[d++] = tt[i++];
        }
        assert(d<MAX_OPTION_LEN-2);
    }
    ttt[d] = 0;
    *len = d;
    strcpy(tt, ttt);
    //&fprintf(dumpOut, "result '%s'\n", tt);
}

/* Not official API, public for unittesting */
int getOptionFromFile(FILE *file, char *text, int *chars_read) {
    int i, c;
    int res;
    bool comment;
    bool quoteInOption;

    c = readChar(file);
    do {
        quoteInOption = false;
        *chars_read = i = 0;
        comment = false;

        /* Skip all white space? */
        while ((c>=0 && c<=' ') || c=='\n' || c=='\t')
            c=readChar(file);

        if (c==EOF) {
            res = EOF;
            goto fini;
        }

        if (c=='\"') {
            /* Read a double quoted string */
            c=readChar(file);
            // Escaping the double quote (\") is not allowed, it creates problems
            // when someone finished a section name by \ reverse slash
            while (c!=EOF && c!='\"') {
                if (i < MAX_OPTION_LEN-1)
                    text[i++]=c;
                c=readChar(file);
            }
            if (c!='\"' && options.taskRegime!=RegimeEditServer) {
                fatalError(ERR_ST, "option string through end of file", XREF_EXIT_ERR);
            }
        } else if (c=='`') {
            text[i++]=c;
            c=readChar(file);
            while (c!=EOF && c!='\n' && c!='`') {
                if (i < MAX_OPTION_LEN-1)
                    text[i++]=c;
                c=readChar(file);
            }
            if (i < MAX_OPTION_LEN-1)
                text[i++]=c;
            if (c!='`'  && options.taskRegime!=RegimeEditServer) {
                errorMessage(ERR_ST, "option string through end of line");
            }
        } else if (c=='[') {
            text[i++] = c;
            c=readChar(file);
            while (c!=EOF && c!='\n' && c!=']') {
                if (i < MAX_OPTION_LEN-1)
                    text[i++]=c;
                c=readChar(file);
            }
            if (c==']')
                text[i++]=c;
        } else {
            while (c!=EOF && c>' ') {
                if (c=='\"')
                    quoteInOption = true;
                if (i < MAX_OPTION_LEN-1)
                    text[i++]=c;
                c=readChar(file);
            }
        }
        text[i]=0;
        if (quoteInOption && options.taskRegime!=RegimeEditServer) {
            static bool messageWritten = false;
            if (! messageWritten) {
                char tmpBuff[TMP_BUFF_SIZE];
                messageWritten = true;
                sprintf(tmpBuff,"option '%s' contains quotes.", text);
                warningMessage(ERR_ST, tmpBuff);
            }
        }
        /* because QNX paths can start with // */
        if (i>=2 && text[0]=='/' && text[1]=='/') {
            while (c!=EOF && c!='\n')
                c=readChar(file);
            comment = true;
        }
    } while (comment);
    if (strcmp(text, END_OF_OPTIONS_STRING)==0) {
        i = 0; res = EOF;
    } else {
        res = 'A';
    }

 fini:
    text[i] = 0;
    *chars_read = i;

    return res;
}

static void processSingleSectionMarker(char *tt,char *section,
                                       int *writeFlag, char *resSection) {
    int sl,casesensitivity=1;
    sl = strlen(tt);
#if defined (__WIN32__)
    casesensitivity = 0;
#endif
    if (pathncmp(tt, section, sl, casesensitivity)==0
        && (section[sl]=='/' || section[sl]=='\\' || section[sl]==0)) {
        if (sl > strlen(resSection)) {
            strcpy(resSection,tt);
            assert(strlen(resSection)+1 < MAX_FILE_NAME_SIZE);
        }
        *writeFlag = 1;
    } else {
        *writeFlag = 0;
    }
}

static void processSectionMarker(char *ttt,int i,char *project,char *section,
                                 int *writeFlag, char *resSection) {
    char        *tt;
    char        firstPath[MAX_FILE_NAME_SIZE];

    ttt[i-1]=0;
    tt = ttt+1;
    firstPath[0]=0;
    log_debug("processing %s for file %s project==%s", tt, section, project);
#if 1
    *writeFlag = 0;
    JavaMapOnPaths(tt, {
            if (firstPath[0]==0) strcpy(firstPath, currentPath);
            if (project!=NULL) {
                if (strcmp(currentPath, project)==0) {
                    strcpy(resSection,currentPath);
                    assert(strlen(resSection)+1 < MAX_FILE_NAME_SIZE);
                    *writeFlag = 1;
                    goto fini;
                } else {
                    *writeFlag = 0;
                }
            } else {
                processSingleSectionMarker(currentPath, section, writeFlag, resSection);
                if (*writeFlag) goto fini;
            }
        });
#else
    strcpy(firstPath, tt);
    if (project!=NULL) {
        if (strcmp(tt, project)==0) {
            strcpy(resSection,tt);
            assert(strlen(resSection)+1 < MAX_FILE_NAME_SIZE);
            *writeFlag = 1;
        } else {
            *writeFlag = 0;
        }
    } else {
        processSingleSectionMarker(tt, section, writeFlag, resSection);
    }
#endif
 fini:;
    if (*writeFlag) {
        // TODO!!! YOU NEED TO ALLOCATE SPACE FOR THIS!!!
        strcpy(s_base, resSection);
        assert(strlen(resSection) < MAX_FILE_NAME_SIZE-1);
        xrefSetenv("__BASE", s_base);
        strcpy(resSection, firstPath);
        // completely wrong, what about file names from command line ?
        //&strncpy(cwd, resSection, MAX_FILE_NAME_SIZE-1);
        //&cwd[MAX_FILE_NAME_SIZE-1] = 0;
    }
}

#define ADD_OPTION_TO_ARGS(memFl,tt,len,argv,argc) {                    \
        char *cc;                                                       \
        cc=NULL;                                                        \
        if (memFl!=MEM_NO_ALLOC) {                                      \
            OPTION_SPACE_ALLOCC(memFl, cc, len+1, char);                \
            assert(cc);                                                 \
            strcpy(cc,tt);                                              \
            /*&fprintf(dumpOut,"option %s readed\n",cc); fflush(dumpOut);&*/ \
            argv[argc] = cc;                                            \
            if (argc < MAX_STD_ARGS-1) argc++;                          \
        }                                                               \
    }


#define OPTION_SPACE_ALLOCC(memFl,cc,num,type) {                        \
        if (memFl==MEM_ALLOC_ON_SM) {                                   \
            SM_ALLOCC(optMemory,cc,num,type);                           \
        } else if (memFl==MEM_ALLOC_ON_PP) {                            \
            PP_ALLOCC(cc,num,type);                                     \
        } else {                                                        \
            assert(0);                                                  \
        }                                                               \
    }

#define ACTIVE_OPTION() (isActiveSection && isActivePass)

bool readOptionFromFile(FILE *file, int *nargc, char ***nargv, int memFl,
                       char *sectionFile, char *project, char *resSection) {
    char text[MAX_OPTION_LEN];
    int len, argc, i, c, isActiveSection, isActivePass, passn=0;
    bool found = false;
    char **aargv,*argv[MAX_STD_ARGS];

    argc = 1; isActiveSection = isActivePass = 1; aargv=NULL;
    resSection[0]=0;
    if (memFl==MEM_ALLOC_ON_SM) SM_INIT(optMemory);
    c = 'a';
    while (c!=EOF) {
        c = getOptionFromFile(file, text, &len);
        if (c==EOF) {
            log_trace("got option from file (@EOF): '%s'", text);
        } else {
            log_trace("got option from file: '%s'", text);
        }
        if (len>=2 && text[0]=='[' && text[len-1]==']') {
            log_trace("checking '%s'", text);
            expandEnvironmentVariables(text+1, MAX_OPTION_LEN, &len, GLOBAL_ENV_ONLY);
            log_trace("expanded '%s'", text);
            processSectionMarker(text,len+1,project,sectionFile,&isActiveSection,resSection);
        } else if (isActiveSection && strncmp(text,"-pass", 5) == 0) {
            sscanf(text+5, "%d", &passn);
            if (passn==currentCppPass || currentCppPass==ANY_CPP_PASS) {
                isActivePass = 1;
            } else {
                isActivePass = 0;
            }
            if (passn > s_cppPassMax) s_cppPassMax = passn;
        } else if (strcmp(text,"-set")==0 && ACTIVE_OPTION()
                   && memFl!=MEM_NO_ALLOC) {
            // pre-evaluation of -set
            found = true;
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
            c = getOptionFromFile(file,text,&len);
            expandEnvironmentVariables(text,MAX_OPTION_LEN,&len,DEFAULT_VALUE);
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
            c = getOptionFromFile(file,text,&len);
            expandEnvironmentVariables(text,MAX_OPTION_LEN,&len,DEFAULT_VALUE);
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
            if (argc < MAX_STD_ARGS) {
                assert(argc>=3);
                mainHandleSetOption(argc, argv, argc-3);
            }
        } else if (c!=EOF && ACTIVE_OPTION()) {
            found = true;
            expandEnvironmentVariables(text,MAX_OPTION_LEN,&len,DEFAULT_VALUE);
            ADD_OPTION_TO_ARGS(memFl,text,len,argv,argc);
        }
    }
    if (argc >= MAX_STD_ARGS-1) errorMessage(ERR_ST,"too many options");
    if (found && memFl!=MEM_NO_ALLOC) {
        OPTION_SPACE_ALLOCC(memFl, aargv, argc, char*);
        for(i=1; i<argc; i++) aargv[i] = argv[i];
    }
    *nargc = argc;
    *nargv = aargv;

    return found;
}

void readOptionFile(char *name, int *nargc, char ***nargv, char *sectionFile, char *project) {
    FILE *ff;
    char realSection[MAX_FILE_NAME_SIZE];
    ff = openFile(name,"r");
    if (ff==NULL) fatalError(ERR_CANT_OPEN,name, XREF_EXIT_ERR);
    readOptionFromFile(ff,nargc,nargv,MEM_ALLOC_ON_PP,sectionFile, project, realSection);
    closeFile(ff);
}

void readOptionPipe(char *comm, int *nargc, char ***nargv, char *sectionFile) {
    FILE *ff;
    char realSection[MAX_FILE_NAME_SIZE];
    ff = popen(comm, "r");
    if (ff==NULL) fatalError(ERR_CANT_OPEN, comm, XREF_EXIT_ERR);
    readOptionFromFile(ff,nargc,nargv,MEM_ALLOC_ON_PP,sectionFile,NULL,realSection);
    closeFile(ff);
}

static char *getClassPath(bool defaultCpAllowed) {
    char *cp;
    cp = options.classpath;
    if (cp == NULL || *cp==0) cp = getenv("CLASSPATH");
    if (cp == NULL || *cp==0) {
        if (defaultCpAllowed) cp = defaultClassPath;
        else cp = NULL;
    }
    return(cp);
}

void javaSetSourcePath(int defaultCpAllowed) {
    char *cp;
    cp = options.sourcePath;
    if (cp == NULL || *cp==0) cp = getenv("SOURCEPATH");
    if (cp == NULL || *cp==0) cp = getClassPath(defaultCpAllowed);
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
            PP_ALLOCC(nn, strlen(np)+1, char);
            strcpy(nn,np);
            PP_ALLOC(*ll, StringList);
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
    char path[MAX_FILE_NAME_SIZE];
    char packagePath[MAX_FILE_NAME_SIZE];
    int topCallFlag;
    void *recurseFlag;
    bool packageFound = false;

    assert(strlen(packageName)<MAX_FILE_NAME_SIZE-1);
    convertPackageNameToPath(packageName, packagePath);

    cp = javaSourcePaths;
    while (cp!=NULL && *cp!=0) {
        int len = copyPathSegment(cp, path);

        pathConcat(path, packagePath);
        assert(strlen(path)<MAX_FILE_NAME_SIZE-1);

        if (dirExists(path)) {
            // it is a package name, process all source files
            //&fprintf(dumpOut,"it is a package\n");
            packageFound = true;
            topCallFlag = 1;
            if (options.recurseDirectories)
                recurseFlag = &topCallFlag;
            else
                recurseFlag = NULL;
            dirInputFile(path, "", NULL, NULL, recurseFlag, &topCallFlag);
        }
        cp += len;
        if (*cp == CLASS_PATH_SEPARATOR) cp++;
    }
    return packageFound;
}

void addSourcePathsCut(void) {
    javaSetSourcePath(1);
    JavaMapOnPaths(javaSourcePaths, {
            addHtmlCutPath(currentPath);
        });
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
            if (dirExists(filename)) {
                /* This is a JRE or JDK < Java 9 */
                sprintf(filename+len-3, "jre%clib%crt.jar", FILE_PATH_SEPARATOR, FILE_PATH_SEPARATOR);
                assert(strlen(filename)<MAX_FILE_NAME_SIZE-1);
                if (fileExists(filename))
                    return filename;
            } else {
                sprintf(filename+len-3, "jrt-fs.jar");
                if (fileExists(filename))
                    /* In Java 9 the rt.jar was removed and replaced by "implementation dependent" files in lib... */
                    log_warning("Found Java > 8 which don't have a jar that we can read...");
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

    path = getenv("JAVA_HOME");
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
    path = getenv("PATH");
    if (path != NULL) {
        JavaMapOnPaths(path, {
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
    if (jdkcp == NULL || *jdkcp==0) jdkcp = getenv("JDKCLASSPATH");
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
    // optimize wild char expand and getenv [5.2.2003]
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

        createOptionString(&options.classpath, cp);  //??? why is this, only optimisation of getenv?
        processClassPathString(cp);
        jdkcp = getJdkClassPathQuickly();
        if (jdkcp != NULL && *jdkcp!=0) {
            createOptionString(&options.jdkClassPath, jdkcp);  //only optimisation of getenv?
            processClassPathString( jdkcp);
        }

        if (LANGUAGE(LANG_JAVA) && options.taskRegime != RegimeEditServer) {
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
                        fprintf(dumpOut,"java runtime == %s\n", jdkcp);
                    }
                    fprintf(dumpOut,"classpath == %s\n", cp);
                    fprintf(dumpOut,"sourcepath == %s\n", javaSourcePaths);
                    fflush(dumpOut);
                }
                messageFlag = true;
            }
        }
    }
}

bool checkReferenceFileCountOption(int newReferenceFileCount) {
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

void getXrefrcFileName(char *filename) {
    int hlen;
    char *home;

    if (options.xrefrc!=NULL) {
        sprintf(filename, "%s", normalizeFileName(options.xrefrc, cwd));
        return;
    }
    home = getenv("HOME");
#ifdef __WIN32__
    if (home == NULL) home = "c:\\";
#else
    if (home == NULL) home = "";
#endif
    hlen = strlen(home);
    if (hlen>0 && (home[hlen-1]=='/' || home[hlen-1]=='\\')) {
        sprintf(filename, "%s%cc-xrefrc", home, FILE_BEGIN_DOT);
    } else {
        sprintf(filename, "%s%c%cc-xrefrc", home, FILE_PATH_SEPARATOR, FILE_BEGIN_DOT);
    }
    assert(strlen(filename) < MAX_FILE_NAME_SIZE-1);
}
