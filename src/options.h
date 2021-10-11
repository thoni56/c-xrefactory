#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "stdinc.h"
#include <stdbool.h>

#include "constants.h"
#include "memory.h"
#include "stringlist.h"
#include "refactorings.h"
#include "server.h"


/* Working regime in which the task is invoked */
typedef enum taskRegimes {
    RegimeUndefined = 0,        /* Explicitly zero so we can assert(regime) */
    RegimeXref, /* Cross referencer called by user from command line */
    RegimeEditServer, /* editor server, called by on-line editing action */
    RegimeRefactory /* refactoring server, called by on-line editing */
} TaskRegimes;

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
    Refactorings theRefactoring;
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
    int javaSlAllowed;          /* TODO: Boolify? */
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
    enum olcxOperations server_operation;
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
    struct stringPointerList *allAllocatedStrings;
    struct memory			 pendingMemory;
    char					 pendingFreeSpace[SIZE_opiMemory];
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
