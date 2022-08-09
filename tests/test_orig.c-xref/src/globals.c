
/*
	$Revision: 1.41 $
	$Date: 2002/09/05 19:25:37 $
*/

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "unigram.h"

#include "protocol.h"
//

char *s_debugSurveyPointer;
int s_fileAbortionEnabled;

#ifdef OLCX_MEMORY_CHECK
void *s_olcx_chech_array[OLCX_CHECK_ARRAY_SIZE];
int s_olcx_chech_array_sizes[OLCX_CHECK_ARRAY_SIZE];
int s_olcx_chech_arrayi = 0;
#endif

int s_wildCharSearch;
int s_lastReturnedLexem;

S_position s_spp[SPP_MAX];

// !!! if changing this, change also s_noRef!!!
S_usageBits s_noUsage = {UsageNone, 0, };

S_reference s_noRef = {{UsageNone, 0, }, {-1, 0, 0}, NULL};

int s_progressOffset=0;
int s_progressFactor=1;

S_availableRefactoring s_availableRefactorings[MAX_AVAILABLE_REFACTORINGS];

S_editorUndo *s_editorUndo = NULL;

z_stream s_defaultZStream = {NULL,};

S_counters s_count;
unsigned s_recFindCl = 1;

S_currentlyParsedCl s_cp;
S_currentlyParsedCl s_cpInit = {0,};

S_currentlyParsedStatics s_cps;
S_currentlyParsedStatics s_cpsInit = {0,};

int s_javaPreScanOnly=0;

/* **************** CACHING ********************** */

S_caching s_cache;

/* **************** cached symbols ********************** */

S_topBlock *s_topBlock;
int ppmMemoryi=0;
int mbMemoryi=0;

/* *********** symbols excluded from cache ************** */

time_t s_expTime;

char s_olSymbolType[COMPLETION_STRING_SIZE];
char s_olSymbolClassType[COMPLETION_STRING_SIZE];

S_position s_paramPosition;
S_position s_paramBeginPosition;
S_position s_paramEndPosition;
S_position s_primaryStartPosition;
S_position s_staticPrefixStartPosition;

S_idIdent s_yyIdentBuf[YYBUFFERED_ID_INDEX];
int s_yyIdentBufi = 0;

S_typeModifiers *s_structRecordCompletionType;
S_typeModifiers *s_upLevelFunctionCompletionType;
S_exprTokenType s_forCompletionType;
S_typeModifiers *s_javaCompletionLastPrimary;

YYSTYPE *uniyylval = &cyylval;
struct yyGlobalState *s_yygstate;
struct yyGlobalState *s_initYygstate;

char *s_input_file_name="";
S_completions s_completions;

S_fileDesc cFile = {0};

S_symTab *s_symTab;
S_javaFqtTab s_javaFqtTab;
S_idTab s_fileTab;
S_refTab s_cxrefTab;

S_fileDesc inStack[INSTACK_SIZE];
int inStacki=0;
S_lexInput macStack[MACSTACK_SIZE];
int macStacki=0;

S_lexInput cInput;

char tmpMemory[SIZE_TMP_MEM];
char memory[SIZE_workMemory];
char ppmMemory[SIZE_ppmMemory];
char mbMemory[SIZE_mbMemory];
char ftMemory[SIZE_ftMemory];
int ftMemoryi = 0;
int next;
char nextus[EXP_COM_SIZE];
char tmpWorkMemory[SIZE_tmpWorkMemory];
int tmpWorkMemoryi = 0;

#ifdef OLD_RLM_MEMORY
void *olcxMemoryFreeList[MAX_BUFFERED_SIZE_olcxMemory] = {NULL};
char olcxMemory[SIZE_olcxMemory];
int olcxMemoryi=0;
#else
int olcxMemoryAllocatedBytes;
#endif

S_memory *cxMemory=NULL;

int s_ifEvaluation = 0;		/* flag for yylex, to not filter '\n' */

S_position s_olcxByPassPos;
S_position s_cxRefPos;
int s_cxRefFlag=0;

time_t s_fileProcessStartTime;

int s_language;
int s_currCppPass;
int s_maximalCppPass;

#if 0
struct olSymbolFoundInformation {
	S_symbolRefItem *symrefs;		/* this is valid */
	S_symbolRefItem *symRefsInfo;	/* additional for error message */
	S_reference *currentRef;
};
#endif

//&S_olSymbolFoundInformation s_oli;
S_userOlcx *s_olcxCurrentUser;
unsigned s_menuFilterOoBits[MAX_MENU_FILTER_LEVEL] = {
	(OOC_VIRT_ANY | OOC_PROFILE_ANY),
//&	(OOC_VIRT_RELATED | OOC_PROFILE_ANY),
	(OOC_VIRT_ANY | OOC_PROFILE_APPLICABLE),
	(OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE),
//&	(OOC_VIRT_APPLICABLE | OOC_PROFILE_APPLICABLE),
//&	(OOC_VIRT_SAME_FUN_CLASS | OOC_PROFILE_APPLICABLE),
//&	(OOC_VIRT_SAME_APPL_FUN_CLASS | OOC_PROFILE_APPLICABLE),
};

// !!!! if you modify this, you will need to modify level# for
//  Emacs, in xref.el !!!!!!!!!
int s_refListFilters[MAX_REF_LIST_FILTER_LEVEL] = {
	UsageMaxOLUsages,
	UsageUsed,
	UsageAddrUsed,
	UsageLvalUsed,
};

char *s_cppVarArgsName = "__VA_ARGS__";
char s_defaultClassPath[] = ".";
S_stringList *s_javaClassPaths;
char *s_javaSourcePaths;

S_idIdent s_javaAnonymousClassName = {"{Anonymous}", NULL, -1,0,0};
S_idIdent s_javaConstructorName = {"<init>", NULL, -1,0,0};

static S_idIdentList s_javaDefaultPackageNameBody[] = {
	{"", NULL, -1,0,0, "", TypePackage, NULL},
};
S_idIdentList *s_javaDefaultPackageName = s_javaDefaultPackageNameBody;

static S_idIdentList s_javaLangNameBody[] = {
	{"lang", NULL, -1,0,0, "lang", TypePackage, &s_javaLangNameBody[1]},
	{"java", NULL, -1,0,0, "java", TypePackage, NULL},
};
S_idIdentList *s_javaLangName = s_javaLangNameBody;

static S_idIdentList s_javaLangStringNameBody[] = {
	{"String", NULL, -1,0,0, "String", TypeStruct, &s_javaLangStringNameBody[1]},
	{"lang",   NULL, -1,0,0, "lang", TypePackage, &s_javaLangStringNameBody[2]},
	{"java",   NULL, -1,0,0, "java", TypePackage, NULL},
};
S_idIdentList *s_javaLangStringName = s_javaLangStringNameBody;

static S_idIdentList s_javaLangCloneableNameBody[] = {
	{"Cloneable", NULL, -1,0,0, "Cloneable", TypeStruct, &s_javaLangCloneableNameBody[1]},
	{"lang",   NULL, -1,0,0, "lang", TypePackage, &s_javaLangCloneableNameBody[2]},
	{"java",   NULL, -1,0,0, "java", TypePackage, NULL},
};
S_idIdentList *s_javaLangCloneableName = s_javaLangCloneableNameBody;

static S_idIdentList s_javaIoSerializableNameBody[] = {
	{"Serializable", NULL, -1,0,0, "Serializable", TypeStruct, &s_javaIoSerializableNameBody[1]},
	{"io",   NULL, -1,0,0, "io", TypePackage, &s_javaIoSerializableNameBody[2]},
	{"java",   NULL, -1,0,0, "java", TypePackage, NULL},
};
S_idIdentList *s_javaIoSerializableName = s_javaIoSerializableNameBody;

static S_idIdentList s_javaLangClassNameBody[] = {
	{"Class", NULL, -1,0,0, "Class", TypeStruct, &s_javaLangClassNameBody[1]},
	{"lang",   NULL, -1,0,0, "lang", TypePackage, &s_javaLangClassNameBody[2]},
	{"java",   NULL, -1,0,0, "java", TypePackage, NULL},
};
S_idIdentList *s_javaLangClassName = s_javaLangClassNameBody;

static S_idIdentList s_javaLangObjectNameBody[] = {
	{"Object", NULL, -1,0,0, "Object", TypeStruct, &s_javaLangObjectNameBody[1]},
	{"lang",   NULL, -1,0,0, "lang", TypePackage, &s_javaLangObjectNameBody[2]},
	{"java",   NULL, -1,0,0, "java", TypePackage, NULL},
};
S_idIdentList *s_javaLangObjectName = s_javaLangObjectNameBody;
char *s_javaLangObjectLinkName="java/lang/Object";

/* ********* vars for on-line additions after EOF ****** */

char s_olstring[MAX_FUN_NAME_SIZE];
int s_olstringFound = 0;
int s_olstringServed = 0;
int s_olstringUsage = 0;
char *s_olstringInMbody = NULL;
int s_olMacro2PassFile;

/* ******************* yytext for yacc ****************** */
char *yytext;

/* ******************* options ************************** */

S_options s_opt;		// current options
S_options s_ropt;		// xref -refactory command line options
S_javaStat *s_javaStat;
S_javaStat s_initJavaStat;

char *s_javaThisPackageName = "";

/* ************************************************************* */
/* ************* constant during the program ******************* */
/* *********** once, the file starts to be parsed ************** */
/* ************************************************************* */

FILE *cxOut;
FILE *fIn;
FILE *ccOut=NULL;

int s_cxResizingBlocked = 0;

int s_input_file_number = -1;
int s_olStringSecondProcessing=0;
int s_olOriginalFileNumber = -1;
int s_olOriginalComFileNumber = -1;
int s_noneFileIndex = -1;
time_t s_expiration;

S_typeModifiers s_defaultIntModifier;
S_symbol s_defaultIntDefinition;
S_typeModifiers s_defaultPackedTypeModifier;
S_typeModifiers s_defaultVoidModifier;
S_symbol s_defaultVoidDefinition;
S_typeModifiers s_errorModifier;
S_symbol s_errorSymbol;
struct stat s_noStat;
S_position s_noPos = {-1, 0, 0};

S_symbol s_javaArrayObjectSymbol;
S_symbol *s_javaStringSymbol;
S_symbol *s_javaCloneableSymbol;
S_symbol *s_javaIoSerializableSymbol;
S_symbol *s_javaObjectSymbol;
S_typeModifiers s_javaStringModifier;
S_typeModifiers s_javaClassModifier;
S_typeModifiers s_javaObjectModifier;

S_zipFileTabItem s_zipArchivTab[MAX_JAVA_ZIP_ARCHIVES];

char s_cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
char s_base[MAX_FILE_NAME_SIZE];

char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

jmp_buf cxmemOverflow;
jmp_buf s_memoryResize;

S_options s_cachedOptions;

/* ********************************************************************** */
/*                            real constants                              */
/* ********************************************************************** */

int s_javaRequiredeAccessibilitiesTable[MAX_REQUIRED_ACCESS+1] = {
	ACC_PUBLIC,
	ACC_PROTECTED,
	ACC_DEFAULT,
	ACC_PRIVATE,
};

S_options s_initOpt = {
	MULE_DEFAULT,		// encoding
	0,					// completeParenthesis
	NID_IMPORT_ON_DEMAND,	// defaultAddImportStrategy
	0,					// referenceListWithoutSource
	0,					// jeditOldCompletions;		// to be removed
	1,					// completionOverloadWizardDeep
	0,					// exit
	0,					// comment moving level
	NULL,				// prune name
	NULL,				// input files
	RC_ZERO,			// continue refactoring
	0,					// completion case sensitive
	NULL,				// xrefrc
	NO_EOL_CONVERSION,	// crlfConversion
	NULL,				// checkVersion
	CUT_OUTERS,			// nestedClassDisplaying
	NULL,				// pushName
	0,					// parnum2
	"",					// refactoring parameter 1
	"",					// refactoring parameter 2
	0,					// refactoring
	0,					// briefoutput
	0,					// cacheIncludes
	0,					// stdopFlag
	NULL,				// renameTo
	RegimeUndefined,	// refactoringRegime
	0,					// xrefactory-II
	NULL,				// moveTargetFile
#if defined (__WIN32__) || defined (__OS2__)			/*SBD*/
	"c;C",				// cFilesSuffixes
	"java;JAV",			// javaFilesSuffixes
	"C;cpp;CC;cc",		// c++FilesSuffixes
#else	          										/*SBD*/
	"c:C",				// cFilesSuffixes
	"java",				// javaFilesSuffixes
	"C:cpp:CC:cc",		// c++FilesSuffixes
#endif          										/*SBD*/
	1,					// fileNamesCaseSensitive
	":",				// htmlLineNumLabel
	0,					// html cut suffix
	TSS_FULL_SEARCH,	// search Tag file specifics
	JAVA_VERSION_1_3,	// java version
	"",					// windel file
	0,					// following is windel line:col x line-col
	0,
	0,
	0,
	"nouser",   // moveToUser
	0,			// noerrors
	0,			// fqtNameToCompletions
	NULL,		// moveTargetClass
	TPC_NONE,	// trivial pre-check
	1,		// urlGenTemporaryFile
	1,		// urlautoredirect
	0,		// javafilesonly
	0,      // exact position
	NULL,   // -o outputFileName
	NULL,   // -line lineFileName
	NULL,   // -I include dirs
	DEFAULT_CXREF_FILE,     // -refs

	NULL,	// file move for safety check
	NULL,
	0,					// first moved line
	MAXIMAL_INT,		// safety check number of lines moved
	0,					// new line number of the first line

	"",		// getValue
	0,		// java2html
	1,		// javaSlAllowed (autoUpdateFromSrc)
	XFILE_HASH_DEFAULT,
	NULL,
	80, 	// -htmlcxlinelen
	"java.applet:java.awt:java.beans:java.io:java.lang:java.math:java.net:java.rmi:java.security:java.sql:java.text:java.util:javax.accessibility:javax.swing:org.omg.CORBA:org.omg.CosNaming",     // -htmljavadocavailable
	0,
	NULL,       // "http://java.sun.com/j2se/1.3/docs/api",
	"", 		// javaDocPath
	0,
	NULL,		// sourcepath
	0,			// htmlDirectX
	"/tmp",
	0,
	0,
	0,
	1,			// allowClassFileRefs
	0,
	"",
	DEFAULT_VALUE,		// manual symbol resolution
	NULL,		// browsed symbol name
	1,
	0,
	0,			// htmlNoUnderline
	"navy",		// htmlLinkColor
	"",			// htmlCutPath
	0,			// htmlCutPathLen
	(OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE), // ooChecksBits
	0,
	0,
	1,          // memory factor ?
	1,
	0,
	NULL,
	0,
	"0:3",		// olcxlccursor
	NULL,
	"",
	79,
	BIN_REL_LICENSE_STRING,
	"",
	"*_",
	EXTR_FUNCTION,
	0,
	//& "__newFunction__",	// extractName
	MAX_COMPLETIONS,
	0,
	0,
	"",
	1,						// recursively dirs
	"",						// default classpath
	8,
	"",						// -htmlroot ?
	0,
	0,
	0,
	0,
	-1,
	-1,
	RegimeXref,
	"nouser",			// -user
	0,
	1,
	0,
	0,
	0,
	"",

	0,
	0,
	0,
	0,
	0,
	0,
	0,

	/* GENERATE options */

	0,
	0,
	0,
	0,
	0,
	0,

	/* CXREF options  */

	0,
	0,
	1,
	0,
	0,
	NULL,
	0,

	// all the rest is initialized to zeros
	{0, },		// get/set end
	{0, },		// -cutpathes

	// penging memory for string values
	NULL,
	{0, },
	{0, },
};

char *s_editCommunicationString = "C@$./@mVeDitznAC";

uchar typeShortChange[MAX_TYPE];
uchar typeLongChange[MAX_TYPE];
uchar typeSignedChange[MAX_TYPE];
uchar typeUnsignedChange[MAX_TYPE];

char s_javaBaseTypeCharCodes[MAX_TYPE];
int s_javaCharCodeBaseTypes[MAX_CHARS];

char s_javaTypePCTIConvert[MAX_TYPE];

char s_javaPrimitiveWideningConversions[MAX_PCTIndex-1][MAX_PCTIndex-1] = {
/* Byte,Short,Char,Int,Long,Float,Double */
    1,   1,    0,   1,  1,   1,    1,	/* Byte, */
    0,   1,    0,   1,  1,   1,    1,  	/* Short, */
    0,   0,    1,   1,  1,   1,    1,  	/* Char, */
    0,   0,    0,   1,  1,   1,    1,  	/* Int, */
    0,   0,    0,   0,  1,   1,    1,  	/* Long, */
    0,   0,    0,   0,  0,   1,    1,  	/* Float, */
    0,   0,    0,   0,  0,   0,    1,  	/* Double, */
};

char *s_tokenName[LAST_TOKEN];
int s_tokenLength[LAST_TOKEN];

S_typeModifiers * s_preCrTypesTab[MAX_TYPE];
S_typeModifiers * s_preCrPtr1TypesTab[MAX_TYPE];
S_typeModifiers * s_preCrPtr2TypesTab[MAX_TYPE];

/* *********************************************************************** */
/* ********************* TABLES TO INIT TABLES *************************** */
/* *********************************************************************** */

S_typeModificationsInit s_typeModificationsInit[] = {
//   type           short          long         signed        unsigned
  {TypeDefault, TypeShortInt, TypeLongInt, TypeSignedInt, TypeUnsignedInt},
  {TypeChar, -1, -1, TypeSignedChar, TypeUnsignedChar},
  {TypeInt, TypeShortInt, TypeLongInt, TypeSignedInt, TypeUnsignedInt},
  {TypeUnsignedInt, TypeShortUnsignedInt, TypeLongUnsignedInt, -1, -1},
  {TypeSignedInt, TypeShortSignedInt, TypeLongSignedInt, -1, -1},
  {TypeShortInt, -1, -1, TypeShortSignedInt, TypeShortUnsignedInt},
  {TypeLongInt, -1, -1, TypeLongSignedInt, TypeLongUnsignedInt},
  {TmodLong, -1, -1, TmodLongSigned, TmodLongUnsigned},
  {TmodShort, -1, -1, TmodShortSigned, TmodShortUnsigned},
  {TmodSigned, TmodShortSigned, TmodLongSigned, -1, -1},
  {TmodUnsigned, TmodShortUnsigned, TmodLongUnsigned, -1, -1},
  {-1,-1,-1,-1,-1}
};

S_tokenNameIni s_tokenNameIniTab[] = {
	{"asm",			ASM_KEYWORD		,LAN_CCC},
	{"auto", 		AUTO			,LAN_C | LAN_YACC | LAN_CCC},
	{"enum", 		ENUM			,LAN_C | LAN_YACC | LAN_CCC},
	{"extern",		EXTERN			,LAN_C | LAN_YACC | LAN_CCC},
	{"register",	REGISTER		,LAN_C | LAN_YACC | LAN_CCC},
	{"signed",		SIGNED			,LAN_C | LAN_YACC | LAN_CCC},
	{"sizeof",		SIZEOF			,LAN_C | LAN_YACC | LAN_CCC},
	{"struct",		STRUCT			,LAN_C | LAN_YACC | LAN_CCC},
	{"typedef",		TYPEDEF			,LAN_C | LAN_YACC | LAN_CCC},
	{"union", 		UNION			,LAN_C | LAN_YACC | LAN_CCC},
	{"unsigned",	UNSIGNED		,LAN_C | LAN_YACC | LAN_CCC},

	{"abstract",	ABSTRACT		,LAN_JAVA},
	{"boolean",		BOOLEAN			,LAN_JAVA},
	{"byte",		BYTE			,LAN_JAVA},
	{"catch",		CATCH			,LAN_JAVA | LAN_CCC},
	{"class",		CLASS			,LAN_JAVA | LAN_CCC},
	{"extends",		EXTENDS			,LAN_JAVA},
	{"final",		FINAL			,LAN_JAVA},
	{"finally",		FINALLY			,LAN_JAVA},
	{"implements",	IMPLEMENTS		,LAN_JAVA},
	{"import",		IMPORT			,LAN_JAVA},
	{"instanceof",	INSTANCEOF		,LAN_JAVA},
	{"interface",	INTERFACE		,LAN_JAVA},
	{"native",		NATIVE			,LAN_JAVA},
	{"new",			NEW				,LAN_JAVA | LAN_CCC},
	{"package",		PACKAGE			,LAN_JAVA},
	{"private",		PRIVATE			,LAN_JAVA | LAN_CCC},
	{"protected",	PROTECTED		,LAN_JAVA | LAN_CCC},
	{"public",		PUBLIC			,LAN_JAVA | LAN_CCC},
	{"super",		SUPER			,LAN_JAVA},

	{"synchronized",SYNCHRONIZED	,LAN_JAVA},
	{"strictfp",	STRICTFP		,LAN_JAVA},
	{"this",		THIS			,LAN_JAVA | LAN_CCC},
	{"throw",		THROW			,LAN_JAVA | LAN_CCC},
	{"throws",		THROWS			,LAN_JAVA},
	{"transient",	TRANSIENT		,LAN_JAVA},
	{"try",			TRY				,LAN_JAVA | LAN_CCC},

	{"true",		TRUE_LITERAL	,LAN_JAVA | LAN_CCC},
	{"false",		FALSE_LITERAL	,LAN_JAVA | LAN_CCC},
	{"null",		NULL_LITERAL	,LAN_JAVA},


	{"bool",				BOOL			,LAN_CCC},
	{"const_cast",			CONST_CAST		,LAN_CCC},
	{"delete",				DELETE			,LAN_CCC},
	{"dynamic_cast",		DYNAMIC_CAST	,LAN_CCC},
	{"explicit",			EXPLICIT		,LAN_CCC},
	{"friend",				FRIEND			,LAN_CCC},
	{"inline",				INLINE			,LAN_CCC},
	{"mutable",				MUTABLE			,LAN_CCC},
	{"namespace",			NAMESPACE		,LAN_CCC},
	{"operator",			OPERATOR		,LAN_CCC},
	{"reinterpret_cast",	REINTERPRET_CAST,LAN_CCC},
	{"static_cast",			STATIC_CAST		,LAN_CCC},
	{"template",			TEMPLATE		,LAN_CCC},
	{"typeid",				TYPEID			,LAN_CCC},
	{"typename",			TYPENAME		,LAN_CCC},
	{"using",				USING			,LAN_CCC},
	{"virtual",				VIRTUAL			,LAN_CCC},
	{"wchar_t",				WCHAR_T			,LAN_CCC},

	{"break", 		BREAK			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"case", 		CASE			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"char", 		CHAR			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"const", 		CONST			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"continue",	CONTINUE		,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"default",		DEFAULT			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"do", 			DO				,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"double",		DOUBLE			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"else", 		ELSE			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"float", 		FLOAT			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"for", 		FOR				,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"goto", 		GOTO			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"if", 			IF				,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"int", 		INT				,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"long", 		LONG			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"return",		RETURN			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"short", 		SHORT			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"static",		STATIC			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"switch",		SWITCH			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"void", 		VOID			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"volatile",	VOLATILE		,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},
	{"while", 		WHILE			,LAN_C | LAN_YACC | LAN_CCC | LAN_JAVA},

/*
	{"token", 		TOKEN			,LAN_YACC},
	{"type", 		TYPE			,LAN_YACC},
*/

	{">>>=", 		URIGHT_ASSIGN	,LAN_JAVA},
	{">>>", 		URIGHT_OP		,LAN_JAVA},
	{"...", 		ELIPSIS			,LAN_C},
	{">>=", 		RIGHT_ASSIGN	,LAN_C},
	{"<<=", 		LEFT_ASSIGN		,LAN_C},
	{"+=", 		ADD_ASSIGN			,LAN_C},
	{"-=", 		SUB_ASSIGN			,LAN_C},
	{"*=", 		MUL_ASSIGN			,LAN_C},
	{"/=", 		DIV_ASSIGN			,LAN_C},
	{"%=", 		MOD_ASSIGN			,LAN_C},
	{"&=", 		AND_ASSIGN			,LAN_C},
	{"^=", 		XOR_ASSIGN			,LAN_C},
	{"|=", 		OR_ASSIGN			,LAN_C},
	{">>", 		RIGHT_OP			,LAN_C},
	{"<<", 		LEFT_OP				,LAN_C},
	{"++", 		INC_OP				,LAN_C},
	{"--", 		DEC_OP				,LAN_C},
	{"->", 		PTR_OP		,LAN_C},
	{"->*",		PTRM_OP		,LAN_CCC},
	{".*",		POINTM_OP	,LAN_CCC},
	{"::",		DPOINT		,LAN_CCC},
	{"&&", 		AND_OP		,LAN_C},
	{"||", 		OR_OP		,LAN_C},
	{"<=", 		LE_OP		,LAN_C},
	{">=", 		GE_OP		,LAN_C},
	{"==", 		EQ_OP		,LAN_C},
	{"!=", 		NE_OP		,LAN_C},
	{";", 		';'		,LAN_C},
	{"{", 		'{'		,LAN_C},
	{"}", 		'}'		,LAN_C},
	{",", 		','		,LAN_C},
	{":", 		':'		,LAN_C},
	{"=", 		'='		,LAN_C},
	{"(", 		'('		,LAN_C},
	{")", 		')'		,LAN_C},
	{"[", 		'['		,LAN_C},
	{"]", 		']'		,LAN_C},
	{".", 		'.'		,LAN_C},
	{"&", 		'&'		,LAN_C},
	{"!", 		'!'		,LAN_C},
	{"~", 		'~'		,LAN_C},
	{"-", 		'-'		,LAN_C},
	{"+", 		'+'		,LAN_C},
	{"*", 		'*'		,LAN_C},
	{"/", 		'/'		,LAN_C},
	{"%", 		'%'		,LAN_C},
	{"<", 		'<'		,LAN_C},
	{">", 		'>'		,LAN_C},
	{"^", 		'^'		,LAN_C},
	{"|", 		'|'		,LAN_C},
	{"?", 		'?'		,LAN_C},


	{"()", 		CCC_OPER_PARENTHESIS			,LAN_CCC},
	{"[]", 		CCC_OPER_BRACKETS				,LAN_CCC},


	{"'CONSTANT'", 				CONSTANT			,LAN_C},
	{"'CONSTANT'", 				LONG_CONSTANT		,LAN_C},
	{"'CONSTANT'", 				FLOAT_CONSTANT		,LAN_C},
	{"'CONSTANT'", 				DOUBLE_CONSTANT		,LAN_C},
	{"'STRING_LITERAL'", 		STRING_LITERAL		,LAN_C},
#ifdef DEBUG
	{"'IDENTIFIER'",			IDENTIFIER			,LAN_C},
	{"'LINE_TOK'", 				LINE_TOK			,LAN_C},
	{"'BLOCK_MARKER'", 			OL_MARKER_TOKEN		,LAN_C},
	{"#INCLUDE", 				CPP_INCLUDE		,LAN_C},
	{"#DEFINE0",				CPP_DEFINE0		,LAN_C},
	{"#DEFINE", 				CPP_DEFINE		,LAN_C},
	{"#IFDEF", 					CPP_IFDEF		,LAN_C},
	{"#IFNDEF",					CPP_IFNDEF		,LAN_C},
	{"#IF",						CPP_IF			,LAN_C},
	{"#ELSE",					CPP_ELSE		,LAN_C},
	{"#ENDIF",					CPP_ENDIF		,LAN_C},
	{"#PRAGMA",					CPP_PRAGMA		,LAN_C},
	{"#LINE",					CPP_LINE		,LAN_C},
	{"##", 						CPP_COLLATION	,LAN_C},
#endif

	{NULL,		0		,LAN_C}			/* sentinel*/
};

S_tokenNameIni s_tokenNameIniTab2[] = {
	{"__const",			CONST			,LAN_C | LAN_YACC | LAN_CCC},
	{"__const__",		CONST			,LAN_C | LAN_YACC | LAN_CCC},
	{"__signed",		SIGNED			,LAN_C | LAN_YACC | LAN_CCC},
	{"__signed__",		SIGNED			,LAN_C | LAN_YACC | LAN_CCC},
//&	{"inline",			ANONYME_MOD		,LAN_C | LAN_YACC },
	{"__inline",		ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__inline__",		ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__volatile",		VOLATILE		,LAN_C | LAN_YACC | LAN_CCC},
	{"__volatile__",	VOLATILE		,LAN_C | LAN_YACC | LAN_CCC},
//&	{"asm",				ASM_KEYWORD		,LAN_C },
	{"__asm",			ASM_KEYWORD		,LAN_C | LAN_CCC},
	{"__asm__",			ASM_KEYWORD		,LAN_C | LAN_CCC},
	{"__label__",		LABEL			,LAN_C | LAN_CCC},
	{"__near",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__far",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__pascal",		ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"_near",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"_far",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"_pascal",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"_const",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__near",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__far",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__pascal",		ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__cdecl",			ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__fastcall",		ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__stdcall",		ANONYME_MOD		,LAN_C | LAN_YACC | LAN_CCC},
	{"__int8",			INT				,LAN_C | LAN_YACC | LAN_CCC},
	{"__int16",			INT				,LAN_C | LAN_YACC | LAN_CCC},
	{"__int32",			INT				,LAN_C | LAN_YACC | LAN_CCC},
	{"__int64",			INT				,LAN_C | LAN_YACC | LAN_CCC},
	{NULL,		0		,LAN_C}			/* sentinel*/
};

S_tokenNameIni s_tokenNameIniTab3[] = {
	{"assert",		ASSERT			,LAN_JAVA},
	{NULL,			0				,LAN_C}			/* sentinel*/
};


S_typeCharCodeIni s_baseTypeCharCodesIniTab[] = {
	{TypeByte, 		'B'},
	{TypeChar, 		'C'},
	{TypeDouble, 	'D'},
	{TypeFloat, 	'F'},
	{TypeInt, 		'I'},
	{TypeLong, 		'J'},
	{TypeShort, 	'S'},
	{TypeBoolean, 	'Z'},
	{TypeVoid,	 	'V'},
	{TypeError,	 	'E'},
	{TypeNull,	 	JAVA_NULL_CODE},	/* this is just my in(con)vention */
	{-1, 			' '},
};

S_javaTypePCTIConvertIni s_javaTypePCTIConvertIniTab[] = {
	{TypeByte,		PCTIndexByte},
	{TypeShort,		PCTIndexShort},
	{TypeChar,		PCTIndexChar},
	{TypeInt,		PCTIndexInt},
	{TypeLong,		PCTIndexLong},
	{TypeFloat,		PCTIndexFloat},
	{TypeDouble,	PCTIndexDouble},
	{-1,-1},
};

char *s_extractStorageName[MAX_STORAGE];

S_intStringTab s_extractStoragesNamesInitTab[] = {
	{StorageExtern,		"extern "},
	{StorageStatic,		"static "},
	{StorageRegister,	"register "},
	{StorageTypedef,	"typedef "},
	{-1, NULL}
};

S_intStringTab s_typesNamesInitTab[] = {
	{TypeDefault ,			"Default"}, 
	{TypeChar ,				"char"},
	{TypeUnsignedChar ,		"unsigned char"},
	{TypeSignedChar ,		"signed char"},
	{TypeInt ,				"int"},
	{TypeUnsignedInt ,		"unsigned int"},
	{TypeSignedInt ,		"signed int"},
	{TypeShortInt ,			"short int"},
	{TypeShortUnsignedInt ,	"short unsigned int"},
	{TypeShortSignedInt ,	"short signed int"},
	{TypeLongInt ,			"long int"},
	{TypeLongUnsignedInt ,	"long unsigned int"},
	{TypeLongSignedInt ,	"long signed int"},
	{TypeFloat ,			"float"},
	{TypeDouble ,			"double"},
	{TypeStruct,			"struct"},
	{TypeUnion,				"union"},
	{TypeEnum ,				"enum"},
	{TypeVoid ,				"void"},
	{TypePointer,			"Pointer"},
	{TypeArray,				"Array"},
	{TypeFunction,			"Function"},
	{TypeAnonymeField,		"AnonymeField"},
	{TypeError,				"Error"},
	{TypeCppIfElse,			"#if-else-fi"},
	{TypeCppInclude,		"#include"},
	{TypeCppCollate,		"##"},

	{TmodLong,				"long"},
	{TmodShort,				"short"},
	{TmodSigned,			"signed"},
	{TmodUnsigned,			"unsigned"},
	{TmodShortSigned,		"short signed"},
	{TmodShortUnsigned,		"short unsigned"},
	{TmodLongSigned,		"long signed"},
	{TmodLongUnsigned,		"long unsigned"},

	{TypeElipsis,			"elipsis"},

	{TypeByte,				"byte"},
	{TypeShort,				"short"},
	{TypeLong,				"long"},
	{TypeBoolean,			"boolean"},
	{TypeNull,				"null"},
	{TypeOverloadedFunction,	"OverloadedFunction"},

	{TypeWchar_t,			"wchar_t"},

	{TypeLabel,				"label"},
	{TypeKeyword,			"keyword"},
	{TypeToken,				"token"},
	{TypeMacro,				"macro"},
	{TypeMacroArg,			"macro argument"},
	{TypeUndefMacro,		"Undefined Macro"},
	{TypePackage,			"package"},
	{TypeYaccSymbol,		"yacc symbol"},
	{TypeCppCollate,		"Cpp##sym"},
	{TypeSpecialComplet,	"(Completion Wizard)"},
	{TypeInheritedFullMethod,	"(Override Wizard)"},
	{TypeNonImportedClass,	"fully qualified name"},
	{-1,NULL}
};

int s_preCrTypesIniTab[] = {
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
	TypeVoid ,
	TypeError ,
	TypeAnonymeField ,
	/* MODIFIERS, */
	TmodLong,
	TmodShort,
	TmodSigned,
	TmodUnsigned,
	TmodShortSigned,
	TmodShortUnsigned,
	TmodLongSigned,
	TmodLongUnsigned,
	/* JAVA_TYPES, */
	TypeByte,
	TypeShort,
	TypeLong,
	TypeBoolean,
	TypeNull,
	/* C++ */
	TypeWchar_t,
	-1,
};


