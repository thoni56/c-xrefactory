#ifndef PROTO_H
#define PROTO_H

#include "stdinc.h"

#ifdef BOOTSTRAP
#include "strTdef.bs"
#else
#include "strTdef.h"
#endif

#include "head.h"
#include "zlib.h"       /*SBD*/
#include "lexmac.h"     /*SBD*/

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
    SETUP_EDITOR_EMACS,
    SETUP_EDITOR_XEMACS,
    SETUP_EDITOR_VEDIT3,
    SETUP_SHELL_SH,
    SETUP_SHELL_CSH,
    SETUP_SHELL_BASH,
    SETUP_SHELL_UNKNOWN,
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
    INPUT_DIRECT,
    INPUT_VIA_UNZIP,
    INPUT_VIA_EDITOR,
    FQT_COMPLETE_ALSO_ON_PACKAGE,
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
    MULE_UTF_16LE,				// utf-16 low endian
    MULE_UTF_16BE,				// utf-16 big endian
};

/* *******************      comment moving levels for refactoring      *************** */

enum commentMovings {
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
  ERR_LICENSE,
  ERR_BIN_LICENSE,
};

/* ************ working regime in which the task is invoked ******** */

enum taskRegimes {
    RegimeUndefined,
    RegimeXref,			/* cross referencer called by user from command line */
    RegimeEditServer,	/* editor server, called by on-line editing action */
    RegimeGenerate,		/* generate str-fill macros, ... */
    RegimeHtmlGenerate,	/* generate html form of input files, ... */
    RegimeRefactory,	/* generate html form of input files, ... */
};

/* ************** on-line (browsing) commands for xrefsrv ********** */

enum olcxOptions {
    OLO_COMPLETION,		/* must be zero */
    OLO_SEARCH,
    OLO_TAG_SEARCH,
    OLO_RENAME,			/* same as push, just another ordering */
    OLO_ENCAPSULATE,	/* same as rename, remove private references */
    OLO_ARG_MANIP,			/* as rename, constructors resolved as functions */
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
    OLO_CGOTO,			/* goto completion item definition */
    OLO_TAGGOTO,		/* goto tag search result */
    OLO_TAGSELECT,		/* select tag search result */
    OLO_CBROWSE,		/* browse javadoc of completion item */
    OLO_REF_FILTER_SET,
    OLO_REF_FILTER_PLUS,
    OLO_REF_FILTER_MINUS,
    OLO_CSELECT,		/* select completion */
    OLO_COMPLETION_BACK,
    OLO_COMPLETION_FORWARD,
    OLO_EXTRACT,		/* extract block into separate function */
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
    OLO_MENU_GO,		/* push references from selected menu items */
    OLO_CHECK_VERSION,	/* check version correspondance */
    OLO_RESET_REF_SUFFIX,	/* set n-th argument after argument insert */
    OLO_TRIVIAL_PRECHECK,	/* trivial pre-refactoring checks */
    OLO_MM_PRE_CHECK,		/* move method pre check */
    OLO_PP_PRE_CHECK,		/* push-down/pull-up method pre check */
    OLO_SAFETY_CHECK_INIT,
    OLO_SAFETY_CHECK1,
    OLO_SAFETY_CHECK2,
    OLO_INTERSECTION,	/* just provide intersection of top references */
    OLO_REMOVE_WIN,		/* just remove window of top references */
    OLO_GOTO_DEF,		/* goto definition reference */
    OLO_GOTO_CALLER,	/* goto caller reference */
    OLO_SHOW_TOP,		/* show top symbol */
    OLO_SHOW_TOP_APPL_CLASS,		/* show current reference appl class */
    OLO_SHOW_TOP_TYPE,		/* show current symbol type */
    OLO_SHOW_CLASS_TREE,		/* show current class tree */
    OLO_TOP_SYMBOL_RES,	/* show top symbols resolution */
    OLO_ACTIVE_PROJECT,		/* show active project name */
    OLO_JAVA_HOME,		/* show inferred jdkclasspath */
    OLO_REPUSH,			/* re-push pop-ed top */
    OLO_CLASS_TREE,     /* display class tree */
    OLO_USELESS_LONG_NAME,   /* display useless long class names */
    OLO_USELESS_LONG_NAME_IN_CLASS,   /* display useless long class names */
    OLO_MAYBE_THIS,   /* display 'this' class dependencies */
    OLO_NOT_FQT_REFS,   /* display not fully qualified names in method */
    OLO_NOT_FQT_REFS_IN_CLASS,   /* display not fully qualified names of class */
    OLO_GET_ENV_VALUE,	/* get a value set by -set */
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

enum trivialPreChecks {
    TPC_NONE,
    TPC_MOVE_FIELD,
    TPC_RENAME_PACKAGE,
    TPC_RENAME_CLASS,
    TPC_MOVE_CLASS,
    TPC_MOVE_STATIC_FIELD,
    TPC_MOVE_STATIC_METHOD,
    TPC_TURN_DYN_METHOD_TO_STATIC,
    TPC_TURN_STATIC_METHOD_TO_DYN,
    TPC_PULL_UP_METHOD,
    TPC_PUSH_DOWN_METHOD,
    TPC_PUSH_DOWN_METHOD_POST_CHECK,
    TPC_PULL_UP_FIELD,
    TPC_PUSH_DOWN_FIELD,
    TPC_GET_LAST_IMPORT_LINE, // not really a check
};

enum extractModes {
    EXTR_FUNCTION,
    EXTR_FUNCTION_ADDRESS_ARGS,
    EXTR_MACRO,
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
    UsageMacroBaseFileUsage, /* reference to input file expanding the macro */
    UsageClassFileDefinition, /* reference got from class file (not shown in searches)*/
    UsageJavaDoc,            /* reference to place javadoc link */
    UsageJavaDocFullEntry,   /* reference to placce full javadoc comment */
    UsageClassTreeDefinition, /* reference for class tree symbol */
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

enum storages {
    // standard C storages
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
};

enum types {
    TypeDefault,
    TypeChar ,
    TypeUnsignedChar ,
    TypeSignedChar ,
    TypeInt ,
    TypeUnsignedInt ,
    TypeSignedInt ,
    TypeShortInt ,
    TypeShortUnsignedInt ,
    TypeShortSignedInt ,
    TypeLongInt ,
    TypeLongUnsignedInt ,
    TypeLongSignedInt ,
    TypeFloat ,
    TypeDouble ,
    TypeStruct,
    TypeUnion,
    TypeEnum ,
    TypeVoid ,
    TypePointer,
    TypeArray,
    TypeFunction,
    TypeAnonymeField,
    TypeError,
    MODIFIERS_START,
    TmodLong,
    TmodShort,
    TmodSigned,
    TmodUnsigned,
    TmodShortSigned,
    TmodShortUnsigned,
    TmodLongSigned,
    TmodLongUnsigned,
    TypeReserve1,
    TypeReserve2,
    TypeReserve3,
    TypeReserve4,
    MODIFIERS_END,
    TypeElipsis,
    JAVA_TYPES,
    TypeByte,
    TypeShort,
    TypeLong,
    TypeBoolean,
    TypeNull,
    TypeOverloadedFunction,
    TypeReserve7,
    TypeReserve8,
    TypeReserve9,
    TypeReserve10,
    TypeReserve11,
    CCC_TYPES,
    TypeWchar_t,
    MAX_CTYPE,
    TypeCppInclude,		/* dummy, the Cpp #include reference */
    TypeMacro,			/* dummy, a macro in the symbol table */
    TypeCppIfElse,		/* dummy, the Cpp #if #else #fi references */
    TypeCppCollate,		/* dummy, the Cpp ## joined string reference */
    TypePackage,		/* dummy, a package in java */
    TypeYaccSymbol,		/* dummy, for yacc grammar references */
    MAX_HTML_LIST_TYPE, /* only types until this will be listed in html lists*/
    TypeLabel,			/* dummy, a label in the symbol table*/
    TypeKeyword,		/* dummy, a keyword in the symbol table, + html ref. */
    TypeToken,			/* dummy, a token for completions */
    TypeUndefMacro,		/* dummy, an undefined macro in the symbol table */
    TypeMacroArg,		/* dummy, a macro argument */
    TypeDefinedOp,		/* dummy, the 'defined' keyword in #if directives */
    TypeCppAny,			/* dummy, a Cpp reference (html only) */
    TypeBlockMarker,	/* dummy, block markers for extract */
    TypeTryCatchMarker,	/* dummy, block markers for extract */
    TypeComment,		/* dummy, a commentary reference (html only) */
    TypeExpression,		/* dummy, an ambig. name evaluated to expression in java */
    TypePackedType,		/* dummy, typemodif, when type is in linkname */
    TypeFunSep,			/* dummy, function separator for HTML */
    TypeSpecialComplet,	/* dummy special completion string (for(;xx!=NULL ..)*/
    TypeNonImportedClass,/* dummy for completion*/
    TypeInducedError,         /* dummy in general*/
    TypeInheritedFullMethod,   /* dummy for completion, complete whole definition */
    TypeSpecialConstructorCompletion, /*dummy completion of constructor 'super'*/
    TypeUnknown,   /* dummy for completion */
    MAX_TYPE,
    /* if this becomes greater than 256, increase SYMTYPES_LN !!!!!!!!!!!!! */
};


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

enum copyFlag {		/* flag for genFillStructBody function*/
    FillGenerate,
    InternalFillGenerate,
    CopyGenerate,
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
struct cctNode {
    S_symbol	*node;
    S_cctNode	*sub;       /* sub[CCT_TREE_INDEX]; */
};

struct position {
    int			file;
    int         line;
    int			coll;
};

struct positionList {
    S_position		p;
    S_positionList   *next;
};

/* return value for IDENTIFIER token from yylex */

struct idIdent {
    char        *name;
    S_symbol	*sd;		/* if yet in symbol table */
    S_position	p;			/* position */
    S_idIdent   *next;
};

struct freeTrail {
    void			(*action)(void*);
    void            *p;
    S_freeTrail		*next;
};

struct topBlock {
/*	char            *stackMemoryBase;*/
    int             firstFreeIndex;
    int				tmpMemoryBasei;
    S_freeTrail		*trail;
    S_topBlock      *previousTopBlock;
};

struct typeModifiers {
    short                   m;
    union typeModifUnion {
        struct funTypeModif {		/* LAN_C/CPP Function */
            S_symbol		*args;
            S_symbol		**thisFunList;	/* only for LAN_CPP overloaded */
        } f;
        //S_symbol			*args;		/* LAN_C Function */
        struct methodTypeModif {		/* LAN_JAVA Function/Method */
            char			*sig;
            S_symbolList	*exceptions;
        } m;
        //char				*sig;		/* LAN_JAVA Function */
        S_symbol			*t;			/* Struct/Union/Enum */
    } u;
    S_symbol				*typedefin;	/* the typedef symbol (if any) */
    S_typeModifiers         *next;
};

struct typeModifiersList {
    S_typeModifiers		*d;
    S_typeModifiersList	*next;
};

struct recFindStr {
    S_symbol			*baseClass;	/* class, application on which is looked*/
    S_symbol			*currClass;	/* current class, NULL for loc vars. */
    S_symbol            *nextRecord;
    unsigned			recsClassCounter;
    int					sti;
    S_symbolList		*st[MAX_INHERITANCE_DEEP];	/* super classes stack */
    int					aui;
    S_symbol			*au[MAX_ANONYMOUS_FIELDS];	/* anonymous unions */
};

struct extRecFindStr {
    S_recFindStr			s;
    S_symbol				*memb;
    S_typeModifiersList		*params;
};

struct nestedSpec {
    S_symbol		*cl;
    char			membFlag;		/* flag whether it is nested in class */
    short unsigned  accFlags;
};

struct symStructSpecific {
    S_symbolList	*super;			/* list of super classes & interfaces */
    S_symbol		*records;		/* str. records, should be a table of   */
    S_cctNode		casts;			/* possible casts                       */
    short int		nnested;		/* # of java nested classes     */
    S_nestedSpec	*nest;			/* array of nested classes		*/
    S_typeModifiers	stype;			/* this structure type */
    S_typeModifiers	sptrtype;		/* this structure pointer type */
    char            currPackage;	/* am I in the currently processed package?, to be removed */
    char			existsDEIarg;   /* class direct enclosing instance exists?, to be removed */
    int				classFile;		/* in java, my class file index
                                       == -1 for none, TODO to change
                                       it to s_noneFileIndex !!!
                                    */
    unsigned		recSearchCounter; /* tmp counter when looking for a record
                                        it flags searched classes
                                     */
};


/* ****************************************************************** */
/*              symbol definition item in symbol table                */

struct symbol {
    char					*name;
    char					*linkName;		/* fully qualified name for cx */
    S_position				pos;			/* definition position for most syms;
                                               import position for imported classes!
                                             */
    struct symbolBits {
        unsigned			record			: 1;  /* whether struct record */
        unsigned			isSingleImported: 1;  /* whether not imported by * import */
        unsigned			accessFlags		: 12; /* java access bits */
        unsigned			javaSourceLoaded: 1;  /* is jsl source file loaded ? */
        unsigned			javaFileLoaded	: 1;  /* is class file loaded ? */

        unsigned			symType			: SYMTYPES_LN;
        /* can be Default/Struct/Union/Enum/Label/Keyword/Macro/Package */
        unsigned			storage			: STORAGES_LN;
        unsigned			npointers		: 4; /*tmp. stored #of dcl. ptrs*/
    } b;
    union defUnion {
        S_typeModifiers		*type;		/* if symType == TypeDefault */
        S_symStructSpecific	*s;			/* if symType == Struct/Union */
        S_symbolList		*enums;		/* if symType == Enum */
        S_macroBody			*mbody;     /* if symType == Macro ! can be NULL!*/
        int					labn;		/* break/continue label index */
        int					keyWordVal; /* if symType == Keyword*/
    } u;
    S_symbol                *next;	/* next table item with the same hash */
};

struct symbolList {
    S_symbol        *d;
    S_symbolList	*next;
};

struct jslSymbolList {
    S_symbol        *d;
    S_position		pos;
    int             isSingleImportedFlag;
    S_jslSymbolList	*next;
};


/* ****************************************************************** */
/*          symbol definition item in cross-reference table           */

struct usageBits {
    unsigned base:8;								// 0 - 128, it should not grow anymore
    unsigned requiredAccess:MAX_REQUIRED_ACCESS_LN;	// required accessibility of the reference
    // local properties (not saved in tag files)
    unsigned dummy:1;								// unused for the moment
};

// !!! if you add a pointer to this structure, then update olcxCopyRefList
struct reference {
    S_usageBits				usg;
    S_position				p;
    S_reference             *next;
};

// !!! if you add a pointer to this structure, then update olcxCopyRefItem!
struct symbolRefItem {
    char						*name;
    unsigned					fileHash;
    int							vApplClass;	/* appl class for java virtuals */
    int							vFunClass;	/* fun class for java virtuals */
    struct symbolRefItemBits {
        unsigned				symType		: SYMTYPES_LN;
        unsigned				storage		: STORAGES_LN;
        unsigned				scope		: SCOPES_LN;
        unsigned				accessFlags	: 12; /* java access bits */
        unsigned				category	: 2;  /* local/global */
        unsigned				htmlWasLn	: 1;  /* html ln generated */
    } b;
    S_reference					*refs;
    S_symbolRefItem             *next;
};

struct symbolRefItemList {
    S_symbolRefItem			*d;
    S_symbolRefItemList		*next;
};

/* ************* class hierarchy  cross referencing ************** */

struct chReference {
    int		ofile;		/* file of origin */
    int     clas;		/* index of super-class */
    S_chReference	*next;
};


/* **************** processing cxref file ************************* */

struct cxScanFileFunctionLink {
    int		recordCode;
    void    (*handleFun)(int size,int ri,char**ccc,char**ffin,S_charBuf*bbb, int additionalArg);
    int		additionalArg;
};

/* ***************** on - line cross referencing ***************** */

struct olCompletion {
    char				*name;
    char				*fullName;
    char				*vclass;
    short int			jindent;
    short int			lineCount;
    char				cat;			/* CatGlobal/CatLocal */
    char				csymType;		/* symtype of completion */
    S_reference			ref;
    S_symbolRefItem		sym;
    S_olCompletion		*next;
};

struct olSymbolFoundInformation {
    S_symbolRefItem *symrefs;		/* this is valid */
    S_symbolRefItem *symRefsInfo;	/* additional for error message */
    S_reference *currentRef;
};

// !!! if you add a pointer to this structure, then update olcxCopyMenuSym!!
struct olSymbolsMenu {
    S_symbolRefItem     s;
    char				selected;
    char				visible;
    unsigned			ooBits;
    char				olUsage;	/* usage of symbol under cursor */
    short int			vlevel;		/* virt. level of applClass <-> olsymbol*/
    short int			refn;
    short int			defRefn;
    char				defUsage;   /* usage of definition reference */
    S_position          defpos;
    int                 outOnLine;
    S_editorMarkerList	*markers;	/* for refactory only */
    S_olSymbolsMenu		*next;
};

// if you add something to this structure, update olcxMoveTopFromAnotherUser()
// !!!!!
struct olcxReferences {
    S_reference         *r;			/* list of references */
    S_reference         *act;		/* actual reference */
    char				command;	/* OLO_PUSH/OLO_LIST/OLO_COMPLETION */
    char				language;	/* C/JAVA/YACC */
    time_t				atime;		/* last acces time */
    // refsuffix is useless now, should be removed !!!!
    char				refsuffix[MAX_OLCX_SUFF_SIZE];
    S_position			cpos;		/* caller position */
    S_olCompletion		*cpls;		/* completions list for OLO_COMPLETION */
    // following two lists should be probably split into hashed tables of lists
    // because of bad performances for class tree and global unused symbols
    S_olSymbolsMenu		*hkSelectedSym; /* resolved symbols under the cursor */
    S_olSymbolsMenu		*menuSym;		/* hkSelectedSyms plus same name */
    int					menuFilterLevel;
    int					refsFilterLevel;
    S_olcxReferences	*previous;
};

// this is useless, (refsuffix is not used), but I keep it for
// some time for case if something more is needed in class tree
struct classTreeData {
    char				refsuffix[MAX_OLCX_SUFF_SIZE];
    int					baseClassIndex;
    S_olSymbolsMenu		*tree;
};

struct olcxReferencesStack {
    S_olcxReferences	*top;
    S_olcxReferences	*root;
};

struct userOlcx {
    char                    *name;
    S_olcxReferencesStack	browserStack;
    S_olcxReferencesStack	completionsStack;
    S_olcxReferencesStack	retrieverStack;
    S_classTreeData			ct;
    S_userOlcx				*next;
};

/* ************************************************************* */
/* **********************  file tab Item *********************** */

struct fileItem {	/* to be renamed to constant pool item */
    char                *name;
    time_t				lastModif;
    time_t				lastInspect;
    time_t				lastUpdateMtime;
    time_t				lastFullUpdateMtime;
    struct fileItemBits {
        unsigned		cxLoading: 1;
        unsigned		cxLoaded : 1;
        unsigned		cxSaved  : 1;
        unsigned		commandLineEntered : 1;
        unsigned		scheduledToProcess : 1;
        unsigned		scheduledToUpdate : 1;
        unsigned		fullUpdateIncludesProcessed : 1;
        unsigned		isInterface : 1;        // class/interface for .class
        unsigned		isFromCxfile : 1;       // is this file indexed in XFiles
        //unsigned		classIsRelatedTo : 20;// tmp var for isRelated function
        unsigned		sourceFile : 20;// file containing the class definition
    } b;
    S_chReference		*sups;			/* super-classes references */
    S_chReference		*infs;			/* sub-classes references   */
    int					directEnclosingInstance;  /* for Java Only  */
    S_fileItem			*next;
};

struct fileItemList {
    S_fileItem		*d;
    S_fileItemList	*next;
};

/* ***************** COMPLETION STRUCTURES ********************** */


struct cline {					/* should be a little bit union-ified */
    char            *s;
    S_symbol        *t;
    short int		symType;
    short int		virtLevel;
//	unsigned		virtClassOrder;		TODO !!!!
    short int		margn;
    char			**margs;
    S_symbol		*vFunClass;
};

struct completions {
    char        idToProcess[MAX_FUN_NAME_SIZE];
    int			idToProcessLen;
    S_position	idToProcessPos;
    int         fullMatchFlag;
    int         isCompleteFlag;
    int         noFocusOnCompletions;
    int         abortFurtherCompletions;
    char        comPrefix[TMP_STRING_SIZE];
    int			maxLen;
    S_cline     a[MAX_COMPLETIONS];
    int         ai;
};

struct completionFunTab {
    int token;
    void (*fun)(S_completions*);
};

struct completionSymFunInfo {
    S_completions       *res;
    unsigned			storage;
};

struct completionSymInfo {
    S_completions       *res;
    unsigned			symType;
};

struct completionFqtMapInfo {
    S_completions       *res;
    int					completionType;
};

/* ************************** INIT STRUCTURES ********************* */

struct tokenNameIni {
    char        *name;
    int         token;
    unsigned    languages;
};

struct typeCharCodeIni {
    int         symType;
    char		code;
};

struct javaTypePCTIConvertIni {
    int		symType;
    int		PCTIndex;
};

struct typeModificationsInit {
    int	type;
    int	modShort;
    int	modLong;
    int	modSigned;
    int	modUnsigned;
};

struct intStringTab {
    int     i;
    char    *s;
};

struct currentlyParsedCl {		// class local, nested for classes
    S_symbol			*function;
    S_extRecFindStr		*erfsForParamsComplet;			// curently parsed method for param completion
    unsigned	funBegPosition;
    int         cxMemiAtFunBegin;
    int			cxMemiAtFunEnd;
    int         cxMemiAtClassBegin;
    int			cxMemiAtClassEnd;
    int			thisMethodMemoriesStored;
    int			thisClassMemoriesStored;
    int			parserPassedMarker;
};

struct currentlyParsedStatics {
    int             extractProcessedFlag;
    int             cxMemiAtBlockBegin;
    int             cxMemiAtBlockEnd;
    S_topBlock      *workMemiAtBlockBegin;
    S_topBlock      *workMemiAtBlockEnd;
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
    S_symbol		*lastDeclaratorType;
    S_symbol		*lastAssignementStruct;
};

/* ************************** JAVAS *********************************** */

struct javaStat {
    S_idIdentList		*className;			/* this class name */
    S_typeModifiers		*thisType;			/* this class type */
    S_symbol			*thisClass;			/* this class definition */
    int					currentNestIndex;	/* currently parsed nested class */
    char				*currentPackage;    /* current package */
    char				*unNamedPackageDir;	/* directory for unnamed package */
    char				*namedPackageDir;	/* inferred source-path for named package */
    S_symTab			*locals;			/* args and local variables */
    S_idIdentList       *lastParsedName;
    unsigned			cpMethodMods;		/* currently parsed method modifs */
    S_currentlyParsedCl	cp;					/* some parsing positions */
    int					classFileInd;		/* this file class index */
    S_javaStat			*next;				/* outer class */
};

/* java composed names */

struct idIdentList {
    S_idIdent       idi;
    char			*fname;			/* fqt name for java */
    uchar			nameType;		/* type of name segment for java */
    S_idIdentList	*next;
};

struct zipArchiveDir {
    union {
        S_zipArchiveDir	*sub;
        unsigned		offset;
    } u;
    S_zipArchiveDir		*next;
    char                name[1];			/* array of char */
};

struct zipFileTabItem {
    char            fn[MAX_FILE_NAME_SIZE];	/* stored with ';' at the end */
    struct stat		st;						/* status of the archiv file */
    S_zipArchiveDir	*dir;
};

/* ***************** unique counters  *********************** */

struct counters {
    int localSym;
    int localVar;
    int anonymousClassCounter;
};

/* **************************** CACHING ****************************** */

struct cachePoint {
    S_topBlock				*topBlock;
    S_topBlock              starTopBlock;
    int                     ppmMemoryi;
    int                     cxMemoryi;
    int                     mbMemoryi;
    char					*lbcc;		/* caching lbcc */
    short int				ibi;		/* caching ibi */
    short int               lineNumber;
    short int               ifDeep;
    S_cppIfStack            *ifstack;
    S_javaStat				*javaCached;
    S_counters				counts;
};

struct caching {
    char			activeCache;		/* whether putting input to cache */
    int				cpi;
    S_cachePoint	cp[MAX_CACHE_POINTS];
    int				ibi;
    int				ib[INCLUDE_CACHE_SIZE];	/* included files numbers */
    char			*lbcc;					/* first free of lb */
    char			lb[LEX_BUF_CACHE_SIZE];	/* lexems buffer */
    char			*lexcc;					/* first not yeat cached lexem */
    char			*cc;					/* cc when input from cache */
    char			*cfin;					/* end of cc, when input ... */
};

/* ************************ PRE-PROCESSOR **************************** */

struct stringList {
    char *d;
    struct stringList *next;
};

struct stringAddrList {
    char **d;
    struct stringAddrList *next;
};

struct macroArgTabElem {
    char *name;
    char *linkName;
    int order;
};

struct macroBody {
    short int argn;
    int size;
    char *name;			/* the name of the macro */
    char **args;		/* names of arguments */
    char *body;
};

struct charBuf {
    char        *cc;				/* first unread */
    char        *fin;				/* first free (invalid)  */
    char        a[CHAR_BUFF_SIZE];
    FILE        *ff;
    unsigned	filePos;			/* how many chars was readed from file */
    int			fileNumber;
    int         lineNum;
    char        *lineBegin;
    short int   collumnOffset;		/* collumn == cc-lineBegin + collumnOffset */
    char		isAtEOF;
    char		inputMethod;		/* unzipp/direct */
    char        z[CHAR_BUFF_SIZE];  /* zip input buffer */
    z_stream	zipStream;
};

struct lexBuf {
    char        *cc;				/* first unread */
    char        *fin;				/* first free (invalid)  */
    char        a[LEX_BUFF_SIZE];
#if 1
    S_position  pRing[LEX_POSITIONS_RING_SIZE];		// file/line/coll position
    unsigned    fpRing[LEX_POSITIONS_RING_SIZE];	// file offset position
    int         posi;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
#endif
    S_charBuf   cb;
};

struct cppIfStack {
    S_position pos;
    S_cppIfStack  *next;
};

struct fileDesc {
    char            *fileName ;
    int             lineNumber ;
    int				ifDeep;						/* deep of #ifs (C only)*/
    S_cppIfStack    *ifstack;					/* #if stack (C only) */
    struct lexBuf   lb;
};

struct lexInput {
    char            *cc;                /* pointer to current lexem */
    char            *fin;               /* end of buffer */
    char            *a;					/* beginning of buffer */
    char            *macname;			/* possible makro name */
    char			margExpFlag;		/* input Flag */
};

/* ********************* MEMORIES *************************** */

struct memory {
    int		(*overflowHandler)(int n);
    int     i;
    int		size;
    double	b;		//  double in order to get it properly alligned
};

/* ************************ HTML **************************** */

struct intlist {
    int         i;
    S_intlist   *next;
};

struct disabledList {
    int             file;
    int             clas;
    S_disabledList  *next;
};

struct htmlData {
    S_position          *cp;
    S_reference         *np;
    S_symbolRefItem     *nri;
};

struct htmlRefList {
    S_symbolRefItem	*s;
    S_reference		*r;
    S_symbolRefItem	*slist;		/* the hash list containing s, for virtuals */
    S_htmlRefList	*next;
};

struct htmlLocalListms {	// local xlist map structure
    FILE    *ff;
    int     fnum;
    char	*fname;
};

/* *********************************************************** */

struct programGraphNode {
    S_reference             *ref;		/* original reference of node */
    S_symbolRefItem         *symRef;
    S_programGraphNode		*jump;
    char					posBits;		/* INSIDE/OUSIDE block */
    char					stateBits;		/* visited + where setted */
    char					classifBits;	/* resulting classification */
    S_programGraphNode		*next;
};

struct exprTokenType {
    S_typeModifiers *t;
    S_reference     *r;
    S_position      *pp;
};

struct nestedConstrTokenType {
    S_typeModifiers *t;
    S_idIdentList	*nid;
    S_position      *pp;
};

struct unsPositionPair {
    unsigned    u;
    S_position	*p;
};

struct symbolPositionPair {
    S_symbol	*s;
    S_position	*p;
};

struct symbolPositionLstPair {
    S_symbol		*s;
    S_positionList	*p;
};

struct intPair {
    int i1;
    int i2;
};

struct typeModifiersListPositionLstPair {
    S_typeModifiersList		*t;
    S_positionList			*p;
};

struct whileExtractData {
    int i1;
    int i2;
    S_symbol *i3;
    S_symbol *i4;
};

struct referencesChangeData {
    char		*linkName;
    int			fnum;
    S_symbol	*cclass;
    int			category;
    int			cxMemBegin;
    int			cxMemEnd;
};

struct pushAllInBetweenData {
    int			minMemi;
    int			maxMemi;
};

struct tpCheckSpecialReferencesData {
    S_pushAllInBetweenData  mm;
    char					*symbolToTest;
    int						classToTest;
    S_symbolRefItem			*foundSpecialRefItem;
    S_reference				*foundSpecialR;
    S_symbolRefItem         *foundRefToTestedClass;
    S_symbolRefItem         *foundRefNotToTestedClass;
    S_reference             *foundOuterScopeRef;
};

struct tpCheckMoveClassData {
    S_pushAllInBetweenData  mm;
    char					*spack;
    char					*tpack;
    int						transPackageMove;
    char					*sclass;
};

/* ***************** Java simple load file ********************** */


struct jslClassStat {
    S_idIdentList	*className;
    S_symbol		*thisClass;
    char			*thisPackage;
    int				annonInnerCounter;	/* counter for anonym inner classes*/
    int				functionInnerCounter; /* counter for function inner class*/
    S_jslClassStat	*next;
};

struct jslStat {
    int                     pass;
    int                     sourceFileNumber;
    int						language;
    S_jslTypeTab            *typeTab;
    S_jslClassStat          *classStat;
    S_symbolList            *waitList;
    void/*YYSTYPE*/			*savedyylval;
    struct yyGlobalState    *savedYYstate;
    int						yyStateSize;
    S_idIdent				yyIdentBuf[YYBUFFERED_ID_INDEX]; // pending idents
    S_jslStat				*next;
};

/* ***************** editor structures ********************** */

struct editorBuffer {
    char            *name;
    int				ftnum;
    char			*fileName;
    struct stat		stat;
    S_editorMarker	*markers;
    struct editorBufferAllocationData {
        int     bufferSize;
        char	*text;
        int		allocatedFreePrefixSize;
        char	*allocatedBlock;
        int		allocatedIndex;
        int		allocatedSize;
    } a;
    struct editorBufferBits {
        unsigned		textLoaded:1;
        unsigned		modified:1;
        unsigned		modifiedSinceLastQuasySave:1;
    } b;
};

struct editorBufferList {
    S_editorBuffer      *f;
    S_editorBufferList	*next;
};

struct editorMemoryBlock {
    struct editorMemoryBlock *next;
};

struct editorMarker {
    S_editorBuffer		*buffer;
    unsigned            offset;
    struct editorMarker *previous;      // previous marker in this buffer
    struct editorMarker *next;          // next marker in this buffer
};

struct editorMarkerList {
    S_editorMarker		*d;
    S_usageBits			usg;
    S_editorMarkerList	*next;
};

struct editorRegion {
    S_editorMarker		*b;
    S_editorMarker		*e;
};

struct editorRegionList {
    S_editorRegion		r;
    S_editorRegionList	*next;
};

struct editorUndo {
    S_editorBuffer		*buffer;
    int					operation;
    union editorUndoUnion {
        struct editorUndoStrReplace {
            unsigned			offset;
            unsigned            size;
            unsigned			strlen;
            char				*str;
        } replace;
        struct editorUndoRenameBuff {
            char				*name;
        } rename;
        struct editorUndoMoveBlock {
            unsigned			offset;
            unsigned			size;
            S_editorBuffer		*dbuffer;
            unsigned			doffset;
        } moveBlock;
    } u;
    S_editorUndo		*next;
};

/* *********************************************************** */

struct availableRefactoring {
    unsigned    available;
    char		*option;
};

/* **************     parse tree with positions    *********** */

// following structures are used in yacc parser, they are always
// containing 'b','e' records for begin and end position of parse tree
// node and additional data 'd' for parsing.

struct bb_int {
    S_position  b, e;
    int		d;
};
struct bb_unsigned {
    S_position      b, e;
    unsigned		d;
};
struct bb_symbol {
    S_position      b, e;
    S_symbol		*d;
};
struct bb_symbolList {
    S_position          b, e;
    S_symbolList		*d;
};
struct bb_typeModifiers {
    S_position          b, e;
    S_typeModifiers		*d;
};
struct bb_typeModifiersList {
    S_position              b, e;
    S_typeModifiersList		*d;
};
struct bb_freeTrail {
    S_position      b, e;
    S_freeTrail		*d;
};
struct bb_idIdent {
    S_position      b, e;
    S_idIdent		*d;
};
struct bb_idIdentList {
    S_position          b, e;
    S_idIdentList		*d;
};
struct bb_exprTokenType {
    S_position          b, e;
    S_exprTokenType		d;
};
struct bb_nestedConstrTokenType {
    S_position                  b, e;
    S_nestedConstrTokenType		d;
};
struct bb_intPair {
    S_position      b, e;
    S_intPair		d;
};
struct bb_whileExtractData {
    S_position              b, e;
    S_whileExtractData		*d;
};
struct bb_position {
    S_position      b, e;
    S_position		d;
};
struct bb_unsPositionPair {
    S_position              b, e;
    S_unsPositionPair		d;
};
struct bb_symbolPositionPair {
    S_position                  b, e;
    S_symbolPositionPair		d;
};
struct bb_symbolPositionLstPair {
    S_position                  b, e;
    S_symbolPositionLstPair		d;
};
struct bb_positionLst {
    S_position          b, e;
    S_positionList		*d;
};
struct bb_typeModifiersListPositionLstPair  {
    S_position                                  b, e;
    S_typeModifiersListPositionLstPair			d;
};



/* *********************************************************** */
/* *********************   options  ************************** */
/* *********************************************************** */

struct htmlCutPathesOpts {
    int pathesNum;
    char *path[MAX_HTML_CUT_PATHES];
    int plen[MAX_HTML_CUT_PATHES];
};

struct setGetEnv {
    int num;
    char *name[MAX_SET_GET_OPTIONS];
    char *value[MAX_SET_GET_OPTIONS];
};

// TODO all strings inside to static string array
struct options {
    int fileEncoding;
    char completeParenthesis;
    int defaultAddImportStrategy;
    char referenceListWithoutSource;
    char jeditOldCompletions;		// to be removed
    int completionOverloadWizardDeep;
    int exit;
    int commentMovingLevel;
    S_stringList *pruneNames;
    S_stringList *inputFiles;
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
    int refactoringRegime;
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
    int urlGenTemporaryFile;
    int urlAutoRedirect;
    int javaFilesOnly;
    int exactPositionResolve;
    char *outputFileName;
    char *lineFileName;
    S_stringList *includeDirs;
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
    //& char *extractName;
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
    int taskRegime;
    char *user;
    int debug;
    int cpp_comment;
    int c_struct_scope;
    int cxrefs;
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
    S_setGetEnv setGetEnv;
    S_htmlCutPathesOpts htmlCut;

    // memory for strings
    S_stringAddrList	*allAllocatedStrings;
    S_memory			pendingMemory;
    char				pendingFreeSpace[SIZE_opiMemory];
};

/* ************************ HASH TABLES ****************************** */

#define HASH_TAB_TYPE struct symTab
#define HASH_ELEM_TYPE S_symbol
#define HASH_FUN_PREFIX symTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#define HASH_TAB_TYPE struct javaFqtTab
#define HASH_ELEM_TYPE S_symbolList
#define HASH_FUN_PREFIX javaFqtTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#define HASH_TAB_TYPE struct jslTypeTab
#define HASH_ELEM_TYPE S_jslSymbolList
#define HASH_FUN_PREFIX jslTypeTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#define HASH_TAB_TYPE struct refTab
#define HASH_ELEM_TYPE S_symbolRefItem
#define HASH_FUN_PREFIX refTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

#define HASH_TAB_TYPE struct idTab
#define HASH_ELEM_TYPE S_fileItem
#define HASH_FUN_PREFIX idTab

#include "hashtab.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX


#define HASH_TAB_TYPE struct editorBufferTab
#define HASH_ELEM_TYPE S_editorBufferList
#define HASH_FUN_PREFIX editorBufferTab

#include "hashlist.th"

#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#undef HASH_FUN_PREFIX

/* ********************************************************************* */
/*                            EXTERN DEFINITIONS                         */
/* ********************************************************************* */

/* ***********************************************************************
** commons.c
*/

void emergencyExit(int exitStatus);
extern void warning(int kod, char *sprava);
extern void error(int kod, char *sprava);
extern void fatalError(int kod, char *sprava, int exitCode);
extern void internalCheckFail(char *expr, char *file, int line);

/* ***********************************************************************
** cct.c
*/
void cctAddSimpleValue(S_cctNode *cc, S_symbol *x, int deepFactor);
int cctIsMember(S_cctNode *cc, S_symbol *x, int deepFactor);
void cctAddCctTree(S_cctNode *cc, S_cctNode *x, int deepFactor);
void cctDump(S_cctNode *cc, int deep);
void cctTest();


/* ***********************************************************************
** misc.c
*/

void ppcGenSynchroRecord();
void ppcIndentOffset();
void ppcGenGotoOffsetPosition(char *fname, int offset);
void ppcGenRecordBegin(char *kind);
void ppcGenRecordWithAttributeBegin(char *kind, char *attr, char *val);
void ppcGenRecordWithNumAttributeBegin(char *kind, char *attr, int val);
void ppcGenRecordEnd(char *kind);
void ppcGenNumericRecordBegin(char *kind, int val);
void ppcGenTwoNumericAndRecordBegin(char *kind, char *attr1, int val1, char *attr2, int val2);
void ppcGenWithNumericAndRecordBegin(char *kind, int val, char *attr, char *attrVal);
void ppcGenAllCompletionsRecordBegin(int nofocus, int len);
void ppcGenTwoNumericsAndrecord(char *kind, char *attr1, int val1, char *attr2, int val2, char *message,char *suff);
void ppcGenRecordWithNumeric(char *kind, char *attr, int val, char *message,char *suff);
void ppcGenNumericRecord(char *kind, int val, char *message, char *suff);
void ppcGenRecord(char *kind, char *message, char *suffix);
void ppcGenTmpBuff();
void ppcGenDisplaySelectionRecord(char *message, int messageType, int continuation);
void ppcGenGotoMarkerRecord(S_editorMarker *pos);
void ppcGenPosition(S_position *p);
void ppcGenDefinitionNotFoundWarning();
void ppcGenDefinitionNotFoundWarningAtBottom();
void ppcGenReplaceRecord(char *file, int offset, char *oldName, int oldNameLen, char *newName);
void ppcGenPreCheckRecord(S_editorMarker *pos, int oldLen);
void ppcGenReferencePreCheckRecord(S_reference *r, char *text);
void ppcGenGotoPositionRecord(S_position *p);
void ppcGenOffsetPosition(char *fn, int offset);
void ppcGenMarker(S_editorMarker *m);
char * stringNumStr( int rr, int ed, int em, int ey, char *own );
void jarFileParse();
void scanJarFilesForTagSearch();
void classFileParse();
void fillTrivialSpecialRefItem( S_symbolRefItem *ddd , char *name);
void setExpirationFromLicenseString();
int optionsOverflowHandler(int n);
int cxMemoryOverflowHandler(int n);

void noSuchRecordError(char *rec);
void methodAppliedOnNonClass(char *rec);
void methodNameNotRecognized(char *rec);
void dumpOptions(int nargc, char **nargv);
void stackMemoryInit();
void *stackMemoryAlloc(int size);
void *stackMemoryPush(void *p, int size);
int  *stackMemoryPushInt(int x);
char *stackMemoryPushString(char *s);
void stackMemoryPop(void *, int size);
void stackMemoryBlockStart();
void stackMemoryBlockFree();
void stackMemoryStartErrZone();
void stackMemoryStopErrZone();
void stackMemoryErrorInZone();
void stackMemoryDump();

void addToTrail (void (*action)(void*),  void *p);
void removeFromTrailUntil(S_freeTrail *untilP);

void symDump(S_symbol *);
void symbolRefItemDump(S_symbolRefItem *ss);
int javaTypeStringSPrint(char *buff, char *str, int nameStyle, int *oNamePos);
void typeSPrint(char *buff,int *size,S_typeModifiers *t,char*name,
                       int dclSepChar, int maxDeep, int typedefexp,
                       int longOrShortName, int *oNamePos);
void throwsSprintf(char *out, int outsize, S_symbolList *exceptions);
void macDefSPrintf(char *buff, int *size, char *name1, char *name2, int argn, char **args, int *oNamePos);
char * string3ConcatInStackMem(char *str1, char *str2, char *str3);

unsigned hashFun(char *s);
void javaSignatureSPrint(char *buff, int *size, char *sig, int classstyle);
void fPutDecimal(FILE *f, int n);
char *strmcpy(char *dest, char *src);
char *simpleFileName(char *fullFileName);
char *directoryName_st(char *fullFileName);
char *simpleFileNameWithoutSuffix_st(char *fullFileName);
int containsWildCharacter(char *ss);
int shellMatch(char *string, int stringLen, char *pattern, int caseSensitive);
void expandWildCharactersInOnePathRec(char *fn, char **outpaths, int *freeolen);
void expandWildCharactersInOnePath(char *fn, char *outpaths, int olen);
void expandWildCharactersInPaths(char *paths, char *outpaths, int freeolen);
char * getRealFileNameStatic(char *fn);
char *create_temporary_filename();
void copyFile(char *src, char *dest);
void createDir(char *dirname);
void removeFile(char *dirname);
int substringIndexWithLimit(char *s, int limit, char *subs);
int stringContainsSubstring(char *s, char *subs);
void javaGetPackageNameFromSourceFileName(char *src, char *opack);
int substringIndex(char *s, char *subs);
int stringEndsBySuffix(char *s, char *suffix);
int fileNameHasOneOfSuffixes(char *fname, char *suffs);
int mapPatternFiles(
        char *pattern,
        void (*fun) (MAP_FUN_PROFILE),
        char *a1,
        char *a2,
        S_completions *a3,
        void *a4,
        int *a5
    );
int mapDirectoryFiles(
        char *dirname,
        void (*fun) (MAP_FUN_PROFILE),
        int allowEditorFilesFlag,
        char *a1,
        char *a2,
        S_completions *a3,
        void *a4,
        int *a5
    );
void javaMapDirectoryFiles1(
        char *packfile,
        void (*fun)(MAP_FUN_PROFILE),
        S_completions *a1,
        void *a2,
        int *a3
    );
void javaMapDirectoryFiles2(
        S_idIdentList *packid,
        void (*fun)(MAP_FUN_PROFILE),
        S_completions *a1,
        void *a2,
        int *a3
    );
char *lastOccurenceInString(char *ss, int ch);
char *lastOccurenceOfSlashOrAntiSlash(char *ss);
char * getFileSuffix(char *fn);
char *javaCutClassPathFromFileName(char *fname);
char *javaCutSourcePathFromFileName(char *fname);
int pathncmp(char *ss1, char *ss2, int n, int caseSensitive);
int fnCmp(char *ss1, char *ss2);
int fnnCmp(char *ss1, char *ss2, int n);

/* ***********************************************************************
** completion.c
*/
void processName(char *name, S_cline *t, int orderFlag, void *c);
void completeForSpecial1(S_completions*);
void completeForSpecial2(S_completions*);
void completeUpFunProfile(S_completions* c);
void completeTypes(S_completions*);
void completeStructs(S_completions*);
void completeRecNames(S_completions*);
void completeEnums(S_completions*);
void completeLabels(S_completions*);
void completeMacros(S_completions*c);
void completeOthers(S_completions*);
void javaCompleteTypeSingleName(S_completions*);
void javaHintImportFqt(S_completions*c);
void javaHintVariableName(S_completions*c);
void javaHintCompleteNonImportedTypes(S_completions*c);
void javaHintCompleteMethodParameters(S_completions *c);
void javaCompleteTypeCompName(S_completions*);
void javaCompleteThisPackageName(S_completions*c);
void javaCompletePackageSingleName(S_completions*);
void javaCompleteExprSingleName(S_completions*);
void javaCompleteUpMethodSingleName(S_completions*);
void javaCompleteFullInheritedMethodHeader(S_completions*c);
void javaCompletePackageCompName(S_completions*);
void javaCompleteExprCompName(S_completions*);
void javaCompleteMethodCompName(S_completions*);
void javaCompleteHintForConstructSingleName(S_completions *c);
void javaCompleteConstructSingleName(S_completions*c);
void javaCompleteConstructCompName(S_completions*c);
void javaCompleteConstructNestNameName(S_completions*c);
void javaCompleteConstructNestPrimName(S_completions*c);
void javaCompleteStrRecordPrimary(S_completions*c);
void javaCompleteStrRecordSuper(S_completions*c);
void javaCompleteStrRecordQualifiedSuper(S_completions*c);
void javaCompleteClassDefinitionNameSpecial(S_completions*c);
void javaCompleteClassDefinitionName(S_completions*c);
void javaCompleteThisConstructor(S_completions*c);
void javaCompleteSuperConstructor(S_completions*c);
void javaCompleteSuperNestedConstructor(S_completions*c);
void completeYaccLexem(S_completions*c);
char *javaGetShortClassName( char *inn);
char *javaGetShortClassNameFromFileNum_st(int fnum);
char *javaGetNudePreTypeName_st( char *inn, int cutMode);

void olCompletionListInit(S_position *originalPos);
void formatOutputLine(char *tt, int startingColumn);
void printCompletionsList(int noFocus);
void printCompletions(S_completions*c);

/* ***********************************************************************
** generate.c
*/

void generate(S_symbol *s);
void genProjections(int n);

/* ***********************************************************************
** cxref.c, cxfile.c
*/

int olcxReferenceInternalLessFunction(S_reference *r1, S_reference *r2);
int olSymbolRefItemLess(S_symbolRefItem *s1, S_symbolRefItem *s2);
int searchStringFitness(char *cxtag, int slen);
char *crTagSearchLineStatic(char *name, S_position *p,
                            int *len1, int *len2, int *len3);
int symbolNameShouldBeHiddenFromReports(char *name);
void searchSymbolCheckReference(S_symbolRefItem  *ss, S_reference *rr);
void tagSearchCompactShortResults();
void printTagSearchResults();
int creatingOlcxRefs();
int olcxTopSymbolExists();
S_olSymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi,int storage);
int itIsSameCxSymbol(S_symbolRefItem *p1, S_symbolRefItem *p2);
int itIsSameCxSymbolIncludingFunClass(S_symbolRefItem *p1, S_symbolRefItem *p2);
int itIsSameCxSymbolIncludingApplClass(S_symbolRefItem *p1, S_symbolRefItem *p2);
int olcxItIsSameCxSymbol(S_symbolRefItem *p1, S_symbolRefItem *p2);
void olcxRecomputeSelRefs( S_olcxReferences *refs );
void olProcessSelectedReferences(S_olcxReferences    *rstack, void (*referencesMapFun)(S_olcxReferences *rstack, S_olSymbolsMenu *ss));
void olcxPopOnly();
S_reference * olcxCopyRefList(S_reference *ll);
void olStackDeleteSymbol( S_olcxReferences *refs);
int getFileNumberFromName(char *name);
int olcxVirtualyAdequate(int usage, int vApplCl, int vFunCl,
                        int olUsage, int olApplCl, int olFunCl);
void generateOnlineCxref(	S_position *p,
                            char *commandString,
                            int usage,
                            char *suffix,
                            char *suffix2
    );
S_reference *olcxAddReferenceNoUsageCheck(S_reference **rlist, S_reference *ref, int bestMatchFlag);
S_reference *olcxAddReference(S_reference **rlist,S_reference *ref,int bestMatchFlag);
int isRelatedClass(int cl1, int cl2);
void olcxFreeReferences(S_reference *r);
int isSmallerOrEqClass(int inf, int sup);
int olcxPushLessFunction(S_reference *r1, S_reference *r2);
int olcxListLessFunction(S_reference *r1, S_reference *r2);
char *getJavaDocUrl_st(S_symbolRefItem *rr);
char *getLocalJavaDocFile_st(char *fileUrl);
char *getFullUrlOfJavaDoc_st(char *fileUrl);
int htmlJdkDocAvailableForUrl(char *ss);
void setIntToZero(void *p);
S_reference *duplicateReference(S_reference *r);
S_reference * addCxReferenceNew(S_symbol *p, S_position *pos, S_usageBits *ub, int vFunCl, int vApplCl);
S_reference * addCxReference(S_symbol *p, S_position *pos, int usage, int vFunClass,int vApplClass);
S_reference *addSpecialFieldReference(char *name, int storage, int fnum, S_position *p, int usage);
void addClassTreeHierarchyReference(int fnum, S_position *p, int usage);
void addCfClassTreeHierarchyRef(int fnum, int usage);
void addTrivialCxReference (char *name, int symType, int storage, S_position *pos, int usage);
int cxFileHashNumber(char *sym);
void genReferenceFile(int updateFlag, char *fname);
void addSubClassItemToFileTab( int sup, int inf, int origin);
void addSubClassesItemsToFileTab(S_symbol *ss, int origin);
void scanCxFile(S_cxScanFileFunctionLink *scanFuns);
void olcxAddReferences(S_reference *list, S_reference **dlist, int fnum, int bestMatchFlag);
void olSetCallerPosition(S_position *pos);
S_olCompletion * olCompletionListPrepend(char *name, char *fullText, char *vclass, int jindent, S_symbol *s, S_symbolRefItem *ri, S_reference *dfpos, int symType, int vFunClass, S_olcxReferences *stack);
S_olSymbolsMenu *olCreateNewMenuItem(
        S_symbolRefItem *sym, int vApplClass, int vFunCl, S_position *defpos, int defusage,
        int selected, int visible,
        unsigned ooBits, int olusage, int vlevel
    );
S_olSymbolsMenu *olAddBrowsedSymbol(
    S_symbolRefItem *sym, S_olSymbolsMenu **list,
    int selected, int visible, unsigned ooBits,
    int olusage, int vlevel,
    S_position *defpos, int defusage);
void renameCollationSymbols(S_olSymbolsMenu *sss);
void olCompletionListReverse();
S_reference **addToRefList(	S_reference **list,
                            S_usageBits *pusage,
                            S_position *pos,
                            int category
                        );
int isInRefList(S_reference *list,
                S_usageBits *pusage,
                S_position *pos,
                int category
                );
char *getXrefEnvironmentValue( char *name );
int byPassAcceptableSymbol(S_symbolRefItem *p);
int itIsSymbolToPushOlRefences(S_symbolRefItem *p, S_olcxReferences *rstack, S_olSymbolsMenu **rss, int checkSelFlag);
void olcxAddReferenceToOlSymbolsMenu(S_olSymbolsMenu  *cms, S_reference *rr,
                          int bestFitTlag);
void putOnLineLoadedReferences(S_symbolRefItem *p);
void genOnLineReferences(	S_olcxReferences *rstack, S_olSymbolsMenu *cms);
S_olSymbolsMenu *createSelectionMenu(S_symbolRefItem *dd);
void mapCreateSelectionMenu(S_symbolRefItem *dd);
int olcxFreeOldCompletionItems(S_olcxReferencesStack *stack);
void olcxInit();
S_userOlcx *olcxSetCurrentUser(char *user);
void linkNamePrettyPrint(char *ff, char *javaLinkName, int maxlen,int argsStyle );
char *simpleFileNameFromFileNum(int fnum);
char *getShortClassNameFromClassNum_st(int fnum);
S_reference * getDefinitionRef(S_reference *rr);
void printSymbolLinkNameString( FILE *ff, char *linkName);
int safetyCheck2ShouldWarn();
void olCreateSelectionMenu(int command);
void printClassFqtNameFromClassNum(FILE *ff, int fnum);
void sprintfSymbolLinkName(char *ttt, S_olSymbolsMenu *ss);
void printSymbolLinkName( FILE *ff, S_olSymbolsMenu *ss);
void olcxPushEmptyStackItem(S_olcxReferencesStack *stack);
void olcxPrintSelectionMenu(S_olSymbolsMenu *);
int ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2);
void olcxPrintClassTree(S_olSymbolsMenu *sss);
void olcxReferencesDiff(S_reference **anr1, S_reference **aor2,S_reference **diff);
int olcxShowSelectionMenu();
int getClassNumFromClassLinkName(char *name, int defaultResult);
void getLineColCursorPositionFromCommandLineOption( int *l, int *c );
void changeClassReferencesUsages(char *linkName, int category, int fnum,
                                 S_symbol *cclass);
int isStrictlyEnclosingClass(int enclosedClass, int enclosingClass);
void changeMethodReferencesUsages(char *linkName, int category, int fnum, S_symbol *cclass);
void olcxPushSpecialCheckMenuSym(int command, char *symname);
int refOccursInRefs(S_reference *r, S_reference *list);
void olcxCheck1CxFileReference(S_symbolRefItem *ss, S_reference *r);
void olcxPushSpecial(char *fieldName, int command);
int isPushAllMethodsValidRefItem(S_symbolRefItem *ri);
int symbolsCorrespondWrtMoving(S_olSymbolsMenu *osym,S_olSymbolsMenu *nsym,int command);
void olcxPrintPushingAction(int opt, int afterMenu);
void olPushAllReferencesInBetween(int minMemi, int maxMemi);
int tpCheckSourceIsNotInnerClass();
void tpCheckFillMoveClassData(S_tpCheckMoveClassData *dd, char *spack, char *tpack);
int tpCheckMoveClassAccessibilities();
int tpCheckSuperMethodReferencesForDynToSt();
int tpCheckOuterScopeUsagesForDynToSt();
int tpCheckMethodReferencesWithApplOnSuperClassForPullUp();
int tpCheckSuperMethodReferencesForPullUp();
int tpCheckSuperMethodReferencesAfterPushDown();
int tpCheckTargetToBeDirectSubOrSupClass(int flag, char *subOrSuper);
int tpPullUpFieldLastPreconditions();
int tpPushDownFieldLastPreconditions();
S_symbol *getMoveTargetClass();
int javaGetSuperClassNumFromClassNum(int cn);
int javaIsSuperClass(int superclas, int clas);
void pushLocalUnusedSymbolsAction();
void mainAnswerEditAction();
void freeOldestOlcx();
S_olSymbolsMenu *olcxFreeSymbolMenuItem(S_olSymbolsMenu *ll);
void olcxFreeResolutionMenu( S_olSymbolsMenu *sym );
int refCharCode(int usage);
int scanReferenceFile(	char *fname, char *fns1, char *fns2,
                        S_cxScanFileFunctionLink *scanFunTab);
int smartReadFileTabFile();
void readOneAppropReferenceFile(char *symname, S_cxScanFileFunctionLink  *scanFunTab);
void scanReferenceFiles(char *fname, S_cxScanFileFunctionLink *scanFunTab);

extern S_cxScanFileFunctionLink s_cxScanFileTab[];
extern S_cxScanFileFunctionLink s_cxFullScanFunTab[];
extern S_cxScanFileFunctionLink s_cxByPassFunTab[];
extern S_cxScanFileFunctionLink s_cxSymbolMenuCreationTab[];
extern S_cxScanFileFunctionLink s_cxSymbolLoadMenuRefs[];
extern S_cxScanFileFunctionLink s_cxScanFunTabFor2PassMacroUsage[];
extern S_cxScanFileFunctionLink s_cxScanFunTabForClassHierarchy[];
extern S_cxScanFileFunctionLink s_cxFullUpdateScanFunTab[];
extern S_cxScanFileFunctionLink s_cxHtmlGlobRefListScanFunTab[];
extern S_cxScanFileFunctionLink s_cxSymbolSearchScanFunTab[];
extern S_cxScanFileFunctionLink s_cxDeadCodeDetectionScanFunTab[];

/* ***********************************************************************
** classh.c
*/

void classHierarchyGenInit();
void setTmpClassBackPointersToMenu(S_olSymbolsMenu *menu);
int chLineOrderLess(S_olSymbolsMenu *r1, S_olSymbolsMenu *r2);
void splitMenuPerSymbolsAndMap(
    S_olSymbolsMenu *rrr,
    void (*fun)(S_olSymbolsMenu *, void *, void *),
    void *p1,
    char *p2
    );
void htmlGenGlobRefLists(S_olSymbolsMenu *rrr, FILE *ff, char *fn);
void genClassHierarchies( FILE *ff, S_olSymbolsMenu *rrr,
                                        int virtFlag, int pass );
int classHierarchyClassNameLess(int c1, int c2);
int classHierarchySupClassNameLess(S_chReference *c1, S_chReference *c2);

/* ***********************************************************************
** semact.c
*/

void unpackPointers(S_symbol *pp);
int displayingErrorMessages();
void deleteSymDef(void *p);
void addSymbol(S_symbol *pp, S_symTab *tab);
void recFindPush(S_symbol *sym, S_recFindStr *rfs);
S_recFindStr * iniFind(S_symbol *s, S_recFindStr *rfs);
int javaOuterClassAccessible(S_symbol *cl);
int javaRecordAccessible(S_recFindStr *rfs, S_symbol *applcl, S_symbol *funcl, S_symbol *rec, unsigned recAccessFlags);
int javaRecordVisibleAndAccessible(S_recFindStr *rfs, S_symbol *applCl, S_symbol *funCl, S_symbol *r);
int javaGetMinimalAccessibility(S_recFindStr *rfs, S_symbol *r);
int findStrRecordSym(	S_recFindStr *ss,
                                char *recname,
                                S_symbol **res,
                                int javaClassif,
                                int accessCheck,
                                int visibilityCheck
                    );
S_symbol *addNewSymbolDef(S_symbol *p, unsigned storage, S_symTab *tab, int usage);
S_symbol *addNewCopyOfSymbolDef(S_symbol *def, unsigned defaultStorage);
S_symbol *addNewDeclaration(S_symbol *btype, S_symbol *decl, S_idIdentList *idl,
                            unsigned storage, S_symTab *tab);
int styyerror(char *s);
int styyErrorRecovery();
void setToNull(void *p);
void allocNewCurrentDefinition();
S_symbol *typeSpecifier1(unsigned t);
void declTypeSpecifier1(S_symbol *d, unsigned t);
S_symbol *typeSpecifier2(S_typeModifiers *t);
void declTypeSpecifier2(S_symbol *d, S_typeModifiers *t);
void declTypeSpecifier21(S_typeModifiers *t, S_symbol *d);
S_typeModifiers *appendComposedType(S_typeModifiers **d, unsigned t);
S_typeModifiers *prependComposedType(S_typeModifiers *d, unsigned t);
void completeDeclarator(S_symbol *t, S_symbol *d);
void addFunctionParameterToSymTable(S_symbol *function, S_symbol *p, int i, S_symTab *tab);
S_typeModifiers *crSimpleTypeMofifier (unsigned t);
S_symbolList *crDefinitionList(S_symbol *d);
S_symbol *crSimpleDefinition(unsigned storage, unsigned t, S_idIdent *id);
int findStrRecord(	S_symbol		*s,
                    char            *recname,	/* can be NULL */
                    S_symbol        **res,
                    int             javaClassif
                );
S_reference * findStrRecordFromSymbol(	S_symbol *str,
                                                S_idIdent *record,
                                                S_symbol **res,
                                                int javaClassif,
                                                S_idIdent *super
                        );
S_reference * findStrRecordFromType(	S_typeModifiers *str,
                            S_idIdent *record,
                            S_symbol **res,
                            int javaClassif
                        );
int mergeArguments(S_symbol *id, S_symbol *ty);
S_typeModifiers *simpleStrUnionSpecifier(	S_idIdent *typeName,
                                            S_idIdent *id,
                                            int usage
                                        );
S_typeModifiers *crNewAnnonymeStrUnion(S_idIdent *typeName);
void specializeStrUnionDef(S_symbol *sd, S_symbol *rec);
S_typeModifiers *simpleEnumSpecifier(S_idIdent *id, int usage);
void setGlobalFileDepNames(char *iname, S_symbol *pp, int memory);
S_typeModifiers *createNewAnonymousEnum(S_symbolList *enums);
void appendPositionToList(S_positionList **list, S_position *pos);
void setParamPositionForFunctionWithoutParams(S_position *lpar);
void setParamPositionForParameter0(S_position *lpar);
void setParamPositionForParameterBeyondRange(S_position *rpar);
S_symbol *crEmptyField();
void handleDeclaratorParamPositions(S_symbol *decl, S_position *lpar,
                                    S_positionList *commas, S_position *rpar,
                                           int hasParam);
void handleInvocationParamPositions(S_reference *ref, S_position *lpar,
                                    S_positionList *commas, S_position *rpar,
                                    int hasParam);
void javaHandleDeclaratorParamPositions(S_position *sym, S_position *lpar,
                                               S_positionList *commas, S_position *rpar);
void setLocalVariableLinkName(struct symbol *p);
void labelReference(S_idIdent *id, int usage);

/* ***********************************************************************
** jsemact.c
*/

void javaCheckForPrimaryStart(S_position *cpos, S_position *pp);
void javaCheckForPrimaryStartInNameList(S_idIdentList *name, S_position *pp);
void javaCheckForStaticPrefixStart(S_position *cpos, S_position *bpos);
void javaCheckForStaticPrefixInNameList(S_idIdentList *name, S_position *pp);
S_position *javaGetNameStartingPosition(S_idIdentList *name);
char *javaCreateComposedName(
                                    char			*prefix,
                                    S_idIdentList   *className,
                                    int             classNameSeparator,
                                    char            *name,
                                    char			*resBuff,
                                    int				resBufSize
                                );
int findTopLevelName(
                                char                *name,
                                S_recFindStr        *resRfs,
                                S_symbol			**resMemb,
                                int                 classif
                                );
int javaClassifySingleAmbigNameToTypeOrPack(S_idIdentList *name,
                                                   S_symbol **str,
                                                   int cxrefFlag
                                                   );
void javaAddImportConstructionReference(S_position *importPos, S_position *pos, int usage);
int javaClassifyAmbiguousName(		S_idIdentList *name,
                                    S_recFindStr *rfs,
                                    S_symbol **str,
                                    S_typeModifiers **expr,
                                    S_reference **oref,
                                    S_reference **rdtoref, int allowUselesFqtRefs,
                                    int classif,
                                    int usage
                                    );
S_reference *javaClassifyToTypeOrPackageName(S_idIdentList *tname, int usage, S_symbol **str, int allowUselesFqtRefs);
S_reference *javaClassifyToTypeName(S_idIdentList *tname, int usage, S_symbol **str, int allowUselesFqtRefs);
S_symbol * javaQualifiedThis(S_idIdentList *tname, S_idIdent *thisid);
void javaClassifyToPackageName( S_idIdentList *id );
void javaClassifyToPackageNameAndAddRefs(S_idIdentList *id, int usage);
char *javaImportSymbolName_st(int file, int line, int coll);
S_typeModifiers *javaClassifyToExpressionName(S_idIdentList *name,S_reference **oref);
S_symbol *javaTypeNameDefinition(S_idIdentList *tname);
void javaSetFieldLinkName(S_symbol *d);
void javaAddPackageDefinition(S_idIdentList *id);
S_symbol *javaAddType(S_idIdentList *clas, int accessFlag, S_position *p);
S_symbol *javaCreateNewMethod(char *name, S_position *pos, int mem);
int javaTypeToString(S_typeModifiers *type, char *pp, int ppSize);
int javaIsYetInTheClass(	S_symbol	*clas,
                                char		*lname,
                                S_symbol	**eq
                                );
int javaSetFunctionLinkName(S_symbol *clas, S_symbol *decl, int mem);
S_symbol * javaGetFieldClass(char *fieldLinkName, char **fieldAdr);
void javaAddNestedClassesAsTypeDefs(S_symbol *cc,
                        S_idIdentList *oclassname, int accessFlags);
int javaTypeFileExist(S_idIdentList *name);
S_symbol *javaTypeSymbolDefinition(S_idIdentList *tname, int accessFlags,int addType);
S_symbol *javaTypeSymbolUsage(S_idIdentList *tname, int accessFlags);
void javaReadSymbolFromSourceFileEnd();
void javaReadSymbolFromSourceFileInit( int sourceFileNum,
                                       S_jslTypeTab *typeTab );
void javaReadSymbolsFromSourceFileNoFreeing(char *fname, char *asfname);
void javaReadSymbolsFromSourceFile(char *fname);
int javaLinkNameIsAnnonymousClass(char *linkname);
int javaLinkNameIsANestedClass(char *cname);
int isANestedClass(S_symbol *ss);
void addSuperMethodCxReferences(int classIndex, S_position *pos);
S_reference * addUselessFQTReference(int classIndex, S_position *pos);
S_reference *addUnimportedTypeLongReference(int classIndex, S_position *pos);
void addThisCxReferences(int classIndex, S_position *pos);
void javaLoadClassSymbolsFromFile(S_symbol *memb);
S_symbol *javaPrependDirectEnclosingInstanceArgument(S_symbol *args);
void addMethodCxReferences(unsigned modif, S_symbol *method, S_symbol *clas);
S_symbol *javaMethodHeader(unsigned modif, S_symbol *type, S_symbol *decl, int storage);
void javaAddMethodParametersToSymTable(S_symbol *method);
void javaMethodBodyBeginning(S_symbol *method);
void javaMethodBodyEnding(S_position *pos);
void javaAddMapedTypeName(
                            char *file,
                            char *path,
                            char *pack,
                            S_completions *c,
                            void *vdirid,
                            int  *storage
                        );
S_symbol *javaFQTypeSymbolDefinition(char *name, char *fqName);
S_symbol *javaFQTypeSymbol(char *name, char *fqName);
S_typeModifiers *javaClassNameType(S_idIdentList *typeName);
S_typeModifiers *javaNewAfterName(S_idIdentList *name, S_idIdent *id, S_idIdentList *idl);
int javaIsInnerAndCanGetUnnamedEnclosingInstance(S_symbol *name, S_symbol **outEi);
S_typeModifiers *javaNestedNewType(S_symbol *expr, S_idIdent *thenew, S_idIdentList *idl);
S_typeModifiers *javaArrayFieldAccess(S_idIdent *id);
S_typeModifiers *javaMethodInvocationN(
                                S_idIdentList *name,
                                S_typeModifiersList *args
                                );
S_typeModifiers *javaMethodInvocationT(	S_typeModifiers *tt,
                                        S_idIdent *name,
                                        S_typeModifiersList *args
                                    );
S_typeModifiers *javaMethodInvocationS(	S_idIdent *super,
                                                S_idIdent *name,
                                                S_typeModifiersList *args
                                    );
S_typeModifiers *javaConstructorInvocation(S_symbol *class,
                                           S_position *pos,
                                           S_typeModifiersList *args
    );
S_extRecFindStr *javaCrErfsForMethodInvocationN(S_idIdentList *name);
S_extRecFindStr *javaCrErfsForMethodInvocationT(S_typeModifiers *tt,S_idIdent *name);
S_extRecFindStr *javaCrErfsForMethodInvocationS(S_idIdent *super,S_idIdent *name);
S_extRecFindStr *javaCrErfsForConstructorInvocation(S_symbol *clas, S_position *pos);
int javaClassIsInCurrentPackage(S_symbol *cl);
int javaFqtNamesAreFromTheSamePackage(char *classFqName, char *fqname2);
int javaMethodApplicability(S_symbol *memb, char *actArgs);
S_symbol *javaGetSuperClass(S_symbol *cc);
S_symbol *javaCurrentSuperClass();
S_typeModifiers *javaCheckNumeric(S_typeModifiers *tt);
S_typeModifiers *javaNumericPromotion(S_typeModifiers *tt);
S_typeModifiers *javaBinaryNumericPromotion(	S_typeModifiers *t1,
                                                S_typeModifiers *t2
                                            );
S_typeModifiers *javaBitwiseLogicalPromotion(	S_typeModifiers *t1,
                                                S_typeModifiers *t2
                                            );
S_typeModifiers *javaConditionalPromotion(	S_typeModifiers *t1,
                                            S_typeModifiers *t2
                                        );
int javaIsStringType(S_typeModifiers *tt);
void javaTypeDump(S_typeModifiers *tt);
void javaAddJslReadedTopLevelClasses(S_jslTypeTab  *typeTab);
struct freeTrail * newAnonClassDefinitionBegin(S_idIdent *interfName);
void javaAddSuperNestedClassToSymbolTab( S_symbol *cc);
struct freeTrail * newClassDefinitionBegin(S_idIdent *name, int accessFlags, S_symbol *anonInterf);
void newClassDefinitionEnd(S_freeTrail *trail);
void javaInitArrayObject();
void javaParsedSuperClass(S_symbol *s);
void javaSetClassSourceInformation(char *package, S_idIdent *cl);
void javaCheckIfPackageDirectoryIsInClassOrSourcePath(char *dir);

/* ***********************************************************************
** cccsemact.c
*/

S_typeModifiers *cccFunApplication(S_typeModifiers *fun,
                                  S_typeModifiersList *args);

/* ***********************************************************************
** zip.c
*/

//& extern voidpf zlibAlloc OF((voidpf opaque, uInt items, uInt size));
//& extern void   zlibFree  OF((voidpf opaque, voidpf address));

/* ***********************************************************************
** cfread.c
*/

void javaHumanizeLinkName( char *inn, char *outn, int size);
S_symbol *cfAddCastsToModule(S_symbol *memb, S_symbol *sup);
void addSuperClassOrInterface( S_symbol *memb, S_symbol *supp, int origin );
int javaCreateClassFileItem( S_symbol *memb);
void addSuperClassOrInterfaceByName(S_symbol *memb, char *super, int origin, int loadSuper);
void fsRecMapOnFiles(S_zipArchiveDir *dir, char *zip, char *path, void (*fun)(char *zip, char *file, void *arg), void *arg);
int fsIsMember(S_zipArchiveDir **dir, char *fn, unsigned offset,
                        int addFlag, S_zipArchiveDir **place);
int zipIndexArchive(char *name);
int zipFindFile(char *name, char **resName, S_zipFileTabItem *zipfile);
void javaMapZipDirFile(
        S_zipFileTabItem *zipfile,
        char *packfile,
        S_completions *a1,
        void *a2,
        int *a3,
        void (*fun)(MAP_FUN_PROFILE),
        char *classPath,
        char *dirname
    );
void javaReadClassFile(char *name, S_symbol *cdef, int loadSuper);

/* ***********************************************************************
** cgram.y
*/
int cyyparse();
void makeCCompletions(char *s, int len, S_position *pos);

/* ***********************************************************************
** javaslgram.y
*/
int javaslyyparse();

/* ***********************************************************************
** javagram.y
*/
int javayyparse();
void makeJavaCompletions(char *s, int len, S_position *pos);

/* ***********************************************************************
** cppgram.y
*/
int cccyyparse();
void makeCccCompletions(char *s, int len, S_position *pos);

/* ***********************************************************************
** yaccgram.y
*/
int yaccyyparse();
void makeYaccCompletions(char *s, int len, S_position *pos);

/* ***********************************************************************
** cexp.y
*/

int cexpyyparse();

/* ***********************************************************************
** lex.c
*/
void charBuffClose(struct charBuf *bb);
int getCharBuf(struct charBuf *bb);
void switchToZippedCharBuff(struct charBuf *bb);
int skipNCharsInCharBuf(struct charBuf *bb, unsigned count);
int getLexBuf(struct lexBuf *lb);
void gotOnLineCxRefs( S_position *ps );

/* ***********************************************************************
** yylex.c
*/
void ppMemInit();
void initAllInputs();
void initInput(FILE *ff, S_editorBuffer *buffer, char *prepend, char *name);
void addIncludeReference(int filenum, S_position *pos);
void addThisFileDefineIncludeReference(int filenum);
void pushNewInclude(FILE *f, S_editorBuffer *buff, char *name, char *prepend);
void popInclude();
void copyDir(char *dest, char *s, int *i);
char *normalizeFileName(char *name, char *relativeto);
int addFileTabItem(char *name, int *fileNumber);
void getOrCreateFileInfo(char *ss, int *fileNumber, char **fileName);
void setOpenFileInfo(char *ss);
void addMacroDefinedByOption(char *opt);
char *placeIdent();
int yylex();
int cachedInputPass(int cpoint, char **cfromto);
int cexpyylex();

extern char *yytext;

/* ***********************************************************************
** cexp.c
*/

void cccLexBackTrack(int lbi);
int cccylex();
int cccGetLexPosition();
extern int s_cccYylexBufi;

/* ***********************************************************************
** cexp.c
*/
int cexpTranslateToken(int tok, int val);

/* ***********************************************************************
** caching.c
*/
int testFileModifTime(int ii);
void initCaching();
void recoverCachePoint(int i, char *readedUntil, int activeCaching);
void recoverFromCache();
void cacheInput();
void cacheInclude(int fileNum);
void poseCachePoint(int inputCacheFlag);
void recoverCxMemory(char *cxMemFreeBase);
void recoverCachePointZero();
void recoverMemoriesAfterOverflow(char *cxMemFreeBase);

/* ***********************************************************************
** options.c
*/
void addSourcePathesCut();
void getXrefrcFileName( char *ttt );
int processInteractiveFlagOption(char **argv, int i);
char *getJavaHome();
char *getJdkClassPathFastly();
void getJavaClassAndSourcePath();
int packageOnCommandLine(char *fn);
void getStandardOptions(int *nargc, char ***nargv);
char *expandSpecialFilePredefinedVariables_st(char *tt);
int readOptionFromFile(FILE *ff, int *nargc, char ***nargv,
                        int memFl, char *sectionFile, char *project, char *resSection);
void readOptionFile(char *name, int *nargc, char ***nargv,char *sectionFile, char *project);
void readOptionPipe(char *command, int *nargc, char ***nargv,char *sectionFile);
void javaSetSourcePath(int defaultCpAllowed);
void getOptionsFromMessage(char *qnxMsgBuff, int *nargc, char ***nargv);
int changeRefNumOption(int newRefNum);

/* ***********************************************************************
** init.c
*/

void initCwd();
void reInitCwd(char *dffname, char *dffsect);
void initTokenNameTab();
void initJavaTypePCTIConvertIniTab();
void initTypeCharCodeTab();
void initArchaicTypes();
void initPreCreatedTypes();
void initTypeModifiersTabs();
void initExtractStoragesNameTab();
void initTypesNamesTab();

/* ***********************************************************************
** version.c
*/

int versionNumber();

/* ***********************************************************************
** html.c
*/

void genClassHierarchyItemLinks(FILE *ff, S_olSymbolsMenu *itt,
                                        int virtFlag);
void htmlGenNonVirtualGlobSymList(FILE *ff, char *fn, S_symbolRefItem *p );
void htmlGenGlobRefsForVirtMethod(FILE *ff, char *fn,
                                                 S_olSymbolsMenu *rrr);
int htmlRefItemsOrderLess(S_olSymbolsMenu *ss1, S_olSymbolsMenu *ss2);
int isThereSomethingPrintable(S_olSymbolsMenu *itt);
void htmlGenEmptyRefsFile();
void javaGetClassNameFromFileNum(int nn, char *tmpOut, int dotify);
void javaSlashifyDotName(char *ss);
void javaDotifyClassName(char *ss);
void javaDotifyFileName( char *ss);
int isAbsolutePath(char *p);
char *htmlNormalizedPath(char *p);
char *htmlGetLinkFileNameStatic(char *link, char *file);
void recursivelyCreateFileDirIfNotExists(char *fpath);
void concatPathes(char *res, int rsize, char *p1, char *p2, char *p3);
void htmlPutChar(FILE *ff, int c);
void htmlPrint(FILE *ff, char *ss);
void htmlGenGlobalReferenceLists(char *cxMemFreeBase);
void htmlAddJavaDocReference(S_symbol  *p, S_position  *pos,
                             int  vFunClass, int  vApplClass);
void generateHtml();
int addHtmlCutPath(char *ss );
void htmlGetDefinitionReferences();
void htmlAddFunctionSeparatorReference();


/* ***********************************************************************
** extract.c
*/

void actionsBeforeAfterExternalDefinition();
void extractActionOnBlockMarker();
void genInternalLabelReference(int counter, int usage);
S_symbol * addContinueBreakLabelSymbol(int labn, char *name);
void deleteContinueBreakLabelSymbol(char *name);
void genContinueBreakReference(char *name);
void genSwitchCaseFork(int lastFlag);

/* ***********************************************************************
** emacsex.c
*/

void genSafetyCheckFailAction();

/* ***********************************************************************
** jslsemact.c
*/

S_symbol *jslTypeSpecifier1(unsigned t);
S_symbol *jslTypeSpecifier2(S_typeModifiers *t);

void jslCompleteDeclarator(S_symbol *t, S_symbol *d);
S_typeModifiers *jslPrependComposedType(S_typeModifiers *d, unsigned t);
S_typeModifiers *jslAppendComposedType(S_typeModifiers **d, unsigned t);
S_symbol *jslPrependDirectEnclosingInstanceArgument(S_symbol *args);
S_symbol *jslMethodHeader(unsigned modif, S_symbol *type, S_symbol *decl, int storage, S_symbolList *throws);
S_symbol *jslTypeNameDefinition(S_idIdentList *tname);
S_symbol *jslTypeSymbolDefinition(char *ttt2, S_idIdentList *packid,
                                  int add, int order, int isSingleImportedFlag);
int jslClassifyAmbiguousTypeName(S_idIdentList *name, S_symbol **str);
void jslAddNestedClassesToJslTypeTab( S_symbol *cc, int order);
void jslAddSuperNestedClassesToJslTypeTab( S_symbol *cc);

void jslAddSuperClassOrInterfaceByName(S_symbol *memb,char *super);
void jslNewClassDefinitionBegin(S_idIdent *name,
                                int accessFlags,
                                S_symbol *anonInterf,
                                int position
    );
void jslAddDefaultConstructor(S_symbol *cl);
void jslNewClassDefinitionEnd();
void jslNewAnonClassDefinitionBegin(S_idIdent *interfName);

void jslAddSuperClassOrInterface(S_symbol *memb,S_symbol *supp);
void jslAddMapedImportTypeName(
                            char *file,
                            char *path,
                            char *pack,
                            S_completions *c,
                            void *vdirid,
                            int  *storage
                        );
void jslAddAllPackageClassesFromFileTab(S_idIdentList *pack);


extern S_jslStat *s_jsl;

/* ***********************************************************************
** main.c
*/

void dirInputFile(MAP_FUN_PROFILE);
void createOptionString(char **dest, char *text);
void xrefSetenv(char *name, char *val);
int mainHandleSetOption(int argc, char **argv, int i );
void copyOptions(S_options *dest, S_options *src);
void resetPendingSymbolMenuData();
char *presetEditServerFileDependingStatics();
void searchDefaultOptionsFile(char *file, char *ttt, char *sect);
void processOptions(int argc, char **argv, int infilesFlag);
void mainSetLanguage(char *inFileName, int *outLanguage);
void getAndProcessXrefrcOptions(char *dffname, char *dffsect, char *project);
char * getInputFile(int *fArgCount);
void getPipedOptions(int *outNargc,char ***outNargv);
void mainCallEditServerInit(int nargc, char **nargv);
void mainCallEditServer(int argc, char **argv,
                               int nargc, char **nargv,
                               int *firstPassing
);
void mainCallXref(int argc, char **argv);
void mainXref(int argc, char **argv);
void writeRelativeProgress(int progress);
void mainTaskEntryInitialisations(int argc, char **argv);
void mainOpenOutputFile(char *ofile);
void mainCloseOutputFile();


/* ***********************************************************************
** editor.c
*/

void editorInit();
int statb(char *path, struct stat  *statbuf);
int editorMarkerLess(S_editorMarker *m1, S_editorMarker *m2);
int editorMarkerLessOrEq(S_editorMarker *m1, S_editorMarker *m2);
int editorMarkerGreater(S_editorMarker *m1, S_editorMarker *m2);
int editorMarkerGreaterOrEq(S_editorMarker *m1, S_editorMarker *m2);
int editorMarkerListLess(S_editorMarkerList *l1, S_editorMarkerList *l2);
int editorRegionListLess(S_editorRegionList *l1, S_editorRegionList *l2);
S_editorBuffer *editorOpenBufferNoFileLoad(char *name, char *fileName);
S_editorBuffer *editorGetOpenedBuffer(char *name);
S_editorBuffer *editorGetOpenedAndLoadedBuffer(char *name);
S_editorBuffer *editorFindFile(char *name);
S_editorBuffer *editorFindFileCreate(char *name);
S_editorMarker *editorCrNewMarkerForPosition(S_position *pos);
S_editorMarkerList *editorReferencesToMarkers(S_reference *refs, int (*filter)(S_reference *, void *), void *filterParam);
S_reference *editorMarkersToReferences(S_editorMarkerList **mms);
void editorRenameBuffer(S_editorBuffer *buff, char *newName, S_editorUndo **undo);
void editorReplaceString(S_editorBuffer *buff, int position, int delsize, char *str, int strlength, S_editorUndo **undo);
void editorMoveBlock(S_editorMarker *dest, S_editorMarker *src, int size, S_editorUndo **undo);
void editorDumpBuffer(S_editorBuffer *buff);
void editorDumpBuffers();
void editorDumpMarker(S_editorMarker *mm);
void editorDumpMarkerList(S_editorMarkerList *mml);
void editorDumpRegionList(S_editorRegionList *mml);
void editorQuasySaveModifiedBuffers();
void editorLoadAllOpenedBufferFiles();
S_editorMarker *editorCrNewMarker(S_editorBuffer *buff, int offset);
S_editorMarker *editorDuplicateMarker(S_editorMarker *mm);
int editorCountLinesBetweenMarkers(S_editorMarker *m1, S_editorMarker *m2);
int editorRunWithMarkerUntil(S_editorMarker *m, int (*until)(int), int step);
int editorMoveMarkerToNewline(S_editorMarker *m, int direction);
int editorMoveMarkerToNonBlank(S_editorMarker *m, int direction);
int editorMoveMarkerBeyondIdentifier(S_editorMarker *m, int direction);
int editorMoveMarkerToNonBlankOrNewline(S_editorMarker *m, int direction);
void editorRemoveBlanks(S_editorMarker *mm, int direction, S_editorUndo **undo);
void editorDumpUndoList(S_editorUndo *uu);
void editorMoveMarkerToLineCol(S_editorMarker *m, int line, int col);
void editorMarkersDifferences(
    S_editorMarkerList **list1, S_editorMarkerList **list2,
    S_editorMarkerList **diff1, S_editorMarkerList **diff2);
void editorFreeMarker(S_editorMarker *marker);
void editorFreeMarkerListNotMarkers(S_editorMarkerList *occs);
void editorFreeMarkersAndRegionList(S_editorRegionList *occs);
void editorFreeRegionListNotMarkers(S_editorRegionList *occs);
void editorSortRegionsAndRemoveOverlaps(S_editorRegionList **regions);
void editorSplitMarkersWithRespectToRegions(
    S_editorMarkerList  **inMarkers,
    S_editorRegionList  **inRegions,
    S_editorMarkerList  **outInsiders,
    S_editorMarkerList  **outOutsiders
    );
void editorRestrictMarkersToRegions(S_editorMarkerList **mm, S_editorRegionList **regions);
S_editorMarker *editorCrMarkerForBufferBegin(S_editorBuffer *buffer);
S_editorMarker *editorCrMarkerForBufferEnd(S_editorBuffer *buffer);
S_editorRegionList *editorWholeBufferRegion(S_editorBuffer *buffer);
void editorScheduleModifiedBuffersToUpdate();
void editorFreeMarkersAndMarkerList(S_editorMarkerList *occs);
int editorMapOnNonexistantFiles(
        char *dirname,
        void (*fun)(MAP_FUN_PROFILE),
        int deep,
        char *a1,
        char *a2,
        S_completions *a3,
        void *a4,
        int *a5
    );
void editorCloseBufferIfClosable(char *name);
void editorCloseAllBuffersIfClosable();
void editorCloseAllBuffers();
void editorTest();


/* ***********************************************************************
** refactory.c
*/

void refactoryAskForReallyContinueConfirmation();
void refactoryDisplayResolutionDialog(char *message,int messageType, int continuation);
void editorApplyUndos(S_editorUndo *undos, S_editorUndo *until, S_editorUndo **undoundo, int gen);
void editorUndoUntil(S_editorUndo *until, S_editorUndo **undoUndo);
void mainRefactory(int argc, char **argv);

/* ***********************************************************************
** globals.c
*/

extern char *s_debugSurveyPointer;
extern int s_fileAbortionEnabled;

#ifdef OLCX_MEMORY_CHECK
extern void *s_olcx_chech_array[OLCX_CHECK_ARRAY_SIZE];
extern int s_olcx_chech_array_sizes[OLCX_CHECK_ARRAY_SIZE];
extern int s_olcx_chech_arrayi;
#endif
extern int s_wildCharSearch;
extern int s_lastReturnedLexem;
extern S_position s_spp[SPP_MAX];
extern S_usageBits s_noUsage;

extern int s_progressFactor;
extern int s_progressOffset;

extern S_availableRefactoring s_availableRefactorings[MAX_AVAILABLE_REFACTORINGS];

extern S_editorUndo *s_editorUndo;
extern z_stream s_defaultZStream;

extern time_t s_expTime;

extern int s_cxResizingBlocked;
extern S_counters s_count;
extern unsigned s_recFindCl;

extern FILE *errOut;
extern FILE *dumpOut;
extern char tmpBuff[TMP_BUFF_SIZE];

extern char s_olSymbolType[COMPLETION_STRING_SIZE];
extern char s_olSymbolClassType[COMPLETION_STRING_SIZE];
extern S_position s_paramPosition;
extern S_position s_paramBeginPosition;
extern S_position s_paramEndPosition;
extern S_position s_primaryStartPosition;
extern S_position s_staticPrefixStartPosition;
extern S_idIdent s_yyIdentBuf[YYBUFFERED_ID_INDEX];
extern int s_yyIdentBufi;

extern char *s_cppVarArgsName;
extern char s_defaultClassPath[];
extern S_idIdent s_javaAnonymousClassName;
extern S_idIdent s_javaConstructorName;
extern S_stringList *s_javaClassPaths;
extern char *s_javaSourcePaths;
extern S_idIdentList *s_javaDefaultPackageName;
extern S_idIdentList *s_javaLangName;
extern S_idIdentList *s_javaLangStringName;
extern S_idIdentList *s_javaLangCloneableName;
extern S_idIdentList *s_javaIoSerializableName;
extern S_idIdentList *s_javaLangClassName;
extern S_idIdentList *s_javaLangObjectName;
extern char *s_javaLangObjectLinkName;

extern S_symbol s_javaArrayObjectSymbol;
extern S_symbol *s_javaStringSymbol;
extern S_symbol *s_javaObjectSymbol;
extern S_symbol *s_javaCloneableSymbol;
extern S_symbol *s_javaIoSerializableSymbol;
extern S_typeModifiers s_javaStringModifier;
extern S_typeModifiers s_javaClassModifier;
extern S_typeModifiers s_javaObjectModifier;

extern S_caching s_cache;

extern FILE *cxOut;
extern FILE *ccOut;

extern int s_javaPreScanOnly;

extern S_currentlyParsedCl s_cp;
extern S_currentlyParsedCl s_cpInit;
extern S_currentlyParsedStatics s_cps;
extern S_currentlyParsedStatics s_cpsInit;
extern S_topBlock *s_topBlock;

extern S_symTab *s_symTab;
extern S_javaFqtTab s_javaFqtTab;
extern S_idTab s_fileTab;

extern S_typeModifiers s_defaultIntModifier;
extern S_symbol s_defaultIntDefinition;
extern S_typeModifiers s_defaultPackedTypeModifier;
extern S_typeModifiers s_defaultVoidModifier;
extern S_symbol s_defaultVoidDefinition;
extern S_typeModifiers s_errorModifier;
extern S_symbol s_errorSymbol;
extern struct stat s_noStat;
extern S_position s_noPos;
extern S_reference s_noRef;

extern uchar typeLongChange[MAX_TYPE];
extern uchar typeShortChange[MAX_TYPE];
extern uchar typeSignedChange[MAX_TYPE];
extern uchar typeUnsignedChange[MAX_TYPE];

extern S_typeModifiers *s_structRecordCompletionType;
extern S_typeModifiers *s_upLevelFunctionCompletionType;
extern S_exprTokenType s_forCompletionType;
extern S_typeModifiers *s_javaCompletionLastPrimary;
extern char *s_tokenName[];
extern int s_tokenLength[];
extern S_tokenNameIni s_tokenNameIniTab[];
extern S_tokenNameIni s_tokenNameIniTab2[];
extern S_tokenNameIni s_tokenNameIniTab3[];
extern int s_preCrTypesIniTab[];
extern S_typeModifiers * s_preCrTypesTab[MAX_TYPE];
extern S_typeModifiers * s_preCrPtr1TypesTab[MAX_TYPE];
extern S_typeModifiers * s_preCrPtr2TypesTab[MAX_TYPE];

extern char s_javaBaseTypeCharCodes[MAX_TYPE];
extern int s_javaCharCodeBaseTypes[MAX_CHARS];
extern char s_javaTypePCTIConvert[MAX_TYPE];
extern S_javaTypePCTIConvertIni s_javaTypePCTIConvertIniTab[];
extern char s_javaPrimitiveWideningConversions[MAX_PCTIndex-1][MAX_PCTIndex-1];

extern S_typeCharCodeIni s_baseTypeCharCodesIniTab[];
extern S_typeModificationsInit s_typeModificationsInit[];

extern S_position s_olcxByPassPos;
extern S_position s_cxRefPos;
extern int s_cxRefFlag;

extern S_refTab s_cxrefTab;				/* symbols cross references */

extern FILE *fIn;

extern int s_input_file_number;
extern int s_olStringSecondProcessing;  /* am I in macro body pass ? */
extern int s_olOriginalFileNumber;      /* original file name */
extern int s_olOriginalComFileNumber;	/* original communication file */
extern int s_noneFileIndex;

extern S_intStringTab s_typesNamesInitTab[];
extern char *s_extractStorageName[MAX_STORAGE];
extern S_intStringTab s_extractStoragesNamesInitTab[];

extern S_fileDesc cFile;

extern char *s_editCommunicationString;

extern time_t s_expiration;

extern S_memory *cxMemory;

extern char tmpMemory[SIZE_TMP_MEM];
extern char memory[SIZE_workMemory];
extern char tmpWorkMemory[SIZE_tmpWorkMemory];
extern int tmpWorkMemoryi;
extern char ppmMemory[SIZE_ppmMemory];
extern int ppmMemoryi;
extern char ftMemory[SIZE_ftMemory];
extern int ftMemoryi;

#ifdef OLD_RLM_MEMORY
extern void *olcxMemoryFreeList[MAX_BUFFERED_SIZE_olcxMemory];
extern char olcxMemory[SIZE_olcxMemory];
extern int olcxMemoryi;
#else
extern int olcxMemoryAllocatedBytes;
#endif


extern jmp_buf cxmemOverflow;
extern jmp_buf s_memoryResize;

extern char *s_input_file_name;

extern time_t s_fileProcessStartTime;

extern int s_language;
extern int s_currCppPass;
extern int s_maximalCppPass;

extern S_completions s_completions;
//&extern S_olSymbolFoundInformation s_oli;
extern S_userOlcx *s_olcxCurrentUser;
extern unsigned s_menuFilterOoBits[MAX_MENU_FILTER_LEVEL];
extern int s_refListFilters[MAX_REF_LIST_FILTER_LEVEL];

/* ********* vars for on-line additions after EOF ****** */

extern char s_olstring[MAX_FUN_NAME_SIZE];
extern int s_olstringFound;
extern int s_olstringServed;
extern int s_olstringUsage;
extern char *s_olstringInMbody;
extern int s_olMacro2PassFile;

/* **************** variables due to cpp **************** */

extern char mbMemory[SIZE_mbMemory];
extern int mbMemoryi;

extern struct fileDesc inStack[INSTACK_SIZE];
extern int inStacki;

extern struct lexInput macStack[MACSTACK_SIZE];
extern int macStacki;

extern S_lexInput cInput;
extern int s_ifEvaluation;		/* flag for yylex, to not filter '\n' */

extern S_options s_opt;			// current options
extern S_options s_ropt;		// xref -refactory command line options
extern S_options s_cachedOptions;
extern int s_javaRequiredeAccessibilitiesTable[MAX_REQUIRED_ACCESS+1];
extern S_options s_initOpt;

extern S_javaStat *s_javaStat;
extern S_javaStat s_initJavaStat;

extern char *s_javaThisPackageName;

extern S_zipFileTabItem s_zipArchivTab[MAX_JAVA_ZIP_ARCHIVES];

/* ******************* options ************************** */

#endif	/* ifndef _PROTO__H*/
