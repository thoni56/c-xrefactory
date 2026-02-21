#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "argumentsvector.h"
#include "proto.h"
#include "head.h"
#include <stdbool.h>
#include <stdio.h>

#include "constants.h"
#include "completion.h"
#include "extract.h"
#include "memory.h"
#include "refactorings.h"
#include "server.h"
#include "stringlist.h"


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
    UPDATE_CREATE,                   // create from scratch, skip compatibility checks
} UpdateType;

/* *******************      comment moving levels for refactoring      *************** */

typedef enum commentMovingMode {
    CM_NO_COMMENT,
    CM_SINGLE_SLASHED,
    CM_SINGLE_STARRED,
    CM_SINGLE_SLASHED_AND_STARRED,
    CM_ALL_SLASHED,
    CM_ALL_STARRED,
    CM_ALL_SLASHED_AND_STARRED,
} CommentMovingMode;

#define NO_CURSOR_OFFSET -1


/* protected data type */
typedef struct pointerLocationList LocationList;

extern void **pointerLocationOf(LocationList *list);
extern LocationList *nextPointerLocationList(LocationList *list);
extern bool containsPointerLocation(LocationList *list, void **location);


/*
 * Option source categories:
 *
 * SESSION     - Command line only, fixed for the server's lifetime.
 * PROJECT     - From .c-xrefrc config file. Cached across requests via
 *               checkpoint. Re-read when config file mtime changes.
 * REQUEST     - Piped from editor client, per-request. Never persisted
 *               across requests.
 * REFACTORING - Set internally by refactoring engine. Only meaningful
 *               during a refactoring operation.
 * INTERNAL    - Bookkeeping, not a user-facing option.
 */
typedef struct options {
    /* --- SESSION: Command line only, fixed for the server's lifetime --- */
    Mode mode;                                  /* SESSION */
    bool exit;                                  /* SESSION */
    bool xref2;                                 /* SESSION */
    char *xrefrc;                               /* SESSION */
    char *commandlog;                           /* SESSION */
    char *outputFileName;                       /* SESSION */
    bool errors;                                /* SESSION */
    bool debug;                                 /* SESSION */
    bool trace;                                 /* SESSION */
    bool lexemTrace;                            /* SESSION */
    bool fileTrace;                             /* SESSION */
    bool statistics;                            /* SESSION */
    UpdateType update;                          /* SESSION/REFACTORING */

    /* --- PROJECT: From .c-xrefrc config file, cached via checkpoint --- */
    char *compiler;                             /* PROJECT */
    char *definitionStrings;                    /* PROJECT */
    StringList *pruneNames;                     /* PROJECT */
    StringList *inputFiles;                     /* PROJECT */
    StringList *includeDirs;                    /* PROJECT */
    char *cxFileLocation;                       /* PROJECT */
    char *cFilesSuffixes;                       /* PROJECT */
    int eolConversion;                          /* PROJECT */
    bool fileNamesCaseSensitive;                /* PROJECT */
    int cxMemoryFactor;                         /* PROJECT */
    bool updateOnlyModifiedFiles;               /* PROJECT */
    int cxFileCount;                            /* PROJECT */
    int tabulator;                              /* PROJECT */

    /* --- REQUEST: Piped from editor client, per-request --- */
    ServerOperation serverOperation;            /* REQUEST */
    char *project;                              /* REQUEST */
    char *pushName;                             /* REQUEST */
    char *browsedName;                          /* REQUEST */
    char *variableToGet;                        /* REQUEST */
    char *olcxlccursor;                         /* REQUEST */
    char *olcxSearchString;                     /* REQUEST */
    SearchKind searchKind;                      /* REQUEST */
    bool completionCaseSensitive;               /* REQUEST */
    bool noErrors;                              /* REQUEST */
    bool exactPositionResolve;                  /* REQUEST */
    int cursorOffset;                           /* REQUEST */
    int markOffset;                             /* REQUEST */
    int filterValue;                            /* REQUEST */
    ResolveDialog manualResolve;                /* REQUEST */
    int lineNumberOfMenuSelection;              /* REQUEST */
    int olineLen;                               /* REQUEST */
    int maxCompletions;                         /* REQUEST */
    int olcxGotoVal;                            /* REQUEST */

    /* --- REFACTORING: Set internally by refactoring engine --- */
    Refactoring theRefactoring;                 /* REFACTORING */
    ContinueRefactoringKind continueRefactoring; /* REFACTORING */
    CommentMovingMode commentMovingMode;        /* REFACTORING */
    ExtractMode extractMode;                    /* REFACTORING */
    char *renameTo;                             /* REFACTORING */
    char *moveTargetFile;                       /* REFACTORING */
    char *refactor_parameter_name;              /* REFACTORING */
    char *refactor_parameter_value;             /* REFACTORING */
    char *refactor_target_line;                 /* REFACTORING */
    int parnum;                                 /* REFACTORING */
    int parnum2;                                /* REFACTORING */
    char *checkFileMovedFrom;                   /* REFACTORING */
    char *checkFileMovedTo;                     /* REFACTORING */
    int checkFirstMovedLine;                    /* REFACTORING */
    int checkLinesMoved;                        /* REFACTORING */
    int checkNewLineNumber;                     /* REFACTORING */

    /* --- INTERNAL: Bookkeeping, not user-facing --- */
    char *detectedProjectRoot;                  /* INTERNAL */
    int variablesCount;                         /* INTERNAL */
    Variable variables[MAX_SET_GET_OPTIONS];     /* INTERNAL */
    LocationList *allUsedStringOptions;          /* INTERNAL */
    LocationList *allUsedStringListOptions;      /* INTERNAL */
    Memory memory;                              /* INTERNAL */
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
extern void recoverMemoryFromIncludeList(void);

extern void processFileArguments(void);
extern void processOptions(ArgumentsVector args, ProcessFileArguments infilesFlag);

/* Handling of allocated string and string list options that need to be "shifted" on deep copy */
extern char *allocateStringForOption(char **pointerToOption, char *string);
extern void addToStringListOption(StringList **pointerToOption, char *string);
extern void deepCopyOptionsFromTo(Options *src, Options *dest);

extern char *expandPredefinedSpecialVariables_static(char *output, char *inputFilename);

extern bool readOptionsIntoArgs(FILE *file, ArgumentsVector *outArgs, Memory *memory,
                                        char *sectionFile, char *project, char *section);
extern void readOptionsFromCommand(char *command, ArgumentsVector *outArgs, char *sectionFile);
extern ArgumentsVector readOptionsFromFile(char *name, char *project, char *foundProjectName);
extern ArgumentsVector readOptionsFromPipe(void);

extern bool currentCxFileCountMatches(int newRefNum);

extern void searchForProjectOptionsFileAndProjectForFile(char *filename, char *optionsFilename,
                                                         char *foundProjectName);
extern void applyConventionBasedDatabasePath(void);

extern void printOptionsMemoryStatistics(void);

#endif
