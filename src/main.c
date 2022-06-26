#include "main.h"

#include "commons.h"
#include "constants.h"
#include "globals.h"
#include "memory.h"
#include "refactory.h"
#include "extract.h"
#include "misc.h"
#include "complete.h"
#include "options.h"
#include "init.h"
#include "jslsemact.h"
#include "editor.h"
#include "reftab.h"
#include "javafqttab.h"
#include "list.h"
#include "jsemact.h"
#include "fileio.h"
#include "filetable.h"

#include "c_parser.h"
#include "server.h"
#include "symboltable.h"
#include "yacc_parser.h"
#include "java_parser.h"
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
#include "macroargumenttable.h"

#include "protocol.h"
#include <stdbool.h>

static char oldStdopFile[MAX_FILE_NAME_SIZE];
static char oldStdopSection[MAX_FILE_NAME_SIZE];
static char oldOnLineClassPath[MAX_OPTION_LEN];
static time_t oldStdopTime;
static int oldLanguage;
static int oldPass;

static void usage() {
    fprintf(stdout, "c-xref - a C/Yacc/Java cross-referencer and refactoring tool\n\n");
#if 0
    fprintf(stdout, "Usage:\n\t\tc-xref <mode> <option>+ <input files>\n\n");
    fprintf(stdout, "mode (one of):\n");
    fprintf(stdout, "\t-xref                     - generate cross-reference data in batch mode\n");
    fprintf(stdout, "\t-server                   - enter edit server mode with c-xref protocol\n");
    fprintf(stdout, "\t-lsp                      - enter edit server mode with Language Server Protocol (LSP)\n");
    fprintf(stdout, "\t-refactor                 - make an automated refactoring\n");
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

static void aboutMessage(void) {
    char output[REFACTORING_TMP_STRING_SIZE];
    sprintf(output, "This is C-xrefactory version %s compiled at %s on %s\n", C_XREF_VERSION_NUMBER, __TIME__, __DATE__);
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
    if (options.exit) exit(XREF_EXIT_BASE);
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

static int handleIncludeOption(int argc, char **argv, int i) {
    int nargc;
    char **nargv;
    NEXT_FILE_ARG(i);

    readOptionsFile(argv[i], &nargc, &nargv, "", NULL);
    processOptions(nargc, nargv, INFILES_DISABLED);

    return i;
}

static struct {
    bool errors;
    bool warnings;
    bool infos;
    bool debug;
    bool trace;
} logging_selected = {false, false, false, false, false};

/* *************************************************************************** */
/*                                      OPTIONS                                */

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
    else if (strcmp(argv[i], "-cacheincludes")==0)
        options.cacheIncludes=1;
    else if (strcmp(argv[i], "-crlfconversion")==0)
        options.eolConversion|=CR_LF_EOL_CONVERSION;
    else if (strcmp(argv[i], "-crconversion")==0)
        options.eolConversion|=CR_EOL_CONVERSION;
    else if (strcmp(argv[i], "-completioncasesensitive")==0)
        options.completionCaseSensitive=1;
    else if (strcmp(argv[i], "-completeparenthesis")==0)
        options.completeParenthesis=1;
    else if (strncmp(argv[i], "-completionoverloadwizdeep=",27)==0)  {
        sscanf(argv[i]+27, "%d", &options.completionOverloadWizardDeep);
    }
    else if (strncmp(argv[i], "-commentmovinglevel=",20)==0) {
        sscanf(argv[i]+20, "%d", (int*)&options.commentMovingMode);
    }
    else if (strcmp(argv[i], "-continuerefactoring")==0) {
        options.continueRefactoring=RC_CONTINUE;
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
        options.displayNestedClasses = NO_OUTERS_CUT;
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
        options.exactPositionResolve = 1;
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
        options.javaFilesOnly = 1;
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
            fatalError(ERR_ST, "memory factor out of range <1,255>", XREF_EXIT_ERR);
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
        options.allowPackagesOnCommandLine = 1;
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

    if (options.taskRegime==RegimeXref && !messageWritten && !isAbsolutePath(argvi)) {
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
        options.refactoringRegime = RegimeRefactory;
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
    else if (strcmp(argv[i], "-task_regime_server")==0) {
        options.taskRegime = RegimeEditServer;
    }
    else return false;
    *argi = i;
    return true;
}

static bool processUOption(int *argIndexP, int argc, char **argv) {
    int i = *argIndexP;
    if (0) {}
    else if (strcmp(argv[i], "-urlmanualredirect")==0)   {
        options.urlAutoRedirect = 0;
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
        options.xref2 = 1;
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

static void scheduleInputFileArgumentToFileTable(char *infile) {
    int topCallFlag;
    void *recurseFlag;

    javaSetSourcePath(true);      // for case of packages on command line
    topCallFlag = 1;
    recurseFlag = &topCallFlag;
    MapOnPaths(infile, {
            dirInputFile(currentPath, "", NULL, NULL, recurseFlag, &topCallFlag);
        });
}

static void processInFileArgument(char *infile) {
    if (infile[0]=='`' && infile[strlen(infile)-1]=='`') {
        int nargc;
        char **nargv, *pp;
        char command[MAX_OPTION_LEN];

        strcpy(command, infile+1);
        pp = strchr(command, '`');
        if (pp!=NULL) *pp = 0;
        readOptionPipe(command, &nargc, &nargv, "");
        for (int i=1; i<nargc; i++) {
            if (nargv[i][0]!='-' && nargv[i][0]!='`') {
                scheduleInputFileArgumentToFileTable(nargv[i]);
            }
        }

    } else {
        scheduleInputFileArgumentToFileTable(infile);
    }
}

void processOptions(int argc, char **argv, int infilesFlag) {
    int i;
    bool matched;

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
            default:
                matched = false;
            }
        } else {
            /* input file */
            matched = true;
            if (infilesFlag == INFILES_ENABLED) {
                addStringListOption(&options.inputFiles, argv[i]);
            }
        }
        if (!matched) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "unknown option %s, (try c-xref -help)\n", argv[i]);
            if (options.taskRegime==RegimeXref) {
                fatalError(ERR_ST, tmpBuff, XREF_EXIT_ERR);
            } else
                errorMessage(ERR_ST, tmpBuff);
        }
    }
}

static void scheduleInputFilesFromArgumentsToFileTable(void) {
    for (StringList *ll=options.inputFiles; ll!=NULL; ll=ll->next) {
        processInFileArgument(ll->string);
    }
}

/* *************************************************************************** */

typedef enum {
    FF_SCHEDULED_TO_PROCESS,
    FF_COMMAND_LINE_ENTERED
} FilesToProcess;


static char *getNextInputFileFromFileTable(int *indexP, FilesToProcess flag) {
    int         i;
    FileItem  *fileItem;

    for (i = getNextExistingFileIndex(*indexP); i != -1; i = getNextExistingFileIndex(i+1)) {
        fileItem = getFileItem(i);
        if (fileItem!=NULL) {
            if (flag==FF_SCHEDULED_TO_PROCESS && fileItem->scheduledToProcess)
                break;
            if (flag==FF_COMMAND_LINE_ENTERED && fileItem->commandLineEntered)
                break;
        }
    }
    *indexP = i;
    if (i != -1)
        return fileItem->name;
    else
        return NULL;
}

char *getNextInputFile(int *indexP) {
    return getNextInputFileFromFileTable(indexP, FF_SCHEDULED_TO_PROCESS);
}

static char *getNextCommandLineFile(int *indexP) {
    return getNextInputFileFromFileTable(indexP, FF_COMMAND_LINE_ENTERED);
}

static void generateReferenceFile(void) {
    static bool updateFlag = false;  /* TODO: WTF - why do we need a
                                        static updateFlag? Maybe we
                                        need to know that we have
                                        generated from scratch so now
                                        we can just update? */

    if (options.cxrefsLocation == NULL)
        return;
    if (!updateFlag && options.update == UPDATE_DEFAULT) {
        genReferenceFile(false, options.cxrefsLocation);
        updateFlag = true;
    } else {
        genReferenceFile(true, options.cxrefsLocation);
    }
}

static void schedulingUpdateToProcess(FileItem *fileItem) {
    if (fileItem->scheduledToUpdate && fileItem->commandLineEntered) {
        fileItem->scheduledToProcess = true;
    }
}

/* NOTE: Map-function */
static void schedulingToUpdate(FileItem *fileItem) {
    if (fileItem == getFileItem(noFileIndex))
        return;

    if (!editorFileExists(fileItem->name)) {
        // removed file, remove it from watched updates, load no reference
        if (fileItem->commandLineEntered) {
            // no messages during refactorings
            if (refactoringOptions.refactoringRegime != RegimeRefactory) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "file %s not accessible", fileItem->name);
                warningMessage(ERR_ST, tmpBuff);
            }
        }
        fileItem->commandLineEntered = false;
        fileItem->scheduledToProcess = false;
        fileItem->scheduledToUpdate = false;
        // (missing of following if) has caused that all class hierarchy items
        // as well as all cxreferences based in .class files were lost
        // on -update, a very serious bug !!!!
        if (fileItem->name[0] != ZIP_SEPARATOR_CHAR) {
            fileItem->cxLoading = true;     /* Hack, to remove references from file */
        }
    } else if (options.update == UPDATE_FULL) {
        if (editorFileModificationTime(fileItem->name) != fileItem->lastFullUpdateMtime) {
            fileItem->scheduledToUpdate = true;
        }
    } else {
        if (editorFileModificationTime(fileItem->name) != fileItem->lastUpdateMtime) {
            fileItem->scheduledToUpdate = true;
        }
    }
    log_trace("Scheduling '%s' to update: %s", fileItem->name, fileItem->scheduledToUpdate?"yes":"no");
}

void searchDefaultOptionsFile(char *filename, char *options_filename, char *section) {
    int fileno;
    bool found=false;
    FILE *options_file;
    int nargc;
    char **nargv;

    options_filename[0] = 0;
    section[0]=0;

    if (filename == NULL || options.no_stdoptions)
        return;

    /* Try to find section in HOME config. */
    getXrefrcFileName(options_filename);
    options_file = openFile(options_filename, "r");
    if (options_file != NULL) {
        found = readOptionFromFile(options_file, &nargc, &nargv, DONT_ALLOCATE, filename, options.project, section);
        if (found) {
            log_debug("options file '%s' section '%s' found", options_filename, section);
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
            strcpy(options_filename, oldStdopFile);
            strcpy(section, oldStdopSection);
            return;
        }
    }
    options_filename[0]=0;
}

static void writeOptionsFileMessage(char *file, char *outFName, char *outSect) {
    char tmpBuff[TMP_BUFF_SIZE];

    if (options.refactoringRegime==RegimeRefactory)
        return;
    if (outFName[0]==0) {
        if (options.project!=NULL) {
            sprintf(tmpBuff, "'%s' project options not found",
                    options.project);
            if (options.taskRegime == RegimeEditServer) {
                errorMessage(ERR_ST, tmpBuff);
            } else {
                fatalError(ERR_ST, tmpBuff, XREF_EXIT_NO_PROJECT);
            }
        } else if (options.xref2) {
            ppcGenRecord(PPC_NO_PROJECT,file);
        } else {
            sprintf(tmpBuff, "no project name covers '%s'",file);
            warningMessage(ERR_ST, tmpBuff);
        }
    } else if (options.taskRegime==RegimeXref) {
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
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        if (showErrorMessage) {
            writeOptionsFileMessage(fileName, outFName, section);
        }
    } else {
        if (*oldStdopFile == 0) {
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
                strcpy(section, oldStdopSection);
            }
            if (outFName[0]==0) {
                strcpy(outFName, oldStdopFile);
            }
            if(strcmp(oldStdopFile,outFName)||strcmp(oldStdopSection,section)){
                if (options.xref2) {
                    char tmpBuff[TMP_BUFF_SIZE];                        \
                    sprintf(tmpBuff, "[Xref] new project: '%s'", section);
                    ppcGenRecord(PPC_INFORMATION, tmpBuff);
                } else {
                    fprintf(dumpOut, "[Xref] new project: '%s'\n", section);
                }
            }
        }
    }
}

static void getOptionsFile(char *file, char *optionsFileName, char *sectionName,
                           bool showErrorMessage) {
    searchDefaultOptionsFile(file, optionsFileName, sectionName);
    handlePathologicProjectCases(file, optionsFileName, sectionName, showErrorMessage);
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
    copyOptions(&options, &presetOptions);

    inputFileNumber   = noFileIndex;
}

static void initDefaultCxrefFileName(char *inputfile) {
    int pathLength;
    static char defaultCxrefFileName[MAX_FILE_NAME_SIZE];

    pathLength = extractPathInto(normalizeFileName(inputfile, cwd), defaultCxrefFileName);
    assert(pathLength < MAX_FILE_NAME_SIZE);
    strcpy(&defaultCxrefFileName[pathLength], DEFAULT_CXREF_FILE);
    assert(strlen(defaultCxrefFileName) < MAX_FILE_NAME_SIZE);
    strcpy(defaultCxrefFileName, getRealFileName_static(normalizeFileName(defaultCxrefFileName, cwd)));
    assert(strlen(defaultCxrefFileName) < MAX_FILE_NAME_SIZE);
    options.cxrefsLocation = defaultCxrefFileName;
}

static void initializationsPerInvocation(void) {
    int i;
    s_cp = s_cpInit;
    s_cps = s_cpsInit;
    for(i=0; i<SPP_MAX; i++) s_spp[i] = noPosition;
    s_cxRefFlag=0;
    s_cxRefPos = noPosition;
    s_olstring[0]=0;
    s_olstringFound = false;
    s_olstringServed = false;
    s_olstringInMbody = NULL;
    s_yygstate = s_initYygstate;
    s_jsl = NULL;
    s_javaObjectSymbol = NULL;
}

/*///////////////////////// parsing /////////////////////////////////// */
static void parseInputFile(void) {
    if (currentLanguage == LANG_JAVA) {
        uniyylval = & s_yygstate->gyylval;
        java_yyparse();
    }
    else if (currentLanguage == LANG_YACC) {
        //printf("Parsing YACC-file\n");
        uniyylval = & yacc_yylval;
        yacc_yyparse();
    }
    else {
        uniyylval = & c_yylval;
        c_yyparse();
    }
    cache.active = false;
    currentFile.fileName = NULL;
}


void mainSetLanguage(char *inFileName, Language *outLanguage) {
    char *suff;
    if (inFileName == NULL
        || fileNameHasOneOfSuffixes(inFileName, options.javaFilesSuffixes)
        || (filenameCompare(simpleFileName(inFileName), "Untitled-", 9)==0)  // jEdit unnamed buffer
    ) {
        *outLanguage = LANG_JAVA;
        typeNamesTable[TypeStruct] = "class";
    } else {
        suff = getFileSuffix(inFileName);
        if (compareFileNames(suff, ".zip")==0 || compareFileNames(suff, ".jar")==0) {
            *outLanguage = LANG_JAR;
        } else if (compareFileNames(suff, ".class")==0) {
            *outLanguage = LANG_CLASS;
        } else if (compareFileNames(suff, ".y")==0) {
            *outLanguage = LANG_YACC;
            typeNamesTable[TypeStruct] = "struct";
        } else {
            *outLanguage = LANG_C;
            typeNamesTable[TypeStruct] = "struct";
        }
    }
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
        readOptionsFile(dffname, &dfargc, &dfargv, dffsect, project);
        // warning, the following can overwrite variables like
        // 's_cxref_file_name' allocated in PPM_MEMORY, then when memory
        // is got back by caching, it may provoke a problem
        processOptions(dfargc, dfargv, INFILES_DISABLED); /* .c-xrefrc opts*/
    }
}

static void checkExactPositionUpdate(bool printMessage) {
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

static void fileProcessingInitialisations(bool *firstPass,
                                              int argc, char **argv,      // command-line options
                                              int nargc, char **nargv,    // piped options
                                              bool *inputOpened,
                                              Language *outLanguage
) {
    char defaultOptionsFileName[MAX_FILE_NAME_SIZE];
    char defaultOptionsSectionName[MAX_FILE_NAME_SIZE];
    time_t modifiedTime;
    char *fileName;
    StringList *tmpIncludeDirs;

    ENTER();

    fileName = inputFilename;
    mainSetLanguage(fileName, outLanguage);
    getOptionsFile(fileName, defaultOptionsFileName, defaultOptionsSectionName, true);
    initAllInputs();

    if (defaultOptionsFileName[0] != 0 )
        modifiedTime = fileModificationTime(defaultOptionsFileName);
    else
        modifiedTime = oldStdopTime;               // !!! just for now

    log_trace("Checking oldcp==%s", oldOnLineClassPath);
    log_trace("Checking newcp==%s", options.classpath);
    if (*firstPass
        || oldPass != currentPass
        || strcmp(oldStdopFile,defaultOptionsFileName)
        || strcmp(oldStdopSection,defaultOptionsSectionName)
        || oldStdopTime != modifiedTime
        || oldLanguage!= *outLanguage
        || strcmp(oldOnLineClassPath, options.classpath)
        || cache.cpIndex == 1     /* some kind of reset was made */
    ) {
        if (*firstPass) {
            initCaching();
            *firstPass = false;
        } else {
            recoverCachePointZero();
        }
        strcpy(oldOnLineClassPath, options.classpath);
        assert(strlen(oldOnLineClassPath)<MAX_OPTION_LEN-1);

        initPreCreatedTypes();
        initCwd();
        initOptions();
        initDefaultCxrefFileName(fileName);

        processOptions(argc, argv, INFILES_DISABLED);   /* command line opts */
        /* piped options (no include or define options)
           must be before .xrefrc file options, but, the s_cachedOptions
           must be set after .c-xrefrc file, but s_cachedOptions can't contain
           piped options, !!! berk.
        */
        processOptions(nargc, nargv, INFILES_DISABLED);
        reInitCwd(defaultOptionsFileName, defaultOptionsSectionName);

        tmpIncludeDirs = options.includeDirs;
        options.includeDirs = NULL;

        getAndProcessXrefrcOptions(defaultOptionsFileName, defaultOptionsSectionName, defaultOptionsSectionName);
        discoverStandardDefines();
        discoverBuiltinIncludePaths();

        LIST_APPEND(StringList, options.includeDirs, tmpIncludeDirs);
        if (options.taskRegime != RegimeEditServer && inputFilename == NULL) {
            *inputOpened = false;
            goto fini;
        }
        copyOptions(&savedOptions, &options);  // before getJavaClassPath, it modifies ???
        processOptions(nargc, nargv, INFILES_DISABLED);
        getJavaClassAndSourcePath();
        *inputOpened = computeAndOpenInputFile();
        strcpy(oldStdopFile,defaultOptionsFileName);
        strcpy(oldStdopSection,defaultOptionsSectionName);
        oldStdopTime = modifiedTime;
        oldLanguage = *outLanguage;
        oldPass = currentPass;

        // this was before 'getAndProcessXrefrcOptions(df...' I hope it will not cause
        // troubles to move it here, because of autodetection of -javaVersion from jdkcp
        initTokenNamesTables();

        cache.active = true;
        placeCachePoint(false);
        cache.active = false;
        assert(cache.lbcc == cache.cp[0].lbcc);
        assert(cache.lbcc == cache.cp[1].lbcc);
    } else {
        copyOptions(&options, &savedOptions);
        processOptions(nargc, nargv, INFILES_DISABLED); /* no include or define options */
        *inputOpened = computeAndOpenInputFile();
    }
    // reset language once knowing all language suffixes
    mainSetLanguage(fileName,  outLanguage);
    inputFileNumber = currentFile.lexBuffer.buffer.fileNumber;
    assert(options.taskRegime);
    if (options.taskRegime==RegimeXref && !javaPreScanOnly) {
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
}

static int power(int x, int y) {
    int res = 1;
    for (int i=0; i<y; i++)
        res *= x;
    return res;
}

static bool optionsOverflowHandler(int n) {
    fatalError(ERR_NO_MEMORY, "opiMemory", XREF_EXIT_ERR);
    return true;
}

static void totalTaskEntryInitialisations() {
    ENTER();

    // Outputs
    errOut = stderr;
    dumpOut = stdout;

    communicationChannel = stdout;
    // TODO: how come it is not always Xref, as set in options?
    if (options.taskRegime == RegimeEditServer)
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
    fileItem->scheduledToProcess = false;
    fileItem->scheduledToUpdate = false;
    fileItem->fullUpdateIncludesProcessed = false;
    fileItem->cxLoaded = false;
    fileItem->cxLoading = false;
    fileItem->cxSaved = false;
}

void mainTaskEntryInitialisations(int argc, char **argv) {
    char temp[MAX_FILE_NAME_SIZE];
    char defaultOptionsFileName[MAX_FILE_NAME_SIZE];
    char defaultOptionsSection[MAX_FILE_NAME_SIZE];
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
                  symbolTable,NULL,AccessDefault,s_cpInit,noFileIndex,NULL);
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
    oldStdopFile[0] = 0;
    oldStdopSection[0] = 0;

    /* now pre-read the option file */
    processOptions(argc, argv, INFILES_ENABLED);
    scheduleInputFilesFromArgumentsToFileTable();

    if (options.refactoringRegime == RegimeRefactory) {
        // some more memory for refactoring task
        assert(options.cxMemoryFactor>=1);
        CX_ALLOCC(sss, 6*options.cxMemoryFactor*CX_MEMORY_CHUNK_SIZE, char);
        CX_FREE_UNTIL(sss);
    }
    if (options.taskRegime==RegimeXref) {
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
    inputFilename = cmdlnInputFile = getNextCommandLineFile(&argcount);
    if (inputFilename==NULL) {
        ss = strmcpy(temp, cwd);
        if (ss!=temp && ss[-1] == FILE_PATH_SEPARATOR)
            ss[-1]=0;
        assert(strlen(temp)+1<MAX_FILE_NAME_SIZE);
        inputFilename=temp;
    } else {
        strcpy(temp, inputFilename);
    }

    getOptionsFile(temp, defaultOptionsFileName, defaultOptionsSection, false);
    reInitCwd(defaultOptionsFileName, defaultOptionsSection);

    if (defaultOptionsFileName[0]!=0) {
        readOptionsFile(defaultOptionsFileName, &dfargc, &dfargv, defaultOptionsSection, defaultOptionsSection);
        if (options.refactoringRegime == RegimeRefactory) {
            inmode = INFILES_DISABLED;
        } else if (options.taskRegime==RegimeEditServer) {
            inmode = INFILES_DISABLED;
        } else if (options.create || options.project!=NULL || options.update != UPDATE_DEFAULT) {
            inmode = INFILES_ENABLED;
        } else {
            inmode = INFILES_DISABLED;
        }
        // disable error reporting on xref task on this pre-reading of .c-xrefrc
        previousNoErrorsOption = options.noErrors;
        if (options.taskRegime==RegimeEditServer) {
            options.noErrors = true;
        }
        // there is a problem with INFILES_ENABLED (update for safetycheck),
        // It should first load cxref file, in order to protect file numbers.
        if (inmode==INFILES_ENABLED && options.update && !options.create) { /* TODO: .update != UPDATE_CREATE?!?! */
            //&fprintf(dumpOut, "PREREADING !!!!!!!!!!!!!!!!\n");
            // this makes a problem: I need to preread cxref file before
            // reading input files in order to preserve hash numbers, but
            // I need to read options first in order to have the name
            // of cxref file.
            // I need to read fstab also to remove removed files on update
            processOptions(dfargc, dfargv, INFILES_DISABLED);
            smartReadReferences();
        }
        processOptions(dfargc, dfargv, inmode);
        // recover value of errors messages
        if (options.taskRegime==RegimeEditServer)
            options.noErrors = previousNoErrorsOption;
        checkExactPositionUpdate(false);
        if (inmode == INFILES_ENABLED)
            scheduleInputFilesFromArgumentsToFileTable();
    }
    recoverCachePointZero();

    initCaching();

    LEAVE();
}

static void referencesOverflowed(char *cxMemFreeBase, LongjmpReason mess) {
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
        fatalError(ERR_ST, "sorry no file for cxrefs, use -refs option", XREF_EXIT_ERR);
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
    if (options.taskRegime==RegimeXref)
        generateReferenceFile();
    recoverMemoriesAfterOverflow(cxMemFreeBase);

    /* ************ start with CXREFS and memories clean ************ */
    bool savingFlag = false;
    for (int i=getNextExistingFileIndex(0); i != -1; i = getNextExistingFileIndex(i+1)) {
        FileItem *fileItem = getFileItem(i);
        if (fileItem->cxLoading) {
            fileItem->cxLoading = false;
            fileItem->cxSaved = true;
            if (fileItem->commandLineEntered)
                savingFlag = true;
            log_trace(" -># '%s'",fileItem->name);
        }
    }
    if (!savingFlag && mess!=LONGJMP_REASON_FILE_ABORT) {
        /* references overflowed, but no whole file readed */
        fatalError(ERR_NO_MEMORY, "cxMemory", XREF_EXIT_ERR);
    }
    LEAVE();
}

void getPipedOptions(int *outNargc,char ***outNargv){
    char nsect[MAX_FILE_NAME_SIZE];
    int c;
    *outNargc = 0;
    assert(options.taskRegime);
    if (options.taskRegime == RegimeEditServer) {
        readOptionFromFile(stdin, outNargc, outNargv, ALLOCATE_IN_SM,
                           "", NULL, nsect);
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

static void fillIncludeRefItem(ReferencesItem *referencesItem, int fileIndex) {
    fillReferencesItem(referencesItem, LINK_NAME_INCLUDE_REFS,
                       cxFileHashNumber(LINK_NAME_INCLUDE_REFS),
                       fileIndex, fileIndex, TypeCppInclude, StorageExtern,
                       ScopeGlobal, AccessDefault, CategoryGlobal);
}

static void makeIncludeClosureOfFilesToUpdate(void) {
    char                *cxFreeBase;
    int                 fileAddedFlag;
    ReferencesItem referenceItem, *member;
    Reference           *rr;

    CX_ALLOCC(cxFreeBase,0,char);
    fullScanFor(LINK_NAME_INCLUDE_REFS);
    // iterate over scheduled files
    fileAddedFlag = true;
    while (fileAddedFlag) {
        fileAddedFlag = false;
        for (int i=getNextExistingFileIndex(0); i != -1; i = getNextExistingFileIndex(i+1)) {
            FileItem *fileItem = getFileItem(i);
            if (fileItem->scheduledToUpdate)
                if (!fileItem->fullUpdateIncludesProcessed) {
                    fileItem->fullUpdateIncludesProcessed = true;
                    bool isJavaFileFlag = fileNameHasOneOfSuffixes(fileItem->name, options.javaFilesSuffixes);
                    fillIncludeRefItem(&referenceItem, i);
                    if (isMemberInReferenceTable(&referenceItem, NULL, &member)) {
                        for (rr=member->references; rr!=NULL; rr=rr->next) {
                            FileItem *includer = getFileItem(rr->position.file);
                            if (!includer->scheduledToUpdate) {
                                includer->scheduledToUpdate = true;
                                fileAddedFlag = true;
                                if (isJavaFileFlag) {
                                    // no transitive closure for Java
                                    includer->fullUpdateIncludesProcessed = true;
                                }
                            }
                        }
                    }
                }

        }
    }
    recoverMemoriesAfterOverflow(cxFreeBase);
}

static void getCxrefFilesListName(char **fileListFileNameP, char **suffixP) {
    if (options.referenceFileCount <= 1) {
        *suffixP           = "";
        *fileListFileNameP = options.cxrefsLocation;
    } else {
        char cxrefsFileName[MAX_FILE_NAME_SIZE];
        *suffixP = REFERENCE_FILENAME_FILES;
        sprintf(cxrefsFileName, "%s%s", options.cxrefsLocation, *suffixP);
        assert(strlen(cxrefsFileName) < MAX_FILE_NAME_SIZE - 1);
        *fileListFileNameP = cxrefsFileName;
    }
}

static void scheduleModifiedFilesToUpdate(void) {
    char        *fileListFileName;
    char        *suffix;

    checkExactPositionUpdate(true);

    getCxrefFilesListName(&fileListFileName, &suffix);
    // TODO: This is probably to get modification time for the fileslist file
    // and then consider that when schedule files to update, but it never did...
    // if (editorFileStatus(fileListFileName, &stat))
    //     stat.st_mtime = 0;

    normalScanReferenceFile(suffix);

    // ... but schedulingToUpdate() does not use the stat data !?!?!?
    mapOverFileTable(schedulingToUpdate);
    // TODO: ... so WTF??!?!?!?

    if (options.update==UPDATE_FULL /*& && !LANGUAGE(LANG_JAVA) &*/) {
        makeIncludeClosureOfFilesToUpdate();
    }
    mapOverFileTable(schedulingUpdateToProcess);
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

static int scheduleFileUsingTheMacro(void) {
    ReferencesItem  references;
    SymbolsMenu     menu, *oldMenu;
    OlcxReferences *tmpc;

    assert(s_olstringInMbody);
    tmpc = NULL;
    fillReferencesItem(&references, s_olstringInMbody,
                       cxFileHashNumber(s_olstringInMbody),
                       noFileIndex, noFileIndex, TypeMacro, StorageExtern,
                       ScopeGlobal, AccessDefault, CategoryGlobal);

    fillSymbolsMenu(&menu, references, 1, true, 0, UsageUsed, 0, 0, 0, UsageNone, noPosition, 0, NULL, NULL);
    if (sessionData.browserStack.top==NULL) {
        olcxPushEmptyStackItem(&sessionData.browserStack);
        tmpc = sessionData.browserStack.top;
    }
    assert(sessionData.browserStack.top);
    oldMenu = sessionData.browserStack.top->menuSym;
    sessionData.browserStack.top->menuSym = &menu;
    s_olMacro2PassFile = noFileIndex;
    scanForMacroUsage(s_olstringInMbody);
    sessionData.browserStack.top->menuSym = oldMenu;
    if (tmpc!=NULL) {
        olStackDeleteSymbol(tmpc);
    }
    log_trace(":scheduling file '%s'", getFileItem(s_olMacro2PassFile)->name);
    if (s_olMacro2PassFile == noFileIndex)
        return noFileIndex;
    return s_olMacro2PassFile;
}

// this is necessary to put new mtimes for header files
static void setFullUpdateMtimesInFileItem(FileItem *fi) {
    if (fi->scheduledToUpdate || options.create) {
        fi->lastFullUpdateMtime = fi->lastModified;
    }
}

static void closeInputFile(void) {
    if (currentFile.lexBuffer.buffer.file!=stdin) {
        closeCharacterBuffer(&currentFile.lexBuffer.buffer);
    }
}

static void editServerParseInputFile(bool *firstPass, bool inputIn) {
    if (inputIn) {
        if (options.serverOperation!=OLO_TAG_SEARCH && options.serverOperation!=OLO_PUSH_NAME) {
            log_trace("parse start");
            recoverFromCache();
            parseInputFile();
            log_trace("parse end");
            *firstPass = false;
        }
        currentFile.lexBuffer.buffer.isAtEOF = false;
        closeInputFile();
    }
}

static bool symbolCanBeIdentifiedByPosition(int fileIndex) {
    int line,col;

    // there is a serious problem with options memory for options got from
    // the .c-xrefrc file. so for the moment this will not work.
    // which problem ??????
    // seems that those options are somewhere in ppmMemory overwritten?
    //&return 0;
    if (!creatingOlcxRefs())
        return false;
    if (options.browsedSymName == NULL)
        return false;
    log_trace("looking for sym %s on %s", options.browsedSymName, options.olcxlccursor);

    // modified file, can't identify the reference
    log_trace(":modif flag == %d", options.modifiedFlag);
    if (options.modifiedFlag)
        return false;

    // here I will need also the symbol name
    // do not bypass commanline entered files, because of local symbols
    // and because references from currently procesed file would
    // be not loaded from the TAG file (it expects they are loaded
    // by parsing).
    FileItem *fileItem = getFileItem(fileIndex);
    log_trace("commandLineEntered %s == %d", fileItem->name, fileItem->commandLineEntered);
    if (fileItem->commandLineEntered)
        return false;

    // if references are not updated do not search it here
    // there were fullUpdate time? why?
    log_trace("checking last modified: %d, update: %d", fileItem->lastModified, fileItem->lastUpdateMtime);
    if (fileItem->lastModified != fileItem->lastUpdateMtime)
        return false;

    // here read one reference file looking for the refs
    // assume s_opt.olcxlccursor is correctly set;
    getLineAndColumnCursorPositionFromCommandLineOptions(&line, &col);
    s_olcxByPassPos = makePosition(fileIndex, line, col);
    olSetCallerPosition(&s_olcxByPassPos);
    scanForBypass(options.browsedSymName);

    // if no symbol found, it may be a local symbol, try by parsing
    log_trace("checking that %d != NULL", sessionData.browserStack.top->hkSelectedSym);
    if (sessionData.browserStack.top->hkSelectedSym == NULL)
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

static void editServerFileSinglePass(int argc, char **argv,
                                      int nargc, char **nargv,
                                      bool *firstPassP
) {
    bool inputOpened = false;
    int ol2procfile;

    olStringSecondProcessing = 0;
    fileProcessingInitialisations(firstPassP, argc, argv,
                                      nargc, nargv, &inputOpened, &currentLanguage);
    smartReadReferences();
    olOriginalFileIndex = inputFileNumber;
    if (symbolCanBeIdentifiedByPosition(inputFileNumber)) {
        if (inputOpened)
            closeInputFile();
        return;
    }
    if (inputOpened)
        editServerParseInputFile(firstPassP, inputOpened);
    if (options.olCursorPos==0 && !LANGUAGE(LANG_JAVA)) {
        // special case, push the file as include reference
        if (creatingOlcxRefs()) {
            Position dpos = makePosition(inputFileNumber, 1, 0);
            gotOnLineCxRefs(&dpos);
        }
        addThisFileDefineIncludeReference(inputFileNumber);
    }
    if (s_olstringFound && !s_olstringServed) {
        // on-line action with cursor in an un-used macro body ???
        ol2procfile = scheduleFileUsingTheMacro();
        if (ol2procfile!=noFileIndex) {
            inputFilename = getFileItem(ol2procfile)->name;
            inputOpened = false;
            olStringSecondProcessing = 1;
            fileProcessingInitialisations(firstPassP, argc, argv,
                                              nargc, nargv, &inputOpened, &currentLanguage);
            if (inputOpened)
                editServerParseInputFile(firstPassP, inputOpened);
        }
    }
}


static void editServerProcessFile(int argc, char **argv,
                                      int nargc, char **nargv,
                                      bool *firstPass
) {
    FileItem *fileItem = getFileItem(olOriginalComFileNumber);

    assert(fileItem->scheduledToProcess);
    maxPasses = 1;
    for (currentPass=1; currentPass<=maxPasses; currentPass++) {
        inputFilename = fileItem->name;
        assert(inputFilename!=NULL);
        editServerFileSinglePass(argc, argv, nargc, nargv, firstPass);
        if (options.serverOperation==OLO_EXTRACT || (s_olstringServed && !creatingOlcxRefs()))
            break;
        if (LANGUAGE(LANG_JAVA))
            break;
    }
    fileItem->scheduledToProcess = false;
}

static char *presetEditServerFileDependingStatics(void) {
    fileProcessingStartTime = time(NULL);

    s_primaryStartPosition = noPosition;
    s_staticPrefixStartPosition = noPosition;

    // This is pretty stupid, there is always only one input file
    // in edit server, otherwise it is an error
    int fileIndex = 0;
    inputFilename = getNextInputFile(&fileIndex);
    if (fileIndex == -1) { /* No more input files... */
        // conservative message, probably macro invoked on nonsaved file, TODO: WTF?
        olOriginalComFileNumber = noFileIndex;
        return NULL;
    }

    /* TODO: This seems strange, we only assert that the first file is scheduled to process.
       Then reset all other files, why? */
    assert(getFileItem(fileIndex)->scheduledToProcess);
    for (int i=getNextExistingFileIndex(fileIndex+1); i != -1; i = getNextExistingFileIndex(i+1)) {
        getFileItem(i)->scheduledToProcess = false;
    }

    olOriginalComFileNumber = fileIndex;

    char *fileName = inputFilename;
    mainSetLanguage(fileName, &currentLanguage);
    // O.K. just to be sure, there is no other input file
    return fileName;
}


static int needToProcessInputFile(void) {
    return options.serverOperation==OLO_COMPLETION
           || options.serverOperation==OLO_SEARCH
           || options.serverOperation==OLO_EXTRACT
           || options.serverOperation==OLO_TAG_SEARCH
           || options.serverOperation==OLO_SET_MOVE_TARGET
           || options.serverOperation==OLO_SET_MOVE_CLASS_TARGET
           || options.serverOperation==OLO_SET_MOVE_METHOD_TARGET
           || options.serverOperation==OLO_GET_CURRENT_CLASS
           || options.serverOperation==OLO_GET_CURRENT_SUPER_CLASS
           || options.serverOperation==OLO_GET_METHOD_COORD
           || options.serverOperation==OLO_GET_CLASS_COORD
           || options.serverOperation==OLO_GET_ENV_VALUE
           || creatingOlcxRefs()
        ;
}


/* *************************************************************** */
/*                          Xref regime                            */
/* *************************************************************** */
static void xrefProcessInputFile(int argc, char **argv, bool *firstPassP,
                                 bool *atLeastOneProcessedP) {
    bool inputOpened;

    maxPasses = 1;
    for (currentPass=1; currentPass<=maxPasses; currentPass++) {
        if (!*firstPassP)
            copyOptions(&options, &savedOptions);
        fileProcessingInitialisations(firstPassP, argc, argv, 0, NULL, &inputOpened, &currentLanguage);
        olOriginalFileIndex    = inputFileNumber;
        olOriginalComFileNumber = olOriginalFileIndex;
        if (inputOpened) {
            recoverFromCache();
            cache.active = false;    /* no caching in cxref */
            parseInputFile();
            closeCharacterBuffer(&currentFile.lexBuffer.buffer);
            inputOpened = false;
            currentFile.lexBuffer.buffer.file = stdin;
            *atLeastOneProcessedP = true;
        } else if (LANGUAGE(LANG_JAR)) {
            jarFileParse(inputFilename);
            *atLeastOneProcessedP = true;
        } else if (LANGUAGE(LANG_CLASS)) {
            classFileParse();
            *atLeastOneProcessedP = true;
        } else {
            errorMessage(ERR_CANT_OPEN, inputFilename);
            fprintf(dumpOut, "\tmaybe forgotten -p option?\n");
        }
        // no multiple passes for java programs
        *firstPassP = false;
        currentFile.lexBuffer.buffer.isAtEOF = false;
        if (LANGUAGE(LANG_JAVA))
            break;
    }
}

static void xrefOneWholeFileProcessing(int argc, char **argv, FileItem *fileItem,
                                       bool *firstPass, bool *atLeastOneProcessed) {
    inputFilename = fileItem->name;
    fileProcessingStartTime = time(NULL);
    // O.K. but this is missing all header files
    fileItem->lastUpdateMtime = fileItem->lastModified;
    if (options.update == UPDATE_FULL || options.create) {
        fileItem->lastFullUpdateMtime = fileItem->lastModified;
    }
    xrefProcessInputFile(argc, argv, firstPass, atLeastOneProcessed);
    // now free the buffer because it tooks too much memory,
    // but I can not free it when refactoring, nor when preloaded,
    // so be very carefull about this!!!
    if (refactoringOptions.refactoringRegime!=RegimeRefactory) {
        editorCloseBufferIfClosable(inputFilename);
        if (!options.cacheIncludes)
            editorCloseAllBuffersIfClosable();
    }
}

static void printPrescanningMessage(void) {
    if (options.xref2) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Prescanning classes, please wait.");
        ppcGenRecord(PPC_INFORMATION, tmpBuff);
    } else {
        log_info("Prescanning classes, please wait.");
    }
}

static bool inputFileItemLess(FileItem *fileItem1, FileItem *fileItem2) {
    int comparison;
    char directoryName1[MAX_FILE_NAME_SIZE];
    char directoryName2[MAX_FILE_NAME_SIZE];

    // first compare directory
    strcpy(directoryName1, directoryName_st(fileItem1->name));
    strcpy(directoryName2, directoryName_st(fileItem2->name));
    comparison = strcmp(directoryName1, directoryName2);
    if (comparison<0)
        return true;
    if (comparison>0)
        return false;
    // then full file name
    comparison = strcmp(fileItem1->name, fileItem2->name);
    if (comparison<0)
        return true;
    if (comparison>0)
        return false;
    return false;
}

static FileItem *createListOfInputFileItems(void) {
    FileItem *fileItem;
    int fileIndex;

    fileItem = NULL;
    fileIndex = 0;
    for (char *fileName=getNextInputFile(&fileIndex); fileName!=NULL; fileIndex++,fileName=getNextInputFile(&fileIndex)) {
        FileItem *current = getFileItem(fileIndex);
        current->next = fileItem;
        fileItem = current;
    }
    LIST_MERGE_SORT(FileItem, fileItem, inputFileItemLess);
    return fileItem;
}

void mainCallXref(int argc, char **argv) {
    // These are static because of the longjmp() maybe happening
    static char *cxFreeBase;
    static bool firstPass, atLeastOneProcessed;
    static FileItem *ffc, *pffc;
    static bool messagePrinted = false;
    static int numberOfInputs;

    LongjmpReason reason = LONGJMP_REASON_NONE;

    currentPass = ANY_PASS;
    CX_ALLOCC(cxFreeBase,0,char);
    cxResizingBlocked = 1;
    if (options.update)
        scheduleModifiedFilesToUpdate();
    atLeastOneProcessed = false;
    ffc = pffc = createListOfInputFileItems();
    LIST_LEN(numberOfInputs, FileItem, ffc);
    for(;;) {
        currentPass = ANY_PASS;
        firstPass = true;
        if ((reason=setjmp(cxmemOverflow))!=0) {
            referencesOverflowed(cxFreeBase,reason);
            if (reason==LONGJMP_REASON_FILE_ABORT) {
                if (pffc!=NULL)
                    pffc=pffc->next;
                else if (ffc!=NULL)
                    ffc=ffc->next;
            }
        } else {
            int inputCounter = 0;

            javaPreScanOnly = true;
            for(; pffc!=NULL; pffc=pffc->next) {
                if (!messagePrinted) {
                    printPrescanningMessage();
                    messagePrinted = true;
                }
                mainSetLanguage(pffc->name, &currentLanguage);
                if (LANGUAGE(LANG_JAVA)) {
                    /* TODO: problematic if a single file generates overflow, e.g. a JAR
                       Can we just reread from the last class file? */
                    xrefOneWholeFileProcessing(argc, argv, pffc, &firstPass, &atLeastOneProcessed);
                }
                if (options.xref2)
                    writeRelativeProgress(10*inputCounter/numberOfInputs);
                inputCounter++;
            }

            javaPreScanOnly = false;
            fileAbortEnabled = true;

            inputCounter = 0;
            for(; ffc!=NULL; ffc=ffc->next) {
                xrefOneWholeFileProcessing(argc, argv, ffc, &firstPass, &atLeastOneProcessed);
                ffc->scheduledToProcess = false;
                ffc->scheduledToUpdate = false;
                if (options.xref2)
                    writeRelativeProgress(10+90*inputCounter/numberOfInputs);
                inputCounter++;
            }
            goto regime1fini;
        }
    }

 regime1fini:
    fileAbortEnabled = false;
    if (atLeastOneProcessed) {
        if (options.taskRegime==RegimeXref) {
            if (options.update==UPDATE_DEFAULT || options.update==UPDATE_FULL) {
                mapOverFileTable(setFullUpdateMtimesInFileItem);
            }
            if (options.xref2) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "Generating '%s'",options.cxrefsLocation);
                ppcGenRecord(PPC_INFORMATION, tmpBuff);
            } else {
                log_info("Generating '%s'",options.cxrefsLocation);
            }
            generateReferenceFile();
        }
    } else if (options.serverOperation == OLO_ABOUT) {
        aboutMessage();
    } else if (options.update==UPDATE_DEFAULT)  {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "no input file");
        errorMessage(ERR_ST, tmpBuff);
    }
    if (options.xref2) {
        writeRelativeProgress(100);
    }
}


static void xref(int argc, char **argv) {
    ENTER();
    mainOpenOutputFile(options.outputFileName);
    editorLoadAllOpenedBufferFiles();

    mainCallXref(argc, argv);
    closeMainOutputFile();
    if (options.xref2) {
        ppcSynchronize();
    }
    //& fprintf(dumpOut, "\n\nDUMP\n\n"); fflush(dumpOut);
    //& mapOverReferenceTable(symbolRefItemDump);
    LEAVE();
}

/* *************************************************************** */
/*                          Edit Server Regime                     */
/* *************************************************************** */

void mainCallEditServerInit(int nargc, char **nargv) {
    initAvailableRefactorings();
    options.classpath = "";
    processOptions(nargc, nargv, INFILES_ENABLED); /* no include or define options */
    scheduleInputFilesFromArgumentsToFileTable();
    if (options.serverOperation == OLO_EXTRACT)
        cache.cpIndex = 2; // !!!! no cache, TODO why is 2 = no cache?
    initCompletions(&s_completions, 0, noPosition);
}

void mainCallEditServer(int argc, char **argv,
                        int nargc, char **nargv,
                        bool *firstPass
) {
    ENTER();
    editorLoadAllOpenedBufferFiles();
    if (creatingOlcxRefs())
        olcxPushEmptyStackItem(&sessionData.browserStack);
    if (needToProcessInputFile()) {
        if (presetEditServerFileDependingStatics() == NULL) {
            errorMessage(ERR_ST, "No input file");
        } else {
            editServerProcessFile(argc, argv, nargc, nargv, firstPass);
        }
    } else {
        if (presetEditServerFileDependingStatics() != NULL) {
            getFileItem(olOriginalComFileNumber)->scheduledToProcess = false;
            // added [26.12.2002] because of loading options without input file
            inputFilename = NULL;
        }
    }
    LEAVE();
}

static void editServer(int argc, char **argv) {
    int nargc;  char **nargv;
    bool firstPass;

    ENTER();
    cxResizingBlocked = 1;
    firstPass = true;
    copyOptions(&savedOptions, &options);
    for(;;) {
        currentPass = ANY_PASS;
        copyOptions(&options, &savedOptions);
        getPipedOptions(&nargc, &nargv);
        // O.K. -o option given on command line should catch also file not found
        // message
        mainOpenOutputFile(options.outputFileName);
        //&dumpOptions(nargc, nargv);
        log_trace("Server: Getting request");
        mainCallEditServerInit(nargc, nargv);
        if (communicationChannel==stdout && options.outputFileName!=NULL) {
            mainOpenOutputFile(options.outputFileName);
        }
        mainCallEditServer(argc, argv, nargc, nargv, &firstPass);
        if (options.serverOperation == OLO_ABOUT) {
            aboutMessage();
        } else {
            mainAnswerEditAction();
        }
        //& options.outputFileName = NULL;  // why this was here ???
        //editorCloseBufferIfNotUsedElsewhere(s_input_file_name);
        editorCloseAllBuffers();
        closeMainOutputFile();
        if (options.serverOperation == OLO_EXTRACT)
            cache.cpIndex = 2; // !!!! no cache
        if (options.xref2)
            ppcSynchronize();
        log_trace("Server: Request answered");
    }
    LEAVE();
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
            fatalError(ERR_ST, "cx_memory resizing required, see file TROUBLES",
                       XREF_EXIT_ERR);
        }
    }

    currentPass = ANY_PASS;
    totalTaskEntryInitialisations();
    mainTaskEntryInitialisations(argc, argv);

    // If there is no configuration file given auto-find it
    // TODO: This should probably be done where all other config file
    // discovery is done...
    if (options.xrefrc == NULL) {
        char *configFileName = findConfigFile(cwd);
        createOptionString(&options.xrefrc, configFileName);
        // And then the storage will be parallel to that
        strcpy(&configFileName[strlen(configFileName)-2], "db");
        setXrefsLocation(configFileName);
    }

    // Ok, so there were these five, now four, no three, main operating modes
    if (options.refactoringRegime == RegimeRefactory)
        refactory();
    if (options.taskRegime == RegimeXref)
        xref(argc, argv);
    if (options.taskRegime == RegimeEditServer)
        editServer(argc, argv);

    LEAVE();
    return 0;
}
