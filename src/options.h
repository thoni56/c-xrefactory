#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "stdinc.h"
#include <stdbool.h>

#include "constants.h"
#include "memory.h"
#include "stringlist.h"

#include "refactorings.h"


/* Working regime in which the task is invoked */
typedef enum taskRegimes {
    RegimeUndefined = 0,        /* Explicitly zero so we can assert(regime) */
    RegimeXref, /* Cross referencer called by user from command line */
    //RegimeHtmlGenerate,   /* generate html form of input files, ... */
    RegimeEditServer, /* editor server, called by on-line editing action */
    RegimeRefactory /* refactoring server, called by on-line editing */
} TaskRegimes;

typedef struct htmlCutPathsOpts {
    int pathsNum;
    char *path[MAX_HTML_CUT_PATHS];
    int plen[MAX_HTML_CUT_PATHS];
} S_htmlCutPathsOpts;

typedef struct setGetEnv {
    int num;
    char *name[MAX_SET_GET_OPTIONS];
    char *value[MAX_SET_GET_OPTIONS];
} S_setGetEnv;

typedef enum updateType {
    UPDATE_CREATE = 0,              // must be zero ! TODO: Why?
    UPDATE_FAST,
    UPDATE_FULL,
} UpdateType;


/* ************** on-line (browsing) operations for c-xref server  ********** */
enum olcxOptions {
    OLO_NOOP = 0,
    OLO_COMPLETION,
    OLO_SEARCH,
    OLO_TAG_SEARCH,
    OLO_RENAME,        /* same as push, just another ordering */
    OLO_ENCAPSULATE,   /* same as rename, remove private references */
    OLO_ARG_MANIP, /* as rename, constructors resolved as functions */
    OLO_VIRTUAL2STATIC_PUSH, /* same as rename, another message on virtuals */
    OLO_GET_AVAILABLE_REFACTORINGS,
    OLO_PUSH,
    OLO_PUSH_NAME,
    OLO_PUSH_SPECIAL_NAME,		/* also reparsing current file */
    OLO_POP,
    OLO_POP_ONLY,
    OLO_NEXT,
    OLO_PREVIOUS,
    OLO_GOTO_CURRENT,
    OLO_GET_CURRENT_REFNUM,
    OLO_GOTO_PARAM_NAME,
    OLO_GLOBAL_UNUSED,
    OLO_LOCAL_UNUSED,
    OLO_LIST,
    OLO_LIST_TOP,
    OLO_PUSH_ONLY,
    OLO_PUSH_AND_CALL_MACRO,
    OLO_PUSH_ALL_IN_METHOD,
    OLO_PUSH_FOR_LOCALM,
    OLO_GOTO,
    OLO_CGOTO,                 /* goto completion item definition */
    OLO_TAGGOTO,               /* goto tag search result */
    OLO_TAGSELECT,             /* select tag search result */
    OLO_CBROWSE,               /* browse javadoc of completion item */
    OLO_REF_FILTER_SET,
    OLO_REF_FILTER_PLUS,
    OLO_REF_FILTER_MINUS,
    OLO_CSELECT,                /* select completion */
    OLO_COMPLETION_BACK,
    OLO_COMPLETION_FORWARD,
    OLO_EXTRACT,            /* extract block into separate function */
    OLO_CT_INSPECT_DEF,		/* inspect definition from class tree */
    OLO_MENU_INSPECT_DEF,	/* inspect definition from symbol menu */
    OLO_MENU_INSPECT_CLASS,	/* inspect class from symbol menu */
    OLO_MENU_SELECT,		/* select the line from symbol menu */
    OLO_MENU_SELECT_ONLY,	/* select only the line from symbol menu */
    OLO_MENU_SELECT_ALL,	/* select all from symbol menu */
    OLO_MENU_SELECT_NONE,	/* select none from symbol menu */
    OLO_MENU_FILTER_SET,	/* more strong filtering */
    OLO_MENU_FILTER_PLUS,	/* more strong filtering */
    OLO_MENU_FILTER_MINUS,	/* smaller filtering */
    OLO_MENU_GO,            /* push references from selected menu items */
    OLO_CHECK_VERSION,      /* check version correspondance */
    OLO_RESET_REF_SUFFIX,	/* set n-th argument after argument insert */
    OLO_TRIVIAL_PRECHECK,	/* trivial pre-refactoring checks */
    OLO_MM_PRE_CHECK,		/* move method pre check */
    OLO_PP_PRE_CHECK,		/* push-down/pull-up method pre check */
    OLO_SAFETY_CHECK_INIT,
    OLO_SAFETY_CHECK1,
    OLO_SAFETY_CHECK2,
    OLO_INTERSECTION,       /* just provide intersection of top references */
    OLO_REMOVE_WIN,         /* just remove window of top references */
    OLO_GOTO_DEF,           /* goto definition reference */
    OLO_GOTO_CALLER,        /* goto caller reference */
    OLO_SHOW_TOP,           /* show top symbol */
    OLO_SHOW_TOP_APPL_CLASS,   /* show current reference appl class */
    OLO_SHOW_TOP_TYPE,         /* show current symbol type */
    OLO_SHOW_CLASS_TREE,       /* show current class tree */
    OLO_TOP_SYMBOL_RES,        /* show top symbols resolution */
    OLO_ACTIVE_PROJECT,        /* show active project name */
    OLO_JAVA_HOME,             /* show inferred jdkclasspath */
    OLO_REPUSH,                /* re-push pop-ed top */
    OLO_CLASS_TREE,            /* display class tree */
    OLO_USELESS_LONG_NAME,     /* display useless long class names */
    OLO_USELESS_LONG_NAME_IN_CLASS, /* display useless long class names */
    OLO_MAYBE_THIS,         /* display 'this' class dependencies */
    OLO_NOT_FQT_REFS,       /* display not fully qualified names in method */
    OLO_NOT_FQT_REFS_IN_CLASS, /* display not fully qualified names of class */
    OLO_GET_ENV_VALUE,      /* get a value set by -set */
    OLO_SET_MOVE_TARGET,	/* set target place for moving action */
    OLO_SET_MOVE_CLASS_TARGET,	/* set target place for xref2 move class */
    OLO_SET_MOVE_METHOD_TARGET,	/* set target place for xref2 move method */
    OLO_GET_CURRENT_CLASS,
    OLO_GET_CURRENT_SUPER_CLASS,
    OLO_GET_METHOD_COORD,	/* get method beginning and end lines */
    OLO_GET_CLASS_COORD,	/* get class beginning and end lines */
    OLO_GET_SYMBOL_TYPE,	/* get type of a symbol */
    OLO_TAG_SEARCH_FORWARD,
    OLO_TAG_SEARCH_BACK,
    OLO_PUSH_ENCAPSULATE_SAFETY_CHECK,
    OLO_ENCAPSULATE_SAFETY_CHECK,
    OLO_SYNTAX_PASS_ONLY,    /* should replace OLO_GET_PRIMARY_START && OLO_GET_PARAM_COORDINATES */
    OLO_GET_PRIMARY_START,   /* get start position of primary expression */
    OLO_GET_PARAM_COORDINATES,
    OLO_ABOUT,
};

typedef struct options {
    /* GENERAL */
    int exit;
    char *compiler;
    int fileEncoding;
    char completeParenthesis;
    int defaultAddImportStrategy;
    char referenceListWithoutSource;
    int completionOverloadWizardDeep;
    int commentMovingLevel;
    struct stringList *pruneNames;
    struct stringList *inputFiles;
    int continueRefactoring;
    int completionCaseSensitive;
    char *xrefrc;
    int eolConversion;
    char *checkVersion;
    int nestedClassDisplaying;
    char *pushName;
    int parnum2;
    char *refpar1;
    char *refpar2;
    AvailableRefactorings theRefactoring;
    bool briefoutput;
    int cacheIncludes;
    int stdopFlag;		// does this serve to anything ?
    char *renameTo;
    enum taskRegimes refactoringRegime;
    int xref2;
    char *moveTargetFile;
    char *cFilesSuffixes;
    char *javaFilesSuffixes;
    char *cppFilesSuffixes;
    int fileNamesCaseSensitive;
    int tagSearchSpecif;
    char *javaVersion;
    char *olcxWinDelFile;
    int olcxWinDelFromLine;
    int olcxWinDelFromCol;
    int olcxWinDelToLine;
    int olcxWinDelToCol;
    char *moveFromUser;
    bool noErrors;
    int fqtNameToCompletions;
    char *moveTargetClass;
    int trivialPreCheckCode;
    bool urlGenTemporaryFile;
    int urlAutoRedirect;
    int javaFilesOnly;
    int exactPositionResolve;
    char *outputFileName;
    char *lineFileName;
    struct stringList *includeDirs;
    char *cxrefFileName;

    char *checkFileMovedFrom;
    char *checkFileMovedTo;
    int checkFirstMovedLine;
    int checkLinesMoved;
    int checkNewLineNumber;

    char *getValue;
    int javaSlAllowed;
    int xfileHashingMethod;

    /* JAVADOC: */
    char *htmlJdkDocAvailable;
    char *htmlJdkDocUrl;

    /* JAVA: */
    char *javaDocPath;
    int allowPackagesOnCommandLine;
    char *sourcePath;
    char *jdocTmpDir;
    int noCxFile;
    bool javaDoc;
    bool noIncludeRefs;
    bool allowClassFileRefs;
    int filterValue;
    char *jdkClassPath;
    int manualResolve;
    char *browsedSymName;
    bool modifiedFlag;
    int olcxMenuSelectLineNum;
    int ooChecksBits;
    int cxMemoryFactor;
    int multiHeadRefsCare;
    bool strictAnsi;
    char *project;
    char *olcxlccursor;
    char *olcxSearchString;
    int olineLen;
    char *olExtractAddrParPrefix;
    int extractMode;
    int maxCompletions;
    int editor;
    int create;
    char *olcxRefSuffix;
    bool recurseDirectories;
    char *classpath;
    int tabulator;
    int olCursorPos;
    int olMarkPos;
    enum taskRegimes taskRegime;
    char *user;
    bool debug;
    bool trace;
    bool cpp_comments;
    enum olcxOptions server_operation;
    int olcxGotoVal;
    char *originalDir;

    bool no_stdop;

    /* CXREF options  */
    bool errors;
    UpdateType update;
    bool updateOnlyModifiedFiles;
    char *last_message;
    int referenceFileCount;

    // all the rest initialized to zeros by default
    struct setGetEnv setGetEnv;

    // memory for strings
    struct stringPointerList	*allAllocatedStrings;
    struct memory			pendingMemory;
    char					pendingFreeSpace[SIZE_opiMemory];
} Options;


/* PUBLIC DATA: */
extern Options options;            // current options
extern Options refactoringOptions; // xref -refactory command line options
extern Options s_cachedOptions;
extern Options s_initOpt;

extern void xrefSetenv(char *name, char *val);

extern void getXrefrcFileName(char *ttt);
extern void addStringListOption(StringList **optlist, char *argvi);
extern char *getJavaHome(void);
extern void getJavaClassAndSourcePath(void);
extern bool packageOnCommandLine(char *packageName);
extern char *expandSpecialFilePredefinedVariables_st(char *tt, char *inputFilename);
extern bool readOptionFromFile(FILE *ff, int *nargc, char ***nargv,
                               int memFl, char *sectionFile, char *project, char *resSection);
extern void readOptionFile(char *name, int *nargc, char ***nargv, char *sectionFile, char *project);
extern void readOptionPipe(char *command, int *nargc, char ***nargv, char *sectionFile);
extern void javaSetSourcePath(int defaultCpAllowed);
extern bool checkReferenceFileCountOption(int newRefNum);

#endif
