#include "main.h"

#include "commons.h"
#include "globals.h"
#include "generate.h"
#include "refactory.h"
#include "extract.h"
#include "misc.h"
#include "complete.h"
#include "html.h"
#include "options.h"
#include "init.h"
#include "jslsemact.h"
#include "editor.h"
#include "enumTxt.h"
#include "reftab.h"
#include "javafqttab.h"
#include "list.h"
#include "jsemact.h"
#include "fileio.h"

#include "c_parser.x"
#include "yacc_parser.x"
#include "java_parser.x"
#include "parsers.h"

#include "characterreader.h"
#include "cxref.h"
#include "cxfile.h"
#include "caching.h"
#include "recyacc.h"
#include "yylex.h"
#include "lexer.h"
#include "log.h"
#include "utils.h"
#include "filedescriptor.h"

#include "protocol.h"

static char oldStdopFile[MAX_FILE_NAME_SIZE];
static char oldStdopSection[MAX_FILE_NAME_SIZE];
static char oldOnLineClassPath[MAX_OPTION_LEN];
static time_t oldStdopTime;
static int oldLanguage;
static int oldCppPass;
static S_options s_tmpOptions;

static void usage(char *s) {
    fprintf(stdout,"usage: \t\t%s <option>+ ",s);
    fprintf(stdout,"<input files>");
    fprintf(stdout,"\n");
    fprintf(stdout,"options:\n");
    fprintf(stdout,"\t-r                        - recursively descent directories\n");
    fprintf(stdout,"\t-html                     - convert sources to html format\n");
    fprintf(stdout,"\t-htmlroot=<dir>           - specifies root dir. for html output\n");
    fprintf(stdout,"\t-htmltab=<n>              - set tabulator to <n> in htmls\n");
    fprintf(stdout,"\t-htmlgxlist               - generate xref-lists for global symbols\n");
    fprintf(stdout,"\t-htmllxlist               - generate xref-lists for local symbols\n");
    fprintf(stdout,"\t-htmldirectx              - link first character to cross references\n");
    fprintf(stdout,"\t-htmlfunseparate          - separate functions by a bar\n");
    fprintf(stdout,"\t-htmlzip=<command>        - zip command, ! stands for the file name\n");
    fprintf(stdout,"\t-htmllinksuffix=<suf>     - add this suffix to file links\n");
    fprintf(stdout,"\t-htmllinenums             - generate line numbers in HTML\n");
    fprintf(stdout,"\t-htmlnocolors             - do not generate colors in HTML\n");
    fprintf(stdout,"\t-htmlnounderline          - not underlined HTML links\n");
    fprintf(stdout,"\t-htmlcutpath=<path>       - cut path in generated html\n");
    fprintf(stdout,"\t-htmlcutcwd               - cut current dir in html\n");
    fprintf(stdout,"\t-htmlcutsourcepaths       - cut source paths in html\n");
    fprintf(stdout,"\t-htmlcutsuffix            - cut language suffix in html\n");
    fprintf(stdout,"\t-htmllinkcolor=<color>    - color of HTML links\n");
    fprintf(stdout,"\t-htmllinenumcolor=<color> - color of line numbers in HTML links\n");
    fprintf(stdout,"\t-htmlgenjavadoclinks      - gen links to javadoc if no definition\n");
    fprintf(stdout,"\t                            only for packages from -htmljavadocavailable\n");
    fprintf(stdout,"\t-javadocurl=<http>        - url to existing java docs\n");
    fprintf(stdout,"\t-javadocpath=<path>       - paths to existing java docs\n");
    fprintf(stdout,"\t-javadocavailable=<packs> - packages for which javadoc is available\n");
    fprintf(stdout,"\t-p <prj>                  - read options from <prj> section\n");
    fprintf(stdout,"\t-I <dir>                  - search for includes in <dir>\n");
    fprintf(stdout,"\t-D<mac>[=<body>]          - define macro <mac> with body <body>\n");
    fprintf(stdout,"\t-packages                 - allow packages as input files\n");
    fprintf(stdout,"\t-sourcepath <path>        - set java sources paths\n");
    fprintf(stdout,"\t-classpath <path>         - set java class path\n");
    fprintf(stdout,"\t-filescasesensitive       - file names are case sensitive\n");
    fprintf(stdout,"\t-filescaseunsensitive     - file names are case unsensitive\n");
    fprintf(stdout,"\t-csuffixes=<paths>        - list of C files suffixes separated by : (or ;)\n");
    fprintf(stdout,"\t-javasuffixes=<paths>     - list of Java files suffixes separated by : (or ;)\n");
    fprintf(stdout,"\t-stdop <file>             - read options from <file>\n");
    fprintf(stdout,"\t-no_cpp_comment           - C++ like comments // not admitted\n");
#if 0
    fprintf(stdout,"\t-olinelen=<n>             - length of lines for on-line output\n");
    fprintf(stdout,"\t-oocheckbits=<n>          - object-oriented resolution for completions\n");
    fprintf(stdout,"\t-olcxsearch               - search info about identifier\n");
    fprintf(stdout,"\t-olcxpush                 - generate and push on-line cxrefs \n");
    fprintf(stdout,"\t-olcxrename               - generate and push xrfs for rename\n");
    fprintf(stdout,"\t-olcxlist                 - generate, push and list on-line cxrefs \n");
    fprintf(stdout,"\t-olcxpop                  - pop on-line cxrefs\n");
    fprintf(stdout,"\t-olcxplus                 - next on-line reference\n");
    fprintf(stdout,"\t-olcxminus                - previous on-line reference\n");
    fprintf(stdout,"\t-olcxgoto<n>              - go to the n-th on-line reference\n");
    fprintf(stdout,"\t-user                     - user logname for olcx\n");
    fprintf(stdout,"\t-o <file>                 - log output to <file>\n");
    fprintf(stdout,"\t-file <file>              - name of the file given to stdin\n");
#endif
    fprintf(stdout,"\t-refs <file>              - name of file with cxrefs\n");
    fprintf(stdout,"\t-refnum=<n>               - number of cxref files\n");
    fprintf(stdout,"\t-refalphahash             - split references alphabetically (-refnum=28)\n");
    fprintf(stdout,"\t-refalpha2hash            - split references alphabetically (-refnum=28*28)\n");
    fprintf(stdout,"\t-exactpositionresolve     - resolve symbols by def. position\n");
    fprintf(stdout,"\t-mf<n>                    - factor increasing cxMemory\n");
#   ifdef DEBUG
    fprintf(stdout,"\t-debug                    - produce debug output of the execution\n");
    fprintf(stdout,"\t-trace                    - produce trace output of the execution\n");
#   endif
#if 0
    fprintf(stdout,"\t-enum_name                - generate enum text names\n");
    fprintf(stdout,"\t-header                   - output is headers file\n");
    fprintf(stdout,"\t-body                     - output is code file\n");
#endif
    fprintf(stdout,"\t-no_enum                  - don't cross reference enumeration constants\n");
    fprintf(stdout,"\t-no_mac                   - don't cross reference macros\n");
    fprintf(stdout,"\t-no_type                  - don't cross reference type names\n");
    fprintf(stdout,"\t-no_str                   - don't cross reference str. records\n");
    fprintf(stdout,"\t-no_local                 - don't cross reference local vars\n");
    fprintf(stdout,"\t-nobrief                  - generate cxrefs in long format\n");
    fprintf(stdout,"\t-update                   - update old 'refs' reference file\n");
    fprintf(stdout,"\t-compiler                 - path to compiler to use for autodiscovered includes and defines\n");
    fprintf(stdout,"\t-fastupdate               - fast update (modified files only)\n");
    //& fprintf(stdout,"\t-keep_old             - keep also all old references from 'refs' file\n");
    fprintf(stdout,"\t-no_stdop                 - don't read the '~/.c-xrefrc' option file \n");
    fprintf(stdout,"\t-errors                   - report all error messages\n");
    fprintf(stdout,"\t-version                  - print version information\n");
}

static void aboutMessage(void) {
    char output[REFACTORING_TMP_STRING_SIZE];
    sprintf(output,"This is C-xrefactory version %s (%s).\n", C_XREF_VERSION_NUMBER, __DATE__);
    sprintf(output+strlen(output),"(c) 1997-2004 by Xref-Tech, http://www.xref-tech.com\n");
    sprintf(output+strlen(output),"Released into GPL 2009 by Marian Vittek (SourceForge)\n");
    sprintf(output+strlen(output),"Work resurrected and continued by Thomas Nilefalk 2015-\n");
    sprintf(output+strlen(output),"(http://github.com/thoni56/c-xrefactory)\n");
    if (s_opt.exit) {
        sprintf(output+strlen(output),"Exiting!");
    }
    if (s_opt.xref2) {
        ppcGenRecord(PPC_INFORMATION, output, "\n");
    } else {
        fprintf(stdout, "%s", output);
    }
    if (s_opt.exit) exit(XREF_EXIT_BASE);
}

#define NEXT_FILE_ARG() {                                               \
    char tmpBuff[TMP_BUFF_SIZE];                                        \
    i++;                                                                \
    if (i >= argc) {                                                    \
        sprintf(tmpBuff,"file name expected after %s",argv[i-1]);     \
        errorMessage(ERR_ST,tmpBuff);                                   \
        usage(argv[0]);                                                 \
        exit(1);                                                        \
    }                                                                   \
}

#define NEXT_ARG() {                                                    \
    char tmpBuff[TMP_BUFF_SIZE];                                        \
    i++;                                                                \
    if (i >= argc) {                                                    \
        sprintf(tmpBuff,"further argument(s) expected after %s",argv[i-1]); \
        errorMessage(ERR_ST,tmpBuff);                                   \
        usage(argv[0]);                                                 \
        exit(1);                                                        \
    }                                                                   \
}

static int fileNameShouldBePruned(char *fn) {
    S_stringList    *pp;
    for(pp=s_opt.pruneNames; pp!=NULL; pp=pp->next) {
        JavaMapOnPaths(pp->d, {
                if (compareFileNames(currentPath, fn)==0) return(1);
            });
    }
    return(0);
}

static void scheduleCommandLineEnteredFileToProcess(char *fn) {
    int fileIndex;

    ENTER();
    fileIndex = addFileTabItem(fn);
    if (s_opt.taskRegime!=RegimeEditServer) {
        // yes in edit server you process also headers, etc.
        s_fileTab.tab[fileIndex]->b.commandLineEntered = true;
    }
    log_trace("recursively process command line argument file #%d '%s'", fileIndex, s_fileTab.tab[fileIndex]->name);
    if (!s_opt.updateOnlyModifiedFiles) {
        s_fileTab.tab[fileIndex]->b.scheduledToProcess = true;
    }
    LEAVE();
}

void dirInputFile(MAP_FUN_PROFILE) {
    char            *dir,*fname, *suff;
    void            *recurseFlag;
    void            *nrecurseFlag;
    struct stat     st;
    char            fn[MAX_FILE_NAME_SIZE];
    char            wcPaths[MAX_OPTION_LEN];
    int             topCallFlag, stt;

    dir = a1; fname = file; recurseFlag = a4; topCallFlag = *a5;
    if (topCallFlag == 0) {
        if (strcmp(fname,".")==0) return;
        if (strcmp(fname,"..")==0) return;
        if (fileNameShouldBePruned(fname)) return;
        sprintf(fn,"%s%c%s",dir,FILE_PATH_SEPARATOR,fname);
        strcpy(fn, normalizeFileName(fn, s_cwd));
        if (fileNameShouldBePruned(fn)) return;
    } else {
        strcpy(fn, normalizeFileName(fname, s_cwd));
    }
    if (strlen(fn) >= MAX_FILE_NAME_SIZE) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "file name %s is too long", fn);
        fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
    }
    suff = getFileSuffix(fname);
    stt = statb(fn, &st);
    if (stt==0  && (st.st_mode & S_IFMT)==S_IFDIR) {
        if (recurseFlag!=NULL) {
            topCallFlag = 0;
            if (s_opt.recursivelyDirs) nrecurseFlag = &topCallFlag;
            else nrecurseFlag = NULL;
            mapDirectoryFiles(fn, dirInputFile,DO_NOT_ALLOW_EDITOR_FILES,
                              fn, NULL, NULL, nrecurseFlag, &topCallFlag);
            editorMapOnNonexistantFiles(fn, dirInputFile, DEPTH_ANY,
                                        fn, NULL, NULL, nrecurseFlag, &topCallFlag);
        } else {
            // no error, let it be
            //& sprintf(tmpBuff,"omitting directory %s, missing '-r' option ?",fn);
            //& warningMessage(ERR_ST,tmpBuff);
        }
    } else if (stt==0) {
        // .class can be inside a jar archive, but this makes problem on
        // recursive read of a directory, it attempts to read .class
        if (    topCallFlag==0
                &&  (! fileNameHasOneOfSuffixes(fname, s_opt.cFilesSuffixes))
                &&  (! fileNameHasOneOfSuffixes(fname, s_opt.javaFilesSuffixes))
#ifdef CCC_ALLOWED
                &&  (! fileNameHasOneOfSuffixes(fname, s_opt.cppFilesSuffixes))
#endif
                &&  compareFileNames(suff,".y")!=0
                ) {
            return;
        }
        if (s_opt.javaFilesOnly && s_opt.taskRegime != RegimeEditServer
            && (! fileNameHasOneOfSuffixes(fname, s_opt.javaFilesSuffixes))
            && (! fileNameHasOneOfSuffixes(fname, "jar:class"))
            ) {
            return;
        }
        scheduleCommandLineEnteredFileToProcess(fn);
    } else if (containsWildcard(fn)) {
        expandWildcardsInOnePath(fn, wcPaths, MAX_OPTION_LEN);
        //&fprintf(dumpOut, "wildcard path %s expanded to %s\n", fn, wcPaths);
        JavaMapOnPaths(wcPaths,{
                dirInputFile(currentPath,"",NULL,NULL,recurseFlag,&topCallFlag);
            });
    } else if (topCallFlag
               && ((!s_opt.allowPackagesOnCl)
                   || packageOnCommandLine(fname)==0)) {
        if (s_opt.taskRegime!=RegimeEditServer) {
            errorMessage(ERR_CANT_OPEN, fn);
        } else {
            // hacked 16.4.2003 in order to can complete in buffers
            // without existing file
            scheduleCommandLineEnteredFileToProcess(fn);
        }
    }
}

#if defined(__WIN32__)
static int isAbsolutePath(char *p) {
    if (p[0]!=0 && p[1]==':' && p[2]==FILE_PATH_SEPARATOR) return(1);
    if (p[0]==FILE_PATH_SEPARATOR) return(1);
    return(0);
}
#else
static int isAbsolutePath(char *p) {
    return(p[0]==FILE_PATH_SEPARATOR);
}
#endif


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
    for(ll=s_opt.allAllocatedStrings; ll!=NULL; ll=ll->next) {
        // reassignement, do not keep two copies
        if (ll->destination == destination) break;
    }
    if (ll==NULL) {
        ll = newStringPointerList(destination, s_opt.allAllocatedStrings);
        s_opt.allAllocatedStrings = ll;
    }
}

static void allocOptionSpace(void **optAddress, int size) {
    char **res;
    res = (char**)optAddress;
    OPT_ALLOCC((*res), size, char);
    optionAddToAllocatedList(res);
}

void createOptionString(char **optAddress, char *text) {
    int alen = strlen(text)+1;
    allocOptionSpace((void**)optAddress, alen);
    strcpy(*optAddress, text);
}

static void copyOptionShiftPointer(char **lld, S_options *dest, S_options *src) {
    char    **dlld;
    int     offset, localOffset;
    offset = ((char*)dest) - ((char*)src);
    localOffset = ((char*)lld) - ((char*)src);
    dlld = ((char**) (((char*)dest) + localOffset));
    // dlld is dest equivalent of *lld from src
    //&fprintf(dumpOut,"shifting (%x->%x) [%x]==%x ([%x]==%x), offsets == %d, %d, size==%d\n", src, dest, lld, *lld, dlld, *dlld, offset, localOffset, sizeof(S_options));
    if (*dlld != *lld) {
        fprintf(dumpOut,"problem %s\n", *lld);
    }
    assert(*dlld == *lld);
    *dlld = *lld + offset;
}

void copyOptions(S_options *dest, S_options *src) {
    StringPointerList    **ll;
    memcpy(dest, src, sizeof(S_options));
    for(ll= &src->allAllocatedStrings; *ll!=NULL; ll = &(*ll)->next) {
        copyOptionShiftPointer((*ll)->destination, dest, src);
        copyOptionShiftPointer(((char**)&(*ll)->destination), dest, src);
        copyOptionShiftPointer(((char**)ll), dest, src);
    }
    //&fprintf(dumpOut,"options copied\n");
}

void xrefSetenv(char *name, char *val) {
    S_setGetEnv *sge;
    int j, n;

    sge = &s_opt.setGetEnv;
    n = sge->num;
    if (n+1>=MAX_SET_GET_OPTIONS) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "maximum of %d -set options reached", MAX_SET_GET_OPTIONS);
        errorMessage(ERR_ST, tmpBuff);
        sge->num--; n--;
    }
    for(j=0; j<n; j++) {
        assert(sge->name[j]);
        if (strcmp(sge->name[j], name)==0) break;
    }
    if (j==n) createOptionString(&(sge->name[j]), name);
    if (j==n || strcmp(sge->value[j], val)!=0) {
        createOptionString(&(sge->value[j]), val);
    }
    //&fprintf(dumpOut,"setting '%s' to '%s'\n", name, val);
    if (j==n) sge->num ++;
}


int mainHandleSetOption( int argc, char **argv, int i ) {
    char *name, *val;

    NEXT_ARG();
    name = argv[i];
    assert(name);
    NEXT_ARG();
    val = argv[i];
    xrefSetenv(name, val);
    return(i);
}

static int mainHandleIncludeOption(int argc, char **argv, int i) {
    int nargc;
    char **nargv;
    NEXT_FILE_ARG();
    s_opt.stdopFlag = 1;
    readOptionFile(argv[i],&nargc,&nargv,"",NULL);
    processOptions(nargc, nargv, INFILES_DISABLED);
    s_opt.stdopFlag = 0;
    return(i);
}

int addHtmlCutPath(char *ss) {
    int i,ln,len, res;

    res = 0;
    ss = htmlNormalizedPath(ss);
    ln = strlen(ss);
    if (ln>=1 && ss[ln-1] == FILE_PATH_SEPARATOR) {
        warningMessage(ERR_ST,"slash at the end of -htmlcutpath path, ignoring it");
        return(res);
    }
    for(i=0; i<s_opt.htmlCut.pathsNum; i++) {
        // if yet in cutpaths, do nothing
        if (strcmp(s_opt.htmlCut.path[i], ss)==0) return(res);
    }
    createOptionString(&(s_opt.htmlCut.path[s_opt.htmlCut.pathsNum]), ss);
    ss = s_opt.htmlCut.path[s_opt.htmlCut.pathsNum];
    s_opt.htmlCut.plen[s_opt.htmlCut.pathsNum] = ln;
    //&fprintf(dumpOut,"adding cutpath %d %s\n",s_opt.htmlCut.pathsNum,ss);
    for(i=0; i<s_opt.htmlCut.pathsNum; i++) {
        // a more specialized path after a more general, exchange them
        len = s_opt.htmlCut.plen[i];
        if (fnnCmp(s_opt.htmlCut.path[i], ss, len)==0) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,
                    "htmlcutpath: '%s' supersede \n\t\t'%s', exchanging them",
                    s_opt.htmlCut.path[i], ss);
            warningMessage(ERR_ST, tmpBuff);
            res = 1;
            s_opt.htmlCut.path[s_opt.htmlCut.pathsNum]=s_opt.htmlCut.path[i];
            s_opt.htmlCut.plen[s_opt.htmlCut.pathsNum]=s_opt.htmlCut.plen[i];
            s_opt.htmlCut.path[i] = ss;
            s_opt.htmlCut.plen[i] = ln;
        }
    }
    if (s_opt.htmlCut.pathsNum+2 >= MAX_HTML_CUT_PATHES) {
        errorMessage(ERR_ST,"# of htmlcutpaths overflow over MAX_HTML_CUT_PATHES");
    } else {
        s_opt.htmlCut.pathsNum++;
    }
    return(res);
}

/* *************************************************************************** */
/*                                      OPTIONS                                */

static int processNegativeOption(int *ii, int argc, char **argv, int infilesFlag) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"--r")==0) {
        if (infilesFlag == INFILES_ENABLED) s_opt.recursivelyDirs = false;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processAOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strncmp(argv[i],"-addimportdefault=",18)==0) {
        sscanf(argv[i]+18, "%d", &s_opt.defaultAddImportStrategy);
    }
    else if (strcmp(argv[i],"-version")==0 || strcmp(argv[i],"-about")==0){
        s_opt.server_operation = OLO_ABOUT;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processBOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-brief")==0)           s_opt.brief_cxref = true;
    else if (strcmp(argv[i],"-briefoutput")==0)     s_opt.briefoutput = true;
    else if (strcmp(argv[i],"-body")==0)            s_opt.generate_body = 1;
    else if (strncmp(argv[i],"-browsedsym=",12)==0)     {
        createOptionString(&s_opt.browsedSymName, argv[i]+12);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processCOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-c_struct_scope")==0)  s_opt.c_struct_scope=1;
    else if (strcmp(argv[i],"-cacheincludes")==0)   s_opt.cacheIncludes=1;
    else if (strcmp(argv[i],"-crlfconversion")==0)  s_opt.eolConversion|=CR_LF_EOL_CONVERSION;
    else if (strcmp(argv[i],"-crconversion")==0)    s_opt.eolConversion|=CR_EOL_CONVERSION;
    else if (strcmp(argv[i],"-completioncasesensitive")==0) s_opt.completionCaseSensitive=1;
    else if (strcmp(argv[i],"-completeparenthesis")==0) s_opt.completeParenthesis=1;
    else if (strncmp(argv[i],"-completionoverloadwizdeep=",27)==0)  {
        sscanf(argv[i]+27, "%d", &s_opt.completionOverloadWizardDeep);
    }
    else if (strcmp(argv[i],"-continuerefactoring")==0)
        s_opt.continueRefactoring=RC_CONTINUE;
    else if (strncmp(argv[i],"-commentmovinglevel=",20)==0) {
        sscanf(argv[i]+20, "%d", &s_opt.commentMovingLevel);
    }
    else if (strcmp(argv[i],"-continuerefactoring=importSingle")==0)    {
        s_opt.continueRefactoring = RC_IMPORT_SINGLE_TYPE;
    }
    else if (strcmp(argv[i],"-continuerefactoring=importOnDemand")==0)  {
        s_opt.continueRefactoring = RC_IMPORT_ON_DEMAND;
    }
    else if (strcmp(argv[i],"-classpath")==0) {
        NEXT_FILE_ARG();
            createOptionString(&s_opt.classpath, argv[i]);
    }
    else if (strncmp(argv[i],"-csuffixes=",11)==0) {
        createOptionString(&s_opt.cFilesSuffixes, argv[i]+11);
    }
    else if (strcmp(argv[i],"-create")==0) s_opt.create = 1;
    else if (strncmp(argv[i],"-compiler=", 10)==0) {
        s_opt.compiler = &argv[i][10];
    } else return(0);
    *ii = i;
    return(1);
}

static int processDOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
#   ifdef DEBUG
    else if (strcmp(argv[i], "-debug")==0)
        s_opt.debug = true;
#   endif
    else if (strcmp(argv[i], "-d")==0)   {
        int ln;
        NEXT_FILE_ARG();
        ln=strlen(argv[i]);
        if (ln>1 && argv[i][ln-1] == FILE_PATH_SEPARATOR) {
            warningMessage(ERR_ST,"slash at the end of -d path");
        }
        if (! isAbsolutePath(argv[i])) {
            warningMessage(ERR_ST,"'-d' option should be followed by an ABSOLUTE path");
        }
        createOptionString(&s_opt.htmlRoot, argv[i]);
    }
    // TODO, do this macro allocation differently!!!!!!!!!!!!!
    // just store macros in options and later add them into pp_memory
    else if (strncmp(argv[i],"-D",2)==0)
        addMacroDefinedByOption(argv[i]+2);
    else if (strcmp(argv[i],"-displaynestedwithouters")==0) {
        s_opt.nestedClassDisplaying = NO_OUTERS_CUT;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processEOption(int *ii, int argc, char **argv) {
    int i = * ii;
    char ttt[TMP_STRING_SIZE];

    if (0) {}
    else if (strcmp(argv[i],"-errors")==0) {
        s_opt.show_errors = true;
    } else if (strcmp(argv[i],"-exit")==0) {
        log_debug("Exiting");
        exit(XREF_EXIT_BASE);
    }
    else if (strcmp(argv[i],"-enum_name")==0) s_opt.generate_enum_name = 1;
    else if (strcmp(argv[i],"-editor=emacs")==0) {
        s_opt.editor = EDITOR_EMACS;
    }
    else if (strcmp(argv[i],"-editor=jedit")==0) {
        s_opt.editor = EDITOR_JEDIT;
    }
    else if (strncmp(argv[i],"-extractAddrParPrefix=",22)==0) {
        sprintf(ttt, "*%s", argv[i]+22);
        createOptionString(&s_opt.olExtractAddrParPrefix, ttt);
    }
    else if (strcmp(argv[i],"-exactpositionresolve")==0) {
        s_opt.exactPositionResolve = 1;
    }
    else if (strncmp(argv[i],"-encoding=", 10)==0) {
        if (s_opt.fileEncoding == MULE_DEFAULT) {
            if (strcmp(argv[i],"-encoding=default")==0) {
                s_opt.fileEncoding = MULE_DEFAULT;
            } else if (strcmp(argv[i],"-encoding=european")==0) {
                s_opt.fileEncoding = MULE_EUROPEAN;
            } else if (strcmp(argv[i],"-encoding=euc")==0) {
                s_opt.fileEncoding = MULE_EUC;
            } else if (strcmp(argv[i],"-encoding=sjis")==0) {
                s_opt.fileEncoding = MULE_SJIS;
            } else if (strcmp(argv[i],"-encoding=utf")==0) {
                s_opt.fileEncoding = MULE_UTF;
            } else if (strcmp(argv[i],"-encoding=utf-8")==0) {
                s_opt.fileEncoding = MULE_UTF_8;
            } else if (strcmp(argv[i],"-encoding=utf-16")==0) {
                s_opt.fileEncoding = MULE_UTF_16;
            } else if (strcmp(argv[i],"-encoding=utf-16le")==0) {
                s_opt.fileEncoding = MULE_UTF_16LE;
            } else if (strcmp(argv[i],"-encoding=utf-16be")==0) {
                s_opt.fileEncoding = MULE_UTF_16BE;
            } else {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff,"unsupported encoding, available values are 'default', 'european', 'euc', 'sjis', 'utf', 'utf-8', 'utf-16', 'utf-16le' and 'utf-16be'.");
                formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
                errorMessage(ERR_ST, tmpBuff);
            }
        }
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processFOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-filescasesensitive")==0) {
        s_opt.fileNamesCaseSensitive = 1;
    }
    else if (strcmp(argv[i],"-filescaseunsensitive")==0) {
        s_opt.fileNamesCaseSensitive = 0;
    }
    else if (strcmp(argv[i],"-fastupdate")==0)  {
        s_opt.update = UP_FAST_UPDATE;
        s_opt.updateOnlyModifiedFiles = true;
    }
    else if (strcmp(argv[i],"-fupdate")==0) {
        s_opt.update = UP_FULL_UPDATE;
        s_opt.updateOnlyModifiedFiles = false;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processGOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-getlastimportline")==0) {
        s_opt.trivialPreCheckCode = TPC_GET_LAST_IMPORT_LINE;
    }
    else if (strcmp(argv[i],"-get")==0) {
        NEXT_ARG();
        createOptionString(&s_opt.getValue, argv[i]);
        s_opt.server_operation = OLO_GET_ENV_VALUE;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processHOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (strncmp(argv[i],"-htmltab=",9)==0)  {
        sscanf(argv[i]+9,"%d",&s_opt.tabulator);
    }
    else if (strcmp(argv[i],"-htmllinenums")==0)    s_opt.htmlLineNums = 1;
    else if (strcmp(argv[i],"-htmlnocolors")==0)    s_opt.htmlNoColors = true;
    else if (strcmp(argv[i],"-htmlgxlist")==0)  s_opt.htmlglobalx = true;
    else if (strcmp(argv[i],"-htmllxlist")==0)  s_opt.htmllocalx = true;
    else if (strcmp(argv[i],"-htmlrichlist")==0)    {
        warningMessage(ERR_ST,"-htmlrichlist option is no longer supported");
        //&         s_opt.htmlRichLists= 1;
    }
    else if (strcmp(argv[i],"-htmlfunseparate")==0)s_opt.htmlFunSeparate=true;
    else if (strcmp(argv[i],"-html")==0) {
        s_opt.taskRegime = RegimeHtmlGenerate;
        s_opt.fileEncoding = MULE_EUROPEAN; // no multibyte encodings
        s_opt.multiHeadRefsCare = 0;    // JUST TEMPORARY !!!!!!!
    }
    else if (strncmp(argv[i],"-htmlroot=",10)==0)   {
        int ln;
        ln=strlen(argv[i]);
        if (ln>11 && argv[i][ln-1] == FILE_PATH_SEPARATOR) {
            warningMessage(ERR_ST,"slash at the end of -htmlroot path");
        }
        createOptionString(&s_opt.htmlRoot, argv[i]+10);
    }
    else if (strcmp(argv[i],"-htmlgenjavadoclinks")==0) {
        s_opt.htmlGenJdkDocLinks = 1;
    }
    else if (strncmp(argv[i],"-htmljavadocavailable=",22)==0) {
        createOptionString(&s_opt.htmlJdkDocAvailable, argv[i]+22);
    }
    else if (strncmp(argv[i],"-htmljavadocpath=",17)==0)    {
        createOptionString(&s_opt.htmlJdkDocUrl, argv[i]+17);
    }
    else if (strncmp(argv[i],"-htmlcutpath=",13)==0)    {
        addHtmlCutPath(argv[i]+13);
    }
    else if (strcmp(argv[i],"-htmlcutcwd")==0)  {
        int tmp;
        tmp = addHtmlCutPath(s_cwd);
        if (s_opt.java2html && tmp) {
            fatalError(ERR_ST, "a path clash occurred, please, run java2html from different directory", XREF_EXIT_ERR);
        }
    }
    else if (strcmp(argv[i],"-htmlcutsourcepaths")==0)  {
        addSourcePathsCut();
    }
    else if (strcmp(argv[i],"-htmlcutsuffix")==0)   {
        s_opt.htmlCutSuffix = 1;
    }
    else if (strncmp(argv[i],"-htmllinenumlabel=", 18)==0)  {
        createOptionString(&s_opt.htmlLineNumLabel, argv[i]+18);
    }
    else if (strcmp(argv[i],"-htmlnounderline")==0) {
        s_opt.htmlNoUnderline = true;
    }
    else if (strcmp(argv[i],"-htmldirectx")==0) {
        s_opt.htmlDirectX = true;
    }
    else if (strncmp(argv[i],"-htmllinkcolor=",15)==0)  {
        createOptionString(&s_opt.htmlLinkColor, argv[i]+15);
    }
    else if (strncmp(argv[i],"-htmllinenumcolor=",18)==0)   {
        createOptionString(&s_opt.htmlLineNumColor, argv[i]+18);
    }
    else if (strncmp(argv[i],"-htmlcxlinelen=",15)==0)  {
        sscanf(argv[i]+15, "%d", &s_opt.htmlCxLineLen);
    }
    else if (strncmp(argv[i],"-htmlzip=",9)==0) {
        createOptionString(&s_opt.htmlZipCommand, argv[i]+9);
    }
    else if (strncmp(argv[i],"-htmllinksuffix=",16)==0) {
        createOptionString(&s_opt.htmlLinkSuffix, argv[i]+16);
    }
    else if (strcmp(argv[i],"-header")==0)      s_opt.generate_header = 1;
    else if (strcmp(argv[i],"-help")==0) {
        usage(argv[0]);
        exit(0);
    }
    else return(0);
    *ii = i;
    return(1);
}

static void mainAddStringListOption(S_stringList **optlist, char *argvi) {
    S_stringList **ll;
    for(ll=optlist; *ll!=NULL; ll= &(*ll)->next)
        ;

    /* TODO refactor out to newOptionString()? */
    allocOptionSpace((void**)ll, sizeof(S_stringList));
    createOptionString(&(*ll)->d, argvi);
    (*ll)->next = NULL;
}

static int processIOption(int *ii, int argc, char **argv) {
    int i = * ii;

    if (0) {}
    else if (strcmp(argv[i],"-I")==0) {
        /* include dir */
        i++;
        if (i >= argc) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"directory name expected after -I");
            errorMessage(ERR_ST,tmpBuff);
            usage(argv[0]);
        }
        mainAddStringListOption(&s_opt.includeDirs, argv[i]);
    }
    else if (strncmp(argv[i],"-I", 2)==0 && argv[i][2]!=0) {
        mainAddStringListOption(&s_opt.includeDirs, argv[i]+2);
    }
    else if (strcmp(argv[i],"-include")==0) {
        warningMessage(ERR_ST,"-include option is deprecated, use -optinclude instead");
        i = mainHandleIncludeOption(argc, argv, i);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processJOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-javadoc")==0)     s_opt.javaDoc = true;
    else if (strcmp(argv[i],"-java2html")==0)   s_opt.java2html = true;
    else if (strcmp(argv[i],"-java1.4")==0)     {
        createOptionString(&s_opt.javaVersion, JAVA_VERSION_1_4);
    }
    else if (strncmp(argv[i],"-jdoctmpdir=",12)==0) {
        int ln;
        ln=strlen(argv[i]);
        if (ln>13 && argv[i][ln-1] == FILE_PATH_SEPARATOR) {
            warningMessage(ERR_ST,"slash at the end of -jdoctmpdir path");
        }
        createOptionString(&s_opt.jdocTmpDir, argv[i]+12);
    }
    else if (strncmp(argv[i],"-javadocavailable=",18)==0)   {
        createOptionString(&s_opt.htmlJdkDocAvailable, argv[i]+18);
    }
    else if (strncmp(argv[i],"-javadocurl=",12)==0) {
        createOptionString(&s_opt.htmlJdkDocUrl, argv[i]+12);
    }
    else if (strncmp(argv[i],"-javadocpath=",13)==0)    {
        createOptionString(&s_opt.javaDocPath, argv[i]+13);
    } else if (strcmp(argv[i],"-javadocpath")==0)   {
        NEXT_FILE_ARG();
        createOptionString(&s_opt.javaDocPath, argv[i]);
    }
    else if (strncmp(argv[i],"-javasuffixes=",14)==0) {
        createOptionString(&s_opt.javaFilesSuffixes, argv[i]+14);
    }
    else if (strcmp(argv[i],"-javafilesonly")==0) {
        s_opt.javaFilesOnly = 1;
    }
    else if (strcmp(argv[i],"-jdkclasspath")==0 || strcmp(argv[i],"-javaruntime")==0) {
        NEXT_FILE_ARG();
        createOptionString(&s_opt.jdkClassPath, argv[i]);
    }
    else if (strcmp(argv[i],"-jeditc1")==0) {
        s_opt.jeditOldCompletions = 1;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processKOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-keep_old")==0)    s_opt.keep_old = 1;
    else return(0);
    *ii = i;
    return(1);
}

static int processLOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strncmp(argv[i], "-log=", 5)==0) {
        ;                       /* Already handled in initLogging() */
    }
    else if (strncmp(argv[i],"-last_message=",14)==0) {
        createOptionString(&s_opt.last_message, argv[i]+14);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processMOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strncmp(argv[i],"-mf",3)==0)   {
        int newmf;
        sscanf(argv[i]+3, "%d", &newmf);
        if (newmf<=0 || newmf>255) {
            fatalError(ERR_ST, "memory factor out of range <1,255>", XREF_EXIT_ERR);
        }
        s_opt.cxMemoryFactor = newmf;
    }
    else if (strncmp(argv[i],"-maxCompls=",11)==0 || strncmp(argv[i],"-maxcompls=",11)==0)  {
        sscanf(argv[i]+11, "%d", &s_opt.maxCompletions);
    }
    else if (strncmp(argv[i],"-movetargetclass=",17)==0) {
        createOptionString(&s_opt.moveTargetClass, argv[i]+17);
    }
    else if (strncmp(argv[i],"-movetargetfile=",16)==0) {
        createOptionString(&s_opt.moveTargetFile, argv[i]+16);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processNOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-noincluderefs")==0)       s_opt.noIncludeRefs = true;
    else if (strcmp(argv[i],"-noincluderefresh")==0)    s_opt.noIncludeRefs=true;
    else if (strcmp(argv[i],"-nocxfile")==0)            s_opt.noCxFile = 1;
    else if (strcmp(argv[i],"-no_cpp_comment")==0)      s_opt.cpp_comment = false;
    else if (strcmp(argv[i],"-nobrief")==0)             s_opt.brief_cxref = false;
    else if (strcmp(argv[i],"-no_enum")==0)             s_opt.no_ref_enumerator = true;
    else if (strcmp(argv[i],"-no_mac")==0)              s_opt.no_ref_macro = true;
    else if (strcmp(argv[i],"-no_type")==0)             s_opt.no_ref_typedef = true;
    else if (strcmp(argv[i],"-no_str")==0)              s_opt.no_ref_records = true;
    else if (strcmp(argv[i],"-no_local")==0)            s_opt.no_ref_locals = true;
    else if (strcmp(argv[i],"-no_cfrefs")==0)           s_opt.allowClassFileRefs = false;
    else if (strcmp(argv[i],"-no_stdop")==0
             || strcmp(argv[i],"-nostdop")==0)          s_opt.no_stdop = true;
    else if (strcmp(argv[i],"-noautoupdatefromsrc")==0) s_opt.javaSlAllowed = 0;
    else if (strcmp(argv[i],"-noerrors")==0)            s_opt.noErrors=1;
    else return(0);
    *ii = i;
    return(1);
}

static int processOOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strncmp(argv[i],"-oocheckbits=",13)==0)    {
        sscanf(argv[i]+13,"%o",&s_opt.ooChecksBits);
    }
    else if (strncmp(argv[i],"-olinelen=",10)==0) {
        sscanf(argv[i]+10,"%d",&s_opt.olineLen);
        if (s_opt.olineLen == 0) s_opt.olineLen =79;
        else s_opt.olineLen--;
    }
    else if (strncmp(argv[i],"-olcursor=",10)==0) {
        sscanf(argv[i]+10,"%d",&s_opt.olCursorPos);
    }
    else if (strncmp(argv[i],"-olmark=",8)==0) {
        sscanf(argv[i]+8,"%d",&s_opt.olMarkPos);
    }
    else if (strncmp(argv[i],"-olcheckversion=",16)==0) {
        createOptionString(&s_opt.checkVersion, argv[i]+16);
        s_opt.server_operation = OLO_CHECK_VERSION;
    }
    else if (strncmp(argv[i],"-olcxrefsuffix=",15)==0)  {
        createOptionString(&s_opt.olcxRefSuffix, argv[i]+15);
    }
    else if (strncmp(argv[i],"-olcxresetrefsuffix=",20)==0)     {
        s_opt.server_operation = OLO_RESET_REF_SUFFIX;
        createOptionString(&s_opt.olcxRefSuffix, argv[i]+20);
    }
    else if (strcmp(argv[i],"-olcxextract")==0) {
        s_opt.server_operation = OLO_EXTRACT;
    }
    else if (strcmp(argv[i],"-olcxtrivialprecheck")==0) {
        s_opt.server_operation = OLO_TRIVIAL_PRECHECK;
    }
    else if (strcmp(argv[i],"-olmanualresolve")==0) {
        s_opt.manualResolve = RESOLVE_DIALOG_ALLWAYS;
    }
    else if (strcmp(argv[i],"-olnodialog")==0) {
        s_opt.manualResolve = RESOLVE_DIALOG_NEVER;
    }
    else if (strcmp(argv[i],"-olexaddress")==0) {
        s_opt.extractMode = EXTR_FUNCTION_ADDRESS_ARGS;
    }
    else if (strcmp(argv[i],"-olchecklinkage")==0) {
        s_opt.ooChecksBits |= OOC_LINKAGE_CHECK;
    }
    else if (strcmp(argv[i],"-olcheckaccess")==0) {
        s_opt.ooChecksBits |= OOC_ACCESS_CHECK;
    }
    else if (strcmp(argv[i],"-olnocheckaccess")==0) {
        s_opt.ooChecksBits &= ~OOC_ACCESS_CHECK;
    }
    else if (strcmp(argv[i],"-olallchecks")==0) {
        s_opt.ooChecksBits |= OOC_ALL_CHECKS;
    }
    else if (strncmp(argv[i],"-olfqtcompletionslevel=",23)==0) {
        s_opt.fqtNameToCompletions = 1;
        sscanf(argv[i]+23, "%d",&s_opt.fqtNameToCompletions);
    }
    else if (strcmp(argv[i],"-olexmacro")==0) s_opt.extractMode=EXTR_MACRO;
    else if (strcmp(argv[i],"-olcxunmodified")==0)  {
        s_opt.modifiedFlag = false;
    }
    else if (strcmp(argv[i],"-olcxmodified")==0)    {
        s_opt.modifiedFlag = true;
    }
    else if (strcmp(argv[i],"-olcxrename")==0)  s_opt.server_operation = OLO_RENAME;
    else if (strcmp(argv[i],"-olcxencapsulate")==0) s_opt.server_operation = OLO_ENCAPSULATE;
    else if (strcmp(argv[i],"-olcxargmanip")==0)    s_opt.server_operation = OLO_ARG_MANIP;
    else if (strcmp(argv[i],"-olcxdynamictostatic1")==0)    s_opt.server_operation = OLO_VIRTUAL2STATIC_PUSH;
    else if (strcmp(argv[i],"-olcxsafetycheckinit")==0) s_opt.server_operation = OLO_SAFETY_CHECK_INIT;
    else if (strcmp(argv[i],"-olcxsafetycheck1")==0) s_opt.server_operation = OLO_SAFETY_CHECK1;
    else if (strcmp(argv[i],"-olcxsafetycheck2")==0) s_opt.server_operation = OLO_SAFETY_CHECK2;
    else if (strcmp(argv[i],"-olcxintersection")==0) s_opt.server_operation = OLO_INTERSECTION;
    else if (strcmp(argv[i],"-olcxsafetycheckmovedfile")==0
             || strcmp(argv[i],"-olcxsafetycheckmoved")==0  /* backward compatibility */
             ) {
        NEXT_ARG();
        createOptionString(&s_opt.checkFileMovedFrom, argv[i]);
        NEXT_ARG();
        createOptionString(&s_opt.checkFileMovedTo, argv[i]);
    }
    else if (strcmp(argv[i],"-olcxwindel")==0) {
        s_opt.server_operation = OLO_REMOVE_WIN;
    }
    else if (strcmp(argv[i],"-olcxwindelfile")==0) {
        NEXT_ARG();
        createOptionString(&s_opt.olcxWinDelFile, argv[i]);
    }
    else if (strncmp(argv[i],"-olcxwindelwin=",15)==0) {
        s_opt.olcxWinDelFromLine = s_opt.olcxWinDelToLine = 0;
        s_opt.olcxWinDelFromCol = s_opt.olcxWinDelToCol = 0;
        sscanf(argv[i]+15,"%d:%dx%d:%d",
               &s_opt.olcxWinDelFromLine, &s_opt.olcxWinDelFromCol,
               &s_opt.olcxWinDelToLine, &s_opt.olcxWinDelToCol);
        //&fprintf(dumpOut,"; delete refs %d:%d-%d:%d\n", s_opt.olcxWinDelFromLine, s_opt.olcxWinDelFromCol, s_opt.olcxWinDelToLine, s_opt.olcxWinDelToCol);
    }
    else if (strncmp(argv[i],"-olcxsafetycheckmovedblock=",27)==0) {
        sscanf(argv[i]+27, "%d:%d:%d", &s_opt.checkFirstMovedLine,
               &s_opt.checkLinesMoved, &s_opt.checkNewLineNumber);
        //&fprintf(dumpOut,"safety check block moved == %d:%d:%d\n", s_opt.checkFirstMovedLine,s_opt.checkLinesMoved, s_opt.checkNewLineNumber);
    }
    else if (strcmp(argv[i],"-olcxgotodef")==0) s_opt.server_operation = OLO_GOTO_DEF;
    else if (strcmp(argv[i],"-olcxgotocaller")==0) s_opt.server_operation = OLO_GOTO_CALLER;
    else if (strcmp(argv[i],"-olcxgotocurrent")==0) s_opt.server_operation = OLO_GOTO_CURRENT;
    else if (strcmp(argv[i],"-olcxgetcurrentrefn")==0)s_opt.server_operation=OLO_GET_CURRENT_REFNUM;
    else if (strncmp(argv[i],"-olcxgotoparname",16)==0) {
        s_opt.server_operation = OLO_GOTO_PARAM_NAME;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+16, "%d", &s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxgetparamcoord",18)==0) {
        s_opt.server_operation = OLO_GET_PARAM_COORDINATES;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+18, "%d", &s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxparnum=",12)==0) {
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+12, "%d", &s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxparnum2=",13)==0) {
        s_opt.parnum2 = 0;
        sscanf(argv[i]+13, "%d", &s_opt.parnum2);
    }
    else if (strcmp(argv[i],"-olcxtops")==0) s_opt.server_operation = OLO_SHOW_TOP;
    else if (strcmp(argv[i],"-olcxtoptype")==0) s_opt.server_operation = OLO_SHOW_TOP_TYPE;
    else if (strcmp(argv[i],"-olcxtopapplcl")==0) s_opt.server_operation = OLO_SHOW_TOP_APPL_CLASS;
    else if (strcmp(argv[i],"-olcxshowctree")==0) s_opt.server_operation = OLO_SHOW_CLASS_TREE;
    else if (strcmp(argv[i],"-olcxedittop")==0) s_opt.server_operation = OLO_TOP_SYMBOL_RES;
    else if (strcmp(argv[i],"-olcxclasstree")==0) s_opt.server_operation = OLO_CLASS_TREE;
    else if (strcmp(argv[i],"-olcxsyntaxpass")==0) s_opt.server_operation = OLO_SYNTAX_PASS_ONLY;
    else if (strcmp(argv[i],"-olcxprimarystart")==0) {
        s_opt.server_operation = OLO_GET_PRIMARY_START;
    }
    else if (strcmp(argv[i],"-olcxuselesslongnames")==0) s_opt.server_operation = OLO_USELESS_LONG_NAME;
    else if (strcmp(argv[i],"-olcxuselesslongnamesinclass")==0) s_opt.server_operation = OLO_USELESS_LONG_NAME_IN_CLASS;
    else if (strcmp(argv[i],"-olcxmaybethis")==0) s_opt.server_operation = OLO_MAYBE_THIS;
    else if (strcmp(argv[i],"-olcxnotfqt")==0) s_opt.server_operation = OLO_NOT_FQT_REFS;
    else if (strcmp(argv[i],"-olcxnotfqtinclass")==0) s_opt.server_operation = OLO_NOT_FQT_REFS_IN_CLASS;
    else if (strcmp(argv[i],"-olcxgetrefactorings")==0)     {
        s_opt.server_operation = OLO_GET_AVAILABLE_REFACTORINGS;
    }
    else if (strcmp(argv[i],"-olcxpush")==0)    s_opt.server_operation = OLO_PUSH;
    else if (strcmp(argv[i],"-olcxrepush")==0)  s_opt.server_operation = OLO_REPUSH;
    else if (strcmp(argv[i],"-olcxpushonly")==0) s_opt.server_operation = OLO_PUSH_ONLY;
    else if (strcmp(argv[i],"-olcxpushandcallmacro")==0) s_opt.server_operation = OLO_PUSH_AND_CALL_MACRO;
    else if (strcmp(argv[i],"-olcxencapsulatesc1")==0) s_opt.server_operation = OLO_PUSH_ENCAPSULATE_SAFETY_CHECK;
    else if (strcmp(argv[i],"-olcxencapsulatesc2")==0) s_opt.server_operation = OLO_ENCAPSULATE_SAFETY_CHECK;
    else if (strcmp(argv[i],"-olcxpushallinmethod")==0) s_opt.server_operation = OLO_PUSH_ALL_IN_METHOD;
    else if (strcmp(argv[i],"-olcxmmprecheck")==0) s_opt.server_operation = OLO_MM_PRE_CHECK;
    else if (strcmp(argv[i],"-olcxppprecheck")==0) s_opt.server_operation = OLO_PP_PRE_CHECK;
    else if (strcmp(argv[i],"-olcxpushforlm")==0) {
        s_opt.server_operation = OLO_PUSH_FOR_LOCALM;
        s_opt.manualResolve = RESOLVE_DIALOG_NEVER;
    }
    else if (strcmp(argv[i],"-olcxpushglobalunused")==0)    s_opt.server_operation = OLO_GLOBAL_UNUSED;
    else if (strcmp(argv[i],"-olcxpushfileunused")==0)  s_opt.server_operation = OLO_LOCAL_UNUSED;
    else if (strcmp(argv[i],"-olcxlist")==0)    s_opt.server_operation = OLO_LIST;
    else if (strcmp(argv[i],"-olcxlisttop")==0) s_opt.server_operation=OLO_LIST_TOP;
    else if (strcmp(argv[i],"-olcxpop")==0)     s_opt.server_operation = OLO_POP;
    else if (strcmp(argv[i],"-olcxpoponly")==0) s_opt.server_operation =OLO_POP_ONLY;
    else if (strcmp(argv[i],"-olcxplus")==0)    s_opt.server_operation = OLO_PLUS;
    else if (strcmp(argv[i],"-olcxminus")==0)   s_opt.server_operation = OLO_MINUS;
    else if (strcmp(argv[i],"-olcxsearch")==0)  s_opt.server_operation = OLO_SEARCH;
    else if (strcmp(argv[i],"-olcxcomplet")==0)s_opt.server_operation=OLO_COMPLETION;
    else if (strcmp(argv[i],"-olcxtarget")==0)  s_opt.server_operation=OLO_SET_MOVE_TARGET;
    else if (strcmp(argv[i],"-olcxmctarget")==0)    s_opt.server_operation=OLO_SET_MOVE_CLASS_TARGET;
    else if (strcmp(argv[i],"-olcxmmtarget")==0)    s_opt.server_operation=OLO_SET_MOVE_METHOD_TARGET;
    else if (strcmp(argv[i],"-olcxcurrentclass")==0)    s_opt.server_operation=OLO_GET_CURRENT_CLASS;
    else if (strcmp(argv[i],"-olcxcurrentsuperclass")==0)   s_opt.server_operation=OLO_GET_CURRENT_SUPER_CLASS;
    else if (strcmp(argv[i],"-olcxmethodlines")==0) s_opt.server_operation=OLO_GET_METHOD_COORD;
    else if (strcmp(argv[i],"-olcxclasslines")==0)  s_opt.server_operation=OLO_GET_CLASS_COORD;
    else if (strcmp(argv[i],"-olcxgetsymboltype")==0) s_opt.server_operation=OLO_GET_SYMBOL_TYPE;
    else if (strcmp(argv[i],"-olcxgetprojectname")==0) {
        s_opt.server_operation=OLO_ACTIVE_PROJECT;
    }
    else if (strcmp(argv[i],"-olcxgetjavahome")==0) {
        s_opt.server_operation=OLO_JAVA_HOME;
    }
    else if (strncmp(argv[i],"-olcxlccursor=",14)==0) {
        // position of the cursor in line:column format
        createOptionString(&s_opt.olcxlccursor, argv[i]+14);
    }
    else if (strncmp(argv[i],"-olcxcplsearch=",15)==0) {
        s_opt.server_operation=OLO_SEARCH;
        createOptionString(&s_opt.olcxSearchString, argv[i]+15);
    }
    else if (strncmp(argv[i],"-olcxtagsearch=",15)==0) {
        s_opt.server_operation=OLO_TAG_SEARCH;
        createOptionString(&s_opt.olcxSearchString, argv[i]+15);
    }
    else if (strcmp(argv[i],"-olcxtagsearchforward")==0) {
        s_opt.server_operation=OLO_TAG_SEARCH_FORWARD;
    }
    else if (strcmp(argv[i],"-olcxtagsearchback")==0) {
        s_opt.server_operation=OLO_TAG_SEARCH_BACK;
    }
    else if (strncmp(argv[i],"-olcxpushname=",14)==0)   {
        s_opt.server_operation = OLO_PUSH_NAME;
        createOptionString(&s_opt.pushName, argv[i]+14);
    }
    else if (strncmp(argv[i],"-olcxpushspecialname=",21)==0)    {
        s_opt.server_operation = OLO_PUSH_SPECIAL_NAME;
        createOptionString(&s_opt.pushName, argv[i]+21);
    }
    else if (strncmp(argv[i],"-olcomplselect",14)==0) {
        s_opt.server_operation=OLO_CSELECT;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+14,"%d",&s_opt.olcxGotoVal);
    }
    else if (strcmp(argv[i],"-olcomplback")==0) {
        s_opt.server_operation=OLO_COMPLETION_BACK;
    }
    else if (strcmp(argv[i],"-olcomplforward")==0) {
        s_opt.server_operation=OLO_COMPLETION_FORWARD;
    }
    else if (strncmp(argv[i],"-olcxcgoto",10)==0) {
        s_opt.server_operation = OLO_CGOTO;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+10,"%d",&s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxtaggoto",12)==0) {
        s_opt.server_operation = OLO_TAGGOTO;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+12,"%d",&s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxtagselect",14)==0) {
        s_opt.server_operation = OLO_TAGSELECT;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+14,"%d",&s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxcbrowse",12)==0) {
        s_opt.server_operation = OLO_CBROWSE;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+12,"%d",&s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxgoto",9)==0) {
        s_opt.server_operation = OLO_GOTO;
        s_opt.olcxGotoVal = 0;
        sscanf(argv[i]+9,"%d",&s_opt.olcxGotoVal);
    }
    else if (strncmp(argv[i],"-olcxfilter=",12)==0) {
        s_opt.server_operation = OLO_REF_FILTER_SET;
        sscanf(argv[i]+12,"%d",&s_opt.filterValue);
    }
    else if (strncmp(argv[i], "-olcxmenusingleselect",21)==0) {
        s_opt.server_operation = OLO_MENU_SELECT_ONLY;
        s_opt.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+21,"%d",&s_opt.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i],"-olcxmenuselect",15)==0) {
        s_opt.server_operation = OLO_MENU_SELECT;
        s_opt.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+15,"%d",&s_opt.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i],"-olcxmenuinspectdef",19)==0) {
        s_opt.server_operation = OLO_MENU_INSPECT_DEF;
        s_opt.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+19,"%d",&s_opt.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i],"-olcxmenuinspectclass",21)==0) {
        s_opt.server_operation = OLO_MENU_INSPECT_CLASS;
        s_opt.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+21,"%d",&s_opt.olcxMenuSelectLineNum);
    }
    else if (strncmp(argv[i],"-olcxctinspectdef",17)==0) {
        s_opt.server_operation = OLO_CT_INSPECT_DEF;
        s_opt.olcxMenuSelectLineNum = 0;
        sscanf(argv[i]+17,"%d",&s_opt.olcxMenuSelectLineNum);
    }
    else if (strcmp(argv[i],"-olcxmenuall")==0) {
        s_opt.server_operation = OLO_MENU_SELECT_ALL;
    }
    else if (strcmp(argv[i],"-olcxmenunone")==0) {
        s_opt.server_operation = OLO_MENU_SELECT_NONE;
    }
    else if (strcmp(argv[i],"-olcxmenugo")==0) {
        s_opt.server_operation = OLO_MENU_GO;
    }
    else if (strncmp(argv[i],"-olcxmenufilter=",16)==0) {
        s_opt.server_operation = OLO_MENU_FILTER_SET;
        sscanf(argv[i]+16,"%d",&s_opt.filterValue);
    }
    else if (strcmp(argv[i],"-optinclude")==0) {
        i = mainHandleIncludeOption(argc, argv, i);
    }
    else if (strcmp(argv[i],"-o")==0) {
        NEXT_FILE_ARG();
        createOptionString(&s_opt.outputFileName, argv[i]);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processPOption(int *ii, int argc, char **argv) {
    char    ttt[MAX_FILE_NAME_SIZE];
    int     i = * ii;
    if (0) {}
    else if (strncmp(argv[i],"-pause",5)==0) {
        /* Pause to be able to attach with debugger... */
        NEXT_ARG();
        sleep(atoi(argv[i]));
    }
    else if (strncmp(argv[i],"-pass",5)==0) {
        errorMessage(ERR_ST,"'-pass' option can't be entered from command line");
    }
    else if (strcmp(argv[i],"-packages")==0) {
        s_opt.allowPackagesOnCl = 1;
    }
    else if (strcmp(argv[i],"-p")==0) {
        NEXT_FILE_ARG();
        //fprintf(dumpOut,"current project '%s'\n", argv[i]);
        createOptionString(&s_opt.project, argv[i]);
    }
    else if (strcmp(argv[i],"-preload")==0) {
        char *file, *fromFile;
        NEXT_FILE_ARG();
        file = argv[i];
        strcpy(ttt, normalizeFileName(file, s_cwd));
        NEXT_FILE_ARG();
        fromFile = argv[i];
        // TODO, maybe do this also through allocated list of options
        // and serve them later ?
        //&sprintf(tmpBuff,"-preload %s %s\n", ttt, fromFile); ppcGenRecord(PPC_IGNORE, tmpBuff, "\n");
        editorOpenBufferNoFileLoad(ttt, fromFile);
    }
    else if (strcmp(argv[i],"-prune")==0) {
        NEXT_ARG();
        mainAddStringListOption(&s_opt.pruneNames, argv[i]);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processQOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else return(0);
    *ii = i;
    return(1);
}

static void setXrefsFile(char *argvi) {
    static int message=0;

    if (s_opt.taskRegime==RegimeXref && message==0 && ! isAbsolutePath(argvi)) {
        char tmpBuff[TMP_BUFF_SIZE];
        message = 1;
        sprintf(tmpBuff,"'%s' is not an absolute path, correct -refs option",argvi);
        warningMessage(ERR_ST, tmpBuff);
    }
    createOptionString(&s_opt.cxrefFileName, normalizeFileName(argvi, s_cwd));
}

static int processROption(int *ii, int argc, char **argv, int infilesFlag) {
    int i = * ii;
    if (0) {}
    else if (strncmp(argv[i],"-refnum=",8)==0)  {
        sscanf(argv[i]+8, "%d", &s_opt.referenceFileCount);
    }
    else if (strcmp(argv[i],"-refalphahash")==0
             || strcmp(argv[i],"-refalpha1hash")==0)    {
        int check;
        s_opt.xfileHashingMethod = XFILE_HASH_ALPHA1;
        check = changeRefNumOption(XFILE_HASH_ALPHA1_REFNUM);
        if (check == 0) {
            assert(s_opt.taskRegime);
            fatalError(ERR_ST,"'-refalphahash' conflicts with '-refnum' option", XREF_EXIT_ERR);
        }
    }
    else if (strcmp(argv[i],"-refalpha2hash")==0)   {
        int check;
        s_opt.xfileHashingMethod = XFILE_HASH_ALPHA2;
        check = changeRefNumOption(XFILE_HASH_ALPHA2_REFNUM);
        if (check == 0) {
            assert(s_opt.taskRegime);
            fatalError(ERR_ST,"'-refalpha2hash' conflicts with '-refnum' option", XREF_EXIT_ERR);
        }
    }
    else if (strcmp(argv[i],"-r")==0) {
        if (infilesFlag == INFILES_ENABLED) s_opt.recursivelyDirs = true;
    }
    else if (strncmp(argv[i],"-renameto=", 10)==0) {
        createOptionString(&s_opt.renameTo, argv[i]+10);
    }
    else if (strcmp(argv[i],"-resetIncludeDirs")==0) {
        s_opt.includeDirs = NULL;
    }
    else if (strcmp(argv[i],"-refs")==0)    {
        NEXT_FILE_ARG();
        setXrefsFile(argv[i]);
    }
    else if (strncmp(argv[i],"-refs=",6)==0)    {
        setXrefsFile(argv[i]+6);
    }
    else if (strcmp(argv[i],"-rlistwithoutsrc")==0) {
        s_opt.referenceListWithoutSource = 1;
    }
    else if (strcmp(argv[i],"-refactory")==0)   {
        s_opt.refactoringRegime = RegimeRefactory;
    }
    else if (strcmp(argv[i],"-rfct-rename")==0) {
        s_opt.theRefactoring = PPC_AVR_RENAME_SYMBOL;
    }
    else if (strcmp(argv[i],"-rfct-rename-class")==0)   {
        s_opt.theRefactoring = PPC_AVR_RENAME_CLASS;
    }
    else if (strcmp(argv[i],"-rfct-rename-package")==0) {
        s_opt.theRefactoring = PPC_AVR_RENAME_PACKAGE;
    }
    else if (strcmp(argv[i],"-rfct-expand")==0) {
        s_opt.theRefactoring = PPC_AVR_EXPAND_NAMES;
    }
    else if (strcmp(argv[i],"-rfct-reduce")==0) {
        s_opt.theRefactoring = PPC_AVR_REDUCE_NAMES;
    }
    else if (strcmp(argv[i],"-rfct-add-param")==0)  {
        s_opt.theRefactoring = PPC_AVR_ADD_PARAMETER;
    }
    else if (strcmp(argv[i],"-rfct-del-param")==0)  {
        s_opt.theRefactoring = PPC_AVR_DEL_PARAMETER;
    }
    else if (strcmp(argv[i],"-rfct-move-param")==0) {
        s_opt.theRefactoring = PPC_AVR_MOVE_PARAMETER;
    }
    else if (strcmp(argv[i],"-rfct-move-field")==0) {
        s_opt.theRefactoring = PPC_AVR_MOVE_FIELD;
    }
    else if (strcmp(argv[i],"-rfct-move-static-field")==0)  {
        s_opt.theRefactoring = PPC_AVR_MOVE_STATIC_FIELD;
    }
    else if (strcmp(argv[i],"-rfct-move-static-method")==0) {
        s_opt.theRefactoring = PPC_AVR_MOVE_STATIC_METHOD;
    }
    else if (strcmp(argv[i],"-rfct-move-class")==0) {
        s_opt.theRefactoring = PPC_AVR_MOVE_CLASS;
    }
    else if (strcmp(argv[i],"-rfct-move-class-to-new-file")==0) {
        s_opt.theRefactoring = PPC_AVR_MOVE_CLASS_TO_NEW_FILE;
    }
    else if (strcmp(argv[i],"-rfct-move-all-classes-to-new-file")==0)   {
        s_opt.theRefactoring = PPC_AVR_MOVE_ALL_CLASSES_TO_NEW_FILE;
    }
    else if (strcmp(argv[i],"-rfct-static-to-dynamic")==0)  {
        s_opt.theRefactoring = PPC_AVR_TURN_STATIC_METHOD_TO_DYNAMIC;
    }
    else if (strcmp(argv[i],"-rfct-dynamic-to-static")==0)  {
        s_opt.theRefactoring = PPC_AVR_TURN_DYNAMIC_METHOD_TO_STATIC;
    }
    else if (strcmp(argv[i],"-rfct-extract-method")==0) {
        s_opt.theRefactoring = PPC_AVR_EXTRACT_METHOD;
    }
    else if (strcmp(argv[i],"-rfct-extract-macro")==0)  {
        s_opt.theRefactoring = PPC_AVR_EXTRACT_MACRO;
    }
    else if (strcmp(argv[i],"-rfct-reduce-long-names-in-the-file")==0)  {
        s_opt.theRefactoring = PPC_AVR_ADD_ALL_POSSIBLE_IMPORTS;
    }
    else if (strcmp(argv[i],"-rfct-add-to-imports")==0) {
        s_opt.theRefactoring = PPC_AVR_ADD_TO_IMPORT;
    }
    else if (strcmp(argv[i],"-rfct-self-encapsulate-field")==0) {
        s_opt.theRefactoring = PPC_AVR_SELF_ENCAPSULATE_FIELD;
    }
    else if (strcmp(argv[i],"-rfct-encapsulate-field")==0)  {
        s_opt.theRefactoring = PPC_AVR_ENCAPSULATE_FIELD;
    }
    else if (strcmp(argv[i],"-rfct-push-down-field")==0)    {
        s_opt.theRefactoring = PPC_AVR_PUSH_DOWN_FIELD;
    }
    else if (strcmp(argv[i],"-rfct-push-down-method")==0)   {
        s_opt.theRefactoring = PPC_AVR_PUSH_DOWN_METHOD;
    }
    else if (strcmp(argv[i],"-rfct-pull-up-field")==0)  {
        s_opt.theRefactoring = PPC_AVR_PULL_UP_FIELD;
    }
    else if (strcmp(argv[i],"-rfct-pull-up-method")==0) {
        s_opt.theRefactoring = PPC_AVR_PULL_UP_METHOD;
    }
#if 0
    else if (strcmp(argv[i],"-rfct-")==0)   {
        s_opt.theRefactoring = PPC_AVR_;
    }
#endif
    else if (strncmp(argv[i], "-rfct-param1=", 13)==0)  {
        createOptionString(&s_opt.refpar1, argv[i]+13);
    }
    else if (strncmp(argv[i], "-rfct-param2=", 13)==0)  {
        createOptionString(&s_opt.refpar2, argv[i]+13);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processSOption(int *ii, int argc, char **argv) {
    int i = * ii;
    char *name, *val;

    if (0) {}
    else if (strcmp(argv[i],"-strict")==0)      s_opt.strictAnsi = true;
    else if (strcmp(argv[i],"-stderr")==0)          errOut = stdout;
    else if (strcmp(argv[i],"-source")==0)  {
        char tmpBuff[TMP_BUFF_SIZE];
        NEXT_ARG();
        if (strcmp(argv[i], JAVA_VERSION_1_3)!=0 && strcmp(argv[i], JAVA_VERSION_1_4)!=0) {
            sprintf(tmpBuff,"wrong -javaversion=<value>, available values are %s, %s",
                    JAVA_VERSION_1_3, JAVA_VERSION_1_4);
            errorMessage(ERR_ST, tmpBuff);
        } else {
            createOptionString(&s_opt.javaVersion, argv[i]);
        }
    }
    else if (strcmp(argv[i],"-sourcepath")==0) {
        NEXT_FILE_ARG();
        createOptionString(&s_opt.sourcePath, argv[i]);
        xrefSetenv("-sourcepath", s_opt.sourcePath);
    }
    else if (strcmp(argv[i],"-stdop")==0) {
        i = mainHandleIncludeOption(argc, argv, i);
    }
    else if (strcmp(argv[i],"-set")==0) {
        i = mainHandleSetOption(argc, argv, i);
    }
    else if (strncmp(argv[i],"-set",4)==0) {
        name = argv[i]+4;
        NEXT_ARG();
        val = argv[i];
        xrefSetenv(name, val);
    }
    else if (strcmp(argv[i],"-searchdef")==0) {
        s_opt.tagSearchSpecif = TSS_SEARCH_DEFS_ONLY;
    }
    else if (strcmp(argv[i],"-searchshortlist")==0) {
        s_opt.tagSearchSpecif = TSS_FULL_SEARCH_SHORT;
    }
    else if (strcmp(argv[i],"-searchdefshortlist")==0) {
        s_opt.tagSearchSpecif = TSS_SEARCH_DEFS_ONLY_SHORT;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processTOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
#ifdef DEBUG
    else if (strcmp(argv[i],"-trace")==0) {
        s_opt.trace = true;
    }
#endif
    else if (strcmp(argv[i],"-task_regime_server")==0) {
        s_opt.taskRegime = RegimeEditServer;
    }
    else if (strcmp(argv[i],"-task_regime_generate")==0) {
        s_opt.taskRegime = RegimeGenerate;
    }
    else if (strcmp(argv[i],"-thread")==0) {
        NEXT_FILE_ARG();
        createOptionString(&s_opt.user, argv[i]);
    }
    else if (strcmp(argv[i],"-tpchrenamepackage")==0) {
        s_opt.trivialPreCheckCode = TPC_RENAME_PACKAGE;
    }
    else if (strcmp(argv[i],"-tpchrenameclass")==0) {
        s_opt.trivialPreCheckCode = TPC_RENAME_CLASS;
    }
    else if (strcmp(argv[i],"-tpchmoveclass")==0) {
        s_opt.trivialPreCheckCode = TPC_MOVE_CLASS;
    }
    else if (strcmp(argv[i],"-tpchmovefield")==0) {
        s_opt.trivialPreCheckCode = TPC_MOVE_FIELD;
    }
    else if (strcmp(argv[i],"-tpchmovestaticfield")==0) {
        s_opt.trivialPreCheckCode = TPC_MOVE_STATIC_FIELD;
    }
    else if (strcmp(argv[i],"-tpchmovestaticmethod")==0) {
        s_opt.trivialPreCheckCode = TPC_MOVE_STATIC_METHOD;
    }
    else if (strcmp(argv[i],"-tpchturndyntostatic")==0) {
        s_opt.trivialPreCheckCode = TPC_TURN_DYN_METHOD_TO_STATIC;
    }
    else if (strcmp(argv[i],"-tpchturnstatictodyn")==0) {
        s_opt.trivialPreCheckCode = TPC_TURN_STATIC_METHOD_TO_DYN;
    }
    else if (strcmp(argv[i],"-tpchpullupmethod")==0) {
        s_opt.trivialPreCheckCode = TPC_PULL_UP_METHOD;
    }
    else if (strcmp(argv[i],"-tpchpushdownmethod")==0) {
        s_opt.trivialPreCheckCode = TPC_PUSH_DOWN_METHOD;
    }
    else if (strcmp(argv[i],"-tpchpushdownmethodpostcheck")==0) {
        s_opt.trivialPreCheckCode = TPC_PUSH_DOWN_METHOD_POST_CHECK;
    }
    else if (strcmp(argv[i],"-tpchpullupfield")==0) {
        s_opt.trivialPreCheckCode = TPC_PULL_UP_FIELD;
    }
    else if (strcmp(argv[i],"-tpchpushdownfield")==0) {
        s_opt.trivialPreCheckCode = TPC_PUSH_DOWN_FIELD;
    }
    else return(0);
    *ii = i;
    return(1);
}

static bool processUOption(int *argIndexP, int argc, char **argv) {
    int i = *argIndexP;
    if (0) {}
    else if (strcmp(argv[i],"-urlmanualredirect")==0)   {
        s_opt.urlAutoRedirect = 0;
    }
    else if (strcmp(argv[i],"-urldirect")==0)   {
        s_opt.urlGenTemporaryFile = false;
    }
    else if (strcmp(argv[i],"-user")==0) {
        NEXT_ARG();
        createOptionString(&s_opt.user, argv[i]);
    }
    else if (strcmp(argv[i],"-update")==0)  {
        s_opt.update = UP_FULL_UPDATE;
        s_opt.updateOnlyModifiedFiles = true;
    }
    else if (strcmp(argv[i],"-updatem")==0) {
        s_opt.update = UP_FULL_UPDATE;
        s_opt.updateOnlyModifiedFiles = false;
    }
    else
        return(false);
    *argIndexP = i;
    return true;
}

static int processVOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-version")==0||strcmp(argv[i],"-about")==0){
        s_opt.server_operation = OLO_ABOUT;
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processWOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else return(0);
    *ii = i;
    return(1);
}

static int processXOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else if (strcmp(argv[i],"-xrefactory-II") == 0){
        s_opt.xref2 = 1;
    }
    else if (strncmp(argv[i],"-xrefrc=",8) == 0) {
        createOptionString(&s_opt.xrefrc, argv[i]+8);
    }
    else if (strcmp(argv[i],"-xrefrc") == 0) {
        NEXT_FILE_ARG();
        createOptionString(&s_opt.xrefrc, argv[i]);
    }
    else return(0);
    *ii = i;
    return(1);
}

static int processYOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
#ifdef YYDEBUG
    else if (strcmp(argv[i],"-yydebug") == 0){
        c_yydebug = 1;
        yacc_yydebug = 1;
        java_yydebug = 1;
    }
#endif
    else return(0);
    *ii = i;
    return(1);
}

static int processZOption(int *ii, int argc, char **argv) {
    int i = * ii;
    if (0) {}
    else return(0);
    *ii = i;
    return(1);
}

static void mainScheduleInputFileOptionToFileTable(char *infile) {
    int topCallFlag;
    void *recursFlag;
    javaSetSourcePath(1);      // for case of packages on command line
    topCallFlag = 1;
    recursFlag = &topCallFlag;
    JavaMapOnPaths(infile, {
            dirInputFile(currentPath,"",NULL,NULL,recursFlag,&topCallFlag);
        });
}

static void mainProcessInFileOption(char *infile) {
    int i;
    if (infile[0]=='`' && infile[strlen(infile)-1]=='`') {
        int nargc;
        char **nargv, *pp;
        char command[MAX_OPTION_LEN];
        s_opt.stdopFlag = 1;
        strcpy(command, infile+1);
        pp = strchr(command, '`');
        if (pp!=NULL) *pp = 0;
        readOptionPipe(command, &nargc, &nargv, "");
        for(i=1; i<nargc; i++) {
            if (nargv[i][0]!='-' && nargv[i][0]!='`') {
                mainScheduleInputFileOptionToFileTable(nargv[i]);
            }
        }
        s_opt.stdopFlag = 0;
    } else {
        mainScheduleInputFileOptionToFileTable(infile);
    }
}

void processOptions(int argc, char **argv, int infilesFlag) {
    int             i, processed;

    for (i=1; i<argc; i++) {
        if (s_opt.taskRegime==RegimeEditServer &&
            strncmp(argv[i],"-last_message=",14)==0) {
            // because of emacs-debug
            log_trace("processing argument '-lastmessage=...'");
        } else {
            log_trace("processing argument '%s'", argv[i]);
        }
        processed = 0;
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case '-': processed = processNegativeOption(&i, argc, argv, infilesFlag);
                break;
            case 'a': case 'A': processed = processAOption(&i, argc, argv);
                break;
            case 'b': case 'B': processed = processBOption(&i, argc, argv);
                break;
            case 'c': case 'C': processed = processCOption(&i, argc, argv);
                break;
            case 'd': case 'D': processed = processDOption(&i, argc, argv);
                break;
            case 'e': case 'E': processed = processEOption(&i, argc, argv);
                break;
            case 'f': case 'F': processed = processFOption(&i, argc, argv);
                break;
            case 'g': case 'G': processed = processGOption(&i, argc, argv);
                break;
            case 'h': case 'H': processed = processHOption(&i, argc, argv);
                break;
            case 'i': case 'I': processed = processIOption(&i, argc, argv);
                break;
            case 'j': case 'J': processed = processJOption(&i, argc, argv);
                break;
            case 'k': case 'K': processed = processKOption(&i, argc, argv);
                break;
            case 'l': case 'L': processed = processLOption(&i, argc, argv);
                break;
            case 'm': case 'M': processed = processMOption(&i, argc, argv);
                break;
            case 'n': case 'N': processed = processNOption(&i, argc, argv);
                break;
            case 'o': case 'O': processed = processOOption(&i, argc, argv);
                break;
            case 'p': case 'P': processed = processPOption(&i, argc, argv);
                break;
            case 'q': case 'Q': processed = processQOption(&i, argc, argv);
                break;
            case 'r': case 'R': processed = processROption(&i, argc, argv, infilesFlag);
                break;
            case 's': case 'S': processed = processSOption(&i, argc, argv);
                break;
            case 't': case 'T': processed = processTOption(&i, argc, argv);
                break;
            case 'u': case 'U': processed = processUOption(&i, argc, argv);
                break;
            case 'v': case 'V': processed = processVOption(&i, argc, argv);
                break;
            case 'w': case 'W': processed = processWOption(&i, argc, argv);
                break;
            case 'x': case 'X': processed = processXOption(&i, argc, argv);
                break;
            case 'y': case 'Y': processed = processYOption(&i, argc, argv);
                break;
            case 'z': case 'Z': processed = processZOption(&i, argc, argv);
                break;
            default: processed = 0;
            }
        } else {
            /* input file */
            processed = 1;
            if (infilesFlag == INFILES_ENABLED) {
                mainAddStringListOption(&s_opt.inputFiles, argv[i]);
            }
        }
        if (! processed) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"unknown option %s, (try xref -help)\n",argv[i]);
            if (s_opt.taskRegime==RegimeXref
                ||  s_opt.taskRegime==RegimeHtmlGenerate) {
                fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
            }
        }
    }
}

static void mainScheduleInputFilesFromOptionsToFileTable(void) {
    S_stringList *ll;
    for(ll=s_opt.inputFiles; ll!=NULL; ll=ll->next) {
        mainProcessInFileOption(ll->d);
    }
}

/* *************************************************************************** */


static char * getInputFileFromFtab(int *fArgCount, int flag) {
    int         i;
    FileItem  *fi;
    for(i= *fArgCount; i<s_fileTab.size; i++) {
        fi = s_fileTab.tab[i];
        if (fi!=NULL) {
            if (flag==FF_SCHEDULED_TO_PROCESS&&fi->b.scheduledToProcess) break;
            if (flag==FF_COMMAND_LINE_ENTERED&&fi->b.commandLineEntered) break;
        }
    }
    *fArgCount = i;
    if (i<s_fileTab.size) return(s_fileTab.tab[i]->name);
    else return(NULL);
}

char * getInputFile(int *fArgCount) {
    return(getInputFileFromFtab(fArgCount,FF_SCHEDULED_TO_PROCESS));
}

static char * getCommandLineFile(int *fArgCount) {
    return(getInputFileFromFtab(fArgCount,FF_COMMAND_LINE_ENTERED));
}

static void mainGenerateReferenceFile(void) {
    static bool updateFlag = false;  /* TODO: WTF - why do we need a
                                        static updateFlag? Maybe we
                                        need to know that we have
                                        generated from scratch so now
                                        we can just update? */

    if (s_opt.cxrefFileName == NULL)
        return;
    if (!updateFlag && s_opt.update == UP_CREATE) {
        genReferenceFile(false, s_opt.cxrefFileName);
        updateFlag = true;
    } else {
        genReferenceFile(true, s_opt.cxrefFileName);
    }
}

static void schedulingUpdateToProcess(FileItem *p) {
    if (p->b.scheduledToUpdate && p->b.commandLineEntered) {
        p->b.scheduledToProcess = true;
    }
}

static void schedulingToUpdate(FileItem *p, void *rs) {
    struct stat fstat, hstat;
    char sss[MAX_FILE_NAME_SIZE];

    if (p == s_fileTab.tab[s_noneFileIndex]) return;
    //& if (s_opt.update==UP_FAST_UPDATE && !p->b.commandLineEntered) return;
    //&fprintf(dumpOut,"checking %s for update\n",p->name); fflush(dumpOut);
    if (statb(p->name, &fstat)) {
        // removed file, remove it from watched updates, load no reference
        if (p->b.commandLineEntered) {
            // no messages during refactorings
            if (s_ropt.refactoringRegime != RegimeRefactory) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff,"file %s not accessible",   p->name);
                warningMessage(ERR_ST, tmpBuff);
            }
        }
        p->b.commandLineEntered = false;
        p->b.scheduledToProcess = false;
        p->b.scheduledToUpdate = false;
        // (missing of following if) has caused that all class hierarchy items
        // as well as all cxreferences based in .class files were lost
        // on -update, a very serious bug !!!!
        if (p->name[0] != ZIP_SEPARATOR_CHAR) {
            p->b.cxLoading = true;     /* Hack, to remove references from file */
        }
    } else if (s_opt.taskRegime == RegimeHtmlGenerate) {
        concatPaths(sss,MAX_FILE_NAME_SIZE,s_opt.htmlRoot,p->name,".html");
        strcat(sss, s_opt.htmlLinkSuffix);
        assert(strlen(sss) < MAX_FILE_NAME_SIZE-2);
        if (statb(sss, &hstat) || fstat.st_mtime >= hstat.st_mtime) {
            p->b.scheduledToUpdate = true;
        }
    } else if (s_opt.update == UP_FULL_UPDATE) {
        if (fstat.st_mtime != p->lastFullUpdateMtime) {
            p->b.scheduledToUpdate = true;
            //&         p->lastFullUpdateMtime = fstat.st_mtime;
            //&         p->lastUpdateMtime = fstat.st_mtime;
        }
    } else {
        if (fstat.st_mtime != p->lastUpdateMtime) {
            p->b.scheduledToUpdate = true;
            //&         p->lastUpdateMtime = fstat.st_mtime;
        }
    }
    //&if (p->b.scheduledToUpdate) {fprintf(dumpOut,"scheduling %s to update\n", p->name); fflush(dumpOut);}
}

void searchDefaultOptionsFile(char *filename, char *options_filename, char *section) {
    int fileno;
    bool found=false;
    FILE *options_file;
    int nargc;
    char **nargv;

    options_filename[0] = 0; section[0]=0;
    if (filename == NULL) return;
    if (s_opt.stdopFlag || s_opt.no_stdop) return;

    /* Try to find section in HOME config. */
    getXrefrcFileName(options_filename);
    options_file = openFile(options_filename, "r");
    if (options_file != NULL) {
        found = readOptionFromFile(options_file,&nargc,&nargv,MEM_NO_ALLOC,filename,s_opt.project,section);
        if (found) {
            log_debug("options file '%s' section '%s'", options_filename, section);
        }
        closeFile(options_file);
    }
    if (found)
        return;

    // If automatic selection did not find project, keep previous one
    if (s_opt.project==NULL) {
        // but do this only if file is from cxfile, would be better to
        // check if it is from active project, but nothing is perfect
        // TODO: Where else could it come from (Xref.opt is not used anymore)?

        // TODO: check whether the project still exists in the .c-xrefrc file
        // it may happen that after deletion of the project, the request for active
        // project will return non-existent project. And then return "not found"?
        fileno = getFileNumberFromName(filename);
        if (fileno != s_noneFileIndex && s_fileTab.tab[fileno]->b.isFromCxfile) {
            strcpy(options_filename, oldStdopFile);
            strcpy(section, oldStdopSection);
            return;
        }
    }
    options_filename[0]=0;
    return;
}

static void writeOptionsFileMessage( char *file,
                                     char *outFName, char *outSect ) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (s_opt.refactoringRegime==RegimeRefactory) return;
    if (outFName[0]==0) {
        if (s_opt.project!=NULL) {
            sprintf(tmpBuff,"'%s' project options not found",
                    s_opt.project);
            if (s_opt.taskRegime == RegimeEditServer) {
                errorMessage(ERR_ST, tmpBuff);
            } else {
                fatalError(ERR_ST, tmpBuff, XREF_EXIT_NO_PROJECT);
            }
        } else if (! JAVA2HTML()) {
            if (s_opt.xref2) {
                ppcGenRecord(PPC_NO_PROJECT,file,"\n");
            } else {
                sprintf(tmpBuff,"no project name covers '%s'",file);
                warningMessage(ERR_ST, tmpBuff);
            }
        }
        //&} else if (s_opt.xref2) {
        //& sprintf(tmpBuff,"C-xrefactory project: %s", outSect);
        //& ppcGenRecord(PPC_BOTTOM_INFORMATION, tmpBuff, "\n");
    } else if (s_opt.taskRegime==RegimeXref) {
        if (s_opt.xref2) {
            sprintf(tmpBuff,"C-xrefactory project: %s", outSect);
            ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
        } else {
            fprintf(dumpOut,"[C-xref] active project: '%s'\n", outSect);
            fflush(dumpOut);
        }
    }
}

static void handlePathologicProjectCases(char *file,char *outFName,char *outSect,int errMessage){
    // all this stuff should be reworked, but be very careful when refactoring it
    // WTF? Why??!?!
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime == RegimeEditServer) {
        if (errMessage!=NO_ERROR_MESSAGE) {
            writeOptionsFileMessage(file, outFName, outSect);
        }
    } else {
        if (*oldStdopFile == 0) {
            static int messageYetWritten=0; /* TODO: bool! "yet" = "already"? */
            if (errMessage!=NO_ERROR_MESSAGE && messageYetWritten == 0) {
                messageYetWritten = 1;
                writeOptionsFileMessage(file, outFName, outSect);
            }
        } else {
            if (outFName[0]==0 || outSect[0]==0) {
                warningMessage(ERR_ST,"no project name covers this file");
            }
            if (outFName[0]==0 && outSect[0]==0) {
                strcpy(outSect, oldStdopSection);
            }
            if (outFName[0]==0) {
                strcpy(outFName, oldStdopFile);
            }
            if(strcmp(oldStdopFile,outFName)||strcmp(oldStdopSection,outSect)){
                if (s_opt.xref2) {
                    char tmpBuff[TMP_BUFF_SIZE];                        \
                    sprintf(tmpBuff, "[Xref] new project: '%s'", outSect);
                    ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
                } else {
                    fprintf(dumpOut,"[Xref] new project: '%s'\n", outSect);
                }
            }
        }
    }
}

static void getOptionsFile(char *file, char *outFName, char *outSect,int errMessage) {
    searchDefaultOptionsFile(file, outFName, outSect);
    handlePathologicProjectCases(file, outFName, outSect, errMessage);
    return;
}

static int computeAndOpenInputFile(void) {
    FILE            *inputIn;
    S_editorBuffer  *inputBuff;
    assert(s_language);
    inputBuff = NULL;
    //!!!! hack for .jar files !!!
    if (LANGUAGE(LANG_JAR) || LANGUAGE(LANG_CLASS)) return(0);
    if (s_input_file_name == NULL) {
        assert(0);
        inputIn = stdin;
        s_input_file_name = "__none__";
        //& } else if (s_opt.server_operation == OLO_GET_ENV_VALUE) {
        // hack for getenv
        // not anymore, parse input for getenv ${__class}, ... settings
        // also if yes, then move this to 'mainEditSrvParseInputFile' !
        //&     return(0);
    } else {
        inputIn = NULL;
        //& inputBuff = editorGetOpenedAndLoadedBuffer(s_input_file_name);
        inputBuff = editorFindFile(s_input_file_name);
        if (inputBuff == NULL) {
#if defined (__WIN32__)
            inputIn = openFile(s_input_file_name,"rb");
#else
            inputIn = openFile(s_input_file_name,"r");
#endif
            if (inputIn == NULL) {
                errorMessage(ERR_CANT_OPEN, s_input_file_name);
            }
        }
    }
    initInput(inputIn, inputBuff, "\n", s_input_file_name);
    if (inputIn==NULL && inputBuff==NULL) {
        return(0);
    } else {
        return(1);
    }
}

static void initOptions(void) {
    copyOptions(&s_opt, &s_initOpt);
    s_opt.stdopFlag = 0;
    s_input_file_number = s_noneFileIndex;
}

static void initDefaultCxrefFileName(char *inputfile) {
    int pathLength;
    static char defaultCxrefFileName[MAX_FILE_NAME_SIZE];

    pathLength = extractPathInto(normalizeFileName(inputfile, s_cwd), defaultCxrefFileName);
    assert(pathLength < MAX_FILE_NAME_SIZE);
    strcpy(&defaultCxrefFileName[pathLength], DEFAULT_CXREF_FILE);
    assert(strlen(defaultCxrefFileName) < MAX_FILE_NAME_SIZE);
    strcpy(defaultCxrefFileName, getRealFileNameStatic(normalizeFileName(defaultCxrefFileName, s_cwd)));
    assert(strlen(defaultCxrefFileName) < MAX_FILE_NAME_SIZE);
    s_opt.cxrefFileName = defaultCxrefFileName;
}

static void initializationsPerInvocation(void) {
    int i;
    s_cp = s_cpInit;
    s_cps = s_cpsInit;
    for(i=0; i<SPP_MAX; i++) s_spp[i] = s_noPos;
    s_cxRefFlag=0;
    s_cxRefPos = s_noPos;
    s_olstring[0]=0;
    s_olstringFound = 0;
    s_olstringServed = 0;
    s_olstringInMbody = NULL;
    s_yygstate = s_initYygstate;
    s_jsl = NULL;
    s_javaObjectSymbol = NULL;
}

/*///////////////////////// parsing /////////////////////////////////// */
static void mainParseInputFile(void) {
    if (s_language == LANG_JAVA) {
        uniyylval = & s_yygstate->gyylval;
        java_yyparse();
    }
    else if (s_language == LANG_YACC) {
        //printf("Parsing YACC-file\n");
        uniyylval = & yacc_yylval;
        yacc_yyparse();
    }
    else {
        uniyylval = & c_yylval;
        c_yyparse();
    }
    s_cache.activeCache = 0;
    currentFile.fileName = NULL;

    /*//////////////////////// after parsing actions ////////////////////// */
#if ZERO
    if (s_opt.taskRegime==RegimeXref || s_opt.taskRegime==RegimeHtmlGenerate) {
        int ui,el,mdp,i,cr,ri;
        S_symbol *pp;
        S_symbolRefItem *rfi;
        S_reference *rr;

        fprintf(dumpOut,"olcxMemoryAllocatedBytes == %d\n", olcxMemoryAllocatedBytes);
        //&{static int mp=0; int p; p=s_topBlock->firstFreeIndex*100/SIZE_workMemory;if (p>mp)mp=p;fprintf(dumpOut,": workmax == (%d%%)\n", mp);}
        //&{static int mp=0; int p; p=ppmMemoryi*100/SIZE_ppmMemory;if (p>mp)mp=p;fprintf(dumpOut,": ppmmax == (%d%%) == %d\n", mp, SIZE_ppmMemory/100*mp);}
#if 1 //ZERO
        //& fprintf(dumpOut,"sizeof(S_symbol,S_fileItem,S_symbolRefItem,S_ref) == %d, %d, %d, %d\n",sizeof(S_symbol),sizeof(S_fileItem),sizeof(S_symbolRefItem),sizeof(S_reference));
        //&fprintf(dumpOut,": mem == %d%% == %d/%d %x-%x\n", s_topBlock->firstFreeIndex*100/SIZE_workMemory, s_topBlock->firstFreeIndex,SIZE_workMemory,memory, memory+SIZE_workMemory);
        //& fprintf(dumpOut,": cxmem == %d/%d %x-%x\n",cxMemory->i,cxMemory->size, ((char*)&cxMemory->b),((char*)&cxMemory->b)+cxMemory->size);
        //& fprintf(dumpOut,": ftmem == %d/%d %x-%x\n",ftMemoryi,SIZE_ftMemory,ftMemory,ftMemory+SIZE_ftMemory);
        //&fprintf(dumpOut,": ppmmem == (%d%%) == %d/%d %x-%x\n",ppmMemoryi*100/SIZE_ppmMemory,ppmMemoryi,SIZE_ppmMemory,ppmMemory,ppmMemory+SIZE_ppmMemory);
        //& fprintf(dumpOut,": tmpMem == %d/%d %x-%x\n",tmpWorkMemoryi,SIZE_tmpWorkMemory,tmpWorkMemory,tmpWorkMemory+SIZE_tmpWorkMemory);
        //& fprintf(dumpOut,": cachedLex == %d/%d in %d cps\n",s_cache.lbcc-s_cache.lb,
        //&                     LEX_BUF_CACHE_SIZE,s_cache.cpi);
        //& fprintf(dumpOut,": cache[cpi].lb == %x <-> %x\n",s_cache.lbcc,
        //&         s_cache.cp[s_cache.cpi-1].lbcc);
#endif
#if 1 //ZERO
        symbolTableStatistics(s_symTab, &ui, &el,&mdp);
        fprintf(dumpOut,": symtab == %d elems, usage %d/%d,\tratio %1.2f, maxdepth==%d\n",
                el,ui,s_symTab->size, ((float)el)/(ui+1e-30), mdp);
        fflush(dumpOut);
        refTabStatistics(&s_cxrefTab, &ui, &el,&mdp);
        fprintf(dumpOut,": reftab == %d elems, usage %d/%d,\tratio %1.2f, maxdepth==%d\n",
                el,ui,s_cxrefTab.size, ((float)el)/(ui+1e-30), mdp);
        for(i=0; i< s_cxrefTab.size; i++) {
            cr = 0;
            for(rfi=s_cxrefTab.tab[i]; rfi!=NULL; rfi=rfi->next) {
                cr++;
                ri = 0;
                for(rr=rfi->refs; rr!=NULL; rr=rr->next) ri++;
                if (ri>1000) {
                    fprintf(dumpOut,"\nsymbol %s has %d references", rfi->name, ri);
                }
            }
            if (cr>1000) {
                fprintf(dumpOut,"\n\n>>symbol exceeding 1000");
                fprintf(dumpOut," at index %d: ", i);
                for(rfi=s_cxrefTab.tab[i]; rfi!=NULL; rfi=rfi->next) {
                    fprintf(dumpOut,"%s ", rfi->name);
                }
                fprintf(dumpOut,"\n");
            }
        }
#endif
    }
#endif
}


void mainSetLanguage(char *inFileName, Language *outLanguage) {
    char *suff;
    if (inFileName == NULL
        || fileNameHasOneOfSuffixes(inFileName, s_opt.javaFilesSuffixes)
        || (fnnCmp(simpleFileName(inFileName), "Untitled-", 9)==0)  // jEdit unnamed buffer
        ) {
        *outLanguage = LANG_JAVA;
        typeEnumName[TypeStruct] = "class";
    } else {
        suff = getFileSuffix(inFileName);
        if (compareFileNames(suff,".zip")==0 || compareFileNames(suff,".jar")==0) {
            *outLanguage = LANG_JAR;
        } else if (compareFileNames(suff,".class")==0) {
            *outLanguage = LANG_CLASS;
        } else if (compareFileNames(suff,".y")==0) {
            *outLanguage = LANG_YACC;
            typeEnumName[TypeStruct] = "struct";
#   ifdef CCC_ALLOWED
        } else if (fileNameHasOneOfSuffixes(inFileName, s_opt.cppFilesSuffixes)) {
            *outLanguage = LAN_CCC;
            typeName[TypeStruct] = "class";
#   endif
        } else {
            *outLanguage = LANG_C;
            typeEnumName[TypeStruct] = "struct";
        }
    }
}


static int getLineFromFile(FILE *ff, char *tt, int ttsize, int *outI) {
    int i,c,res;
    i = 0;
    c = getc(ff);
    while ((c>=0 && c<=' ') || c=='\n' || c=='\t') c=getc(ff);
    if (c==EOF) {
        res = EOF;
        goto fini;
    }
    while (c!=EOF && c!='\n') {
        if (i < ttsize-1)   tt[i++]=c;
        c=getc(ff);
    }
    res = 'A';
 fini:
    tt[i] = 0;
    *outI  = i;
    return(res);
}

static void discoverBuiltinIncludePaths(void) {
    char line[MAX_OPTION_LEN];
    int len;
    char *tempfile_name, *lang;
    FILE *tempfile;
    struct stat stt;
    char command[TMP_BUFF_SIZE];
    bool found = false;

    ENTER();
    if (LANGUAGE(LANG_C) || LANGUAGE(LANG_YACC)) {
        lang = "c";
    }
    else {
        LEAVE();
        return;
    }
    tempfile_name = create_temporary_filename();
    assert(strlen(tempfile_name)+1 < MAX_FILE_NAME_SIZE);

    /* Ensure output is in C locale */
    sprintf(command, "LANG=C %s -v -x %s -o /dev/null /dev/null >%s 2>&1", s_opt.compiler, lang, tempfile_name);

    (void)system(command);

    tempfile = openFile(tempfile_name, "r");
    if (tempfile==NULL) return;
    while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF) {
        if (strncmp(line,"#include <...> search starts here:",34)==0) {
            found = true;
            break;
        }
    }
    if (found)
        do {
            if (strncmp(line, "End of search list.", 19) == 0)
                break;
            if (statb(line,&stt) == 0 && (stt.st_mode & S_IFMT) == S_IFDIR) {
                log_trace("Add include '%s'", line);
                mainAddStringListOption(&s_opt.includeDirs, line);
            }
        } while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF);

    closeFile(tempfile);
    removeFile(tempfile_name);
    LEAVE();
}


static char *gnuisms[] = {
                          "__attribute__(xxx)",
                          "__alignof__(xxx) 8",
                          "__typeof__(xxx) int",
                          "__gnuc_va_list void",
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

    if (!(LANGUAGE(LANG_C) || LANGUAGE(LANG_YACC))) {
        return;
    }
    tempfile_name = create_temporary_filename();
    assert(strlen(tempfile_name)+1 < MAX_FILE_NAME_SIZE);

    sprintf(command, "%s -E -dM - >%s 2>&1", s_opt.compiler, tempfile_name);

    /* Need to pipe an empty file into gcc, an alternative would be to
       create an empty file, but that seems as much work */
    FILE *p = popen(command, "w");
    closeFile(p);

    tempfile = openFile(tempfile_name, "r");
    if (tempfile==NULL) return;
    while (getLineFromFile(tempfile, line, MAX_OPTION_LEN, &len) != EOF) {
        if (strncmp(line, "#define", strlen("#define")))
            log_error("Expected #define from compiler standard definitions");
        log_trace("Add definition '%s'", line);
        addMacroDefinedByOption(&line[strlen("#define")+1]);
    }

    /* Also define some GNU-isms */
    for (int i=0; i<sizeof(gnuisms)/sizeof(gnuisms[0]); i++) {
        log_trace("Add definition '%s'", gnuisms[i]);
        addMacroDefinedByOption(gnuisms[i]);
    }
 }

static void getAndProcessXrefrcOptions(char *dffname, char *dffsect,char *project) {
    int dfargc;
    char **dfargv;
    if (*dffname != 0 && s_opt.stdopFlag==0 && !s_opt.no_stdop) {
        readOptionFile(dffname,&dfargc,&dfargv,dffsect,project);
        // warning, the following can overwrite variables like
        // 's_cxref_file_name' allocated in PPM_MEMORY, then when memory
        // is got back by caching, it may provoke a problem
        processOptions(dfargc, dfargv, INFILES_DISABLED); /* .c-xrefrc opts*/
    }
}

static void checkExactPositionUpdate(int message) {
    if (s_opt.update == UP_FAST_UPDATE && s_opt.exactPositionResolve) {
        s_opt.update = UP_FULL_UPDATE;
        if (message) {
            warningMessage(ERR_ST,"-exactpositionresolve implies full update");
        }
    }
}

static void writeProgressInformation(int progress) {
    static int      lastprogress;
    static time_t   timeZero;
    static int      dialogDisplayed = 0;
    static int      initialCall = 1;
    time_t          ct;
    if (progress == 0 || initialCall) {
        initialCall = 0;
        dialogDisplayed = 0;
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
        if (! dialogDisplayed) {
            // display progress bar
            fprintf(stdout,"<%s>0 \n", PPC_PROGRESS);
            dialogDisplayed = 1;
        }
        fprintf(stdout,"<%s>%d \n", PPC_PROGRESS, progress);
        fflush(stdout);
        lastprogress = progress;
    }
}

void writeRelativeProgress(int val) {
    writeProgressInformation((100*s_progressOffset + val)/s_progressFactor);
    if (val==100) s_progressOffset++;
}

static void mainFileProcessingInitialisations(
                                              int *firstPass,
                                              int argc, char **argv,      // command-line options
                                              int nargc, char **nargv,    // piped options
                                              int *outInputIn,
                                              Language *outLanguage
                                              ) {
    char            dffname[MAX_FILE_NAME_SIZE];
    char            dffsect[MAX_FILE_NAME_SIZE];
    struct stat     dffstat;
    char            *fileName;
    S_stringList    *tmpIncludeDirs;

    fileName = s_input_file_name;
    mainSetLanguage(fileName, outLanguage);
    getOptionsFile(fileName, dffname, dffsect,DEFAULT_VALUE);
    initAllInputs();
    if (dffname[0] != 0 ) stat(dffname, &dffstat);
    else dffstat.st_mtime = oldStdopTime;               // !!! just for now
    //&fprintf(dumpOut,"checking oldcp==%s\n",oldOnLineClassPath);
    //&fprintf(dumpOut,"checking newcp==%s\n",s_opt.classpath);
    if (    *firstPass
            || oldCppPass != s_currCppPass
            || strcmp(oldStdopFile,dffname)
            || strcmp(oldStdopSection,dffsect)
            || oldStdopTime != dffstat.st_mtime
            || oldLanguage!= *outLanguage
            || strcmp(oldOnLineClassPath, s_opt.classpath)
            || s_cache.cpi == 1     /* some kind of reset was made */
            ) {
        if (*firstPass) {
            initCaching();
            *firstPass = 0;
        } else {
            recoverCachePointZero();
        }
        strcpy(oldOnLineClassPath, s_opt.classpath);
        assert(strlen(oldOnLineClassPath)<MAX_OPTION_LEN-1);

        s_opt.stdopFlag = 0;

        initPreCreatedTypes();
        initCwd();
        initOptions();
        initDefaultCxrefFileName(fileName);

        processOptions(argc, argv, INFILES_DISABLED);   /* command line opts */
        /* piped options (no include or define options)
           must be befor .xrefrc file options, but, the s_cachedOPtions
           must be set after .c-xrefrc file, but s_cachedOptions can't contain
           piped options, !!! berk.
        */
        {
            copyOptions(&s_tmpOptions, &s_opt);
            processOptions(nargc, nargv, INFILES_DISABLED);
            // get options file once more time, because of -license ???
            // if takes into account the -p option from piped options
            // but copy new project name into old to avoid warning message
            strcpy(oldStdopFile,dffname);
            strcpy(oldStdopSection,dffsect);
            getOptionsFile(fileName, dffname, dffsect,DEFAULT_VALUE);
            //&     s_tmpOptions.setGetEnv = s_opt.setGetEnv; // hack, take new env. vals
            copyOptions(&s_opt, &s_tmpOptions);
        }
        reInitCwd(dffname, dffsect);
        tmpIncludeDirs = s_opt.includeDirs;
        s_opt.includeDirs = NULL;

        getAndProcessXrefrcOptions(dffname, dffsect, dffsect);
        discoverStandardDefines();
        discoverBuiltinIncludePaths();

        LIST_APPEND(S_stringList, s_opt.includeDirs, tmpIncludeDirs);
        if (s_opt.taskRegime != RegimeEditServer && s_input_file_name == NULL) {
            *outInputIn = 0;
            goto fini;
        }
        copyOptions(&s_cachedOptions, &s_opt);  // before getJavaClassPath, it modifies ???
        processOptions(nargc, nargv, INFILES_DISABLED);
        getJavaClassAndSourcePath();
        *outInputIn = computeAndOpenInputFile();
        strcpy(oldStdopFile,dffname);
        strcpy(oldStdopSection,dffsect);
        oldStdopTime = dffstat.st_mtime;
        oldLanguage = *outLanguage;
        oldCppPass = s_currCppPass;
        // this was before 'getAndProcessXrefrcOptions(df...' I hope it will not cause
        // troubles to move it here, because of autodetection of -javaVersion from jdkcp
        initTokenNameTab();
        s_cache.activeCache = 1;
        placeCachePoint(0);
        s_cache.activeCache = 0;
        assert(s_cache.lbcc == s_cache.cp[0].lbcc);
        assert(s_cache.lbcc == s_cache.cp[1].lbcc);
    } else {
        copyOptions(&s_opt, &s_cachedOptions);
        processOptions(nargc, nargv, INFILES_DISABLED); /* no include or define options */
        *outInputIn = computeAndOpenInputFile();
    }
    // reset language once knowing all language suffixes
    mainSetLanguage(fileName,  outLanguage);
    s_input_file_number = currentFile.lexBuffer.buffer.fileNumber;
    assert(s_opt.taskRegime);
    if (    (s_opt.taskRegime==RegimeXref
             || s_opt.taskRegime==RegimeHtmlGenerate)
            && (! s_javaPreScanOnly)) {
        if (s_opt.xref2) {
            ppcGenRecord(PPC_INFORMATION, getRealFileNameStatic(s_input_file_name), "\n");
        } else {
            log_info("Processing '%s'", getRealFileNameStatic(s_input_file_name));
        }
    }
 fini:
    initializationsPerInvocation();
    // some final touch to options
    if (s_opt.keep_old) {
        s_opt.update = UP_FAST_UPDATE;
        s_opt.updateOnlyModifiedFiles = false;
    }
    if (s_opt.debug) errOut = dumpOut;
    checkExactPositionUpdate(0);
    // so s_input_file_number is not set if the file is not really opened!!!
}

static void createXrefrcDefaultLicense(void) {
    char            fn[MAX_FILE_NAME_SIZE];
    struct stat     st;
    FILE            *ff;

    getXrefrcFileName(fn);
    if (stat(fn, &st)!=0) {
        // does not exists
        ff = openFile(fn,"w");
        if (ff == NULL) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "home directory %s does not exists", fn);
            fatalError(ERR_ST,tmpBuff, XREF_EXIT_ERR);
        } else {
            closeFile(ff);
        }
    }
}

static int power(int x, int y) {
    int i,res = 1;
    for(i=0; i<y; i++) res *= x;
    return(res);
}

static bool optionsOverflowHandler(int n) {
    fatalError(ERR_NO_MEMORY, "opiMemory", XREF_EXIT_ERR);
    return(1);
}

static void mainTotalTaskEntryInitialisations(int argc, char **argv) {
    int mm;

    errOut = stderr;
    dumpOut = stdout;

    s_fileAbortionEnabled = 0;
    cxOut = stdout;
    ccOut = stdout;
    if (s_opt.taskRegime == RegimeEditServer) errOut = stdout;

    assert(MAX_TYPE < power(2,SYMTYPES_LN));
    assert(MAX_STORAGE < power(2,STORAGES_LN));
    assert(MAX_SCOPES < power(2,SCOPES_LN));
    assert(MAX_REQUIRED_ACCESS < power(2, MAX_REQUIRED_ACCESS_LN));

    assert(PPC_MAX_AVAILABLE_REFACTORINGS < MAX_AVAILABLE_REFACTORINGS);

    // initialize cxMemory
    mm = cxMemoryOverflowHandler(1);
    assert(mm);
    // initoptions
    initMemory(((S_memory*)&s_initOpt.pendingMemory),
               optionsOverflowHandler, SIZE_opiMemory);

    // just for very beginning
    s_fileProcessStartTime = time(NULL);

    // following will be displayed only at third pass or so, because
    // s_opt.debug is set only after passing through option processing
    log_debug("Initialisations.");
    memset(&s_count, 0, sizeof(S_counters));
    s_opt.includeDirs = NULL;
    SM_INIT(ftMemory);
    FT_ALLOCC(s_fileTab.tab, MAX_FILES, struct fileItem *);
    initFileTab(&s_fileTab);
    fillPosition(&s_noPos, s_noneFileIndex, 0, 0);
    fillUsageBits(&s_noUsage, UsageNone, 0);
    fill_reference(&s_noRef, s_noUsage, s_noPos, NULL);
    s_input_file_number = s_noneFileIndex;
    s_javaAnonymousClassName.p = s_noPos;
    olcxInit();
    editorInit();
}

static void mainReinitFileTabEntry(FileItem *ft) {
    ft->inferiorClasses = ft->superClasses = NULL;
    ft->directEnclosingInstance = s_noneFileIndex;
    ft->b.scheduledToProcess = false;
    ft->b.scheduledToUpdate = false;
    ft->b.fullUpdateIncludesProcessed = false;
    ft->b.cxLoaded = ft->b.cxLoading = ft->b.cxSaved = false;
}

void mainTaskEntryInitialisations(int argc, char **argv) {
    char        tt[MAX_FILE_NAME_SIZE];
    char        dffname[MAX_FILE_NAME_SIZE];
    char        dffsect[MAX_FILE_NAME_SIZE];
    char        *ss;
    int         dfargc;
    char        **dfargv;
    int         argcount;
    char        *sss,*cmdlnInputFile;
    int         inmode, noerropt;
    static int  firstmemory=0;

    s_fileAbortionEnabled = 0;

    // supposing that file table is still here, but reinit it
    fileTabMap(&s_fileTab, mainReinitFileTabEntry);

    DM_INIT(cxMemory);
    // the following causes long jump, berk.
    CX_ALLOCC(sss, CX_MEMORY_CHUNK_SIZE, char);
    CX_FREE_UNTIL(sss);
    CX_ALLOCC(s_cxrefTab.tab,MAX_CXREF_SYMBOLS, struct symbolReferenceItem *);
    refTabNoAllocInit( &s_cxrefTab,MAX_CXREF_SYMBOLS);
    if (firstmemory==0) {firstmemory=1;}
    SM_INIT(ppmMemory);
    ppMemInit();
    stackMemoryInit();

    // init options as soon as possible! for exampl initCwd needs them
    initOptions();

    /* TODO: should go into a newSymbolTable() function... */
    XX_ALLOC(s_symbolTable, S_symbolTable);
    symbolTableInit(s_symbolTable, MAX_SYMBOLS);
    fillJavaStat(&s_initJavaStat,NULL,NULL,NULL,0, NULL, NULL, NULL,
                  s_symbolTable,NULL,ACCESS_DEFAULT,s_cpInit,s_noneFileIndex,NULL);
    XX_ALLOC(s_javaStat, S_javaStat);
    *s_javaStat = s_initJavaStat;
    javaFqtTabInit( &s_javaFqtTab, FQT_CLASS_TAB_SIZE);
    // initialize recursive java parsing
    XX_ALLOC(s_yygstate, struct yyGlobalState);
    memset(s_yygstate, 0, sizeof(struct yyGlobalState));
    s_initYygstate = s_yygstate;

    initAllInputs();
    oldStdopFile[0] = 0;    oldStdopSection[0] = 0;
    initCwd();
    initTypeCharCodeTab();
    //initTypeModifiersTabs();
    initJavaTypePCTIConvertIniTab();
    initTypesNamesTab();
    initExtractStoragesNameTab();
    setupCaching();
    initArchaicTypes();
    oldStdopFile[0] = oldStdopSection[0] = 0;
    /* now pre-read the option file */
    processOptions(argc, argv, INFILES_ENABLED);
    mainScheduleInputFilesFromOptionsToFileTable();
    if (s_opt.refactoringRegime == RegimeRefactory) {
        // some more memory for refactoring task
        assert(s_opt.cxMemoryFactor>=1);
        CX_ALLOCC(sss, 6*s_opt.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE, char);
        CX_FREE_UNTIL(sss);
    }
    if (s_opt.taskRegime==RegimeXref || s_opt.taskRegime==RegimeHtmlGenerate) {
        // get some memory if cross referencing
        assert(s_opt.cxMemoryFactor>=1);
        CX_ALLOCC(sss, 3*s_opt.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE, char);
        CX_FREE_UNTIL(sss);
    }
    if (s_opt.cxMemoryFactor > 1) {
        // reinit cxmemory taking into account -mf
        // just make an allocation provoking resizing
        CX_ALLOCC(sss, s_opt.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE, char);
        CX_FREE_UNTIL(sss);
    }
    // must be after processing command line options
    createXrefrcDefaultLicense();
    initCaching();
    // enclosed in cache point, because of persistent #define in XrefEdit
    argcount = 0;
    s_input_file_name = cmdlnInputFile = getCommandLineFile(&argcount);
    if (s_input_file_name==NULL) {
        ss = strmcpy(tt, s_cwd);
        if (ss!=tt && ss[-1] == FILE_PATH_SEPARATOR) ss[-1]=0;
        assert(strlen(tt)+1<MAX_FILE_NAME_SIZE);
        s_input_file_name=tt;
        //&fprintf(dumpOut,"here we are '%s'\n",tt);fflush(dumpOut);
    } else {
        strcpy(tt, s_input_file_name);
    }
    getOptionsFile(tt, dffname, dffsect, NO_ERROR_MESSAGE);
    reInitCwd(dffname, dffsect);
    if (dffname[0]!=0) {
        readOptionFile(dffname, &dfargc, &dfargv, dffsect, dffsect);
        if (s_opt.refactoringRegime == RegimeRefactory) {
            inmode = INFILES_DISABLED;
        } else if (s_opt.taskRegime==RegimeEditServer) {
            inmode = INFILES_DISABLED;
        } else if (s_opt.create || s_opt.project!=NULL || s_opt.update != UP_CREATE) {
            inmode = INFILES_ENABLED;
        } else {
            inmode = INFILES_DISABLED;
        }
        // disable error reporting on xref task on this pre-reading of .c-xrefrc
        noerropt = s_opt.noErrors;
        if (s_opt.taskRegime==RegimeEditServer) {
            s_opt.noErrors = 1;
        }
        // there is a problem with INFILES_ENABLED (update for safetycheck),
        // It should first load cxref file, in order to protect file numbers.
        if (inmode==INFILES_ENABLED && s_opt.update && !s_opt.create) {
            //&fprintf(dumpOut,"PREREADING !!!!!!!!!!!!!!!!\n");
            // this makes a problem: I need to preread cxref file before
            // reading input files in order to preserve hash numbers, but
            // I need to read options first in order to have the name
            // of cxref file.
            // I need to read fstab also to remove removed files on update
            processOptions(dfargc, dfargv, INFILES_DISABLED);
            smartReadFileTabFile();
        }
        processOptions(dfargc, dfargv, inmode);
        // recover value of errors messages
        if (s_opt.taskRegime==RegimeEditServer)
            s_opt.noErrors = noerropt;
        checkExactPositionUpdate(0);
        if (inmode == INFILES_ENABLED)
            mainScheduleInputFilesFromOptionsToFileTable();
    }
    recoverCachePointZero();

    s_opt.stdopFlag = 0;
    //& getJavaClassAndSourcePath();
    initCaching();

    log_debug("Leaving all task initialisations.");
}

static void mainReferencesOverflowed(char *cxMemFreeBase, int mess) {
    int i,fi,savingFlag;

    if (mess!=MESS_NONE && s_opt.taskRegime!=RegimeHtmlGenerate) {
        if (s_opt.xref2) {
            ppcGenRecord(PPC_INFORMATION,"swapping references on disk", "\n");
            ppcGenRecord(PPC_INFORMATION,"", "\n");
        } else {
            fprintf(dumpOut,"\nswapping references on disk (please wait)\n");
            fflush(dumpOut);
        }
    }
    if (s_opt.cxrefFileName == NULL) {
        fatalError(ERR_ST,"sorry no file for cxrefs, use -refs option", XREF_EXIT_ERR);
    }
    for(i=0; i<inStacki; i++) {
        if (inStack[i].lexBuffer.buffer.file != stdin) {
            fi = inStack[i].lexBuffer.buffer.fileNumber;
            assert(s_fileTab.tab[fi]);
            s_fileTab.tab[fi]->b.cxLoading = false;
            if (inStack[i].lexBuffer.buffer.file!=NULL)
                closeCharacterBuffer(&inStack[i].lexBuffer.buffer);
        }
    }
    if (currentFile.lexBuffer.buffer.file != stdin) {
        fi = currentFile.lexBuffer.buffer.fileNumber;
        assert(s_fileTab.tab[fi]);
        s_fileTab.tab[fi]->b.cxLoading = false;
        if (currentFile.lexBuffer.buffer.file!=NULL)
            closeCharacterBuffer(&currentFile.lexBuffer.buffer);
    }
    if (s_opt.taskRegime==RegimeHtmlGenerate) {
        if (s_opt.noCxFile) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"cross-references overflowed, use -mf<n> option");
            fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
        }
        generateHtml();
    }
    if (s_opt.taskRegime==RegimeXref) mainGenerateReferenceFile();
    recoverMemoriesAfterOverflow(cxMemFreeBase);
    /* ************ start with CXREFS and memories clean ************ */
    savingFlag = 0;
    for(i=0; i<s_fileTab.size; i++) {
        if (s_fileTab.tab[i]!=NULL) {
            if (s_fileTab.tab[i]->b.cxLoading) {
                s_fileTab.tab[i]->b.cxLoading = false;
                s_fileTab.tab[i]->b.cxSaved = 1;
                if (s_fileTab.tab[i]->b.commandLineEntered
                    || !s_opt.multiHeadRefsCare) savingFlag = 1;
                // before, but do not work as scheduledToProcess is auto-cleared
                //&             if (s_fileTab.tab[i]->b.scheduledToProcess
                //&                 || !s_opt.multiHeadRefsCare) savingFlag = 1;
                //&fprintf(dumpOut," -># '%s'\n",s_fileTab.tab[i]->name);fflush(dumpOut);
            }
        }
    }
    if (savingFlag==0 && mess!=MESS_FILE_ABORT) {
        /* references overflowed, but no whole file readed */
        fatalError(ERR_NO_MEMORY,"cxMemory", XREF_EXIT_ERR);
    }
}

void getPipedOptions(int *outNargc,char ***outNargv){
    char nsect[MAX_FILE_NAME_SIZE];
    int c;
    *outNargc = 0;
    assert(s_opt.taskRegime);
    if (s_opt.taskRegime == RegimeEditServer) {
        readOptionFromFile(stdin,outNargc,outNargv,MEM_ALLOC_ON_SM,
                           "",NULL,nsect);
        /* those options can't contain include or define options, */
        /* sections neither */
        c = getc(stdin);
        if (c==EOF) {
            /* Just log and exit since we don't know if there is someone there... */
            /* We also want a clean exit() if we are going for coverage */
            log_error("Broken pipe");
            exit(-1);
            fatalError(ERR_INTERNAL, "broken input pipe", XREF_EXIT_ERR);
        }
    }
}

static void fillIncludeRefItem( SymbolReferenceItem *ddd , int fnum) {
    fillSymbolRefItemExceptBits(ddd, LINK_NAME_INCLUDE_REFS,
                                cxFileHashNumber(LINK_NAME_INCLUDE_REFS),
                                fnum, fnum);
    fillSymbolRefItemBits(&ddd->b, TypeCppInclude, StorageExtern,
                           ScopeGlobal, ACCESS_DEFAULT, CategoryGlobal,0);
}

static void makeIncludeClosureOfFilesToUpdate(void) {
    char                *cxFreeBase;
    int                 i,ii,fileAddedFlag, isJavaFileFlag;
    FileItem          *fi,*includer;
    SymbolReferenceItem     ddd,*memb;
    Reference         *rr;
    CX_ALLOCC(cxFreeBase,0,char);
    readOneAppropReferenceFile(LINK_NAME_INCLUDE_REFS,
                               fullUpdateFunctionSequence); // get include refs
    // iterate over scheduled files
    fileAddedFlag = 1;
    while (fileAddedFlag) {
        fileAddedFlag = 0;
        for(i=0; i<s_fileTab.size; i++) {
            fi = s_fileTab.tab[i];
            if (fi!=NULL && fi->b.scheduledToUpdate
                && !fi->b.fullUpdateIncludesProcessed) {
                fi->b.fullUpdateIncludesProcessed = true;
                isJavaFileFlag = fileNameHasOneOfSuffixes(fi->name, s_opt.javaFilesSuffixes);
                fillIncludeRefItem( &ddd, i);
                if (refTabIsMember(&s_cxrefTab, &ddd, &ii,&memb)) {
                    for(rr=memb->refs; rr!=NULL; rr=rr->next) {
                        includer = s_fileTab.tab[rr->p.file];
                        assert(includer);
                        if (!includer->b.scheduledToUpdate) {
                            includer->b.scheduledToUpdate = true;
                            fileAddedFlag = 1;
                            if (isJavaFileFlag) {
                                // no transitive closure for Java
                                includer->b.fullUpdateIncludesProcessed = true;
                            }
                        }
                    }
                }
            }
        }
    }
    recoverMemoriesAfterOverflow(cxFreeBase);
}

static void scheduleModifiedFilesToUpdate(void) {
    char        ttt[MAX_FILE_NAME_SIZE];
    char        *filestab;
    struct stat refStat;
    char        *suffix;
    checkExactPositionUpdate(1);
    if (s_opt.referenceFileCount <= 1) {
        suffix = "";
        filestab = s_opt.cxrefFileName;
    } else {
        suffix = REFERENCE_FILENAME_FILES;
        sprintf(ttt,"%s%s", s_opt.cxrefFileName, suffix);
        assert(strlen(ttt) < MAX_FILE_NAME_SIZE-1);
        filestab = ttt;
    }
    if (statb(filestab, &refStat)) refStat.st_mtime = 0;
    scanReferenceFile(s_opt.cxrefFileName, suffix,"", normalScanFunctionSequence);
    fileTabMap2(&s_fileTab, schedulingToUpdate, &refStat);
    if (s_opt.update==UP_FULL_UPDATE /*& && !LANGUAGE(LANG_JAVA) &*/) {
        makeIncludeClosureOfFilesToUpdate();
    }
    fileTabMap(&s_fileTab, schedulingUpdateToProcess);
}


void mainOpenOutputFile(char *ofile) {
    closeMainOutputFile();
    if (ofile!=NULL) {
        //&fprintf(dumpOut,"OPENING OUTPUT FILE %s\n", s_opt.outputFileName);
#if defined (__WIN32__)
        // open it as binary file, so that record lengths will be correct
        ccOut = openFile(ofile,"wb");
#else
        ccOut = openFile(ofile,"w");
#endif
    } else {
        ccOut = stdout;
    }
    if (ccOut == NULL) {
        errorMessage(ERR_CANT_OPEN, ofile);
        ccOut = stdout;
    }
    errOut = ccOut;
    dumpOut = ccOut;
}

static int scheduleFileUsingTheMacro(void) {
    SymbolReferenceItem     ddd;
    S_olSymbolsMenu     mm, *oldMenu;
    S_olcxReferences    *tmpc;
    assert(s_olstringInMbody);
    tmpc = NULL;
    fillSymbolRefItemExceptBits(&ddd, s_olstringInMbody,
                                cxFileHashNumber(s_olstringInMbody),
                                s_noneFileIndex, s_noneFileIndex);
    fillSymbolRefItemBits(&ddd.b, TypeMacro, StorageExtern,
                           ScopeGlobal, ACCESS_DEFAULT, CategoryGlobal, 0);

    //& rr = refTabIsMember(&s_cxrefTab, &ddd, &ii, &memb);
    //& assert(rr);
    //& if (rr==0) return(s_noneFileIndex);

    fill_olSymbolsMenu(&mm, ddd, 1,1,0,UsageUsed,0,0,0,UsageNone,s_noPos,0, NULL, NULL);
    if (s_olcxCurrentUser==NULL || s_olcxCurrentUser->browserStack.top==NULL) {
        olcxSetCurrentUser(s_opt.user);
        olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
        assert(s_olcxCurrentUser);
        tmpc = s_olcxCurrentUser->browserStack.top;
    }
    assert(s_olcxCurrentUser && s_olcxCurrentUser->browserStack.top);
    oldMenu = s_olcxCurrentUser->browserStack.top->menuSym;
    s_olcxCurrentUser->browserStack.top->menuSym = &mm;
    s_olMacro2PassFile = s_noneFileIndex;
    //&fprintf(dumpOut,":here I am, looking for usage of %s\n",memb->name);
    readOneAppropReferenceFile(s_olstringInMbody,secondPassMacroUsageFunctionSequence);
    s_olcxCurrentUser->browserStack.top->menuSym = oldMenu;
    if (tmpc!=NULL) {
        olStackDeleteSymbol(tmpc);
    }
    //&fprintf(dumpOut,":scheduling file %s\n", s_fileTab.tab[s_olMacro2PassFile]->name); fflush(dumpOut);
    if (s_olMacro2PassFile == s_noneFileIndex) return(s_noneFileIndex);
    return(s_olMacro2PassFile);
}

// this is necessary to put new mtimies for header files
static void setFullUpdateMtimesInFileTab(FileItem *fi) {
    if (fi->b.scheduledToUpdate || s_opt.create) {
        fi->lastFullUpdateMtime = fi->lastModified;
    }
}

static void mainCloseInputFile(int inputIn ) {
    if (inputIn) {
        if (currentFile.lexBuffer.buffer.file!=stdin) {
            closeCharacterBuffer(&currentFile.lexBuffer.buffer);
        }
    }
}

static void mainEditSrvParseInputFile(int *firstPassing, int inputIn ) {
    //&fprintf(dumpOut,":here I am %s\n",s_fileTab.tab[s_input_file_number]->name);
    if (inputIn) {
        //&fprintf(dumpOut,"parse start\n");fflush(dumpOut);
        if (s_opt.server_operation!=OLO_TAG_SEARCH && s_opt.server_operation!=OLO_PUSH_NAME) {
            recoverFromCache();
            mainParseInputFile();
            //&fprintf(dumpOut,"parse stop\n");fflush(dumpOut);
            *firstPassing = 0;
        }
        currentFile.lexBuffer.buffer.isAtEOF = false;
        mainCloseInputFile(inputIn);
    }
}

static bool mainSymbolCanBeIdentifiedByPosition(int fnum) {
    int line,col;

    // there is a serious problem with options memory for options got from
    // the .c-xrefrc file. so for the moment this will not work.
    // which problem ??????
    // seems that those options are somewhere in ppmMemory overwritten?
    //&return(0);
    if (!creatingOlcxRefs()) return(0);
    if (s_opt.browsedSymName == NULL) return(0);
    log_trace("looking for sym %s on %s",s_opt.browsedSymName,s_opt.olcxlccursor);
    // modified file, can't identify the reference
    log_trace(":modif flag == %d", s_opt.modifiedFlag);
    if (s_opt.modifiedFlag)
        return false;

    // here I will need also the symbol name
    // do not bypass commanline entered files, because of local symbols
    // and because references from currently procesed file would
    // be not loaded from the TAG file (it expects they are loaded
    // by parsing).
    log_trace("checking if cmd %s, == %d\n", s_fileTab.tab[fnum]->name,s_fileTab.tab[fnum]->b.commandLineEntered);
    if (s_fileTab.tab[fnum]->b.commandLineEntered)
        return false;

    // if references are not updated do not search it here
    // there were fullUpdate time? why?
    //&fprintf(dumpOut,"checking that lastmodif %d, == %d\n", s_fileTab.tab[fnum]->lastModified, s_fileTab.tab[fnum]->lastUpdateMtime);
    if (s_fileTab.tab[fnum]->lastModified!=s_fileTab.tab[fnum]->lastUpdateMtime)
        return false;

    // here read one reference file looking for the refs
    // assume s_opt.olcxlccursor is correctly set;
    getLineColCursorPositionFromCommandLineOption( &line, &col);
    fillPosition(&s_olcxByPassPos, fnum, line, col);
    olSetCallerPosition(&s_olcxByPassPos);
    readOneAppropReferenceFile(s_opt.browsedSymName, byPassFunctionSequence);
    // if no symbol found, it may be a local symbol, try by parsing
    log_trace("checking that %d, != NULL", s_olcxCurrentUser->browserStack.top->hkSelectedSym);
    if (s_olcxCurrentUser->browserStack.top->hkSelectedSym==NULL)
        return false;

    // here I should set caching to 1 and recover the cachePoint ???
    // yes, because last file references are still stored, even if I
    // update the cxref file, so do it only if switching file?
    // but how to ensure that next pass will start parsing?
    // By recovering of point 0 handled as such a special case.
    recoverCachePointZero();
    log_trace("yes, it can be identified by position");

    return true;
}

static void mainEditSrvFileSingleCppPass( int argc, char **argv,
                                          int nargc, char **nargv,
                                          int *firstPassing
                                          ) {
    int inputIn;
    int ol2procfile;

    inputIn = 0;
    s_olStringSecondProcessing = 0;
    mainFileProcessingInitialisations(firstPassing, argc, argv,
                                      nargc, nargv, &inputIn, &s_language);
    smartReadFileTabFile();
    s_olOriginalFileNumber = s_input_file_number;
    if (mainSymbolCanBeIdentifiedByPosition(s_input_file_number)) {
        mainCloseInputFile(inputIn);
        return;
    }
    mainEditSrvParseInputFile( firstPassing, inputIn);
    if (s_opt.olCursorPos==0 && !LANGUAGE(LANG_JAVA)) {
        // special case, push the file as include reference
        if (creatingOlcxRefs()) {
            Position dpos;
            fillPosition(&dpos, s_input_file_number, 1, 0);
            gotOnLineCxRefs(&dpos);
        }
        addThisFileDefineIncludeReference(s_input_file_number);
    }
    if (s_olstringFound && s_olstringServed==0) {
        // on-line action with cursor in an un-used macro body ???
        ol2procfile = scheduleFileUsingTheMacro();
        if (ol2procfile!=s_noneFileIndex) {
            s_input_file_name = s_fileTab.tab[ol2procfile]->name;
            inputIn = 0;
            s_olStringSecondProcessing=1;
            mainFileProcessingInitialisations(firstPassing, argc, argv,
                                              nargc, nargv, &inputIn, &s_language);
            mainEditSrvParseInputFile( firstPassing, inputIn);
        }
    }
}


static void mainEditServerProcessFile( int argc, char **argv,
                                       int nargc, char **nargv,
                                       int *firstPassing
                                       ) {
    assert(s_fileTab.tab[s_olOriginalComFileNumber]->b.scheduledToProcess);
    s_cppPassMax = 1;           /* WTF? */
    s_currCppPass = 1;
    for(s_currCppPass=1; s_currCppPass<=s_cppPassMax; s_currCppPass++) {
        s_input_file_name = s_fileTab.tab[s_olOriginalComFileNumber]->name;
        assert(s_input_file_name!=NULL);
        mainEditSrvFileSingleCppPass( argc, argv, nargc, nargv, firstPassing);
        if (s_opt.server_operation==OLO_EXTRACT
            || (s_olstringServed && ! creatingOlcxRefs()))
            goto fileParsed; /* TODO: break? */
        if (LANGUAGE(LANG_JAVA))
            goto fileParsed;
    }
 fileParsed:
    s_fileTab.tab[s_olOriginalComFileNumber]->b.scheduledToProcess = false;
}

static char *presetEditServerFileDependingStatics(void) {
    int     i, fArgCount;
    char    *fileName;
    s_fileProcessStartTime = time(NULL);
    //&s_paramPosition = s_noPos;
    //&s_paramBeginPosition = s_noPos;
    //&s_paramEndPosition = s_noPos;
    s_primaryStartPosition = s_noPos;
    s_staticPrefixStartPosition = s_noPos;
    // THIS is pretty stupid, there is always only one input file
    // in edit server, otherwise it is an eror
    fArgCount = 0; s_input_file_name = getInputFile(&fArgCount);
    if (fArgCount>=s_fileTab.size) {
        // conservative message, probably macro invoked on nonsaved file
        s_olOriginalComFileNumber = s_noneFileIndex;
        return(NULL);
    }
    assert(fArgCount>=0 && fArgCount<s_fileTab.size && s_fileTab.tab[fArgCount]->b.scheduledToProcess);
    for(i=fArgCount+1; i<s_fileTab.size; i++) {
        if (s_fileTab.tab[i] != NULL) {
            s_fileTab.tab[i]->b.scheduledToProcess = false;
        }
    }
    s_olOriginalComFileNumber = fArgCount;
    fileName = s_input_file_name;
    mainSetLanguage(fileName,  &s_language);
    // O.K. just to be sure, there is no other input file
    return(fileName);
}


static int needToProcessInputFile(void) {
    return(
           s_opt.server_operation==OLO_COMPLETION
           || s_opt.server_operation==OLO_SEARCH
           || s_opt.server_operation==OLO_EXTRACT
           || s_opt.server_operation==OLO_TAG_SEARCH
           || s_opt.server_operation==OLO_SET_MOVE_TARGET
           || s_opt.server_operation==OLO_SET_MOVE_CLASS_TARGET
           || s_opt.server_operation==OLO_SET_MOVE_METHOD_TARGET
           || s_opt.server_operation==OLO_GET_CURRENT_CLASS
           || s_opt.server_operation==OLO_GET_CURRENT_SUPER_CLASS
           || s_opt.server_operation==OLO_GET_METHOD_COORD
           || s_opt.server_operation==OLO_GET_CLASS_COORD
           || s_opt.server_operation==OLO_GET_ENV_VALUE
           || creatingOlcxRefs()
           );
}


/* *************************************************************** */
/*                          Xref regime                            */
/* *************************************************************** */
static void mainXrefProcessInputFile(int argc, char **argv, int *_inputIn, int *_firstPassing, int *_atLeastOneProcessed ) {
    int inputIn = *_inputIn;
    int firstPassing = *_firstPassing;
    int atLeastOneProcessed = *_atLeastOneProcessed;
    s_cppPassMax = 1;
    for(s_currCppPass=1; s_currCppPass<=s_cppPassMax; s_currCppPass++) {
        if (! firstPassing) copyOptions(&s_opt, &s_cachedOptions);
        mainFileProcessingInitialisations(&firstPassing,
                                          argc, argv, 0, NULL, &inputIn,
                                          &s_language);
        s_olOriginalFileNumber = s_input_file_number;
        s_olOriginalComFileNumber = s_olOriginalFileNumber;
        if (inputIn) {
            recoverFromCache();
            s_cache.activeCache = 0;    /* no caching in cxref */
            mainParseInputFile();
            closeCharacterBuffer(&currentFile.lexBuffer.buffer);
            inputIn = 0;
            currentFile.lexBuffer.buffer.file = stdin;
            atLeastOneProcessed=1;
        } else if (LANGUAGE(LANG_JAR)) {
            jarFileParse(s_input_file_name);
            atLeastOneProcessed=1;
        } else if (LANGUAGE(LANG_CLASS)) {
            classFileParse();
            atLeastOneProcessed=1;
        } else {
            errorMessage(ERR_CANT_OPEN,s_input_file_name);
            fprintf(dumpOut,"\tmaybe forgotten -p option?\n");
        }
        // no multiple passes for java programs
        firstPassing = 0;
        currentFile.lexBuffer.buffer.isAtEOF = false;
        if (LANGUAGE(LANG_JAVA)) goto fileParsed;
    }

 fileParsed:
    *_inputIn = inputIn;
    *_firstPassing = firstPassing;
    *_atLeastOneProcessed = atLeastOneProcessed;
}

static void mainXrefOneWholeFileProcessing(int argc, char **argv,
                                           FileItem *ff,
                                           int *firstPassing, int *atLeastOneProcessed) {
    int         inputIn;
    s_input_file_name = ff->name;
    s_fileProcessStartTime = time(NULL);
    // O.K. but this is missing all header files
    ff->lastUpdateMtime = ff->lastModified;
    if (s_opt.update == UP_FULL_UPDATE || s_opt.create) {
        ff->lastFullUpdateMtime = ff->lastModified;
    }
    mainXrefProcessInputFile(argc, argv, &inputIn,
                             firstPassing, atLeastOneProcessed);
    // now free the buffer because it tooks too much memory,
    // but I can not free it when refactoring, nor when preloaded,
    // so be very carefull about this!!!
    if (s_ropt.refactoringRegime!=RegimeRefactory) {
        editorCloseBufferIfClosable(s_input_file_name);
        if (! s_opt.cacheIncludes) editorCloseAllBuffersIfClosable();
    }
}

static void printPrescanningMessage(void) {
    if (s_opt.xref2) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Prescanning classes, please wait.");
        ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
    } else {
        log_info("Prescanning classes, please wait.");
    }
}

static bool inputFileItemLess(FileItem *f1, FileItem *f2) {
    int cc;
    char dd1[MAX_FILE_NAME_SIZE];
    char dd2[MAX_FILE_NAME_SIZE];
    // first compare directory
    strcpy(dd1, directoryName_st(f1->name));
    strcpy(dd2, directoryName_st(f2->name));
    cc = strcmp(dd1, dd2);
    if (cc<0) return true;
    if (cc>0) return false;
    // then full file name
    cc = strcmp(f1->name, f2->name);
    if (cc<0) return true;
    if (cc>0) return false;
    return false;
}

static FileItem *mainCreateListOfInputFiles(void) {
    FileItem *res;
    char *nn;
    int n;
    res = NULL;
    n = 0;
    for(nn=getInputFile(&n); nn!=NULL; n++,nn=getInputFile(&n)) {
        s_fileTab.tab[n]->next = res;
        res = s_fileTab.tab[n];
    }
    LIST_MERGE_SORT(FileItem, res, inputFileItemLess);
    return(res);
}

void mainCallXref(int argc, char **argv) {
    static char *cxFreeBase0, *cxFreeBase;
    static int firstPassing, mess, atLeastOneProcessed;
    static FileItem *ffc, *pffc;
    static int messagePrinted = 0;
    static int numberOfInputs, inputCounter, pinputCounter;

    s_currCppPass = ANY_CPP_PASS;
    CX_ALLOCC(cxFreeBase0,0,char);
    if (s_opt.taskRegime == RegimeHtmlGenerate && ! s_opt.noCxFile) {
        htmlGetDefinitionReferences();
    }
    CX_ALLOCC(cxFreeBase,0,char);
    s_cxResizingBlocked = 1;
    if (s_opt.update) scheduleModifiedFilesToUpdate();
    atLeastOneProcessed = 0;
    ffc = pffc = mainCreateListOfInputFiles();
    inputCounter = pinputCounter = 0;
    LIST_LEN(numberOfInputs, FileItem, ffc);
    for(;;) {
        s_currCppPass = ANY_CPP_PASS;
        firstPassing = 1;
        if ((mess=setjmp(cxmemOverflow))!=0) {
            mainReferencesOverflowed(cxFreeBase,mess);
            if (mess==MESS_FILE_ABORT) {
                if (pffc!=NULL) pffc=pffc->next;
                else if (ffc!=NULL) ffc=ffc->next;
            }
        } else {
            s_javaPreScanOnly = 1;
            for(; pffc!=NULL; pffc=pffc->next) {
                if (! messagePrinted) {
                    printPrescanningMessage();
                    messagePrinted = 1;
                }
                mainSetLanguage(pffc->name, &s_language);
                if (LANGUAGE(LANG_JAVA)) {
                    mainXrefOneWholeFileProcessing(argc, argv, pffc, &firstPassing, &atLeastOneProcessed);
                }
                if (s_opt.xref2) writeRelativeProgress(10*pinputCounter/numberOfInputs);
                pinputCounter++;
            }
            s_javaPreScanOnly = 0;
            s_fileAbortionEnabled = 1;
            for(; ffc!=NULL; ffc=ffc->next) {
                mainXrefOneWholeFileProcessing(argc, argv, ffc, &firstPassing, &atLeastOneProcessed);
                ffc->b.scheduledToProcess = false;
                ffc->b.scheduledToUpdate = false;
                if (s_opt.xref2) writeRelativeProgress(10+90*inputCounter/numberOfInputs);
                inputCounter++;
                CHECK_FINAL();
            }
            goto regime1fini;
        }
    }
 regime1fini:
    s_fileAbortionEnabled = 0;
    if (atLeastOneProcessed) {
        if (s_opt.taskRegime==RegimeHtmlGenerate) {
            // following is for case if an internalCheckFail, will rejump here
            atLeastOneProcessed = 0;
            generateHtml();
            if (s_opt.noCxFile) CX_ALLOCC(cxFreeBase0,0,char);
            htmlGenGlobalReferenceLists(cxFreeBase0);
            //& if (s_opt.htmlglobalx || s_opt.htmllocalx) htmlGenEmptyRefsFile();
        }
        if (s_opt.taskRegime==RegimeXref) {
            if (s_opt.update==0 || s_opt.update==UP_FULL_UPDATE) {
                fileTabMap(&s_fileTab, setFullUpdateMtimesInFileTab);
            }
            if (s_opt.xref2) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "Generating '%s'",s_opt.cxrefFileName);
                ppcGenRecord(PPC_INFORMATION, tmpBuff, "\n");
            } else {
                log_info("Generating '%s'",s_opt.cxrefFileName);
            }
            mainGenerateReferenceFile();
        }
    } else if (s_opt.server_operation == OLO_ABOUT) {
        aboutMessage();
    } else if (! s_opt.update)  {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"no input file");
        errorMessage(ERR_ST, tmpBuff);
    }
    if (s_opt.xref2) {
        writeRelativeProgress(100);
    }
}


static void mainXref(int argc, char **argv) {
    ENTER();
    mainOpenOutputFile(s_opt.outputFileName);
    editorLoadAllOpenedBufferFiles();

    mainCallXref(argc, argv);
    closeMainOutputFile();
    if (s_opt.xref2) {
        ppcGenSynchroRecord();
    }
    if (s_opt.last_message!=NULL) {
        fflush(dumpOut);
        fprintf(dumpOut,"%s\n\n", s_opt.last_message); fflush(dumpOut);
        fflush(dumpOut);
    }
    //& fprintf(dumpOut,"\n\nDUMP\n\n"); fflush(dumpOut);
    //& refTabMap(&s_cxrefTab, symbolRefItemDump);
    LEAVE();
}

/* *************************************************************** */
/*                          Edit regime                            */
/* *************************************************************** */

void mainCallEditServerInit(int nargc, char **nargv) {
    initAvailableRefactorings();
    s_opt.classpath = "";
    processOptions(nargc, nargv, INFILES_ENABLED); /* no include or define options */
    mainScheduleInputFilesFromOptionsToFileTable();
    if (s_opt.server_operation == OLO_EXTRACT)
        s_cache.cpi = 2; // !!!! no cache, TODO why is 2 = no cache?
    olcxSetCurrentUser(s_opt.user);
    initCompletions(&s_completions, 0, s_noPos);
}

void mainCallEditServer(int argc, char **argv,
                        int nargc, char **nargv,
                        int *firstPassing
                        ) {

    ENTER();
    editorLoadAllOpenedBufferFiles();
    olcxSetCurrentUser(s_opt.user);
    if (creatingOlcxRefs()) olcxPushEmptyStackItem(&s_olcxCurrentUser->browserStack);
    if (needToProcessInputFile()) {
        if (presetEditServerFileDependingStatics() == NULL) {
            errorMessage(ERR_ST, "No input file");
        } else {
            mainEditServerProcessFile(argc,argv,nargc,nargv,firstPassing);
        }
    } else {
        if (presetEditServerFileDependingStatics() != NULL) {
            s_fileTab.tab[s_olOriginalComFileNumber]->b.scheduledToProcess = false;
            // added [26.12.2002] because of loading options without input file
            s_input_file_name = NULL;
        }
    }
    LEAVE();
}

static void mainEditServer(int argc, char **argv) {
    int     nargc;  char **nargv;
    int     firstPassing;

    ENTER();
    s_cxResizingBlocked = 1;
    firstPassing = 1;
    copyOptions(&s_cachedOptions, &s_opt);
    for(;;) {
        s_currCppPass = ANY_CPP_PASS;
        copyOptions(&s_opt, &s_cachedOptions);
        getPipedOptions(&nargc, &nargv);
        // O.K. -o option given on command line should catch also file not found
        // message
        mainOpenOutputFile(s_opt.outputFileName);
        //&dumpOptions(nargc, nargv);
        log_trace("getting request");
        mainCallEditServerInit(nargc, nargv);
        if (ccOut==stdout && s_opt.outputFileName!=NULL) {
            mainOpenOutputFile(s_opt.outputFileName);
        }
        mainCallEditServer(argc, argv, nargc, nargv, &firstPassing);
        if (s_opt.server_operation == OLO_ABOUT) {
            aboutMessage();
        } else {
            mainAnswerEditAction();
        }
        //& s_opt.outputFileName = NULL;  // why this was here ???
        //editorCloseBufferIfNotUsedElsewhere(s_input_file_name);
        editorCloseAllBuffers();
        closeMainOutputFile();
        if (s_opt.server_operation == OLO_EXTRACT)
            s_cache.cpi = 2; // !!!! no cache
        if (s_opt.last_message != NULL) {
            fprintf(ccOut,"%s",s_opt.last_message);
            fflush(ccOut);
        }
        if (s_opt.xref2) ppcGenSynchroRecord();
        /*fprintf(dumpOut,"request answered\n\n");fflush(dumpOut);*/
    }
    LEAVE();
}

/* *************************************************************** */
/*                       Generate regime                           */
/* *************************************************************** */

static void mainGenerate(int argc, char **argv) {
    int     inputIn;
    int     fArgCount, firstPassing;
    s_currCppPass = ANY_CPP_PASS;
    s_fileProcessStartTime = time(NULL);
    fArgCount = 0; s_input_file_name = getInputFile(&fArgCount);
    s_cppPassMax = 0;
    firstPassing = 1;
    mainFileProcessingInitialisations(&firstPassing, argc, argv, 0, NULL,
                                      &inputIn, &s_language);
    s_olOriginalFileNumber = s_input_file_number;
    s_olOriginalComFileNumber = s_olOriginalFileNumber;
    if (inputIn) {
        recoverFromCache();
        mainParseInputFile();
        currentFile.lexBuffer.buffer.isAtEOF = false;
    }
    symbolTableMap(s_symbolTable, generate);
}

/* initLogging() is called as the first thing in main() so we look for log filename */
static void initLogging(int argc, char *argv[]) {
    char fileName[MAX_FILE_NAME_SIZE+1] = "";
#ifdef DEBUG
    bool debug = false;
    bool trace = false;
#endif

    for (int i=0; i<argc; i++) {
        if (strncmp(argv[i], "-log=", 5)==0)
            strcpy(fileName, &argv[i][5]);
#ifdef DEBUG
        if (strcmp(argv[i], "-debug") == 0)
            debug = true;
        if (strcmp(argv[i], "-trace") == 0)
            trace = true;
#endif
    }
    if (fileName[0] != '\0') {
        FILE *tempFile = openFile(fileName, "w");
        if (tempFile != NULL)
            log_set_fp(tempFile);
    }

#ifdef DEBUG
    if (trace)
        log_set_file_level(LOG_TRACE);
    else if (debug)
        log_set_file_level(LOG_DEBUG);
    else
#endif
        log_set_file_level(LOG_INFO);

    /* Always log errors and above to console */
    log_set_console_level(LOG_ERROR);
}

/* setupLogging() is called as part of the normal argument handling so can only change level */
static void setupLogging(void) {
#ifdef DEBUG
    if (s_opt.trace)
        log_set_file_level(LOG_TRACE);
    else if (s_opt.debug)
        log_set_file_level(LOG_DEBUG);
    else
#endif
        log_set_file_level(LOG_INFO);
}


/* *********************************************************************** */
/* **************************       MAIN      **************************** */
/* *********************************************************************** */

int main(int argc, char **argv) {
    /* Options are read very late down below, so we need some sensible defaults until then */
    initLogging(argc, argv);
    ENTER();

    /* There is something interesting going on here, some mysterious
       CX_ALLOCC always makes one longjmp back to here before we can
       start processing for real ... Allocating initial memory? */
    setjmp(memoryResizeJumpTarget);
    if (s_cxResizingBlocked) {
        fatalError(ERR_ST,"cx_memory resizing required, see the TROUBLES section of README file",
                   XREF_EXIT_ERR);
    }

    s_currCppPass = ANY_CPP_PASS;
    mainTotalTaskEntryInitialisations(argc, argv);
    mainTaskEntryInitialisations(argc, argv);

    setupLogging();
    //&editorTest();

    // Ok, so there are these five main operating modes
    /* TODO: Is there an underlying reason for not doing this as a switch()? */
    /* And why s_opt.refactoringRegime and not s_opt.taskRegime == RegimeRefactory? */
    if (s_opt.refactoringRegime == RegimeRefactory) mainRefactory(argc, argv);
    if (s_opt.taskRegime == RegimeXref) mainXref(argc, argv);
    if (s_opt.taskRegime == RegimeHtmlGenerate) mainXref(argc, argv);
    if (s_opt.taskRegime == RegimeEditServer) mainEditServer(argc, argv);
    if (s_opt.taskRegime == RegimeGenerate) mainGenerate(argc, argv);

    LEAVE();
    return(0);
}
