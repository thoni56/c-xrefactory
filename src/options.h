#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "proto.h"
#include "stdinc.h"
#include <stdbool.h>

#include "constants.h"
#include "memory.h"
#include "stringlist.h"
#include "refactorings.h"
#include "server.h"


/* Working regime in which the task is invoked */
typedef enum {
    RegimeUndefined = 0, /* Explicitly zero so we can assert(regime) */
    RegimeXref,          /* Cross referencer called by user from command line */
    RegimeEditServer,    /* editor server, called by on-line editing action */
    RegimeRefactory      /* refactoring server, called by on-line editing */
} TaskRegimes;

typedef struct setGetEnv {
    int num;
    char *name[MAX_SET_GET_OPTIONS];
    char *value[MAX_SET_GET_OPTIONS];
} S_setGetEnv;

typedef enum updateType {
    UPDATE_DEFAULT = 0,              // must be zero ! TODO: Why?
    UPDATE_FAST,
    UPDATE_FULL,
} UpdateType;



typedef struct options {
    /* GENERAL */
    bool exit;
    char *compiler;
    int fileEncoding;
    char completeParenthesis;
    int defaultAddImportStrategy;
    char referenceListWithoutSource;
    int completionOverloadWizardDeep;
    CommentMovingMode commentMovingMode;
    struct stringList *pruneNames;
    struct stringList *inputFiles;
    int continueRefactoring;
    int completionCaseSensitive;
    char *xrefrc;
    int eolConversion;
    char *checkVersion;
    CutOuters displayNestedClasses;
    char *pushName;
    int parnum2;
    char *refpar1;
    char *refpar2;
    Refactorings theRefactoring;
    bool briefoutput;
    int cacheIncludes;
    char *renameTo;
    TaskRegimes refactoringRegime;
    bool xref2;
    char *moveTargetFile;
    char *cFilesSuffixes;
    char *javaFilesSuffixes;
    char *cppFilesSuffixes;
    int fileNamesCaseSensitive;
    int tagSearchSpecif;
    char *olcxWinDelFile;
    int olcxWinDelFromLine;
    int olcxWinDelFromCol;
    int olcxWinDelToLine;
    int olcxWinDelToCol;
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
    char *cxrefsLocation;

    char *checkFileMovedFrom;
    char *checkFileMovedTo;
    int checkFirstMovedLine;
    int checkLinesMoved;
    int checkNewLineNumber;

    char *getValue;
    int javaSlAllowed;          /* TODO: Boolify? */

    /* JAVADOC: */
    char *htmlJdkDocAvailable;
    char *htmlJdkDocUrl;

    /* JAVA: */
    char *javaDocPath;
    int allowPackagesOnCommandLine;
    char *sourcePath;
    char *jdocTmpDir;

    /* MIXED THINGS... */
    bool noIncludeRefs;
    bool allowClassFileRefs;
    int filterValue;
    char *jdkClassPath;
    ResolveDialog manualResolve;
    char *browsedSymName;
    bool modifiedFlag;
    int olcxMenuSelectLineNum;
    int ooChecksBits;
    int cxMemoryFactor;
    bool strictAnsi;
    char *project;
    char *olcxlccursor;
    char *olcxSearchString;
    int olineLen;
    char *olExtractAddrParPrefix;
    int extractMode;
    int maxCompletions;
    int editor;
    bool create;
    char *classpath;
    int tabulator;
    int olCursorPos;
    int olMarkPos;
    TaskRegimes taskRegime;
    bool debug;
    bool trace;
    ServerOperation serverOperation;
    int olcxGotoVal;
    char *originalDir;

    bool no_stdoptions;

    /* CXREF options  */
    bool errors;
    UpdateType update;
    bool updateOnlyModifiedFiles;
    int referenceFileCount;

    // all the rest initialized to zeros by default
    struct setGetEnv setGetEnv;

    // list of strings
    struct stringPointerList *allAllocatedStrings;
    // Memory area
    struct memory			 memory; /* TODO: WTF: this structs last field is used to overrun... */
    char					 pendingFreeSpace[SIZE_optMemory]; /* ... into this area! */
} Options;


/* PUBLIC DATA: */
extern Options options;            // current options
extern Options refactoringOptions; // xref -refactory command line options
extern Options cachedOptions;
extern Options presetOptions;

/* FUNCTIONS */
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
extern void javaSetSourcePath(bool defaultClassPathAllowed);
extern bool checkReferenceFileCountOption(int newRefNum);

extern char *findConfigFile(char *cwd);

#endif
