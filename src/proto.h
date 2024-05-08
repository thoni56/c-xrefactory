#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED

#include "stdinc.h"
#include <stdbool.h>
#include <unistd.h>

typedef unsigned char uchar;

/* ***********************************************************************

   Here follows some enums which were split off a gigantic enum
   Miscellaneus (Duh!). In order to ensure they all have different
   values they are started with a value higher than the last value in
   the previous.

   TODO: should probably distribute these to where they needs to be defined...

*/

typedef enum longjmpReason {
    LONGJMP_REASON_NONE = 0,
    LONGJUMP_REASON_REFERENCE_OVERFLOW,
    LONGJMP_REASON_FILE_ABORT
} LongjmpReason;

typedef enum resolveDialog {
    RESOLVE_DIALOG_DEFAULT = LONGJMP_REASON_FILE_ABORT + 1,
    RESOLVE_DIALOG_ALWAYS,
    RESOLVE_DIALOG_NEVER,
} ResolveDialog;

typedef enum {
    DONT_ALLOCATE = RESOLVE_DIALOG_NEVER + 1,
    ALLOCATE_IN_SM,
    ALLOCATE_IN_PP
} MemoryKind;

typedef enum {
    DONT_PROCESS_FILE_ARGUMENTS = ALLOCATE_IN_PP + 1,
    PROCESS_FILE_ARGUMENTS
} ProcessFileArguments;

typedef enum {
    DONT_CHECK_NULL = PROCESS_FILE_ARGUMENTS + 1,
    CHECK_NULL
} CheckNull;

typedef enum {
    SEARCH_FULL = CHECK_NULL + 1,
    SEARCH_DEFINITIONS,
    SEARCH_FULL_SHORT,
    SEARCH_DEFINITIONS_SHORT
} SearchKind;

typedef enum {
    MARKER_IS_IN_CODE,
    MARKER_IS_IN_SLASH_COMMENT,
    MARKER_IS_IN_STAR_COMMENT
} MarkerLocationKind;

enum miscellaneous { /* misc. constants */
    DEFAULT_VALUE = SEARCH_DEFINITIONS_SHORT + 1,
    SEARCH_SYMBOL,
    SHORT_NAME,
    LONG_NAME,
    DO_NOT_CHECK_IF_SELECTED,
    DIFF_MISSING_REF,
    DIFF_UNEXPECTED_REF,
    GEN_FULL_OUTPUT,
    GEN_NO_OUTPUT,
    GET_PRIMARY_START,
    GET_STATIC_PREFIX_START,
    ALLOW_EDITOR_FILES,
    DO_NOT_ALLOW_EDITOR_FILES,
    DEPTH_ONE,
    DEPTH_ANY,
};


/* *******************      encodings      *************** */

enum fileEncodings {
    MULE_DEFAULT,  // in fact utf-8 or utf-16
    MULE_EUROPEAN, // byte per char
    MULE_EUC,      // euc
    MULE_SJIS,     // Shift-JIS
    MULE_UTF,      // utf-8 or utf-16
    MULE_UTF_8,    // utf-8
    MULE_UTF_16,   // utf-16
    MULE_UTF_16LE, // utf-16 little endian
    MULE_UTF_16BE, // utf-16 big endian
};

/* *******************      refactoring continuations      *************** */

typedef enum {
    RC_NONE,     // do not continue, keep interactive
    RC_CONTINUE, // continue, no special info
} ContinueRefactoringKind;

/* *******************      error messages type      *************** */

enum {
    ERR_ST, /* standard, no pre-prepared message*/
    ERR_CANT_OPEN,
    ERR_CANT_OPEN_FOR_READ,
    ERR_CANT_OPEN_FOR_WRITE,
    ERR_NO_MEMORY,
    ERR_INTERNAL,
    ERR_INTERNAL_CHECK,
    ERR_CFG,
};

/* ************************** refactorings **************************** */

enum memoryClass {
    MEMORY_XX,                  /* Means StackMemory... */
    MEMORY_PPM,
};

enum menuFilterLevels {
    FilterAllOfSameName,
    FilterSameProfile,
    FilterSameProfileRelatedClass,
    MAX_MENU_FILTER_LEVEL,
};

enum refsFilterLevels {
    RFilterAll,
    RFilterAddrVal, // also toplevel usage
    RFilterLVal,    // also extend usage
    RFilterDefinitions,
    MAX_REF_LIST_FILTER_LEVEL,
};

typedef enum result {
    RESULT_OK,
    RESULT_NOT_FOUND,
    RESULT_ERR
} Result;


/* ******************************************************************** */
/*                               STRUCTURES                             */
/* ******************************************************************** */

#include "position.h"

/* return value for IDENTIFIER token from yylex */

#include "id.h"

#include "typemodifier.h"
#include "constants.h"


typedef struct recFindStr {
    struct symbol     *baseClass;    /* class, application on which is looked*/
    struct symbol     *currentClass; /* current class, NULL for loc vars. */
    struct symbol     *nextRecord;
    unsigned           recsClassCounter;
    int                superClassesCount;
    struct symbolList *superClasses[MAX_INHERITANCE_DEEP]; /* super classes stack */
    int                anonymousUnionsCount;
    struct symbol     *anonymousUnions[MAX_ANONYMOUS_FIELDS]; /* anonymous unions */
} S_recFindStr;

typedef struct extRecFindStr {
    struct recFindStr        s;
    struct symbol           *memb;
    struct typeModifierList *params;
} S_extRecFindStr;

typedef struct nestedSpec {
    struct symbol *cl;
    bool           membFlag; /* flag whether it is nested in class */
    short unsigned accFlags;
} S_nestedSpec;


typedef struct symStructSpec {
    struct symbolList  *super;          /* list of super classes & interfaces */
    struct symbol      *records;        /* str. records, should be a table of   */
    short int           nestedCount;    /* # of java nested classes     */
    struct nestedSpec  *nestedClasses;  /* array of nested classes		*/
    struct typeModifier type;           /* this structure type */
    struct typeModifier ptrtype;        /* this structure pointer type */
    int                 classFileNumber; /* in java, my class file index
                                           == -1 for none, TODO to change
                                           it to s_noneFileIndex !!!
                                        */
    unsigned recSearchCounter;          /* tmp counter when looking for a record
                                              it flags searched classes
                                           */
} S_symStructSpec;

typedef struct jslSymbolList {
    struct symbol        *d;
    struct position       position;
    bool                  isExplicitlyImported;
    struct jslSymbolList *next;
} JslSymbolList;

/* ****************************************************************** */
/*          symbol definition item in cross-reference table           */

#include "reference.h"
#include "server.h"
#include "head.h"


/* ***************** on - line cross referencing ***************** */

typedef struct olcxReferences {
    struct reference    *references;     /* list of references */
    struct reference    *actual;         /* actual reference */
    ServerOperation      command;        /* OLO_PUSH/OLO_LIST/OLO_COMPLETION */
    time_t               accessTime;     /* last access time */
    struct position      callerPosition; /* caller position */
    struct completion *completions;    /* completions list for OLO_COMPLETION */
    // following two lists should be probably split into hashed tables of lists
    // because of bad performances for class tree and global unused symbols
    struct SymbolsMenu    *hkSelectedSym; /* resolved symbols under the cursor */
    struct SymbolsMenu    *menuSym;       /* hkSelectedSyms plus same name */
    int                    menuFilterLevel;
    int                    refsFilterLevel;
    struct olcxReferences *previous;
} OlcxReferences;

typedef struct classTreeData {
    int                 baseClassFileNumber;
    struct SymbolsMenu *treeMenu;
} ClassTreeData;

typedef struct OlcxReferencesStack {
    OlcxReferences *top;
    OlcxReferences *root;
} OlcxReferencesStack;


typedef struct currentlyParsedInfo {
    bool              extractProcessedFlag;
    bool              marker1Flag;
    int               marker2Flag;
    char              setTargetAnswerClass[TMP_STRING_SIZE];
    bool              moveTargetApproved;
    char              currentPackageAnswer[TMP_STRING_SIZE];
    char              currentClassAnswer[TMP_STRING_SIZE];
    char              currentSuperClassAnswer[TMP_STRING_SIZE];
    int               methodCoordEndLine;
    int               classCoordEndLine;
    struct codeBlock *workMemoryIndexAtBlockBegin;
    struct codeBlock *workMemoryIndexAtBlockEnd;
    struct symbol    *function;
    unsigned          functionBeginPosition;
    int               cxMemoryIndexAtFunctionBegin;
    int               cxMemoryIndexAtFunctionEnd;
    int               cxMemoryIndexAtBlockBegin;
    int               cxMemoryIndexAtBlockEnd;
    int               lastImportLine;
    struct symbol    *lastDeclaratorType;
    struct symbol    *lastAssignmentStruct;
} ParsedInfo;

/* *********************************************************** */

typedef struct expressionTokenType {
    struct typeModifier *typeModifier;
    struct reference    *reference;
    struct position     *position;
} ExpressionTokenType;

typedef struct nestedConstrTokenType {
    struct typeModifier *typeModifier;
    struct idList       *idList;
    struct position     *position;
} NestedConstrTokenType;

typedef struct unsignedPositionPair {
    unsigned         u;
    struct position *position;
} UnsignedPositionPair;

typedef struct symbolPositionPair {
    struct symbol   *symbol;
    struct position *position;
} SymbolPositionPair;

typedef struct symbolPositionListPair {
    struct symbol       *symbol;
    struct positionList *positionList;
} SymbolPositionListPair;

typedef struct intPair {
    int i1;
    int i2;
} IntPair;

typedef struct typeModifiersListPositionListPair {
    struct typeModifierList *typeModifierList;
    struct positionList     *positionList;
} TypeModifiersListPositionListPair;

#endif
