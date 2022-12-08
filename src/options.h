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

typedef struct variable {
    int count;
    char *name[MAX_SET_GET_OPTIONS];
    char *value[MAX_SET_GET_OPTIONS];
} Variable;

typedef enum updateType {
    UPDATE_DEFAULT = 0,              // must be zero because of tests like 'if (options.update)...'
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
    NestedClassesDisplay displayNestedClasses;
    char *pushName;
    int parnum2;
    char *refpar1;
    char *refpar2;
    Refactoring theRefactoring;
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

    char *variableToGet;
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
    int olCursorPosition;
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
    struct variable variables;

    // list of strings
    struct stringPointerList *allAllocatedStrings;

    // Memory area for option strings
    Memory memory;
} Options;


/* PUBLIC DATA: */
extern Options options;            // current options
extern Options savedOptions;
extern Options presetOptions;


/* FUNCTIONS */

extern void aboutMessage(void);

extern void xrefSetenv(char *name, char *val);
extern char *getVariable(char *name);

extern void dirInputFile(MAP_FUN_SIGNATURE);

extern void processFileArguments(void);
extern void processOptions(int argc, char **argv, ProcessFileArguments infilesFlag);
extern void createOptionString(char **optAddress, char *text);
extern void copyOptionsFromTo(Options *src, Options *dest);
extern void getXrefrcFileName(char *ttt);
extern void addStringListOption(StringList **optlist, char *argvi);
extern char *getJavaHome(void);
extern void getJavaClassAndSourcePath(void);
extern bool packageOnCommandLine(char *packageName);
extern char *expandPredefinedSpecialVariables_static(char *output, char *inputFilename);
extern bool readOptionsFromFileIntoArgs(FILE *ff, int *nargc, char ***nargv,
                               MemoryKind memFl, char *sectionFile, char *project, char *resSection);
extern void readOptionsFromFile(char *name, int *nargc, char ***nargv, char *sectionFile, char *project);
extern void readOptionsFromCommand(char *command, int *nargc, char ***nargv, char *sectionFile);
extern void getPipedOptions(int *outNargc,char ***outNargv);
extern void javaSetSourcePath(bool defaultClassPathAllowed);
extern bool referenceFileCountMatches(int newRefNum);

extern char *findConfigFile(char *cwd);
extern void searchStandardOptionsFileAndSectionForFile(char *filename, char *optionsFilename, char *section);

#endif
