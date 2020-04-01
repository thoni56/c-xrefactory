#ifndef _PROTO_H_
#define _PROTO_H_

/*

  Structures herein will be subject to the filler, structure typedef
  and enum strings generation done in the Makefile.common. See
  description on the wiki.

 */

#include "stdinc.h"
#include <stdbool.h>

#include "head.h"
#include "zlib.h"
#include "lexmac.h"


/* ******************************  TYPEDEFS  ***************************** */

typedef unsigned char uchar;

/* *********************************************************************** */

enum miscellaneous {						/* misc. constants */
    DEFAULT_VALUE,
    TYPE_ADD_YES,
    TYPE_ADD_NO,
    ADD_YES,
    ADD_NO,
    CLASS_TO_TYPE,
    CLASS_TO_EXPR,
    CLASS_TO_METHOD,
    CLASS_TO_ANY,
    ACC_CHECK_YES,
    ACC_CHECK_NO,
    VISIB_CHECK_YES,
    VISIB_CHECK_NO,
    CUT_OVERRIDEN_YES,
    CUT_OVERRIDEN_NO,
    CX_FILE_ITEM_GEN,
    NO_CX_FILE_ITEM_GEN,
    CX_GENERATE_OUTPUT,
    CX_JUST_READ,
    CX_HTML_FIRST_PASS,
    CX_HTML_SECOND_PASS,
    CX_MENU_CREATION,
    CX_BY_PASS,
    HTML_GEN,
    HTML_NO_GEN,
    MEM_ALLOC_ON_SM,
    MEM_ALLOC_ON_PP,
    MEM_NO_ALLOC,
    MESS_REFERENCE_OVERFLOW,
    MESS_FILE_ABORT,
    MESS_NONE,
    INFILES_ENABLED,
    INFILES_DISABLED,
    BEFORE_BLOCK,
    INSIDE_BLOCK,
    AFTER_BLOCK,
    EXTRACT_LOCAL_VAR,
    EXTRACT_VALUE_ARGUMENT,
    EXTRACT_LOCAL_OUT_ARGUMENT,
    EXTRACT_OUT_ARGUMENT,
    EXTRACT_IN_OUT_ARGUMENT,
    EXTRACT_ADDRESS_ARGUMENT,
    EXTRACT_RESULT_VALUE,
    EXTRACT_IN_RESULT_VALUE,
    EXTRACT_LOCAL_RESULT_VALUE,
    EXTRACT_NONE,
    EXTRACT_ERROR,
    INVOCATION_XREF,
    INVOCATION_SETUP,
    INVOCATION_SPELLER,
    LOADING_SYMBOL,
    SEARCH_SYMBOL,
    FF_SCHEDULED_TO_PROCESS,
    FF_COMMAND_LINE_ENTERED,
    OL_LOOKING_2_PASS_MACRO_USAGE,
    SHORT_NAME,
    LONG_NAME,
    GEN_HTML,
    GEN_JAVA_DOC,
    ONLINE_ONLY,
    ALL_REFS,
    COUNT_ONLY,
    CUT_OUTERS,
    NO_OUTERS_CUT,
    VIRT_ITEM,
    SINGLE_VIRT_ITEM,
    FIRST_PASS,
    SECOND_PASS,
    DO_NOT_CHECK_IF_SELECTED,
    PUSH_AFTER_MENU,
    CHECK_NULL,
    RESULT_IS_CLASS_FILE,
    RESULT_IS_JAVA_FILE,
    RESULT_NO_FILE_FOUND,
    TRAILED_YES,
    TRAILED_NO,
    CPOS_FUNCTION_INNER,
    CPOS_ST,
    DIRECT_ONLY,
    SUPERCLASS_NESTED_TOO,
    DIFF_MISSING_REF,
    DIFF_UNEXPECTED_REF,
    CONSTRUCTOR_INVOCATION,
    REGULAR_METHOD,
    SUPER_METHOD_INVOCATION,
    RESOLVE_DIALOG_ALLWAYS,
    RESOLVE_DIALOG_NEVER,
    GLOBAL_ENV_ONLY,
    NO_ERROR_MESSAGE,
    ADD_CX_REFS,
    NO_CX_REFS,
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
    DOTIFY_NAME,
    KEEP_SLASHES,
    LOAD_SUPER,
    DO_NOT_LOAD_SUPER,
    ADD_MAYBE_THIS_REFERENCE,
    TSS_FULL_SEARCH,
    TSS_SEARCH_DEFS_ONLY,
    TSS_FULL_SEARCH_SHORT,
    TSS_SEARCH_DEFS_ONLY_SHORT,
    GEN_FULL_OUTPUT,
    GEN_PRECHECKS,
    GEN_NO_OUTPUT,
    CHECK_FOR_ADD_PARAM,
    CHECK_FOR_DEL_PARAM,
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
    DEEP_ONE,
    DEEP_ANY,
    DEAD_CODE_DETECTION,
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
    PULLING_UP,
    PUSHING_DOWN,
};

/* *******************      encodings      *************** */

enum fileEncodings {
    MULE_DEFAULT,				// in fact utf-8 or utf-16
    MULE_EUROPEAN,				// byte per char
    MULE_EUC,					// euc
    MULE_SJIS,					// Shift-JIS
    MULE_UTF,					// utf-8 or utf-16
    MULE_UTF_8,					// utf-8
    MULE_UTF_16,				// utf-16
    MULE_UTF_16LE,				// utf-16 little endian
    MULE_UTF_16BE,				// utf-16 big endian
};

/* *******************      comment moving levels for refactoring      *************** */

enum commentMovement {
    CM_NO_COMMENT,
    CM_SINGLE_SLASHED,
    CM_SINGLE_STARED,
    CM_SINGLE_SLASHED_AND_STARED,
    CM_ALL_SLASHED,
    CM_ALL_STARED,
    CM_ALL_SLASHED_AND_STARED,
};

/* *******************      generating imports defaults      *************** */

// do to touch this! order is used, with arithmetics!
enum addImportsDefault {
    NID_IMPORT_ON_DEMAND,
    NID_SINGLE_TYPE_IMPORT,
    NID_KEPP_FQT_NAME,
};

/* *******************      parsing deep      *************** */

enum {
    PD_ZERO,
    PD_SHALLOW,
    PD_DEEP,
    PD_DEEPEST,
};

/* *******************      refactoring continuations      *************** */

enum {
    RC_ZERO,				// do not continue, keep interactive
    RC_CONTINUE,			// continue, no special info
    RC_IMPORT_SINGLE_TYPE,
    RC_IMPORT_ON_DEMAND,
};

/* *******************      error messages type      *************** */

enum {
  ERR_ST,				/* standard, no pre-prepared message*/
  ERR_CANT_OPEN,
  ERR_CANT_EXECUTE,
  ERR_NO_MEMORY,
  ERR_INTERNAL,
  ERR_INTERNAL_CHECK,
  ERR_CFG,
};

/* ************ working regime in which the task is invoked ******** */

enum taskRegimes {
    RegimeUndefined,
    RegimeXref,           /* cross referencer called by user from command line */
    RegimeHtmlGenerate,   /* generate html form of input files, ... */
    RegimeEditServer,     /* editor server, called by on-line editing action */
    RegimeRefactory,      /* refactoring server, called by on-line editing */
    RegimeGenerate,       /* generate str-fill macros, ... */
};

/* ************** on-line (browsing) operations for c-xref server  ********** */

enum olcxOptions {
    OLO_COMPLETION,             /* must be zero */
    OLO_SEARCH,
    OLO_TAG_SEARCH,
    OLO_RENAME,        /* same as push, just another ordering */
    OLO_ENCAPSULATE,   /* same as rename, remove private references */
    OLO_ARG_MANIP, /* as rename, constructors resolved as functions */
    OLO_VIRTUAL2STATIC_PUSH, /* same as rename, another message on virtuals */
    OLO_GET_AVAILABLE_REFACTORINGS,
    OLO_PUSH,
    OLO_PUSH_NAME,
    OLO_PUSH_SPECIAL_NAME,		/* also reparsing current file */
    OLO_POP,
    OLO_POP_ONLY,
    OLO_PLUS,
    OLO_MINUS,
    OLO_GOTO_CURRENT,
    OLO_GET_CURRENT_REFNUM,
    OLO_GOTO_PARAM_NAME,
    OLO_GLOBAL_UNUSED,
    OLO_LOCAL_UNUSED,
    OLO_LIST,
    OLO_LIST_TOP,
    OLO_PUSH_ONLY,
    OLO_PUSH_AND_CALL_MACRO,
    OLO_PUSH_ALL_IN_METHOD,
    OLO_PUSH_FOR_LOCALM,
    OLO_GOTO,
    OLO_CGOTO,                 /* goto completion item definition */
    OLO_TAGGOTO,               /* goto tag search result */
    OLO_TAGSELECT,             /* select tag search result */
    OLO_CBROWSE,               /* browse javadoc of completion item */
    OLO_REF_FILTER_SET,
    OLO_REF_FILTER_PLUS,
    OLO_REF_FILTER_MINUS,
    OLO_CSELECT,                /* select completion */
    OLO_COMPLETION_BACK,
    OLO_COMPLETION_FORWARD,
    OLO_EXTRACT,            /* extract block into separate function */
    OLO_CT_INSPECT_DEF,		/* inspect definition from class tree */
    OLO_MENU_INSPECT_DEF,	/* inspect definition from symbol menu */
    OLO_MENU_INSPECT_CLASS,	/* inspect class from symbol menu */
    OLO_MENU_SELECT,		/* select the line from symbol menu */
    OLO_MENU_SELECT_ONLY,	/* select only the line from symbol menu */
    OLO_MENU_SELECT_ALL,	/* select all from symbol menu */
    OLO_MENU_SELECT_NONE,	/* select none from symbol menu */
    OLO_MENU_FILTER_SET,	/* more strong filtering */
    OLO_MENU_FILTER_PLUS,	/* more strong filtering */
    OLO_MENU_FILTER_MINUS,	/* smaller filtering */
    OLO_MENU_GO,            /* push references from selected menu items */
    OLO_CHECK_VERSION,      /* check version correspondance */
    OLO_RESET_REF_SUFFIX,	/* set n-th argument after argument insert */
    OLO_TRIVIAL_PRECHECK,	/* trivial pre-refactoring checks */
    OLO_MM_PRE_CHECK,		/* move method pre check */
    OLO_PP_PRE_CHECK,		/* push-down/pull-up method pre check */
    OLO_SAFETY_CHECK_INIT,
    OLO_SAFETY_CHECK1,
    OLO_SAFETY_CHECK2,
    OLO_INTERSECTION,       /* just provide intersection of top references */
    OLO_REMOVE_WIN,         /* just remove window of top references */
    OLO_GOTO_DEF,           /* goto definition reference */
    OLO_GOTO_CALLER,        /* goto caller reference */
    OLO_SHOW_TOP,           /* show top symbol */
    OLO_SHOW_TOP_APPL_CLASS,   /* show current reference appl class */
    OLO_SHOW_TOP_TYPE,         /* show current symbol type */
    OLO_SHOW_CLASS_TREE,       /* show current class tree */
    OLO_TOP_SYMBOL_RES,        /* show top symbols resolution */
    OLO_ACTIVE_PROJECT,        /* show active project name */
    OLO_JAVA_HOME,             /* show inferred jdkclasspath */
    OLO_REPUSH,                /* re-push pop-ed top */
    OLO_CLASS_TREE,            /* display class tree */
    OLO_USELESS_LONG_NAME,     /* display useless long class names */
    OLO_USELESS_LONG_NAME_IN_CLASS, /* display useless long class names */
    OLO_MAYBE_THIS,         /* display 'this' class dependencies */
    OLO_NOT_FQT_REFS,       /* display not fully qualified names in method */
    OLO_NOT_FQT_REFS_IN_CLASS, /* display not fully qualified names of class */
    OLO_GET_ENV_VALUE,      /* get a value set by -set */
    OLO_SET_MOVE_TARGET,	/* set target place for moving action */
    OLO_SET_MOVE_CLASS_TARGET,	/* set target place for xref2 move class */
    OLO_SET_MOVE_METHOD_TARGET,	/* set target place for xref2 move method */
    OLO_GET_CURRENT_CLASS,
    OLO_GET_CURRENT_SUPER_CLASS,
    OLO_GET_METHOD_COORD,	/* get method beginning and end lines */
    OLO_GET_CLASS_COORD,	/* get class beginning and end lines */
    OLO_GET_SYMBOL_TYPE,	/* get type of a symbol */
    OLO_TAG_SEARCH_FORWARD,
    OLO_TAG_SEARCH_BACK,
    OLO_PUSH_ENCAPSULATE_SAFETY_CHECK,
    OLO_ENCAPSULATE_SAFETY_CHECK,
    OLO_SYNTAX_PASS_ONLY,    /* should replace OLO_GET_PRIMARY_START && OLO_GET_PARAM_COORDINATES */
    OLO_GET_PRIMARY_START,   /* get start position of primary expression */
    OLO_GET_PARAM_COORDINATES,
    OLO_ABOUT,
};


/* ************************** refactorings **************************** */

enum editorUndoOperations {
    UNDO_REPLACE_STRING,
    UNDO_RENAME_BUFFER,
    UNDO_MOVE_BLOCK,
};

enum editors {
    ED_NONE,
    ED_EMACS,
    ED_JEDIT,
};

enum memories {
    MEM_CF,
    MEM_XX,
    MEM_PP,
};

enum refCategories {
    CatGlobal,
    CatLocal,
    MAX_CATEGORIES
};

enum refScopes {
    ScopeDefault,
    ScopeGlobal,
    ScopeFile,
    ScopeAuto,
    MAX_SCOPES
};

// !!!!!!!!!!!!! All this stuff is to be removed, new way of defining usages
// !!!!!!!!!!!!! is to set various bits in usg structure

enum usages {
// filter3  == all filters
    ureserve0,
    UsageOLBestFitDefined,
    UsageJavaNativeDeclared,
    ureserve1,
    UsageDefined,
    ureserve2,
    UsageDeclared,
    ureserve3,
// filter2
    UsageLvalUsed,	// == USAGE_EXTEND_USAGE
    UsageLastUselessInClassOrMethod,	// last useless part of useless FQT name in
                                // current method (only in regime server)
    ureserve4,
// filter1
    UsageAddrUsed,	// == USAGE_TOP_LEVEL_USED
    ureserve5,
    UsageConstructorUsed,			// can be put anywhere before 'Used'
    ureserve6,
    UsageMaybeThisInClassOrMethod,			// reference inside current method
    UsageMaybeQualifThisInClassOrMethod,   // where 'this' may be inserted
    UsageNotFQTypeInClassOrMethod,
    UsageNotFQFieldInClassOrMethod,
    UsageNonExpandableNotFQTNameInClassOrMethod,
    UsageLastUseless,		// last part of useless FQT name
    ureserve7,
//filter0
    UsageUsed,
    UsageUndefinedMacro,
    UsageMethodInvokedViaSuper, // a method invoked by super.method(...)
    UsageConstructorDefinition,	// Usage for Java type name, when used in constructor def.
    ureserve8,
    UsageOtherUseless,	// a useless part of useless FQT name (not last)
    UsageThisUsage,		// 'this' reference
    ureserve9,
    UsageFork,			/* only in program graph for branching on label */
    ureserve10,
    ureserve11,
//INVISIBLE USAGES
    UsageMaxOLUsages,

    UsageNone,			/* also forgotten rval usage of lval usage ex. l=3;
                           also new Nested() resolved to error because of
                                enclosing instance
                         */
    UsageMacroBaseFileUsage,    /* reference to input file expanding the macro */
    UsageClassFileDefinition,   /* reference got from class file (not shown in searches)*/
    UsageJavaDoc,               /* reference to place javadoc link */
    UsageJavaDocFullEntry,      /* reference to placce full javadoc comment */
    UsageClassTreeDefinition,   /* reference for class tree symbol */
    UsageMaybeThis,				/* reference where 'this' maybe inserted */
    UsageMaybeQualifiedThis,	/* reference where qualified 'this' may be inserted */
    UsageThrown,				/* extract method exception information */
    UsageCatched,				/* extract method exception information */
    UsageTryCatchBegin,
    UsageTryCatchEnd,
    UsageSuperMethod,
    UsageNotFQType,
    UsageNotFQField,
    UsageNonExpandableNotFQTName,
    MAX_USAGES,
    USAGE_ANY,
    USAGE_FILTER,
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
    RFilterAddrVal,			// also toplevel usage
    RFilterLVal,			// also extend usage
    RFilterDefinitions,
    MAX_REF_LIST_FILTER_LEVEL,
};

typedef enum storage {
    // standard C storage modes
    StorageError,
    StorageAuto,
    StorageGlobal,			/* not used anymore, backward compatibility */
    StorageDefault,
    StorageExtern,
    StorageConstant,		/* enumerator definition */
    StorageStatic,
    StorageThreadLocal,
    StorageTypedef,
    StorageMutable,
    StorageRegister,
    // some "artificial" Java storages
    StorageConstructor,		/* storage for class constructors */
    StorageField,			/* storage for class fields */
    StorageMethod,			/* storage for class methods */
    //
    StorageNone,
    MAX_STORAGE,
    /* If this becomes greater than 32 increase STORAGES_LN !!!!!!!! */
} Storage;

#include "types.h"

enum javaPCTIndex {		/* java Primitive Conversion Table Indexes */
    PCTIndexError=0,
    PCTIndexByte,
    PCTIndexShort,
    PCTIndexChar,
    PCTIndexInt,
    PCTIndexLong,
    PCTIndexFloat,
    PCTIndexDouble,
    MAX_PCTIndex
};

enum sFunResult {
    RESULT_OK,
    RESULT_ERR
};

enum inputType {
    II_NORMAL,
    II_MACRO,
    II_MACRO_ARG,
    II_CACHE,
};

enum updateModifiers {
    UP_NO_UPDATE = 0,			// must be zero !
    UP_FAST_UPDATE,
    UP_FULL_UPDATE,
};

enum syntaxPassParsedImportantPosition {
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
};

/* ******************************************************************** */
/*                               STRUCTURES                             */
/* ******************************************************************** */

/* class cast tree */
typedef struct cctNode {
    struct symbol	*node;
    struct cctNode	*sub;       /* sub[CCT_TREE_INDEX]; */
} S_cctNode;

#include "position.h"

/* return value for IDENTIFIER token from yylex */

typedef struct id {
    char *name;
    struct symbol *sd;
    struct position	p;
    struct id *next;
} S_id;

typedef struct freeTrail {
    void             (*action)(void*);
    void             *p;
    struct freeTrail *next;
} S_freeTrail;

typedef struct topBlock {
    int              firstFreeIndex;
    int              tmpMemoryBasei;
    struct freeTrail *trail;
    struct topBlock  *previousTopBlock;
} S_topBlock;

#include "typemodifier.h"

typedef struct recFindStr {
    struct symbol			*baseClass;	/* class, application on which is looked*/
    struct symbol			*currClass;	/* current class, NULL for loc vars. */
    struct symbol           *nextRecord;
    unsigned                recsClassCounter;
    int                     sti;
    struct symbolList		*st[MAX_INHERITANCE_DEEP];	/* super classes stack */
    int                     aui;
    struct symbol			*au[MAX_ANONYMOUS_FIELDS];	/* anonymous unions */
} S_recFindStr;

typedef struct extRecFindStr {
    struct recFindStr			s;
    struct symbol				*memb;
    struct typeModifierList	*params;
} S_extRecFindStr;

typedef struct nestedSpec {
    struct symbol		*cl;
    char			membFlag;		/* flag whether it is nested in class */
    short unsigned  accFlags;
} S_nestedSpec;

typedef struct symStructSpec {
    struct symbolList		*super;			/* list of super classes & interfaces */
    struct symbol			*records;		/* str. records, should be a table of   */
    struct cctNode			casts;			/* possible casts                       */
    short int				nnested;		/* # of java nested classes     */
    struct nestedSpec		*nest;			/* array of nested classes		*/
    struct typeModifier	stype;              /* this structure type */
    struct typeModifier	sptrtype;           /* this structure pointer type */
    int						classFile;		/* in java, my class file index
                                               == -1 for none, TODO to change
                                               it to s_noneFileIndex !!!
                                            */
    unsigned				recSearchCounter; /* tmp counter when looking for a record
                                                it flags searched classes
                                             */
} S_symStructSpec;


typedef struct jslSymbolList {
    struct symbol           *d;
    struct position			pos;
    int						isSingleImportedFlag;
    struct jslSymbolList	*next;
} JslSymbolList;


/* ****************************************************************** */
/*          symbol definition item in cross-reference table           */

typedef struct usageBits {
    unsigned base:8;								// 0 - 128, it should not grow anymore
    unsigned requiredAccess:MAX_REQUIRED_ACCESS_LN;	// required accessibility of the reference
    // local properties (not saved in tag files)
    unsigned dummy:1;								// unused for the moment
} S_usageBits;

// !!! if you add a pointer to this structure, then update olcxCopyRefList
typedef struct reference {
    struct usageBits			usage;
    struct position				p;
    struct reference            *next;
} S_reference;

typedef struct symbolRefItemBits {
    unsigned				symType		: SYMTYPES_LN;
    unsigned				storage		: STORAGES_LN;
    unsigned				scope		: SCOPES_LN;
    unsigned				accessFlags	: 12; /* java access bits */
    unsigned				category	: 2;  /* local/global */
    unsigned				htmlWasLn	: 1;  /* html ln generated */
} S_symbolRefItemBits;

// !!! if you add a pointer to this structure, then update olcxCopyRefItem!
typedef struct symbolRefItem {
    char						*name;
    unsigned					fileHash;
    int							vApplClass;	/* appl class for java virtuals */
    int							vFunClass;	/* fun class for java virtuals */
    struct symbolRefItemBits b;
    struct reference			*refs;
    struct symbolRefItem        *next;
} S_symbolRefItem;

typedef struct symbolRefItemList {
    struct symbolRefItem		*d;
    struct symbolRefItemList	*next;
} S_symbolRefItemList;

/* ***************** on - line cross referencing ***************** */

typedef struct olCompletion {
    char					*name;
    char					*fullName;
    char					*vclass;
    short int				jindent;
    short int				lineCount;
    char					cat;			/* CatGlobal/CatLocal */
    char					csymType;		/* symtype of completion */
    struct reference		ref;
    struct symbolRefItem	sym;
    struct olCompletion		*next;
} S_olCompletion;

typedef struct olSymbolFoundInformation {
    struct symbolRefItem	*symrefs;		/* this is valid */
    struct symbolRefItem	*symRefsInfo;	/* additional for error message */
    struct reference		*currentRef;
} S_olSymbolFoundInformation;

typedef struct olSymbolsMenu {
    struct symbolRefItem	s;
    char					selected;
    char					visible;
    unsigned				ooBits;
    char					olUsage;	/* usage of symbol under cursor */
    short int				vlevel;		/* virt. level of applClass <-> olsymbol*/
    short int				refn;
    short int				defRefn;
    char					defUsage;   /* usage of definition reference */
    struct position         defpos;
    int                     outOnLine;
    struct editorMarkerList	*markers;	/* for refactory only */
    struct olSymbolsMenu	*next;
} S_olSymbolsMenu;

// if you add something to this structure, update olcxMoveTopFromAnotherUser()
// !!!!!
typedef struct olcxReferences {
    struct reference        *r;			/* list of references */
    struct reference        *act;		/* actual reference */
    char					command;	/* OLO_PUSH/OLO_LIST/OLO_COMPLETION */
    char					language;	/* C/JAVA/YACC */
    time_t					atime;		/* last access time */
    // refsuffix is useless now, should be removed !!!!
    char					refsuffix[MAX_OLCX_SUFF_SIZE];
    struct position			cpos;		/* caller position */
    struct olCompletion		*cpls;		/* completions list for OLO_COMPLETION */
    // following two lists should be probably split into hashed tables of lists
    // because of bad performances for class tree and global unused symbols
    struct olSymbolsMenu	*hkSelectedSym; /* resolved symbols under the cursor */
    struct olSymbolsMenu	*menuSym;		/* hkSelectedSyms plus same name */
    int						menuFilterLevel;
    int						refsFilterLevel;
    struct olcxReferences	*previous;
} S_olcxReferences;

// this is useless, (refsuffix is not used), but I keep it for
// some time for case if something more is needed in class tree
typedef struct classTreeData {
    char					refsuffix[MAX_OLCX_SUFF_SIZE];
    int						baseClassIndex;
    struct olSymbolsMenu	*tree;
} S_classTreeData;

typedef struct olcxReferencesStack {
    struct olcxReferences	*top;
    struct olcxReferences	*root;
} S_olcxReferencesStack;



/* ***************** COMPLETION STRUCTURES ********************** */


typedef struct cline {					/* should be a little bit union-ified */
    char            *s;
    struct symbol   *t;
    short int		symType;
    short int		virtLevel;
    short int		margn;
    char			**margs;
    struct symbol	*vFunClass;
} S_cline;

typedef struct completions {
    char            idToProcess[MAX_FUN_NAME_SIZE];
    int				idToProcessLen;
    struct position	idToProcessPos;
    int             fullMatchFlag;
    int             isCompleteFlag;
    int             noFocusOnCompletions;
    int             abortFurtherCompletions;
    char            comPrefix[TMP_STRING_SIZE];
    int				maxLen;
    struct cline    a[MAX_COMPLETIONS];
    int             ai;
} S_completions;


/* ************************** INIT STRUCTURES ********************* */

typedef struct typeCharCodeIni {
    int         symType;
    char		code;
} S_typeCharCodeIni;

typedef struct javaTypePCTIConvertIni {
    int		symType;
    int		PCTIndex;
} S_javaTypePCTIConvertIni;

typedef struct currentlyParsedCl {		// class local, nested for classes
    struct symbol			*function;
    struct extRecFindStr	*erfsForParamsComplet;			// curently parsed method for param completion
    unsigned				funBegPosition;
    int                     cxMemiAtFunBegin;
    int						cxMemiAtFunEnd;
    int                     cxMemiAtClassBegin;
    int						cxMemiAtClassEnd;
    int						thisMethodMemoriesStored;
    int						thisClassMemoriesStored;
    int						parserPassedMarker;
} S_currentlyParsedCl;

typedef struct currentlyParsedStatics {
    int             extractProcessedFlag;
    int             cxMemiAtBlockBegin;
    int             cxMemiAtBlockEnd;
    struct topBlock *workMemiAtBlockBegin;
    struct topBlock *workMemiAtBlockEnd;
    int				marker1Flag;
    int				marker2Flag;
    char			setTargetAnswerClass[TMP_STRING_SIZE];	// useless for xref2
    int				moveTargetApproved;
    char			currentPackageAnswer[TMP_STRING_SIZE];
    char			currentClassAnswer[TMP_STRING_SIZE];
    char			currentSuperClassAnswer[TMP_STRING_SIZE];
    int				methodCoordEndLine;        // to be removed
    int				cxMemiAtMethodBeginning;
    int				cxMemiAtMethodEnd;
    int				classCoordEndLine;
    int				cxMemiAtClassBeginning;
    int				cxMemiAtClassEnd;
    int				lastImportLine;
    struct symbol	*lastDeclaratorType;
    struct symbol	*lastAssignementStruct;
} S_currentlyParsedStatics;

/* ************************** JAVAS *********************************** */

/* java composed names */

typedef struct idList {
    struct id id;
    char *fname;                /* fqt name for java */
    enum type nameType;             /* type of name segment for java */
    struct idList *next;
} S_idList;


/* ***************** unique counters  *********************** */

typedef struct counters {
    int localSym;
    int localVar;
    int anonymousClassCounter;
} S_counters;


/* ************************ PRE-PROCESSOR **************************** */

typedef struct stringAddrList {
    char **d;
    struct stringAddrList *next;
} S_stringAddrList;

typedef enum {
    INPUT_DIRECT,
    INPUT_VIA_UNZIP,
    INPUT_VIA_EDITOR
} InputMethod;

typedef struct CharacterBuffer {
    char        *next;				/* first unread */
    char        *end;				/* pointing after valid characters */
    char        chars[CHAR_BUFF_SIZE];
    FILE        *file;
    unsigned	filePos;			/* how many chars was read from file */
    int			fileNumber;
    int         lineNum;
    char        *lineBegin;
    int         columnOffset;		/* column == cc-lineBegin + columnOffset */
    bool		isAtEOF;
    InputMethod	inputMethod;		/* unzip/direct */
    char        z[CHAR_BUFF_SIZE];  /* zip input buffer */
    z_stream	zipStream;
} CharacterBuffer;

typedef struct lexBuf {
    char            *next;				/* next to read */
    char            *end;				/* pointing *after* last valid char */
    char            chars[LEX_BUFF_SIZE];
    struct position pRing[LEX_POSITIONS_RING_SIZE];		// file/line/col position
    unsigned        fpRing[LEX_POSITIONS_RING_SIZE];	// file offset position
    int             posi;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
    struct CharacterBuffer buffer;
} S_lexBuf;

typedef struct cppIfStack {
    struct position pos;
    struct cppIfStack *next;
} S_cppIfStack;


/* **************** processing cxref file ************************* */

typedef struct cxScanFileFunctionLink {
    int		recordCode;
    void    (*handleFun)(int size,int ri,char**ccc,char**ffin,CharacterBuffer*bbb, int additionalArg);
    int		additionalArg;
} ScanFileFunctionStep;

/* ********************* MEMORIES *************************** */

typedef struct memory {
    int		(*overflowHandler)(int n);
    int     i;
    int		size;
    double  b;		//  double in order to get it properly alligned
} S_memory;


/* *********************************************************** */

typedef struct programGraphNode {
    struct reference            *ref;		/* original reference of node */
    struct symbolRefItem        *symRef;
    struct programGraphNode		*jump;
    char						posBits;		/* INSIDE/OUSIDE block */
    char						stateBits;		/* visited + where setted */
    char						classifBits;	/* resulting classification */
    struct programGraphNode		*next;
} S_programGraphNode;

typedef struct exprTokenType {
    struct typeModifier *t;
    struct reference     *r;
    struct position      *pp;
} S_exprTokenType;

typedef struct nestedConstrTokenType {
    struct typeModifier	*t;
    struct idList		*nid;
    struct position			*pp;
} S_nestedConstrTokenType;

typedef struct unsignedPositionPair {
    unsigned		u;
    struct position	*p;
} S_unsignedPositionPair;

typedef struct symbolPositionPair {
    struct symbol	*s;
    struct position	*p;
} S_symbolPositionPair;

typedef struct symbolPositionListPair {
    struct symbol		*s;
    struct positionList	*p;
} S_symbolPositionListPair;

typedef struct intPair {
    int i1;
    int i2;
} S_intPair;

typedef struct typeModifiersListPositionListPair {
    struct typeModifierList	*t;
    struct positionList			*p;
} S_typeModifiersListPositionListPair;

typedef struct whileExtractData {
    int				i1;
    int             i2;
    struct symbol	*i3;
    struct symbol	*i4;
} S_whileExtractData;

typedef struct pushAllInBetweenData {
    int			minMemi;
    int			maxMemi;
} S_pushAllInBetweenData;



/* *********************************************************** */

/* **************     parse tree with positions    *********** */

// following structures are used in yacc parser, they always
// contain 'b','e' records for begin and end position of parse tree
// node and additional data 'd' for parsing.

typedef struct {
    struct position  b, e;
    int d;
} Ast_int;
typedef struct {
    struct position b, e;
    unsigned d;
} Ast_unsigned;
typedef struct {
    struct position b, e;
    struct symbol *d;
} Ast_symbol;
typedef struct {
    struct position b, e;
    struct symbolList *d;
} Ast_symbolList;
typedef struct {
    struct position          b, e;
    struct typeModifier		*d;
} Ast_typeModifiers;
typedef struct {
    struct position              b, e;
    struct typeModifierList		*d;
} Ast_typeModifiersList;
typedef struct {
    struct position      b, e;
    struct freeTrail		*d;
} Ast_freeTrail;
typedef struct {
    struct position      b, e;
    struct id		*d;
} Ast_id;
typedef struct {
    struct position          b, e;
    struct idList		*d;
} Ast_idList;
typedef struct {
    struct position          b, e;
    struct exprTokenType		d;
} Ast_exprTokenType;
typedef struct {
    struct position                  b, e;
    struct nestedConstrTokenType		d;
} Ast_nestedConstrTokenType;
typedef struct {
    struct position      b, e;
    struct intPair		d;
} Ast_intPair;
typedef struct {
    struct position              b, e;
    struct whileExtractData		*d;
} Ast_whileExtractData;
typedef struct {
    struct position      b, e;
    struct position		d;
} Ast_position;
typedef struct {
    struct position              b, e;
    struct unsignedPositionPair		d;
} Ast_unsPositionPair;
typedef struct {
    struct position                  b, e;
    struct symbolPositionPair		d;
} Ast_symbolPositionPair;
typedef struct {
    struct position                  b, e;
    struct symbolPositionListPair		d;
} Ast_symbolPositionListPair;
typedef struct {
    struct position          b, e;
    struct positionList		*d;
} Ast_positionList;
typedef struct {
    struct position                                  b, e;
    struct typeModifiersListPositionListPair			d;
} Ast_typeModifiersListPositionListPair;



/* *********************************************************** */
/* *********************   options  ************************** */
/* *********************************************************** */

typedef struct htmlCutPathsOpts {
    int pathsNum;
    char *path[MAX_HTML_CUT_PATHES];
    int plen[MAX_HTML_CUT_PATHES];
} S_htmlCutPathsOpts;

typedef struct setGetEnv {
    int num;
    char *name[MAX_SET_GET_OPTIONS];
    char *value[MAX_SET_GET_OPTIONS];
} S_setGetEnv;

// TODO all strings inside to static string array
typedef struct options {
    int fileEncoding;
    char completeParenthesis;
    int defaultAddImportStrategy;
    char referenceListWithoutSource;
    char jeditOldCompletions;		// to be removed
    int completionOverloadWizardDeep;
    int exit;
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
    int theRefactoring;
    int briefoutput;
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
    char *htmlLineNumLabel;
    int htmlCutSuffix;
    int tagSearchSpecif;
    char *javaVersion;
    char *olcxWinDelFile;
    int olcxWinDelFromLine;
    int olcxWinDelFromCol;
    int olcxWinDelToLine;
    int olcxWinDelToCol;
    char *moveFromUser;
    int noErrors;
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
    int java2html;
    int javaSlAllowed;
    int xfileHashingMethod;
    char *htmlLineNumColor;
    int htmlCxLineLen;
    char *htmlJdkDocAvailable;
    int htmlGenJdkDocLinks;
    char *htmlJdkDocUrl;
    char *javaDocPath;
    int allowPackagesOnCl;
    char *sourcePath;
    int htmlDirectX;
    char *jdocTmpDir;
    int noCxFile;
    int javaDoc;
    int noIncludeRefs;
    int allowClassFileRefs;
    int filterValue;
    char *jdkClassPath;
    int manualResolve;
    char *browsedSymName;
    int modifiedFlag;
    int olcxMenuSelectLineNum;
    int htmlNoUnderline;
    char *htmlLinkColor;
    char *htmlCutPath;
    int htmlCutPathLen;
    int ooChecksBits;
    int htmlLineNums;
    int htmlNoColors;
    int cxMemoryFaktor;
    int multiHeadRefsCare;
    int strictAnsi;
    char * project;
    int updateOnlyModifiedFiles;
    char *olcxlccursor;
    char *htmlZipCommand;
    char *olcxSearchString;
    int olineLen;
    char *htmlLinkSuffix;
    char *olExtractAddrParPrefix;
    int extractMode;
    int htmlFunSeparate;
    int maxCompletions;
    int editor;
    int create;
    char *olcxRefSuffix;
    int recursivelyDirs;
    char *classpath;
    int tabulator;
    char *htmlRoot;
    int htmlRichLists;
    int	htmlglobalx;
    int	htmllocalx;
    int cIsCplusplus;
    int olCursorPos;
    int olMarkPos;
    enum taskRegimes taskRegime;
    char *user;
    int debug;
    int trace;
    int cpp_comment;
    int c_struct_scope;
    enum olcxOptions server_operation;
    int olcxGotoVal;
    char *originalDir;

    int no_ref_locals;
    int no_ref_records;
    int no_ref_enumerator;
    int no_ref_typedef;
    int no_ref_macro;
    int no_stdop;
    int qnxMessages;

    /* GENERATE options */

    int typedefg;
    int str_fill;
    int enum_name;
    int body;
    int header;
    int str_copy;

    /* CXREF options  */

    int err;
    int long_cxref;
    int brief;
    int update;
    int keep_old;
    char *last_message;
    int refnum;

    // all the rest initialized to zeros by default
    struct setGetEnv setGetEnv;
    struct htmlCutPathsOpts htmlCut;

    // memory for strings
    struct stringAddrList	*allAllocatedStrings;
    struct memory			pendingMemory;
    char					pendingFreeSpace[SIZE_opiMemory];
} S_options;

#endif	/* ifndef _PROTO__H */
