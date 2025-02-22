#ifndef PROTO_H_INCLUDED
#define PROTO_H_INCLUDED

#include "stdinc.h"
#include <stdbool.h>

#include "zlib.h"


typedef unsigned char uchar;

/* ***********************************************************************

   Here follows some enums which were split off a gigantic enum
   Miscellaneus (Duh!). In order to ensure they all have different
   values they are started with a value higher than the last value in
   the previous.

   TODO: should probably distribute these to where they needs to be defined...

*/

typedef enum addYesNo {
    ADD_DEFAULT = 0,
    ADD_YES,
    ADD_NO
} AddYesNo;

typedef enum accessibilityCheckYesNo {
    ACCESSIBILITY_CHECK_DEFAULT = ADD_NO + 1,
    ACCESSIBILITY_CHECK_YES,
    ACCESSIBILITY_CHECK_NO
} AccessibilityCheckYesNo;

typedef enum visibilityCheckYesNo {
    VISIBILITY_CHECK_DEFAULT = ACCESSIBILITY_CHECK_NO + 1,
    VISIBILITY_CHECK_YES,
    VISIBILITY_CHECK_NO
} VisibilityCheckYesNo;

typedef enum longjmpReason {
    LONGJMP_REASON_NONE = VISIBILITY_CHECK_NO + 1,
    LONGJUMP_REASON_REFERENCE_OVERFLOW,
    LONGJMP_REASON_FILE_ABORT
} LongjmpReason;

typedef enum resolveDialog {
    RESOLVE_DIALOG_DEFAULT = LONGJMP_REASON_FILE_ABORT + 1,
    RESOLVE_DIALOG_ALWAYS,
    RESOLVE_DIALOG_NEVER,
} ResolveDialog;

typedef enum cutOuters {
    DONT_DISPLAY_NESTED_CLASSES = RESOLVE_DIALOG_NEVER + 1,
    DISPLAY_NESTED_CLASSES,
} NestedClassesDisplay;

typedef enum dotifyMode {
    DOTIFY_NAME = DISPLAY_NESTED_CLASSES + 1,
    KEEP_SLASHES,
} DotifyMode;

typedef enum includeCxrefs {
    NO_CX_REFS = KEEP_SLASHES + 1,
    ADD_CX_REFS
} IncludeCxrefs;

typedef enum {
    DONT_ALLOCATE = ADD_CX_REFS + 1,
    ALLOCATE_IN_SM,
    ALLOCATE_IN_PP
} MemoryKind;

typedef enum {
    DONT_PROCESS_FILE_ARGUMENTS = ALLOCATE_IN_PP,
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

enum miscellaneous { /* misc. constants */
    DEFAULT_VALUE = SEARCH_DEFINITIONS_SHORT + 1,
    CLASS_TO_TYPE,
    CLASS_TO_EXPR,
    CLASS_TO_METHOD,
    CLASS_TO_ANY,
    CX_FILE_ITEM_GEN,
    NO_CX_FILE_ITEM_GEN,
    LOADING_SYMBOL,
    SEARCH_SYMBOL,
    SHORT_NAME,
    LONG_NAME,
    GEN_JAVA_DOC,
    ONLINE_ONLY,
    ALL_REFS,
    COUNT_ONLY,
    VIRT_ITEM,
    SINGLE_VIRT_ITEM,
    FIRST_PASS,
    SECOND_PASS,
    DO_NOT_CHECK_IF_SELECTED,
    CPOS_FUNCTION_INNER,
    CPOS_ST,
    DIRECT_ONLY,
    SUPERCLASS_NESTED_TOO,
    DIFF_MISSING_REF,
    DIFF_UNEXPECTED_REF,
    CONSTRUCTOR_INVOCATION,
    REGULAR_METHOD,
    SUPER_METHOD_INVOCATION,
    GLOBAL_ENV_ONLY,
    ORDER_PREPEND,
    ORDER_APPEND,
    MEMBER_CLASS,
    MEMBER_TYPE,
    GEN_VIRTUALS,
    GEN_NON_VIRTUALS,
    REQ_FIELD,
    REQ_METHOD,
    REQ_STATIC,
    REQ_NONSTATIC,
    REQ_SUPERCLASS,
    REQ_SUBCLASS,
    REQ_CLASS,
    REQ_PACKAGE,
    INSPECT_DEF,
    INSPECT_CLASS,
    ADD_MAYBE_THIS_REFERENCE,
    GEN_FULL_OUTPUT,
    GEN_PRECHECKS,
    GEN_NO_OUTPUT,
    GET_PRIMARY_START,
    GET_STATIC_PREFIX_START,
    APPLY_CHECKS,
    NO_CHECKS,
    CONTINUATION_ENABLED,
    CONTINUATION_DISABLED,
    NO_OVERWRITE_CHECK,
    CHECK_OVERWRITE,
    ALLOW_EDITOR_FILES,
    DO_NOT_ALLOW_EDITOR_FILES,
    DEPTH_ONE,
    DEPTH_ANY,
    INTERACTIVE_YES,
    INTERACTIVE_NO,
    MARKER_IS_IN_CODE,
    MARKER_IS_IN_SLASH_COMMENT,
    MARKER_IS_IN_STAR_COMMENT,
    PROFILE_NOT_APPLICABLE,
    PROFILE_APPLICABLE,
    PROFILE_PARTIALLY_APPLICABLE,
    USELESS_FQT_REFS_ALLOWED,
    USELESS_FQT_REFS_DISALLOWED,
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

/* *******************      generating imports defaults      *************** */

// do not touch this! order is used, with arithmetics!
typedef enum addImportsDefault {
    IMPORT_ON_DEMAND,
    IMPORT_SINGLE_TYPE,
    IMPORT_KEEP_FQT_NAME,
} AddImportStrategyKind;

/* *******************      refactoring continuations      *************** */

typedef enum {
    RC_NONE,     // do not continue, keep interactive
    RC_CONTINUE, // continue, no special info
    RC_IMPORT_SINGLE_TYPE,
    RC_IMPORT_ON_DEMAND,
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
    MEMORY_CF,                  /* Seems to only be used inSecondJslPass() */
    MEMORY_XX,                  /* Means StackMemory... */
    MEMORY_PPM,
};

enum menuFilterLevels {
    FilterAllOfSameName,
    FilterSameProfile,
    FilterSameProfileRelatedClass,
    //&	FilterVirtualAdequate,
    //&	FilterVirtualSameAppl,
    MAX_MENU_FILTER_LEVEL,
};

enum refsFilterLevels {
    RFilterAll,
    RFilterAddrVal, // also toplevel usage
    RFilterLVal,    // also extend usage
    RFilterDefinitions,
    MAX_REF_LIST_FILTER_LEVEL,
};

#include "storage.h"
#include "type.h"

enum javaPCTIndex { /* java Primitive Conversion Table Indexes */
    PCTIndexError = 0,
    PCTIndexByte,
    PCTIndexShort,
    PCTIndexChar,
    PCTIndexInt,
    PCTIndexLong,
    PCTIndexFloat,
    PCTIndexDouble,
    MAX_PCTIndex
};

typedef enum result {
    RESULT_OK,
    RESULT_NOT_FOUND,
    RESULT_ERR
} Result;

typedef enum syntaxPassParsedImportantPosition {
    SPP_LAST_TOP_LEVEL_CLASS_POSITION,
    SPP_ASSIGNMENT_OPERATOR_POSITION,
    SPP_ASSIGNMENT_END_POSITION,
    SPP_CLASS_DECLARATION_BEGIN_POSITION,
    SPP_CLASS_DECLARATION_END_POSITION,
    SPP_CLASS_DECLARATION_TYPE_BEGIN_POSITION,
    SPP_CLASS_DECLARATION_TYPE_END_POSITION,
    SPP_METHOD_DECLARATION_BEGIN_POSITION,
    SPP_METHOD_DECLARATION_END_POSITION,
    SPP_METHOD_DECLARATION_TYPE_BEGIN_POSITION,
    SPP_METHOD_DECLARATION_TYPE_END_POSITION,
    SPP_FIELD_DECLARATION_BEGIN_POSITION,
    SPP_FIELD_DECLARATION_END_POSITION,
    SPP_FIELD_DECLARATION_TYPE_BEGIN_POSITION,
    SPP_FIELD_DECLARATION_TYPE_END_POSITION,
    SPP_CAST_LPAR_POSITION,
    SPP_CAST_RPAR_POSITION,
    SPP_CAST_TYPE_BEGIN_POSITION,
    SPP_CAST_TYPE_END_POSITION,
    SPP_CAST_EXPRESSION_BEGIN_POSITION,
    SPP_CAST_EXPRESSION_END_POSITION,
    SPP_PARENTHESED_EXPRESSION_LPAR_POSITION,
    SPP_PARENTHESED_EXPRESSION_RPAR_POSITION,
    SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION,
    SPP_PARENTHESED_EXPRESSION_END_POSITION,
    SPP_MAX,
} SyntaxPassParsedImportantPosition;

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
    Language             language;       /* C/JAVA/YACC */
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


typedef struct currentlyParsedClassInfo { // class local, nested for classes
    struct symbol        *function;
    struct extRecFindStr *erfsForParameterCompletion; // currently parsed method for param completion
    unsigned              functionBeginPosition;
    int                   cxMemoryIndexAtFunctionBegin;
    int                   cxMemoryIndexAtFunctionEnd;
    int                   cxMemoryIndexdiAtClassBegin;
    int                   cxMemoryIndexAtClassEnd;
    int                   thisMethodMemoriesStored;
    int                   thisClassMemoriesStored;
    int                   parserPassedMarker;
} CurrentlyParsedClassInfo;

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
    int               cxMemoryIndexAtBlockBegin;
    int               cxMemoryIndexAtBlockEnd;
    int               cxMemoryIndexAtMethodBegin;
    int               cxMemoryIndexAtMethodEnd;
    int               cxMemoryIndexAtClassBeginning;
    int               cxMemoryIndexAtClassEnd;
    int               lastImportLine;
    struct symbol    *lastDeclaratorType;
    struct symbol    *lastAssignmentStruct;
} CurrentlyParsedInfo;

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
