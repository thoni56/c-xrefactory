#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED

#include <stdbool.h>
#include <unistd.h>
#include <time.h>


typedef unsigned char uchar;

/* ***********************************************************************

   Here follows some enums which were split off a gigantic enum
   Miscellaneus (Duh!). In order to ensure they all have different
   values they are started with a value higher than the last value in
   the previous.

   TODO: should probably distribute these to where they needs to be defined...

*/

typedef enum longjmpReason {
    LONGJMP_REASON_NONE = 0,    /* Make sure it's zero */
    LONGJMP_REASON_REFERENCES_OVERFLOW,
    LONGJMP_REASON_FILE_ABORT
} LongjmpReason;

typedef enum resolveDialog {
    RESOLVE_DIALOG_DEFAULT,
    RESOLVE_DIALOG_ALWAYS,
    RESOLVE_DIALOG_NEVER,
} ResolveDialog;

typedef enum {
    ALLOCATE_NONE,
    ALLOCATE_IN_SM,
    ALLOCATE_IN_PP
} AllocateMemoryKind;

typedef enum {
    PROCESS_FILE_ARGUMENTS_NO,
    PROCESS_FILE_ARGUMENTS_YES
} ProcessFileArguments;

typedef enum {
    CHECK_NULL_NO,
    CHECK_NULL_YES
} CheckNull;

typedef enum {
    SEARCH_FULL,
    SEARCH_DEFINITIONS,
    SEARCH_FULL_SHORT,
    SEARCH_DEFINITIONS_SHORT
} SearchKind;

typedef enum {
    MARKER_IS_IN_CODE,
    MARKER_IS_IN_SLASH_COMMENT,
    MARKER_IS_IN_STAR_COMMENT
} MarkerLocationKind;

typedef enum {
    GET_PRIMARY_START,
    GET_STATIC_PREFIX_START
} ExpressionStartKind;

enum miscellaneous { /* misc. constants */
    DEFAULT_VALUE = GET_STATIC_PREFIX_START + 1,
    SEARCH_SYMBOL,
    DO_NOT_CHECK_IF_SELECTED,
    DIFF_MISSING_REF,
    DIFF_UNEXPECTED_REF,
    GEN_FULL_OUTPUT,
    GEN_NO_OUTPUT,
    ALLOW_EDITOR_FILES,
    DO_NOT_ALLOW_EDITOR_FILES,
};

typedef enum {
    DEPTH_ONE,
    DEPTH_ANY
} SearchDepth;


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


typedef enum memoryClass {
    MEMORY_XX,                  /* Means StackMemory... */
    MEMORY_PPM,
} MemoryClass;

typedef enum result {
    RESULT_OK,
    RESULT_NOT_FOUND,
    RESULT_ERR
} Result;


/* ******************************************************************** */
/*                               STRUCTURES                             */
/* ******************************************************************** */

#include "typemodifier.h"
#include "constants.h"


typedef struct {
    struct symbol     *currentStructure; /* current class, NULL for loc vars. */
    struct symbol     *nextMember;
    unsigned           memberFindCount;
    int                anonymousUnionsCount;
    struct symbol     *anonymousUnions[MAX_ANONYMOUS_FIELDS]; /* anonymous unions */
} StructMemberFindInfo;


typedef struct structSpec {
    struct symbol      *members;        /* str. records, should be a table of   */
    struct typeModifier type;           /* this structure type */
    struct typeModifier ptrtype;        /* this structure pointer type */
    unsigned memberSearchCounter;       /* tmp counter when looking for a record it
                                           flags searched classes */
} StructSpec;


typedef struct parsedInfo {
    bool              extractProcessedFlag;
    bool              regionMarkerAtCursorInserted;
    bool              regionMarkerAtMarkInserted;
    bool              moveTargetAccepted;
    int               methodCoordEndLine;
    struct codeBlock *blockAtBegin;
    struct codeBlock *blockAtEnd;
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
