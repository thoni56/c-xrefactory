#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED

#include <stdbool.h>
#include <unistd.h>
#include <time.h>


typedef unsigned char uchar;

/* ***********************************************************************

   TODO: should probably distribute these to where they needs to be defined...

*/

typedef enum resolveDialog {
    RESOLVE_DIALOG_DEFAULT,
    RESOLVE_DIALOG_ALWAYS,
    RESOLVE_DIALOG_NEVER,
} ResolveDialog;

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


/* *******************      confirmation responses      *************** */

typedef enum {
    RC_NONE,     // no response yet
    RC_CONTINUE, // user confirmed, continue
    RC_CANCEL,   // user declined, cancel
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
    int               declaratorCount;  /* In the top level declaration being parsed */
    unsigned          functionBeginPosition;
    unsigned          previousFunctionBeginPosition;
    int               functionNameLine; /* Where the name of the function begins */
    int               blockBeginLine;
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
