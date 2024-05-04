#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "proto.h"
#include "stdinc.h"
#include <stdbool.h>

#include "constants.h"
#include "completion.h"
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
    char *name;
    char *value;
} Variable;

typedef enum updateType {
    UPDATE_DEFAULT = 0,              // must be zero because of tests like 'if (options.update)...'
    UPDATE_FAST,
    UPDATE_FULL,
} UpdateType;


/* protected data type */
typedef struct pointerLocationList LocationList;

extern void **pointerLocationOf(LocationList *list);
extern LocationList *nextPointerLocationList(LocationList *list);
extern bool containsPointerLocation(LocationList *list, void **location);


typedef struct options {
    /* GENERAL */
    bool exit;
    char *commandlog;
    char *compiler;
    int fileEncoding;
    bool completeParenthesis;
    bool referenceListWithoutSource;
    CommentMovingMode commentMovingMode;
    StringList *pruneNames;
    StringList *inputFiles;
    ContinueRefactoringKind continueRefactoring;
    bool completionCaseSensitive;
    char *xrefrc;
    int eolConversion;
    char *checkVersion;
    char *pushName;
    int parnum2;
    char *refpar1;
    char *refpar2;
    Refactoring theRefactoring;
    char *renameTo;
    Mode refactoringMode;
    bool xref2;
    char *moveTargetFile;
    char *cFilesSuffixes;
    char *javaFilesSuffixes;    /* Keep this for warnings about Java not being supported... */
    bool fileNamesCaseSensitive;
    SearchKind searchKind;
    bool noErrors;
    bool exactPositionResolve;
    char *outputFileName;
    StringList *includeDirs;
    char *cxrefsLocation;

    char *checkFileMovedFrom;
    char *checkFileMovedTo;
    int checkFirstMovedLine;
    int checkLinesMoved;
    int checkNewLineNumber;

    char *variableToGet;

    /* MIXED THINGS... */
    bool noIncludeRefs;
    int filterValue;
    ResolveDialog manualResolve;
    char *browsedSymName;
    int olcxMenuSelectLineNum;
    int ooChecksBits;
    int cxMemoryFactor;
    bool strictAnsi;
    char *project;
    char *olcxlccursor;
    char *olcxSearchString;
    int olineLen;
    char *olExtractAddrParPrefix; /* Prefix for parameter names when extraction requires out arguments */
    ExtractMode extractMode;
    int maxCompletions;
    bool create;
    int tabulator;
    int olCursorOffset;
    int olMarkOffset;
    Mode mode;
    bool debug;
    bool trace;
    ServerOperation serverOperation;
    int olcxGotoVal;

    /* CXREF options  */
    bool errors;
    UpdateType update;
    bool updateOnlyModifiedFiles;
    int referenceFileCount;

    int variablesCount;
    Variable variables[MAX_SET_GET_OPTIONS];

    // list of strings - well actually allocated areas
    LocationList *allUsedStringOptions;
    LocationList *allUsedStringListOptions;

    // Memory for allocated option strings and lists
    Memory2 memory;
} Options;


/* PUBLIC DATA: */
extern Options options;            // current options
extern Options savedOptions;
extern Options presetOptions;


/* FUNCTIONS */

extern void aboutMessage(void);

extern void setOptionVariable(char *name, char *val);
extern char *getOptionVariable(char *name);

extern void dirInputFile(MAP_FUN_SIGNATURE);

extern void processFileArguments(void);
extern void processOptions(int argc, char **argv, ProcessFileArguments infilesFlag);

/* Handling of allocated string and string list options that need to be "shifted" on deep copy */
extern char *allocateStringForOption(char **pointerToOption, char *string);
extern void addToStringListOption(StringList **pointerToOption, char *string);
extern void deepCopyOptionsFromTo(Options *src, Options *dest);

extern char *expandPredefinedSpecialVariables_static(char *output, char *inputFilename);
extern bool readOptionsFromFileIntoArgs(FILE *ff, int *nargc, char ***nargv,
                    MemoryKind memFl, char *sectionFile, char *project, char *resSection);
extern void readOptionsFromFile(char *name, int *nargc, char ***nargv, char *project, char *foundProjectName);
extern void readOptionsFromCommand(char *command, int *nargc, char ***nargv, char *sectionFile);
extern void getPipedOptions(int *outNargc,char ***outNargv);
extern bool currentReferenceFileCountMatches(int newRefNum);

extern void searchStandardOptionsFileAndProjectForFile(char *filename, char *optionsFilename,
                               char *foundProjectName);

/* Experimental */
extern char *findConfigFile(char *cwd);

#endif
