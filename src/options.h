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
#include "extract.h"


/* Working mode in which the task is invoked */
typedef enum {
    UndefinedMode = 0, /* Explicitly zero so we can assert(mode) */
    XrefMode,          /* Cross referencer called by user from command line */
    ServerMode,        /* editor server, called by on-line editing action */
    RefactoryMode      /* refactoring server, called by on-line editing */
} Mode;

typedef struct setGetEnv {
    int num;
    char *name[MAX_SET_GET_OPTIONS];
    char *value[MAX_SET_GET_OPTIONS];
} SetGetEnv;

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
    bool completeParenthesis;
    int defaultAddImportStrategy;
    char referenceListWithoutSource;
    int completionOverloadWizardDeep;
    CommentMovingMode commentMovingMode;
    struct stringList *pruneNames;
    struct stringList *inputFiles;
    ContinueRefactoringKind continueRefactoring;
    bool completionCaseSensitive;
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
    char *renameTo;
    Mode refactoringMode;
    bool xref2;
    char *moveTargetFile;
    char *cFilesSuffixes;
    char *javaFilesSuffixes;
    bool fileNamesCaseSensitive;
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
    bool urlAutoRedirect;
    bool javaFilesOnly;
    bool exactPositionResolve;
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
    bool javaSlAllowed;

    /* JAVADOC: */
    char *htmlJdkDocAvailable;
    char *htmlJdkDocUrl;

    /* JAVA: */
    char *javaDocPath;
    bool allowPackagesOnCommandLine;
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
    ExtractMode extractMode;
    int maxCompletions;
    int editor;
    bool create;
    char *classpath;
    int tabulator;
    int olCursorPos;
    int olMarkPos;
    Mode mode;
    bool debug;
    bool trace;
    ServerOperation serverOperation;
    int olcxGotoVal;

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
extern Options savedOptions;
extern Options presetOptions;

/* FUNCTIONS */
extern void xrefSetenv(char *name, char *val);

extern void dirInputFile(MAP_FUN_SIGNATURE);

extern void processOptions(int argc, char **argv, ProcessFileArguments infilesFlag);
extern void createOptionString(char **optAddress, char *text);
extern void copyOptionsFromTo(Options *src, Options *dest);
extern void getXrefrcFileName(char *ttt);
extern void addStringListOption(StringList **optlist, char *argvi);
extern char *getJavaHome(void);
extern void getJavaClassAndSourcePath(void);
extern bool packageOnCommandLine(char *packageName);
extern char *expandSpecialFilePredefinedVariables_st(char *tt, char *inputFilename);
extern bool readOptionsFromFileIntoArgs(FILE *ff, int *nargc, char ***nargv,
                               MemoryKind memFl, char *sectionFile, char *project, char *resSection);
extern void readOptionsFromFile(char *name, int *nargc, char ***nargv, char *sectionFile, char *project);
extern void readOptionsFromCommand(char *command, int *nargc, char ***nargv, char *sectionFile);
extern void javaSetSourcePath(bool defaultClassPathAllowed);
extern bool referenceFileCountMatches(int newRefNum);

extern char *findConfigFile(char *cwd);

#endif
