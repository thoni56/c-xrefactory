#ifndef _PROTO__H
#define _PROTO__H

#include "stdinc.h"
#include "strTdef.h"
#include "head.h"
#include "zlib.h" 		/*SBD*/
#include "lexmac.h" 	/*SBD*/

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
	OLO_SYNTAX_PASS_ONLY,	 /* should replace OLO_GET_PRIMARY_START && OLO_GET_PARAM_COORDINATES */
	OLO_GET_PRIMARY_START,	 /* get start position of primary expression */
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

struct fileStat {
	char			validFlag;
	struct stat		stat;
};

/* class cast tree */
struct cctNode {
	S_symbol	*node;
	S_cctNode	*sub;       /* sub[CCT_TREE_INDEX]; */
};

struct position {
	int			file;
	int 		line;
	int			coll;
};

struct positionLst {
	S_position		p;
	S_positionLst 	*next;
};

/* return value for IDENTIFIER token from yylex */

struct idIdent {
	char 		*name;
	S_symbol	*sd;		/* if yet in symbol table */
	S_position	p;			/* position */
};

struct freeTrail {
	void			(*action)C_ARG((void*));
	void 			*p;
	S_freeTrail		*next;
};

struct topBlock {
/*	char 			*stackMemoryBase;*/
	int 			firstFreeIndex;
	int				tmpMemoryBasei;
	S_freeTrail		*trail;
	S_topBlock 		*previousTopBlock;
};

struct typeModifiers {
	short 					m;
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
	S_typeModifiers 		*next;
};

struct typeModifiersList {
	S_typeModifiers		*d;
	S_typeModifiersList	*next;
};

struct recFindStr {
	S_symbol			*baseClass;	/* class, application on which is looked*/
	S_symbol			*currClass;	/* current class, NULL for loc vars. */
	S_symbol 			*nextRecord;
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
	short unsigned 	accFlags;
};

struct symStructSpecific {
	S_symbolList	*super;			/* list of super classes & interfaces */
	S_symbol		*records;		/* str. records, should be a table of 	*/
	S_cctNode		casts;			/* possible casts 						*/
	short int		nnested;		/* # of java nested classes 	*/
	S_nestedSpec	*nest;			/* array of nested classes		*/
	S_typeModifiers	stype;			/* this structure type */
	S_typeModifiers	sptrtype;		/* this structure pointer type */
	char	 		currPackage;	/* am I in the currently processed package?, to be removed */
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
		S_macroBody			*mbody; 	/* if symType == Macro ! can be NULL!*/
		int					labn;		/* break/continue label index */
		int					keyWordVal; /* if symType == Keyword*/
	} u;
	S_symbol 				*next;	/* next table item with the same hash */
};

struct symbolList {
	S_symbol 		*d;
	S_symbolList	*next;
};

struct jslSymbolList {
	S_symbol 		*d;
	S_position		pos;
	int 			isSingleImportedFlag;
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
	S_reference 			*next;
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
	S_symbolRefItem 			*next;
};

struct symbolRefItemList {
	S_symbolRefItem			*d;
	S_symbolRefItemList		*next;
};

/* ************* class hierarchy  cross referencing ************** */

struct chReference {
	int		ofile;		/* file of origin */
	int 	clas;		/* index of super-class */
	S_chReference	*next;
};


/* **************** processing cxref file ************************* */

struct cxScanFileFunctionLink {
	int		recordCode;
	void 	(*handleFun)C_ARG((int size,int ri,char**ccc,char**ffin,S_charBuf*bbb, int additionalArg));
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
	S_symbolRefItem 	s;
	char				selected;
	char				visible;
	unsigned			ooBits;
	char				olUsage;	/* usage of symbol under cursor */
	short int			vlevel;		/* virt. level of applClass <-> olsymbol*/
	short int			refn;
	short int			defRefn;
	char				defUsage;   /* usage of definition reference */
	S_position 			defpos;
	int 				outOnLine;
	S_editorMarkerList	*markers;	/* for refactory only */
	S_olSymbolsMenu		*next;
};

// if you add something to this structure, update olcxMoveTopFromAnotherUser()
// !!!!!
struct olcxReferences {
	S_reference 		*r;			/* list of references */
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
	char 					*name;
	S_olcxReferencesStack	browserStack;
	S_olcxReferencesStack	completionsStack;
	S_olcxReferencesStack	retrieverStack;
	S_classTreeData			ct;
	S_userOlcx				*next;
};

/* ************************************************************* */
/* **********************  file tab Item *********************** */

struct fileItem {	/* to be renamed to constant pool item */
	char 				*name;
	time_t				lastModif;
	time_t				lastInspect;
	time_t				lastUpdateMtime;
	time_t				lastFullUpdateMtime;
	struct fileItemBits {
		unsigned		cxLoading: 1;
		unsigned		cxLoaded : 1;
		unsigned		cxSaved	 : 1;
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
	char 			*s;
	S_symbol 		*t;
	short int		symType;
	short int		virtLevel;
//	unsigned		virtClassOrder;		TODO !!!!
	short int		margn;
	char			**margs;
	S_symbol		*vFunClass;
};

struct completions {
	char 		idToProcess[MAX_FUN_NAME_SIZE];
	int			idToProcessLen;
	S_position	idToProcessPos;
	int 		fullMatchFlag;
	int 		isCompleteFlag;
	int 		noFocusOnCompletions;
	int 		abortFurtherCompletions;
	char 		comPrefix[TMP_STRING_SIZE];
	int			maxLen;
	S_cline 	a[MAX_COMPLETIONS];
	int 		ai;
};

struct completionFunTab {
	int token;
	void (*fun)C_ARG((S_completions*));
};

struct completionSymFunInfo {
	S_completions 		*res;
	unsigned			storage;
};

struct completionSymInfo {
	S_completions 		*res;
	unsigned			symType;
};

struct completionFqtMapInfo {
	S_completions 		*res;
	int					completionType;
};

/* ************************** INIT STRUCTURES ********************* */

struct tokenNameIni {
	char 		*name;
	int 		token;
	unsigned 	languages;
};

struct typeCharCodeIni {
	int 		symType;
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
	int 	i;
	char 	*s;
};

struct currentlyParsedCl {		// class local, nested for classes
	S_symbol			*function;
	S_extRecFindStr		*erfsForParamsComplet;			// curently parsed method for param completion 
	unsigned	funBegPosition;
	int 		cxMemiAtFunBegin;
	int			cxMemiAtFunEnd;
	int 		cxMemiAtClassBegin;
	int			cxMemiAtClassEnd;
	int			thisMethodMemoriesStored;
	int			thisClassMemoriesStored;
	int			parserPassedMarker;
};

struct currentlyParsedStatics {
	int 			extractProcessedFlag;
	int 			cxMemiAtBlockBegin;
	int 			cxMemiAtBlockEnd;
	S_topBlock 		*workMemiAtBlockBegin;
	S_topBlock 		*workMemiAtBlockEnd;
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
	S_idIdentList 		*lastParsedName;
	unsigned			cpMethodMods;		/* currently parsed method modifs */
	S_currentlyParsedCl	cp;					/* some parsing positions */
	int					classFileInd;		/* this file class index */
	S_javaStat			*next;				/* outer class */
};

/* java composed names */

struct idIdentList {
	S_idIdent 		idi;
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
	char 				name[1];			/* array of char */
};

struct zipFileTabItem {
	char 			fn[MAX_FILE_NAME_SIZE];	/* stored with ';' at the end */
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
	S_topBlock 				starTopBlock;
	int 					ppmMemoryi;
	int 					cxMemoryi;
	int 					mbMemoryi;
	char					*lbcc;		/* caching lbcc */
	short int				ibi;		/* caching ibi */
	short int 				lineNumber;
	short int 				ifDeep;
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
	char 		*cc;				/* first unread */
	char 		*fin;				/* first free (invalid)  */
	char 		a[CHAR_BUFF_SIZE];
	FILE 		*ff;
	unsigned	filePos;			/* how many chars was readed from file */
	int			fileNumber;
	int 		lineNum;
	char 		*lineBegin;
	short int 	collumnOffset;		/* collumn == cc-lineBegin + collumnOffset */
	char		isAtEOF;
	char		inputMethod;		/* unzipp/direct */
	char 		z[CHAR_BUFF_SIZE];  /* zip input buffer */
	z_stream	zipStream;
};

struct lexBuf {
	char 		*cc;				/* first unread */
	char 		*fin;				/* first free (invalid)  */
	char 		a[LEX_BUFF_SIZE];
#if 1
	S_position 	pRing[LEX_POSITIONS_RING_SIZE];		// file/line/coll position
	unsigned 	fpRing[LEX_POSITIONS_RING_SIZE];	// file offset position
	int 		posi;				/* pRing[posi%LEX_POSITIONS_RING_SIZE] */
#endif
	S_charBuf 	cb;
};

struct cppIfStack {
	S_position pos;
	S_cppIfStack  *next;
};

struct fileDesc {
	char 			*fileName ;
	int 			lineNumber ;
	int				ifDeep;						/* deep of #ifs (C only)*/
	S_cppIfStack 	*ifstack;					/* #if stack (C only) */
	struct lexBuf 	lb;
};

struct lexInput {
	char 			*cc;                /* pointer to current lexem */
	char 			*fin;               /* end of buffer */
	char 			*a;					/* beginning of buffer */
	char 			*macname;			/* possible makro name */
	char			margExpFlag;		/* input Flag */
};

/* ********************* MEMORIES *************************** */

struct memory {
	int		(*overflowHandler)(int n);
	int 	i;
	int		size;
	double	b;		//  double in order to get it properly alligned
};

/* ************************ HTML **************************** */

struct intlist {
	int 		i;
	S_intlist 	*next;
};

struct disabledList {
	int 			file;
	int 			clas;
	S_disabledList 	*next;
};

struct htmlData {
	S_position 			*cp;
	S_reference 		*np;
	S_symbolRefItem 	*nri;
};

struct htmlRefList {
	S_symbolRefItem	*s;
	S_reference		*r;
	S_symbolRefItem	*slist;		/* the hash list containing s, for virtuals */
	S_htmlRefList	*next;
};

struct htmlLocalListms {	// local xlist map structure
	FILE 	*ff;
	int  	fnum;
	char	*fname;
};

/* *********************************************************** */

struct programGraphNode {
	S_reference 			*ref;		/* original reference of node */
	S_symbolRefItem 		*symRef;
	S_programGraphNode		*jump;
	char					posBits;		/* INSIDE/OUSIDE block */
	char					stateBits;		/* visited + where setted */
	char					classifBits;	/* resulting classification */
	S_programGraphNode		*next;
};

struct exprTokenType {
	S_typeModifiers *t;
	S_reference 	*r;
	S_position 		*pp;
};

struct nestedConstrTokenType {
	S_typeModifiers *t;
	S_idIdentList	*nid;
	S_position 		*pp;
};

struct unsPositionPair {
	unsigned 	u;
	S_position	*p;
};

struct symbolPositionPair {
	S_symbol	*s;
	S_position	*p;
};

struct symbolPositionLstPair {
	S_symbol		*s;
	S_positionLst	*p;
};

struct intPair {
	int i1;
	int i2;
};

struct typeModifiersListPositionLstPair {
	S_typeModifiersList		*t;
	S_positionLst			*p;
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
	S_pushAllInBetweenData 	mm;
	char					*symbolToTest;
	int						classToTest;
	S_symbolRefItem			*foundSpecialRefItem;
	S_reference				*foundSpecialR;
	S_symbolRefItem 		*foundRefToTestedClass;
	S_symbolRefItem 		*foundRefNotToTestedClass;
	S_reference 			*foundOuterScopeRef;
};

struct tpCheckMoveClassData {
	S_pushAllInBetweenData 	mm;
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
	int 					pass;
	int 					sourceFileNumber;
	int						language;
	S_jslTypeTab 			*typeTab;
	S_jslClassStat 			*classStat;
	S_symbolList 			*waitList;
	void/*YYSTYPE*/			*savedyylval;
	struct yyGlobalState 	*savedYYstate;
	int						yyStateSize;
	S_idIdent				yyIdentBuf[YYBUFFERED_ID_INDEX]; // pending idents
	S_jslStat				*next;
};

/* ***************** editor structures ********************** */

struct editorBuffer {
	char 			*name;
	int				ftnum;
	char			*fileName;
	struct stat		stat;
	S_editorMarker	*markers;
	struct editorBufferAllocationData {
		int 	bufferSize;
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
	S_editorBuffer 		*f;
	S_editorBufferList	*next;
};

struct editorMemoryBlock {
	struct editorMemoryBlock *next;
};

struct editorMarker {
	S_editorBuffer		*buffer;
	unsigned 			offset;
	struct editorMarker *previous;   	// previous marker in this buffer
	struct editorMarker *next;   		// next marker in this buffer
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
			unsigned 			size;
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
	unsigned 	available;
	char		*option;
};

/* **************     parse tree with positions    *********** */

// following structures are used in yacc parser, they are always
// containing 'b','e' records for begin and end position of parse tree
// node and additional data 'd' for parsing.

struct bb_int {
	S_position 	b, e;
	int		d;
};
struct bb_unsigned {
	S_position 		b, e;
	unsigned		d;
};
struct bb_symbol {
	S_position 		b, e;
	S_symbol		*d;
};
struct bb_symbolList {
	S_position 			b, e;
	S_symbolList		*d;
};
struct bb_typeModifiers {
	S_position 			b, e;
	S_typeModifiers		*d;
};
struct bb_typeModifiersList {
	S_position 				b, e;
	S_typeModifiersList		*d;
};
struct bb_freeTrail {
	S_position 		b, e;
	S_freeTrail		*d;
};
struct bb_idIdent {
	S_position 		b, e;
	S_idIdent		*d;
};
struct bb_idIdentList {
	S_position 			b, e;
	S_idIdentList		*d;
};
struct bb_exprTokenType {
	S_position 			b, e;
	S_exprTokenType		d;
};
struct bb_nestedConstrTokenType {
	S_position 					b, e;
	S_nestedConstrTokenType		d;
};
struct bb_intPair {
	S_position 		b, e;
	S_intPair		d;
};
struct bb_whileExtractData {
	S_position 				b, e;
	S_whileExtractData		*d;
};
struct bb_position {
	S_position 		b, e;
	S_position		d;
};
struct bb_unsPositionPair {
	S_position 				b, e;
	S_unsPositionPair		d;
};
struct bb_symbolPositionPair {
	S_position 					b, e;
	S_symbolPositionPair		d;
};
struct bb_symbolPositionLstPair {
	S_position 					b, e;
	S_symbolPositionLstPair		d;
};
struct bb_positionLst {
	S_position 			b, e;
	S_positionLst		*d;
};
struct bb_typeModifiersListPositionLstPair  {
	S_position 									b, e;
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
	char licenseString[TMP_STRING_SIZE];
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

struct optionsList {
	char 				*section;
	S_stringList		*opts;
	S_options			flgopts;
	S_optionsList		*next;
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

void emergencyExit C_ARG((int exitStatus));
extern void warning C_ARG((int kod, char *sprava));
extern void error C_ARG((int kod, char *sprava));
extern void fatalError C_ARG((int kod, char *sprava, int exitCode));
extern void internalCheckFail C_ARG((char *expr, char *file, int line));

/* ***********************************************************************
** cct.c
*/
void cctAddSimpleValue C_ARG((S_cctNode *cc, S_symbol *x, int deepFactor));
int cctIsMember C_ARG((S_cctNode *cc, S_symbol *x, int deepFactor));
void cctAddCctTree C_ARG((S_cctNode *cc, S_cctNode *x, int deepFactor));
void cctDump C_ARG((S_cctNode *cc, int deep));
void cctTest C_ARG(());


/* ***********************************************************************
** misc.c
*/

void ppcGenSynchroRecord C_ARG(());
void ppcIndentOffset C_ARG(());
void ppcGenGotoOffsetPosition C_ARG((char *fname, int offset));
void ppcGenRecordBegin C_ARG((char *kind));
void ppcGenRecordWithAttributeBegin C_ARG((char *kind, char *attr, char *val));
void ppcGenRecordWithNumAttributeBegin C_ARG((char *kind, char *attr, int val));
void ppcGenRecordEnd C_ARG((char *kind));
void ppcGenNumericRecordBegin C_ARG((char *kind, int val));
void ppcGenTwoNumericAndRecordBegin C_ARG((char *kind, char *attr1, int val1, char *attr2, int val2));
void ppcGenWithNumericAndRecordBegin C_ARG((char *kind, int val, char *attr, char *attrVal));
void ppcGenAllCompletionsRecordBegin C_ARG((int nofocus, int len));
void ppcGenTwoNumericsAndrecord C_ARG((char *kind, char *attr1, int val1, char *attr2, int val2, char *message,char *suff));
void ppcGenRecordWithNumeric C_ARG((char *kind, char *attr, int val, char *message,char *suff));
void ppcGenNumericRecord C_ARG((char *kind, int val, char *message, char *suff));
void ppcGenRecord C_ARG((char *kind, char *message, char *suffix));
void ppcGenTmpBuff C_ARG(());
void ppcGenDisplaySelectionRecord C_ARG((char *message, int messageType, int continuation));
void ppcGenGotoMarkerRecord C_ARG((S_editorMarker *pos));
void ppcGenPosition C_ARG((S_position *p));
void ppcGenDefinitionNotFoundWarning C_ARG(());
void ppcGenDefinitionNotFoundWarningAtBottom C_ARG(());
void ppcGenReplaceRecord C_ARG((char *file, int offset, char *oldName, int oldNameLen, char *newName));
void ppcGenPreCheckRecord C_ARG((S_editorMarker *pos, int oldLen));
void ppcGenReferencePreCheckRecord C_ARG((S_reference *r, char *text));
void ppcGenGotoPositionRecord C_ARG((S_position *p));
void ppcGenOffsetPosition C_ARG((char *fn, int offset));
void ppcGenMarker C_ARG((S_editorMarker *m));
char * stringNumStr C_ARG(( int rr, int ed, int em, int ey, char *own ));
void jarFileParse C_ARG(());
void scanJarFilesForTagSearch C_ARG(());
void classFileParse C_ARG(());
void fillTrivialSpecialRefItem C_ARG(( S_symbolRefItem *ddd , char *name));
void setExpirationFromLicenseString C_ARG(());
int optionsOverflowHandler C_ARG((int n));
int cxMemoryOverflowHandler C_ARG((int n));

void noSuchRecordError C_ARG((char *rec));
void methodAppliedOnNonClass C_ARG((char *rec));
void methodNameNotRecognized C_ARG((char *rec));
void dumpOptions C_ARG((int nargc, char **nargv));
void stackMemoryInit C_ARG(());
void *stackMemoryAlloc C_ARG((int size));
void *stackMemoryPush C_ARG((void *p, int size));
int  *stackMemoryPushInt C_ARG((int x));
char *stackMemoryPushString C_ARG((char *s));
void stackMemoryPop C_ARG((void *, int size));
void stackMemoryBlockStart C_ARG(());
void stackMemoryBlockFree C_ARG(());
void stackMemoryStartErrZone C_ARG(());
void stackMemoryStopErrZone C_ARG(());
void stackMemoryErrorInZone C_ARG(());
void stackMemoryDump C_ARG(());

void addToTrail  C_ARG((void (*action)(void*),  void *p));
void removeFromTrailUntil C_ARG((S_freeTrail *untilP));

void symDump C_ARG((S_symbol *));
void symbolRefItemDump C_ARG((S_symbolRefItem *ss));
int javaTypeStringSPrint C_ARG((char *buff, char *str, int nameStyle, int *oNamePos));
void typeSPrint C_ARG((char *buff,int *size,S_typeModifiers *t,char*name, 
					   int dclSepChar, int maxDeep, int typedefexp, 
					   int longOrShortName, int *oNamePos));
void throwsSprintf C_ARG((char *out, int outsize, S_symbolList *exceptions));
void macDefSPrintf C_ARG((char *buff, int *size, char *name1, char *name2, int argn, char **args, int *oNamePos));
char * string3ConcatInStackMem C_ARG((char *str1, char *str2, char *str3));

unsigned hashFun C_ARG((char *s));
void javaSignatureSPrint C_ARG((char *buff, int *size, char *sig, int classstyle));
void fPutDecimal C_ARG((FILE *f, int n));
char *strmcpy C_ARG((char *dest, char *src));
char *simpleFileName C_ARG((char *fullFileName));
char *directoryName_st C_ARG((char *fullFileName));
char *simpleFileNameWithoutSuffix_st C_ARG((char *fullFileName));
int containsWildCharacter C_ARG((char *ss));
int shellMatch C_ARG((char *string, int stringLen, char *pattern, int caseSensitive));
void expandWildCharactersInOnePathRec C_ARG((char *fn, char **outpaths, int *freeolen));
void expandWildCharactersInOnePath C_ARG((char *fn, char *outpaths, int olen));
void expandWildCharactersInPaths C_ARG((char *paths, char *outpaths, int freeolen));
char * getRealFileNameStatic C_ARG((char *fn));
char *crTmpFileName_st();
void copyFile C_ARG((char *src, char *dest));
void createDir C_ARG((char *dirname));
void removeFile C_ARG((char *dirname));
int substringIndexWithLimit C_ARG((char *s, int limit, char *subs));
int stringContainsSubstring C_ARG((char *s, char *subs));
void javaGetPackageNameFromSourceFileName C_ARG((char *src, char *opack));
int substringIndex C_ARG((char *s, char *subs));
int stringEndsBySuffix C_ARG((char *s, char *suffix));
int fileNameHasOneOfSuffixes C_ARG((char *fname, char *suffs));
int mapPatternFiles C_ARG((
		char *pattern,
		void (*fun) (MAP_FUN_PROFILE),
		char *a1,
		char *a2,
		S_completions *a3,
		void *a4,
		int *a5
	));
int mapDirectoryFiles C_ARG((
		char *dirname,
		void (*fun) (MAP_FUN_PROFILE),
		int allowEditorFilesFlag,
		char *a1,
		char *a2,
		S_completions *a3,
		void *a4,
		int *a5
	));
void javaMapDirectoryFiles1 C_ARG((
		char *packfile,
		void (*fun)(MAP_FUN_PROFILE),
		S_completions *a1,
		void *a2,
		int *a3
	));
void javaMapDirectoryFiles2 C_ARG((
		S_idIdentList *packid,
		void (*fun)(MAP_FUN_PROFILE),
		S_completions *a1,
		void *a2,
		int *a3
	));
char *lastOccurenceInString C_ARG((char *ss, int ch));
char *lastOccurenceOfSlashOrAntiSlash C_ARG((char *ss));
char * getFileSuffix C_ARG((char *fn));
char *javaCutClassPathFromFileName C_ARG((char *fname));
char *javaCutSourcePathFromFileName C_ARG((char *fname));
int pathncmp C_ARG((char *ss1, char *ss2, int n, int caseSensitive));
int fnCmp C_ARG((char *ss1, char *ss2));
int fnnCmp C_ARG((char *ss1, char *ss2, int n));

/* ***********************************************************************
** completion.c
*/
void processName C_ARG((char *name, S_cline *t, int orderFlag, void *c));
void completeForSpecial1 C_ARG((S_completions*));
void completeForSpecial2 C_ARG((S_completions*));
void completeUpFunProfile C_ARG((S_completions* c));
void completeTypes C_ARG((S_completions*));
void completeStructs C_ARG((S_completions*));
void completeRecNames C_ARG((S_completions*));
void completeEnums C_ARG((S_completions*));
void completeLabels C_ARG((S_completions*));
void completeMacros C_ARG((S_completions*c));
void completeOthers C_ARG((S_completions*));
void javaCompleteTypeSingleName C_ARG((S_completions*));
void javaHintImportFqt C_ARG((S_completions*c));
void javaHintVariableName C_ARG((S_completions*c));
void javaHintCompleteNonImportedTypes C_ARG((S_completions*c));
void javaHintCompleteMethodParameters C_ARG((S_completions *c));
void javaCompleteTypeCompName C_ARG((S_completions*));
void javaCompleteThisPackageName C_ARG((S_completions*c));
void javaCompletePackageSingleName C_ARG((S_completions*));
void javaCompleteExprSingleName C_ARG((S_completions*));
void javaCompleteUpMethodSingleName C_ARG((S_completions*));
void javaCompleteFullInheritedMethodHeader C_ARG((S_completions*c));
void javaCompletePackageCompName C_ARG((S_completions*));
void javaCompleteExprCompName C_ARG((S_completions*));
void javaCompleteMethodCompName C_ARG((S_completions*));
void javaCompleteHintForConstructSingleName C_ARG((S_completions *c));
void javaCompleteConstructSingleName C_ARG((S_completions*c));
void javaCompleteConstructCompName C_ARG((S_completions*c));
void javaCompleteConstructNestNameName C_ARG((S_completions*c));
void javaCompleteConstructNestPrimName C_ARG((S_completions*c));
void javaCompleteStrRecordPrimary C_ARG((S_completions*c));
void javaCompleteStrRecordSuper C_ARG((S_completions*c));
void javaCompleteStrRecordQualifiedSuper C_ARG((S_completions*c));
void javaCompleteClassDefinitionNameSpecial C_ARG((S_completions*c));
void javaCompleteClassDefinitionName C_ARG((S_completions*c));
void javaCompleteThisConstructor C_ARG((S_completions*c));
void javaCompleteSuperConstructor C_ARG((S_completions*c));
void javaCompleteSuperNestedConstructor C_ARG((S_completions*c));
void completeYaccLexem C_ARG((S_completions*c));
char *javaGetShortClassName C_ARG(( char *inn));
char *javaGetShortClassNameFromFileNum_st C_ARG((int fnum));
char *javaGetNudePreTypeName_st C_ARG(( char *inn, int cutMode));

void olCompletionListInit C_ARG((S_position *originalPos));
void formatOutputLine C_ARG((char *tt, int startingColumn));
void printCompletionsList C_ARG((int noFocus));
void printCompletions C_ARG((S_completions*c));

/* ***********************************************************************
** generate.c
*/

void generate C_ARG((S_symbol *s));
void genProjections C_ARG((int n));

/* ***********************************************************************
** cxref.c, cxfile.c
*/

int olcxReferenceInternalLessFunction C_ARG((S_reference *r1, S_reference *r2));
int olSymbolRefItemLess C_ARG((S_symbolRefItem *s1, S_symbolRefItem *s2));
int searchStringFitness C_ARG((char *cxtag, int slen));
char *crTagSearchLineStatic C_ARG((char *name, S_position *p, 
							int *len1, int *len2, int *len3));
int symbolNameShouldBeHiddenFromReports C_ARG((char *name));
void searchSymbolCheckReference C_ARG((S_symbolRefItem  *ss, S_reference *rr));
void tagSearchCompactShortResults();
void printTagSearchResults();
int creatingOlcxRefs();
int olcxTopSymbolExists C_ARG(());
S_olSymbolsMenu *olCreateSpecialMenuItem C_ARG((char *fieldName, int cfi,int storage));
int itIsSameCxSymbol C_ARG((S_symbolRefItem *p1, S_symbolRefItem *p2));
int itIsSameCxSymbolIncludingFunClass C_ARG((S_symbolRefItem *p1, S_symbolRefItem *p2));
int itIsSameCxSymbolIncludingApplClass C_ARG((S_symbolRefItem *p1, S_symbolRefItem *p2));
int olcxItIsSameCxSymbol C_ARG((S_symbolRefItem *p1, S_symbolRefItem *p2));
void olcxRecomputeSelRefs C_ARG(( S_olcxReferences *refs ));
void olProcessSelectedReferences C_ARG((S_olcxReferences  	*rstack, void (*referencesMapFun)(S_olcxReferences *rstack, S_olSymbolsMenu *ss)));
void olcxPopOnly C_ARG(());
S_reference * olcxCopyRefList C_ARG((S_reference *ll));
void olStackDeleteSymbol C_ARG(( S_olcxReferences *refs));
int getFileNumberFromName C_ARG((char *name));
int olcxVirtualyAdequate C_ARG((int usage, int vApplCl, int vFunCl, 
						int olUsage, int olApplCl, int olFunCl));
void generateOnlineCxref C_ARG((	S_position *p, 
							char *commandString,
							int usage,
							char *suffix,
							char *suffix2
	));
S_reference *olcxAddReferenceNoUsageCheck C_ARG((S_reference **rlist, S_reference *ref, int bestMatchFlag));
S_reference *olcxAddReference C_ARG((S_reference **rlist,S_reference *ref,int bestMatchFlag));
int isRelatedClass C_ARG((int cl1, int cl2));
void olcxFreeReferences C_ARG((S_reference *r));
int isSmallerOrEqClass C_ARG((int inf, int sup));
int olcxPushLessFunction C_ARG((S_reference *r1, S_reference *r2));
int olcxListLessFunction C_ARG((S_reference *r1, S_reference *r2));
char *getJavaDocUrl_st C_ARG((S_symbolRefItem *rr));
char *getLocalJavaDocFile_st C_ARG((char *fileUrl));
char *getFullUrlOfJavaDoc_st C_ARG((char *fileUrl));
int htmlJdkDocAvailableForUrl C_ARG((char *ss));
void setIntToZero C_ARG((void *p));
S_reference *duplicateReference C_ARG((S_reference *r));
S_reference * addCxReferenceNew C_ARG((S_symbol *p, S_position *pos, S_usageBits *ub, int vFunCl, int vApplCl));
S_reference * addCxReference C_ARG((S_symbol *p, S_position *pos, int usage, int vFunClass,int vApplClass));
S_reference *addSpecialFieldReference C_ARG((char *name, int storage, int fnum, S_position *p, int usage));
void addClassTreeHierarchyReference C_ARG((int fnum, S_position *p, int usage));
void addCfClassTreeHierarchyRef C_ARG((int fnum, int usage));
void addTrivialCxReference  C_ARG((char *name, int symType, int storage, S_position *pos, int usage));
int cxFileHashNumber C_ARG((char *sym));
void genReferenceFile C_ARG((int updateFlag, char *fname));
void addSubClassItemToFileTab C_ARG(( int sup, int inf, int origin));
void addSubClassesItemsToFileTab C_ARG((S_symbol *ss, int origin));
void scanCxFile C_ARG((S_cxScanFileFunctionLink *scanFuns));
void olcxAddReferences C_ARG((S_reference *list, S_reference **dlist, int fnum, int bestMatchFlag));
void olSetCallerPosition C_ARG((S_position *pos));
S_olCompletion * olCompletionListPrepend C_ARG((char *name, char *fullText, char *vclass, int jindent, S_symbol *s, S_symbolRefItem *ri, S_reference *dfpos, int symType, int vFunClass, S_olcxReferences *stack));
S_olSymbolsMenu *olCreateNewMenuItem C_ARG(( 
		S_symbolRefItem *sym, int vApplClass, int vFunCl, S_position *defpos, int defusage,
		int selected, int visible, 
		unsigned ooBits, int olusage, int vlevel 
	));
S_olSymbolsMenu *olAddBrowsedSymbol C_ARG((
	S_symbolRefItem *sym, S_olSymbolsMenu **list, 
	int selected, int visible, unsigned ooBits,
	int olusage, int vlevel, 
	S_position *defpos, int defusage));
void renameCollationSymbols C_ARG((S_olSymbolsMenu *sss));
void olCompletionListReverse();
S_reference **addToRefList C_ARG((	S_reference **list,
							S_usageBits *pusage,				
							S_position *pos,
							int category
						));
int isInRefList C_ARG((S_reference *list,
				S_usageBits *pusage,				
				S_position *pos,
				int category
				));
char *getXrefEnvironmentValue C_ARG(( char *name ));
int byPassAcceptableSymbol C_ARG((S_symbolRefItem *p));
int itIsSymbolToPushOlRefences C_ARG((S_symbolRefItem *p, S_olcxReferences *rstack, S_olSymbolsMenu **rss, int checkSelFlag));
void olcxAddReferenceToOlSymbolsMenu C_ARG((S_olSymbolsMenu  *cms, S_reference *rr, 
						  int bestFitTlag));
void putOnLineLoadedReferences C_ARG((S_symbolRefItem *p));
void genOnLineReferences C_ARG((	S_olcxReferences *rstack, S_olSymbolsMenu *cms));
S_olSymbolsMenu *createSelectionMenu C_ARG((S_symbolRefItem *dd));
void mapCreateSelectionMenu C_ARG((S_symbolRefItem *dd));
int olcxFreeOldCompletionItems C_ARG((S_olcxReferencesStack *stack));
void olcxInit C_ARG(());
S_userOlcx *olcxSetCurrentUser C_ARG((char *user));
void linkNamePrettyPrint C_ARG((char *ff, char *javaLinkName, int maxlen,int argsStyle ));
char *simpleFileNameFromFileNum C_ARG((int fnum));
char *getShortClassNameFromClassNum_st C_ARG((int fnum));
S_reference * getDefinitionRef C_ARG((S_reference *rr));
void printSymbolLinkNameString C_ARG(( FILE *ff, char *linkName));
int safetyCheck2ShouldWarn C_ARG(());
void olCreateSelectionMenu C_ARG((int command));
void printClassFqtNameFromClassNum C_ARG((FILE *ff, int fnum));
void sprintfSymbolLinkName C_ARG((char *ttt, S_olSymbolsMenu *ss));
void printSymbolLinkName C_ARG(( FILE *ff, S_olSymbolsMenu *ss));
void olcxPushEmptyStackItem C_ARG((S_olcxReferencesStack *stack));
void olcxPrintSelectionMenu C_ARG((S_olSymbolsMenu *));
int ooBitsGreaterOrEqual C_ARG((unsigned oo1, unsigned oo2));
void olcxPrintClassTree C_ARG((S_olSymbolsMenu *sss));
void olcxReferencesDiff C_ARG((S_reference **anr1, S_reference **aor2,S_reference **diff));
int olcxShowSelectionMenu C_ARG(());
int getClassNumFromClassLinkName C_ARG((char *name, int defaultResult));
void getLineColCursorPositionFromCommandLineOption C_ARG(( int *l, int *c ));
void changeClassReferencesUsages C_ARG((char *linkName, int category, int fnum, 
								 S_symbol *cclass));
int isStrictlyEnclosingClass C_ARG((int enclosedClass, int enclosingClass));
void changeMethodReferencesUsages C_ARG((char *linkName, int category, int fnum, S_symbol *cclass));
void olcxPushSpecialCheckMenuSym C_ARG((int command, char *symname));
int refOccursInRefs C_ARG((S_reference *r, S_reference *list));
void olcxCheck1CxFileReference C_ARG((S_symbolRefItem *ss, S_reference *r));
void olcxPushSpecial C_ARG((char *fieldName, int command));
int isPushAllMethodsValidRefItem C_ARG((S_symbolRefItem *ri));
int symbolsCorrespondWrtMoving C_ARG((S_olSymbolsMenu *osym,S_olSymbolsMenu *nsym,int command));
void olcxPrintPushingAction C_ARG((int opt, int afterMenu));
void olPushAllReferencesInBetween C_ARG((int minMemi, int maxMemi));
int tpCheckSourceIsNotInnerClass C_ARG(());
void tpCheckFillMoveClassData C_ARG((S_tpCheckMoveClassData *dd, char *spack, char *tpack));
int tpCheckMoveClassAccessibilities C_ARG(());
int tpCheckSuperMethodReferencesForDynToSt C_ARG(());
int tpCheckOuterScopeUsagesForDynToSt C_ARG(());
int tpCheckMethodReferencesWithApplOnSuperClassForPullUp C_ARG(());
int tpCheckSuperMethodReferencesForPullUp C_ARG(());
int tpCheckSuperMethodReferencesAfterPushDown C_ARG(());
int tpCheckTargetToBeDirectSubOrSupClass C_ARG((int flag, char *subOrSuper));
int tpPullUpFieldLastPreconditions C_ARG(());
int tpPushDownFieldLastPreconditions C_ARG(());
S_symbol *getMoveTargetClass C_ARG(());
int javaGetSuperClassNumFromClassNum C_ARG((int cn));
int javaIsSuperClass C_ARG((int superclas, int clas));
void pushLocalUnusedSymbolsAction C_ARG(());
void mainAnswerEditAction C_ARG(());
void freeOldestOlcx();
S_olSymbolsMenu *olcxFreeSymbolMenuItem C_ARG((S_olSymbolsMenu *ll));
void olcxFreeResolutionMenu C_ARG(( S_olSymbolsMenu *sym ));
int refCharCode C_ARG((int usage));
int scanReferenceFile C_ARG((	char *fname, char *fns1, char *fns2,
						S_cxScanFileFunctionLink *scanFunTab));
int smartReadFileTabFile();
void readOneAppropReferenceFile C_ARG((char *symname, S_cxScanFileFunctionLink  *scanFunTab));
void scanReferenceFiles C_ARG((char *fname, S_cxScanFileFunctionLink *scanFunTab));

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

void classHierarchyGenInit C_ARG(());
void setTmpClassBackPointersToMenu C_ARG((S_olSymbolsMenu *menu));
int chLineOrderLess C_ARG((S_olSymbolsMenu *r1, S_olSymbolsMenu *r2));
void splitMenuPerSymbolsAndMap C_ARG((
	S_olSymbolsMenu *rrr, 
	void (*fun)(S_olSymbolsMenu *, void *, void *),
	void *p1,
	char *p2
	));
void htmlGenGlobRefLists C_ARG((S_olSymbolsMenu *rrr, FILE *ff, char *fn));
void genClassHierarchies C_ARG(( FILE *ff, S_olSymbolsMenu *rrr, 
										int virtFlag, int pass ));
int classHierarchyClassNameLess C_ARG((int c1, int c2));
int classHierarchySupClassNameLess C_ARG((S_chReference *c1, S_chReference *c2));

/* ***********************************************************************
** semact.c
*/

void unpackPointers C_ARG((S_symbol *pp));
int displayingErrorMessages C_ARG(());
void deleteSymDef C_ARG((void *p));
void addSymbol C_ARG((S_symbol *pp, S_symTab *tab));
void recFindPush C_ARG((S_symbol *sym, S_recFindStr *rfs));
S_recFindStr * iniFind C_ARG((S_symbol *s, S_recFindStr *rfs));
int javaOuterClassAccessible C_ARG((S_symbol *cl));
int javaRecordAccessible C_ARG((S_recFindStr *rfs, S_symbol *applcl, S_symbol *funcl, S_symbol *rec, unsigned recAccessFlags));
int javaRecordVisibleAndAccessible C_ARG((S_recFindStr *rfs, S_symbol *applCl, S_symbol *funCl, S_symbol *r));
int javaGetMinimalAccessibility C_ARG((S_recFindStr *rfs, S_symbol *r));
int findStrRecordSym C_ARG((	S_recFindStr *ss,
								char *recname,
								S_symbol **res,
								int javaClassif,
								int accessCheck,
								int visibilityCheck
					));
S_symbol *addNewSymbolDef C_ARG((S_symbol *p, unsigned storage, S_symTab *tab, int usage));
S_symbol *addNewCopyOfSymbolDef C_ARG((S_symbol *def, unsigned defaultStorage));
S_symbol *addNewDeclaration C_ARG((S_symbol *btype, S_symbol *decl, 
							unsigned storage, S_symTab *tab));
int styyerror C_ARG((char *s));
int styyErrorRecovery C_ARG(());
void setToNull C_ARG((void *p));
void allocNewCurrentDefinition C_ARG(());
S_symbol *typeSpecifier1 C_ARG((unsigned t));
void declTypeSpecifier1 C_ARG((S_symbol *d, unsigned t));
S_symbol *typeSpecifier2 C_ARG((S_typeModifiers *t));
void declTypeSpecifier2 C_ARG((S_symbol *d, S_typeModifiers *t));
void declTypeSpecifier21 C_ARG((S_typeModifiers *t, S_symbol *d));
S_typeModifiers *appendComposedType C_ARG((S_typeModifiers **d, unsigned t));
S_typeModifiers *prependComposedType C_ARG((S_typeModifiers *d, unsigned t));
void completeDeclarator C_ARG((S_symbol *t, S_symbol *d));
void addFunctionParameterToSymTable C_ARG((S_symbol *function, S_symbol *p, int i, S_symTab *tab));
S_typeModifiers *crSimpleTypeMofifier  C_ARG((unsigned t));
S_symbolList *crDefinitionList C_ARG((S_symbol *d));
S_symbol *crSimpleDefinition C_ARG((unsigned storage, unsigned t, S_idIdent *id));
int findStrRecord C_ARG((	S_symbol		*s,
					char 			*recname,	/* can be NULL */
					S_symbol 		**res,
					int 			javaClassif
				));
S_reference * findStrRecordFromSymbol C_ARG((	S_symbol *str, 
												S_idIdent *record,
												S_symbol **res,
												int javaClassif,
												S_idIdent *super
						));
S_reference * findStrRecordFromType C_ARG((	S_typeModifiers *str, 
							S_idIdent *record,
							S_symbol **res,
							int javaClassif
						));
int mergeArguments C_ARG((S_symbol *id, S_symbol *ty));
S_typeModifiers *simpleStrUnionSpecifier C_ARG((	S_idIdent *typeName, 
											S_idIdent *id, 
											int usage
										));
S_typeModifiers *crNewAnnonymeStrUnion C_ARG((S_idIdent *typeName));
void specializeStrUnionDef C_ARG((S_symbol *sd, S_symbol *rec));
S_typeModifiers *simpleEnumSpecifier C_ARG((S_idIdent *id, int usage));
void setGlobalFileDepNames C_ARG((char *iname, S_symbol *pp, int memory));
S_typeModifiers *crNewAnnonymeEnum C_ARG((S_symbolList *enums));
void appendPositionToList C_ARG((S_positionLst **list, S_position *pos));
void setParamPositionForFunctionWithoutParams C_ARG((S_position *lpar));
void setParamPositionForParameter0 C_ARG((S_position *lpar));
void setParamPositionForParameterBeyondRange C_ARG((S_position *rpar));
S_symbol *crEmptyField C_ARG(());
void handleDeclaratorParamPositions C_ARG((S_symbol *decl, S_position *lpar,
									S_positionLst *commas, S_position *rpar,
										   int hasParam));
void handleInvocationParamPositions C_ARG((S_reference *ref, S_position *lpar,
									S_positionLst *commas, S_position *rpar,
									int hasParam));
void javaHandleDeclaratorParamPositions C_ARG((S_position *sym, S_position *lpar,
											   S_positionLst *commas, S_position *rpar));
void setLocalVariableLinkName C_ARG((struct symbol *p));
void labelReference C_ARG((S_idIdent *id, int usage));

/* ***********************************************************************
** jsemact.c
*/

void javaCheckForPrimaryStart C_ARG((S_position *cpos, S_position *pp));
void javaCheckForPrimaryStartInNameList C_ARG((S_idIdentList *name, S_position *pp));
void javaCheckForStaticPrefixStart C_ARG((S_position *cpos, S_position *bpos));
void javaCheckForStaticPrefixInNameList C_ARG((S_idIdentList *name, S_position *pp));
S_position *javaGetNameStartingPosition C_ARG((S_idIdentList *name));
char *javaCreateComposedName C_ARG((
									char			*prefix,
									S_idIdentList 	*className,
									int 			classNameSeparator,
									char 			*name,
									char			*resBuff,
									int				resBufSize
								));
int findTopLevelName C_ARG((
								char	 			*name,
								S_recFindStr    	*resRfs,
								S_symbol			**resMemb,
								int 				classif
								));
int javaClassifySingleAmbigNameToTypeOrPack C_ARG((S_idIdentList *name,
												   S_symbol **str,
												   int cxrefFlag
												   ));
void javaAddImportConstructionReference C_ARG((S_position *importPos, S_position *pos, int usage));
int javaClassifyAmbiguousName C_ARG((		S_idIdentList *name, 
									S_recFindStr *rfs,
									S_symbol **str,
									S_typeModifiers **expr,
									S_reference **oref,
									S_reference **rdtoref, int allowUselesFqtRefs,
									int classif, 
									int usage
									));
S_reference *javaClassifyToTypeOrPackageName C_ARG((S_idIdentList *tname, int usage, S_symbol **str, int allowUselesFqtRefs));
S_reference *javaClassifyToTypeName C_ARG((S_idIdentList *tname, int usage, S_symbol **str, int allowUselesFqtRefs));
S_symbol * javaQualifiedThis C_ARG((S_idIdentList *tname, S_idIdent *thisid));
void javaClassifyToPackageName C_ARG(( S_idIdentList *id ));
void javaClassifyToPackageNameAndAddRefs C_ARG((S_idIdentList *id, int usage));
char *javaImportSymbolName_st C_ARG((int file, int line, int coll));
S_typeModifiers *javaClassifyToExpressionName C_ARG((S_idIdentList *name,S_reference **oref));
S_symbol *javaTypeNameDefinition C_ARG((S_idIdentList *tname));
void javaSetFieldLinkName C_ARG((S_symbol *d));
void javaAddPackageDefinition C_ARG((S_idIdentList *id));
S_symbol *javaAddType C_ARG((S_idIdentList *clas, int accessFlag, S_position *p));
S_symbol *javaCreateNewMethod C_ARG((char *name, S_position *pos, int mem));
int javaTypeToString C_ARG((S_typeModifiers *type, char *pp, int ppSize));
int javaIsYetInTheClass C_ARG((	S_symbol	*clas,
								char		*lname,
								S_symbol	**eq
								));
int javaSetFunctionLinkName C_ARG((S_symbol *clas, S_symbol *decl, int mem));
S_symbol * javaGetFieldClass C_ARG((char *fieldLinkName, char **fieldAdr));
void javaAddNestedClassesAsTypeDefs C_ARG((S_symbol *cc, 
						S_idIdentList *oclassname, int accessFlags));
int javaTypeFileExist C_ARG((S_idIdentList *name));
S_symbol *javaTypeSymbolDefinition C_ARG((S_idIdentList *tname, int accessFlags,int addType));
S_symbol *javaTypeSymbolUsage C_ARG((S_idIdentList *tname, int accessFlags));
void javaReadSymbolFromSourceFileEnd C_ARG(());
void javaReadSymbolFromSourceFileInit C_ARG(( int sourceFileNum, 
									   S_jslTypeTab *typeTab ));
void javaReadSymbolsFromSourceFileNoFreeing C_ARG((char *fname, char *asfname));
void javaReadSymbolsFromSourceFile C_ARG((char *fname));
int javaLinkNameIsAnnonymousClass C_ARG((char *linkname));
int javaLinkNameIsANestedClass C_ARG((char *cname));
int isANestedClass C_ARG((S_symbol *ss));
void addSuperMethodCxReferences C_ARG((int classIndex, S_position *pos));
S_reference * addUselessFQTReference C_ARG((int classIndex, S_position *pos));
S_reference *addUnimportedTypeLongReference C_ARG((int classIndex, S_position *pos));
void addThisCxReferences C_ARG((int classIndex, S_position *pos));
void javaLoadClassSymbolsFromFile C_ARG((S_symbol *memb));
S_symbol *javaPrependDirectEnclosingInstanceArgument C_ARG((S_symbol *args));
void addMethodCxReferences C_ARG((unsigned modif, S_symbol *method, S_symbol *clas));
S_symbol *javaMethodHeader C_ARG((unsigned modif, S_symbol *type, S_symbol *decl, int storage));
void javaAddMethodParametersToSymTable C_ARG((S_symbol *method));
void javaMethodBodyBeginning C_ARG((S_symbol *method));
void javaMethodBodyEnding C_ARG((S_position *pos));
void javaAddMapedTypeName C_ARG((
							char *file,
							char *path,
							char *pack,
							S_completions *c,
							void *vdirid,
							int  *storage
						));
S_symbol *javaFQTypeSymbolDefinition C_ARG((char *name, char *fqName));
S_symbol *javaFQTypeSymbol C_ARG((char *name, char *fqName));
S_typeModifiers *javaClassNameType C_ARG((S_idIdentList *typeName));
S_typeModifiers *javaNewAfterName C_ARG((S_idIdentList *name, S_idIdent *id, S_idIdentList *idl));
int javaIsInnerAndCanGetUnnamedEnclosingInstance C_ARG((S_symbol *name, S_symbol **outEi));
S_typeModifiers *javaNestedNewType C_ARG((S_symbol *expr, S_idIdent *thenew, S_idIdentList *idl));
S_typeModifiers *javaArrayFieldAccess C_ARG((S_idIdent *id));
S_typeModifiers *javaMethodInvocationN C_ARG((
								S_idIdentList *name,
								S_typeModifiersList *args
								));
S_typeModifiers *javaMethodInvocationT C_ARG((	S_typeModifiers *tt,
										S_idIdent *name,
										S_typeModifiersList *args
									));
S_typeModifiers *javaMethodInvocationS C_ARG((	S_idIdent *super,
												S_idIdent *name,
												S_typeModifiersList *args
									));
S_typeModifiers *javaConstructorInvocation C_ARG((S_symbol *class,
										   S_position *pos,
										   S_typeModifiersList *args
	));
S_extRecFindStr *javaCrErfsForMethodInvocationN C_ARG((S_idIdentList *name));
S_extRecFindStr *javaCrErfsForMethodInvocationT C_ARG((S_typeModifiers *tt,S_idIdent *name));
S_extRecFindStr *javaCrErfsForMethodInvocationS C_ARG((S_idIdent *super,S_idIdent *name));
S_extRecFindStr *javaCrErfsForConstructorInvocation C_ARG((S_symbol *clas, S_position *pos));
int javaClassIsInCurrentPackage C_ARG((S_symbol *cl));
int javaFqtNamesAreFromTheSamePackage C_ARG((char *classFqName, char *fqname2));
int javaMethodApplicability C_ARG((S_symbol *memb, char *actArgs));
S_symbol *javaGetSuperClass C_ARG((S_symbol *cc));
S_symbol *javaCurrentSuperClass C_ARG(());
S_typeModifiers *javaCheckNumeric C_ARG((S_typeModifiers *tt));
S_typeModifiers *javaNumericPromotion C_ARG((S_typeModifiers *tt));
S_typeModifiers *javaBinaryNumericPromotion C_ARG((	S_typeModifiers *t1,
												S_typeModifiers *t2
											));
S_typeModifiers *javaBitwiseLogicalPromotion C_ARG((	S_typeModifiers *t1,
												S_typeModifiers *t2
											));
S_typeModifiers *javaConditionalPromotion C_ARG((	S_typeModifiers *t1,
											S_typeModifiers *t2
										));
int javaIsStringType C_ARG((S_typeModifiers *tt));
void javaTypeDump C_ARG((S_typeModifiers *tt));
void javaAddJslReadedTopLevelClasses C_ARG((S_jslTypeTab  *typeTab));
struct freeTrail * newAnonClassDefinitionBegin C_ARG((S_idIdent *interfName));
void javaAddSuperNestedClassToSymbolTab C_ARG(( S_symbol *cc));
struct freeTrail * newClassDefinitionBegin C_ARG((S_idIdent *name, int accessFlags, S_symbol *anonInterf));
void newClassDefinitionEnd C_ARG((S_freeTrail *trail));
void javaInitArrayObject C_ARG(());
void javaParsedSuperClass C_ARG((S_symbol *s));
void javaSetClassSourceInformation C_ARG((char *package, S_idIdent *cl));
void javaCheckIfPackageDirectoryIsInClassOrSourcePath C_ARG((char *dir));

/* ***********************************************************************
** cccsemact.c
*/

S_typeModifiers *cccFunApplication C_ARG((S_typeModifiers *fun,
								  S_typeModifiersList *args));

/* ***********************************************************************
** zip.c
*/

//& extern voidpf zlibAlloc OF((voidpf opaque, uInt items, uInt size));
//& extern void   zlibFree  OF((voidpf opaque, voidpf address));

/* ***********************************************************************
** cfread.c
*/

void javaHumanizeLinkName C_ARG(( char *inn, char *outn, int size));
S_symbol *cfAddCastsToModule C_ARG((S_symbol *memb, S_symbol *sup));
void addSuperClassOrInterface C_ARG(( S_symbol *memb, S_symbol *supp, int origin ));
int javaCreateClassFileItem C_ARG(( S_symbol *memb));
void addSuperClassOrInterfaceByName C_ARG((S_symbol *memb, char *super, int origin, int loadSuper));
void fsRecMapOnFiles C_ARG((S_zipArchiveDir *dir, char *zip, char *path, void (*fun)(char *zip, char *file, void *arg), void *arg));
int fsIsMember C_ARG((S_zipArchiveDir **dir, char *fn, unsigned offset, 
						int addFlag, S_zipArchiveDir **place));
int zipIndexArchive C_ARG((char *name));
int zipFindFile C_ARG((char *name, char **resName, S_zipFileTabItem *zipfile));
void javaMapZipDirFile C_ARG((
		S_zipFileTabItem *zipfile,
		char *packfile,
		S_completions *a1,
		void *a2,
		int *a3,
		void (*fun)(MAP_FUN_PROFILE),
		char *classPath,
		char *dirname
	));
void javaReadClassFile C_ARG((char *name, S_symbol *cdef, int loadSuper));

/* ***********************************************************************
** cgram.y
*/
int cyyparse C_ARG(());
void makeCCompletions C_ARG((char *s, int len, S_position *pos));

/* ***********************************************************************
** javaslgram.y
*/
int javaslyyparse C_ARG(());

/* ***********************************************************************
** javagram.y
*/
int javayyparse C_ARG(());
void makeJavaCompletions C_ARG((char *s, int len, S_position *pos));

/* ***********************************************************************
** cppgram.y
*/
int cccyyparse C_ARG(());
void makeCccCompletions C_ARG((char *s, int len, S_position *pos));

/* ***********************************************************************
** yaccgram.y
*/
int yaccyyparse C_ARG(());
void makeYaccCompletions C_ARG((char *s, int len, S_position *pos));

/* ***********************************************************************
** cexp.y
*/

int cexpyyparse C_ARG(());

/* ***********************************************************************
** lex.c
*/
void charBuffClose C_ARG((struct charBuf *bb));
int getCharBuf C_ARG((struct charBuf *bb));
void switchToZippedCharBuff C_ARG((struct charBuf *bb));
int skipNCharsInCharBuf C_ARG((struct charBuf *bb, unsigned count));
int getLexBuf C_ARG((struct lexBuf *lb));
void gotOnLineCxRefs C_ARG(( S_position *ps ));

/* ***********************************************************************
** yylex.c
*/
void ppMemInit C_ARG(());
void initAllInputs C_ARG(());
void initInput C_ARG((FILE *ff, S_editorBuffer *buffer, char *prepend, char *name));
void addIncludeReference C_ARG((int filenum, S_position *pos));
void addThisFileDefineIncludeReference C_ARG((int filenum));
void pushNewInclude C_ARG((FILE *f, S_editorBuffer *buff, char *name, char *prepend));
void popInclude C_ARG(());
void copyDir C_ARG((char *dest, char *s, int *i));
char *normalizeFileName C_ARG((char *name, char *relativeto));
int addFileTabItem C_ARG((char *name, int *fileNumber));
void getOrCreateFileInfo C_ARG((char *ss, int *fileNumber, char **fileName));
void setOpenFileInfo C_ARG((char *ss));
void addMacroDefinedByOption C_ARG((char *opt));
char *placeIdent C_ARG(());
int yylex C_ARG(());
int cachedInputPass C_ARG((int cpoint, char **cfromto));
int cexpyylex C_ARG(());

extern char *yytext;

/* ***********************************************************************
** cexp.c
*/

void cccLexBackTrack C_ARG((int lbi));
int cccylex C_ARG(());
int cccGetLexPosition C_ARG(());
extern int s_cccYylexBufi;

/* ***********************************************************************
** cexp.c
*/
int cexpTranslateToken C_ARG((int tok, int val));

/* ***********************************************************************
** caching.c
*/
int testFileModifTime C_ARG((int ii));
void initCaching C_ARG(());
void recoverCachePoint C_ARG((int i, char *readedUntil, int activeCaching));
void recoverFromCache C_ARG(());
void cacheInput C_ARG(());
void cacheInclude C_ARG((int fileNum));
void poseCachePoint C_ARG((int inputCacheFlag));
void recoverCxMemory C_ARG((char *cxMemFreeBase));
void recoverCachePointZero C_ARG(());
void recoverMemoriesAfterOverflow C_ARG((char *cxMemFreeBase));

/* ***********************************************************************
** options.c
*/
void addSourcePathesCut();
void getXrefrcFileName( char *ttt );
void optionsVisualEdit();
int processInteractiveFlagOption(char **argv, int i);
char *getJavaHome();
char *getJdkClassPathFastly();
void getJavaClassAndSourcePath C_ARG(());
int packageOnCommandLine C_ARG((char *fn));
void getStandardOptions C_ARG((int *nargc, char ***nargv));
char *expandSpecialFilePredefinedVariables_st C_ARG((char *tt));
int readOptionFromFile C_ARG((FILE *ff, int *nargc, char ***nargv, 
						int memFl, char *sectionFile, char *project, char *resSection));
void readOptionFile C_ARG((char *name, int *nargc, char ***nargv,char *sectionFile, char *project));
void readOptionPipe C_ARG((char *command, int *nargc, char ***nargv,char *sectionFile));
void javaSetSourcePath C_ARG((int defaultCpAllowed));
void getOptionsFromMessage C_ARG((char *qnxMsgBuff, int *nargc, char ***nargv));
int changeRefNumOption C_ARG((int newRefNum));

/* ***********************************************************************
** init.c
*/

void initCwd C_ARG(());
void reInitCwd C_ARG((char *dffname, char *dffsect));
void initTokenNameTab C_ARG(());
void initJavaTypePCTIConvertIniTab C_ARG(());
void initTypeCharCodeTab C_ARG(());
void initArchaicTypes C_ARG(());
void initPreCreatedTypes C_ARG(());
void initTypeModifiersTabs();
void initExtractStoragesNameTab();
void initTypesNamesTab();

/* ***********************************************************************
** version.c
*/

int versionNumber C_ARG(());

/* ***********************************************************************
** html.c
*/

void genClassHierarchyItemLinks C_ARG(( FILE *ff, S_olSymbolsMenu *itt,
										int virtFlag));
void htmlGenNonVirtualGlobSymList C_ARG(( FILE *ff, char *fn, S_symbolRefItem *p ));
void htmlGenGlobRefsForVirtMethod C_ARG((FILE *ff, char *fn, 
												 S_olSymbolsMenu *rrr));
int htmlRefItemsOrderLess C_ARG((S_olSymbolsMenu *ss1, S_olSymbolsMenu *ss2));
int isThereSomethingPrintable C_ARG((S_olSymbolsMenu *itt));
void htmlGenEmptyRefsFile();
void javaGetClassNameFromFileNum C_ARG((int nn, char *tmpOut, int dotify));
void javaSlashifyDotName C_ARG((char *ss));
void javaDotifyClassName C_ARG((char *ss));
void javaDotifyFileName C_ARG(( char *ss));
int isAbsolutePath C_ARG((char *p));
char *htmlNormalizedPath C_ARG((char *p));
char *htmlGetLinkFileNameStatic C_ARG((char *link, char *file));
void recursivelyCreateFileDirIfNotExists C_ARG((char *fpath));
void concatPathes C_ARG((char *res, int rsize, char *p1, char *p2, char *p3));
void htmlPutChar C_ARG((FILE *ff, int c));
void htmlPrint C_ARG((FILE *ff, char *ss));
void htmlGenGlobalReferenceLists C_ARG((char *cxMemFreeBase));
void htmlAddJavaDocReference C_ARG((S_symbol  *p, S_position  *pos,
							 int  vFunClass, int  vApplClass));
void generateHtml C_ARG(());
int addHtmlCutPath C_ARG((char *ss ));
void htmlGetDefinitionReferences();
void htmlAddFunctionSeparatorReference();


/* ***********************************************************************
** extract.c
*/

void actionsBeforeAfterExternalDefinition();
void extractActionOnBlockMarker();
void genInternalLabelReference C_ARG((int counter, int usage));
S_symbol * addContinueBreakLabelSymbol C_ARG((int labn, char *name));
void deleteContinueBreakLabelSymbol C_ARG((char *name));
void genContinueBreakReference C_ARG((char *name));
void genSwitchCaseFork C_ARG((int lastFlag));

/* ***********************************************************************
** emacsex.c
*/

void genSafetyCheckFailAction();

/* ***********************************************************************
** jslsemact.c
*/

S_symbol *jslTypeSpecifier1 C_ARG((unsigned t) );
S_symbol *jslTypeSpecifier2 C_ARG((S_typeModifiers *t) );

void jslCompleteDeclarator C_ARG((S_symbol *t, S_symbol *d) );
S_typeModifiers *jslPrependComposedType C_ARG((S_typeModifiers *d, unsigned t) );
S_typeModifiers *jslAppendComposedType C_ARG((S_typeModifiers **d, unsigned t) );
S_symbol *jslPrependDirectEnclosingInstanceArgument C_ARG((S_symbol *args));
S_symbol *jslMethodHeader C_ARG((unsigned modif, S_symbol *type, S_symbol *decl, int storage, S_symbolList *throws) );
S_symbol *jslTypeNameDefinition C_ARG((S_idIdentList *tname) );
S_symbol *jslTypeSymbolDefinition C_ARG((char *ttt2, S_idIdentList *packid, 
								  int add, int order, int isSingleImportedFlag) );
int jslClassifyAmbiguousTypeName C_ARG((S_idIdentList *name, S_symbol **str) );
void jslAddNestedClassesToJslTypeTab C_ARG(( S_symbol *cc, int order) );
void jslAddSuperNestedClassesToJslTypeTab C_ARG(( S_symbol *cc) );

void jslAddSuperClassOrInterfaceByName C_ARG((S_symbol *memb,char *super));
void jslNewClassDefinitionBegin C_ARG((S_idIdent *name, 
								int accessFlags,
								S_symbol *anonInterf,
								int position
	) );
void jslAddDefaultConstructor C_ARG((S_symbol *cl));
void jslNewClassDefinitionEnd C_ARG(() );
void jslNewAnonClassDefinitionBegin C_ARG((S_idIdent *interfName) );

void jslAddSuperClassOrInterface C_ARG((S_symbol *memb,S_symbol *supp));
void jslAddMapedImportTypeName C_ARG((
							char *file,
							char *path,
							char *pack,
							S_completions *c,
							void *vdirid,
							int  *storage
						) );
void jslAddAllPackageClassesFromFileTab C_ARG((S_idIdentList *pack));


extern S_jslStat *s_jsl;

/* ***********************************************************************
** main.c
*/

void dirInputFile C_ARG((MAP_FUN_PROFILE));
void crOptionStr C_ARG((char **dest, char *text));
void xrefSetenv C_ARG((char *name, char *val));
int mainHandleSetOption C_ARG(( int argc, char **argv, int i ));
void copyOptions C_ARG((S_options *dest, S_options *src));
void resetPendingSymbolMenuData();
char *presetEditServerFileDependingStatics C_ARG(());
void searchDefaultOptionsFile C_ARG((char *file, char *ttt, char *sect));
void processOptions C_ARG((int argc, char **argv, int infilesFlag));
void mainSetLanguage C_ARG((char *inFileName, int *outLanguage));
void getAndProcessXrefrcOptions C_ARG((char *dffname, char *dffsect, char *project));
char * getInputFile C_ARG((int *fArgCount));
void getPipedOptions C_ARG((int *outNargc,char ***outNargv));
void mainCallEditServerInit C_ARG((int nargc, char **nargv));
void mainCallEditServer C_ARG((int argc, char **argv, 
							   int nargc, char **nargv, 
							   int *firstPassing
));
void mainCallXref C_ARG((int argc, char **argv));
void mainXref C_ARG((int argc, char **argv));
void writeRelativeProgress C_ARG((int progress));
void mainTaskEntryInitialisations C_ARG((int argc, char **argv));
void mainOpenOutputFile C_ARG((char *ofile));
void mainCloseOutputFile C_ARG(());


/* ***********************************************************************
** editor.c
*/

void editorInit C_ARG(());
int statb C_ARG((char *path, struct stat  *statbuf));
int editorMarkerLess C_ARG((S_editorMarker *m1, S_editorMarker *m2));
int editorMarkerLessOrEq C_ARG((S_editorMarker *m1, S_editorMarker *m2));
int editorMarkerGreater C_ARG((S_editorMarker *m1, S_editorMarker *m2));
int editorMarkerGreaterOrEq C_ARG((S_editorMarker *m1, S_editorMarker *m2));
int editorMarkerListLess C_ARG((S_editorMarkerList *l1, S_editorMarkerList *l2));
int editorRegionListLess C_ARG((S_editorRegionList *l1, S_editorRegionList *l2));
S_editorBuffer *editorOpenBufferNoFileLoad C_ARG((char *name, char *fileName));
S_editorBuffer *editorGetOpenedBuffer C_ARG((char *name));
S_editorBuffer *editorGetOpenedAndLoadedBuffer C_ARG((char *name));
S_editorBuffer *editorFindFile C_ARG((char *name));
S_editorBuffer *editorFindFileCreate C_ARG((char *name));
S_editorMarker *editorCrNewMarkerForPosition C_ARG((S_position *pos));
S_editorMarkerList *editorReferencesToMarkers C_ARG((S_reference *refs, int (*filter)(S_reference *, void *), void *filterParam));
S_reference *editorMarkersToReferences C_ARG((S_editorMarkerList **mms));
void editorRenameBuffer C_ARG((S_editorBuffer *buff, char *newName, S_editorUndo **undo));
void editorReplaceString C_ARG((S_editorBuffer *buff, int position, int delsize, char *str, int strlength, S_editorUndo **undo));
void editorMoveBlock C_ARG((S_editorMarker *dest, S_editorMarker *src, int size, S_editorUndo **undo));
void editorDumpBuffer C_ARG((S_editorBuffer *buff));
void editorDumpBuffers C_ARG(());
void editorDumpMarker C_ARG((S_editorMarker *mm));
void editorDumpMarkerList C_ARG((S_editorMarkerList *mml));
void editorDumpRegionList C_ARG((S_editorRegionList *mml));
void editorQuasySaveModifiedBuffers C_ARG(());
void editorLoadAllOpenedBufferFiles C_ARG(());
S_editorMarker *editorCrNewMarker C_ARG((S_editorBuffer *buff, int offset));
S_editorMarker *editorDuplicateMarker C_ARG((S_editorMarker *mm));
int editorCountLinesBetweenMarkers C_ARG((S_editorMarker *m1, S_editorMarker *m2));
int editorRunWithMarkerUntil C_ARG((S_editorMarker *m, int (*until)(int), int step));
int editorMoveMarkerToNewline C_ARG((S_editorMarker *m, int direction));
int editorMoveMarkerToNonBlank C_ARG((S_editorMarker *m, int direction));
int editorMoveMarkerBeyondIdentifier C_ARG((S_editorMarker *m, int direction));
int editorMoveMarkerToNonBlankOrNewline C_ARG((S_editorMarker *m, int direction));
void editorRemoveBlanks C_ARG((S_editorMarker *mm, int direction, S_editorUndo **undo));
void editorDumpUndoList C_ARG((S_editorUndo *uu));
void editorMoveMarkerToLineCol C_ARG((S_editorMarker *m, int line, int col));
void editorMarkersDifferences C_ARG((
	S_editorMarkerList **list1, S_editorMarkerList **list2,
	S_editorMarkerList **diff1, S_editorMarkerList **diff2));
void editorFreeMarker C_ARG((S_editorMarker *marker));
void editorFreeMarkerListNotMarkers C_ARG((S_editorMarkerList *occs));
void editorFreeMarkersAndRegionList C_ARG((S_editorRegionList *occs));
void editorFreeRegionListNotMarkers C_ARG((S_editorRegionList *occs));
void editorSortRegionsAndRemoveOverlaps C_ARG((S_editorRegionList **regions));
void editorSplitMarkersWithRespectToRegions C_ARG((
	S_editorMarkerList 	**inMarkers, 
	S_editorRegionList 	**inRegions,
	S_editorMarkerList 	**outInsiders, 
	S_editorMarkerList 	**outOutsiders
	));
void editorRestrictMarkersToRegions C_ARG((S_editorMarkerList **mm, S_editorRegionList **regions));
S_editorMarker *editorCrMarkerForBufferBegin C_ARG((S_editorBuffer *buffer));
S_editorMarker *editorCrMarkerForBufferEnd C_ARG((S_editorBuffer *buffer));
S_editorRegionList *editorWholeBufferRegion C_ARG((S_editorBuffer *buffer));
void editorScheduleModifiedBuffersToUpdate C_ARG(());
void editorFreeMarkersAndMarkerList C_ARG((S_editorMarkerList *occs));
int editorMapOnNonexistantFiles C_ARG((
		char *dirname,
		void (*fun)(MAP_FUN_PROFILE),
		int deep,
		char *a1,
		char *a2,
		S_completions *a3,
		void *a4,
		int *a5
	));
void editorCloseBufferIfClosable C_ARG((char *name));
void editorCloseAllBuffersIfClosable C_ARG(());
void editorCloseAllBuffers C_ARG(());
void editorTest C_ARG(());


/* ***********************************************************************
** refactory.c
*/

void refactoryAskForReallyContinueConfirmation C_ARG(());
void refactoryDisplayResolutionDialog C_ARG((char *message,int messageType, int continuation));
void editorApplyUndos C_ARG((S_editorUndo *undos, S_editorUndo *until, S_editorUndo **undoundo, int gen));
void editorUndoUntil C_ARG((S_editorUndo *until, S_editorUndo **undoUndo));
void mainRefactory C_ARG((int argc, char **argv));

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
extern int next;
extern char nextus[EXP_COM_SIZE];
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
extern char s_cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
extern char s_base[MAX_FILE_NAME_SIZE];
extern char s_file[MAX_FILE_NAME_SIZE];
extern char s_path[MAX_FILE_NAME_SIZE];
extern char s_name[MAX_FILE_NAME_SIZE];
extern char ppcTmpBuff[MAX_PPC_RECORD_SIZE];


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
