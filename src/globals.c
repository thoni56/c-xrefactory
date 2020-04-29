#include "globals.h"

#include "stdinc.h"
#include "head.h"
#include "proto.h"
#include "parsers.h"

#include "protocol.h"


int s_fileAbortionEnabled;

#ifdef OLCX_MEMORY_CHECK
void *s_olcx_check_array[OLCX_CHECK_ARRAY_SIZE];
int s_olcx_check_array_sizes[OLCX_CHECK_ARRAY_SIZE];
int s_olcx_check_arrayi = 0;
#endif

bool s_wildcardSearch;
int s_lastReturnedLexem;

Position s_spp[SPP_MAX];

// !!! if changing this, change also s_noRef!!!
UsageBits s_noUsage = {UsageNone, 0, };

Reference s_noRef = {{UsageNone, 0, }, {-1, 0, 0}, NULL};

int s_progressOffset=0;
int s_progressFactor=1;

z_stream s_defaultZStream = {NULL,};

S_counters s_count;
unsigned s_recFindCl = 1;

S_currentlyParsedCl s_cp;
S_currentlyParsedCl s_cpInit = {0,};

S_currentlyParsedStatics s_cps;
S_currentlyParsedStatics s_cpsInit = {0,};

int s_javaPreScanOnly=0;

/* **************** cached symbols ********************** */

int ppmMemoryi=0;
int mbMemoryi=0;

/* *********** symbols excluded from cache ************** */

time_t s_expTime;

char s_olSymbolType[COMPLETION_STRING_SIZE];
char s_olSymbolClassType[COMPLETION_STRING_SIZE];

Position s_paramPosition;
Position s_paramBeginPosition;
Position s_paramEndPosition;
Position s_primaryStartPosition;
Position s_staticPrefixStartPosition;

Id s_yyIdentBuf[YYBUFFERED_ID_INDEX];
int s_yyIdentBufi = 0;

TypeModifier *s_structRecordCompletionType;
TypeModifier *s_upLevelFunctionCompletionType;
S_exprTokenType s_forCompletionType;
TypeModifier *s_javaCompletionLastPrimary;

struct yyGlobalState *s_yygstate;
struct yyGlobalState *s_initYygstate;

char *s_input_file_name="";
Completions s_completions;

S_fileTab s_fileTab;

char tmpMemory[SIZE_TMP_MEM];
char ppmMemory[SIZE_ppmMemory];
char mbMemory[SIZE_mbMemory];

char tmpBuff[TMP_BUFF_SIZE];


int olcxMemoryAllocatedBytes;

bool s_ifEvaluation = false;     /* flag for yylex, to not filter '\n' TODO: move, duh!*/

Position s_olcxByPassPos;
Position s_cxRefPos;
int s_cxRefFlag=0;

time_t s_fileProcessStartTime;

Language s_language;
int s_currCppPass;
int s_maximalCppPass;

unsigned s_menuFilterOoBits[MAX_MENU_FILTER_LEVEL] = {
    (OOC_VIRT_ANY | OOC_PROFILE_ANY),
    //& (OOC_VIRT_RELATED | OOC_PROFILE_ANY),
    (OOC_VIRT_ANY | OOC_PROFILE_APPLICABLE),
    (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE),
    //& (OOC_VIRT_APPLICABLE | OOC_PROFILE_APPLICABLE),
    //& (OOC_VIRT_SAME_FUN_CLASS | OOC_PROFILE_APPLICABLE),
    //& (OOC_VIRT_SAME_APPL_FUN_CLASS | OOC_PROFILE_APPLICABLE),
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

Id s_javaAnonymousClassName = {"{Anonymous}", NULL, {-1,0,0}};
Id s_javaConstructorName = {"<init>", NULL, {-1,0,0}};

static IdList s_javaDefaultPackageNameBody[] = {
    {{"", NULL, {-1,0,0}, NULL}, "", TypePackage, NULL},
};
IdList *s_javaDefaultPackageName = s_javaDefaultPackageNameBody;

static IdList s_javaLangNameBody[] = {
    {{"lang", NULL, {-1,0,0}, NULL}, "lang", TypePackage, &s_javaLangNameBody[1]},
    {{"java", NULL, {-1,0,0}, NULL}, "java", TypePackage, NULL},
};
IdList *s_javaLangName = s_javaLangNameBody;

static IdList s_javaLangStringNameBody[] = {
    {{"String", NULL, {-1,0,0}, NULL}, "String", TypeStruct, &s_javaLangStringNameBody[1]},
    {{"lang",   NULL, {-1,0,0}, NULL}, "lang", TypePackage, &s_javaLangStringNameBody[2]},
    {{"java",   NULL, {-1,0,0}, NULL}, "java", TypePackage, NULL},
};
IdList *s_javaLangStringName = s_javaLangStringNameBody;

static IdList s_javaLangCloneableNameBody[] = {
    {{"Cloneable", NULL, {-1,0,0}, NULL}, "Cloneable", TypeStruct, &s_javaLangCloneableNameBody[1]},
    {{"lang",   NULL, {-1,0,0}, NULL}, "lang", TypePackage, &s_javaLangCloneableNameBody[2]},
    {{"java",   NULL, {-1,0,0}, NULL}, "java", TypePackage, NULL},
};
IdList *s_javaLangCloneableName = s_javaLangCloneableNameBody;

static IdList s_javaIoSerializableNameBody[] = {
    {{"Serializable", NULL, {-1,0,0}, NULL}, "Serializable", TypeStruct, &s_javaIoSerializableNameBody[1]},
    {{"io",   NULL, {-1,0,0}, NULL}, "io", TypePackage, &s_javaIoSerializableNameBody[2]},
    {{"java",   NULL, {-1,0,0}, NULL}, "java", TypePackage, NULL},
};
IdList *s_javaIoSerializableName = s_javaIoSerializableNameBody;

static IdList s_javaLangClassNameBody[] = {
    {{"Class", NULL, {-1,0,0}, NULL}, "Class", TypeStruct, &s_javaLangClassNameBody[1]},
    {{"lang",   NULL, {-1,0,0}, NULL}, "lang", TypePackage, &s_javaLangClassNameBody[2]},
    {{"java",   NULL, {-1,0,0}, NULL}, "java", TypePackage, NULL},
};
IdList *s_javaLangClassName = s_javaLangClassNameBody;

static IdList s_javaLangObjectNameBody[] = {
    {{"Object", NULL, {-1,0,0}, NULL}, "Object", TypeStruct, &s_javaLangObjectNameBody[1]},
    {{"lang",   NULL, {-1,0,0}, NULL}, "lang", TypePackage, &s_javaLangObjectNameBody[2]},
    {{"java",   NULL, {-1,0,0}, NULL}, "java", TypePackage, NULL},
};
IdList *s_javaLangObjectName = s_javaLangObjectNameBody;
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

S_options s_opt;        // current options
S_options s_ropt;       // xref -refactory command line options

char *s_javaThisPackageName = "";

/* ************************************************************* */
/* ************* constant during the program ******************* */
/* *********** once, the file starts to be parsed ************** */
/* ************************************************************* */

FILE *cxOut;
FILE *fIn;
FILE *ccOut=NULL;
FILE *dumpOut=NULL;
FILE *errOut=NULL;

int s_cxResizingBlocked = 0;

int s_input_file_number = -1;
int s_olStringSecondProcessing=0;
int s_olOriginalFileNumber = -1;
int s_olOriginalComFileNumber = -1;
time_t s_expiration;

TypeModifier s_defaultIntModifier;
Symbol s_defaultIntDefinition;
TypeModifier s_defaultPackedTypeModifier;
TypeModifier s_defaultVoidModifier;
Symbol s_defaultVoidDefinition;
TypeModifier s_errorModifier;
Symbol s_errorSymbol;
struct stat s_noStat;
Position s_noPos = {-1, 0, 0};

Symbol s_javaArrayObjectSymbol;
Symbol *s_javaStringSymbol;
Symbol *s_javaCloneableSymbol;
Symbol *s_javaIoSerializableSymbol;
Symbol *s_javaObjectSymbol;
TypeModifier s_javaStringModifier;
TypeModifier s_javaClassModifier;
TypeModifier s_javaObjectModifier;

char s_cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
char s_base[MAX_FILE_NAME_SIZE];

char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

jmp_buf cxmemOverflow;

S_options s_cachedOptions;

/* ********************************************************************** */
/*                            real constants                              */
/* ********************************************************************** */

int s_javaRequiredeAccessibilitiesTable[MAX_REQUIRED_ACCESS+1] = {
    ACCESS_PUBLIC,
    ACCESS_PROTECTED,
    ACCESS_DEFAULT,
    ACCESS_PRIVATE,
};

S_options s_initOpt = {
    MULE_DEFAULT,       // encoding
    0,                  // completeParenthesis
    NID_IMPORT_ON_DEMAND,   // defaultAddImportStrategy
    0,                  // referenceListWithoutSource
    0,                  // jeditOldCompletions;     // to be removed
    1,                  // completionOverloadWizardDeep
    0,                  // exit
    0,                  // comment moving level
    NULL,               // prune name
    NULL,               // input files
    RC_ZERO,            // continue refactoring
    0,                  // completion case sensitive
    NULL,               // xrefrc
    NO_EOL_CONVERSION,  // crlfConversion
    NULL,               // checkVersion
    CUT_OUTERS,         // nestedClassDisplaying
    NULL,               // pushName
    0,                  // parnum2
    "",                 // refactoring parameter 1
    "",                 // refactoring parameter 2
    0,                  // refactoring
    false,              // briefoutput
    0,                  // cacheIncludes
    0,                  // stdopFlag
    NULL,               // renameTo
    RegimeUndefined,    // refactoringRegime
    0,                  // xrefactory-II
    NULL,               // moveTargetFile
#if defined (__WIN32__)                                 /*SBD*/
    "c;C",              // cFilesSuffixes
    "java;JAV",         // javaFilesSuffixes
    "C;cpp;CC;cc",      // c++FilesSuffixes
#else                                                   /*SBD*/
    "c:C",              // cFilesSuffixes
    "java",             // javaFilesSuffixes
    "C:cpp:CC:cc",      // c++FilesSuffixes
#endif                                                  /*SBD*/
    1,                  // fileNamesCaseSensitive
    ":",                // htmlLineNumLabel
    0,                  // html cut suffix
    TSS_FULL_SEARCH,    // search Tag file specifics
    JAVA_VERSION_1_3,   // java version
    "",                 // windel file
    0,                  // following is windel line:col x line-col
    0,
    0,
    0,
    "nouser",   // moveToUser
    0,          // noerrors
    0,          // fqtNameToCompletions
    NULL,       // moveTargetClass
    0,          // TPC_NON, trivial pre-check
    1,          // urlGenTemporaryFile
    1,          // urlautoredirect
    0,          // javafilesonly
    0,          // exact position
    NULL,       // -o outputFileName
    NULL,       // -line lineFileName
    NULL,       // -I include dirs
    DEFAULT_CXREF_FILE,     // -refs

    NULL,   // file move for safety check
    NULL,
    0,                  // first moved line
    MAXIMAL_INT,        // safety check number of lines moved
    0,                  // new line number of the first line

    "",     // getValue
    false,  // java2html
    1,      // javaSlAllowed (autoUpdateFromSrc)
    XFILE_HASH_DEFAULT,
    NULL,
    80,     // -htmlcxlinelen
    "java.applet:java.awt:java.beans:java.io:java.lang:java.math:java.net:java.rmi:java.security:java.sql:java.text:java.util:javax.accessibility:javax.swing:org.omg.CORBA:org.omg.CosNaming",     // -htmljavadocavailable
    0,
    NULL,       // "http://java.sun.com/j2se/1.3/docs/api",
    "",         // javaDocPath
    0,          // allowPackagesOnCl
    NULL,       // sourcepath
    false,      // htmlDirectX
    "/tmp",     // jdocTmpDir
    0,          // noCxFile
    false,      // javaDoc
    false,      // noIncludeRefs
    true,       // allowClassFileRefs
    0,
    "",
    DEFAULT_VALUE,      // manual symbol resolution
    NULL,       // browsed symbol name
    true,       // modifiedFlag
    0,
    false,                      // htmlNoUnderline
    "navy",     // htmlLinkColor
    "",         // htmlCutPath
    0,          // htmlCutPathLen
    (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE), // ooChecksBits
    0,
    0,
    1,          // cxMemoryFactor
    1,
    0,
    NULL,
    false,                      // updateOnlyModifiedFiles
    "0:3",      // olcxlccursor
    NULL,       /* htmlZipCommand */
    "",         /* olcxSearchString */
    79,         /* olineLen */
    "",         /* htmlLinkSuffix */
    "*_",       /* olExtractAddrParPrefix */
    0,          // extractMode, must be zero
    false,      /* htmlFunSeparate */
    //& "__newFunction__",  // extractName
    MAX_COMPLETIONS,
    0,
    0,
    "",
    true,                       // recursively dirs
    "",                         // default classpath
    8,                          /* tabulator */
    "",                     // -htmlroot
    false,                  /* htmlglobalx */
    false,                  /* htmllocalx */
    -1,
    -1,
    RegimeXref,
    "nouser",           // -user
    false,              /* debug */
    false,              /* trace */
    true,               /* cpp_comment */
    0,
    0,
    0,
    "",

    false,                      /* no_ref_locals */
    false,                      /* no_ref_records */
    false,                      /* no_ref_enum */
    false,                      /* no_ref_typedef */
    false,                      /* no_ref_macro */
    false,                      /* no_stdop */

    /* GENERATE options */

    0,                          /* enum_name */
    0,                          /* body */
    0,                          /* header */

    /* CXREF options  */

    0,                          /* err */
    true,                       /* brief_cxref */
    0,                          /* update */
    0,                          /* keep_old */
    NULL,                       /* last_message */
    0,                          /* refnum */

    // all the rest is initialized to zeros
    {0, },      // get/set end
    {0, },      // -cutpaths

    // pending memory for string values
    NULL,                       /* allAllocatedStrings */
    {0, },                      /* pendingMemory */
    {0, },                      /* pendingFreeSpace[] */
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
    {1,     1,    0,   1,  1,   1,    1},   /* Byte, */
    {0,     1,    0,   1,  1,   1,    1},   /* Short, */
    {0,     0,    1,   1,  1,   1,    1},   /* Char, */
    {0,     0,    0,   1,  1,   1,    1},   /* Int, */
    {0,     0,    0,   0,  1,   1,    1},   /* Long, */
    {0,     0,    0,   0,  0,   1,    1},   /* Float, */
    {0,     0,    0,   0,  0,   0,    1},   /* Double, */
};

char *s_tokenName[LAST_TOKEN];
int s_tokenLength[LAST_TOKEN];

TypeModifier * s_preCreatedTypesTable[MAX_TYPE];
TypeModifier * s_preCrPtr1TypesTab[MAX_TYPE];
TypeModifier * s_preCrPtr2TypesTab[MAX_TYPE];

/* *********************************************************************** */
/* ********************* TABLES TO INIT TABLES *************************** */
/* *********************************************************************** */

S_tokenNameIni s_tokenNameIniTab[] = {
    {"asm",         ASM_KEYWORD     ,LANG_C | LANG_YACC},
    {"auto",        AUTO            ,LANG_C | LANG_YACC},
    {"enum",        ENUM            ,LANG_C | LANG_YACC},
    {"extern",      EXTERN          ,LANG_C | LANG_YACC},
    {"inline",      INLINE          ,LANG_C | LANG_YACC},
    {"register",    REGISTER        ,LANG_C | LANG_YACC},
    {"signed",      SIGNED          ,LANG_C | LANG_YACC},
    {"sizeof",      SIZEOF          ,LANG_C | LANG_YACC},
    {"struct",      STRUCT          ,LANG_C | LANG_YACC},
    {"typedef",     TYPEDEF         ,LANG_C | LANG_YACC},
    {"union",       UNION           ,LANG_C | LANG_YACC},
    {"unsigned",    UNSIGNED        ,LANG_C | LANG_YACC},

    {"abstract",    ABSTRACT        ,LANG_JAVA},
    {"boolean",     BOOLEAN         ,LANG_JAVA},
    {"byte",        BYTE            ,LANG_JAVA},
    {"catch",       CATCH           ,LANG_JAVA},
    {"class",       CLASS           ,LANG_JAVA},
    {"extends",     EXTENDS         ,LANG_JAVA},
    {"final",       FINAL           ,LANG_JAVA},
    {"finally",     FINALLY         ,LANG_JAVA},
    {"implements",  IMPLEMENTS      ,LANG_JAVA},
    {"import",      IMPORT          ,LANG_JAVA},
    {"instanceof",  INSTANCEOF      ,LANG_JAVA},
    {"interface",   INTERFACE       ,LANG_JAVA},
    {"native",      NATIVE          ,LANG_JAVA},
    {"new",         NEW             ,LANG_JAVA},
    {"package",     PACKAGE         ,LANG_JAVA},
    {"private",     PRIVATE         ,LANG_JAVA},
    {"protected",   PROTECTED       ,LANG_JAVA},
    {"public",      PUBLIC          ,LANG_JAVA},
    {"super",       SUPER           ,LANG_JAVA},

    {"synchronized",SYNCHRONIZED    ,LANG_JAVA},
    {"strictfp",    STRICTFP        ,LANG_JAVA},
    {"this",        THIS            ,LANG_JAVA},
    {"throw",       THROW           ,LANG_JAVA},
    {"throws",      THROWS          ,LANG_JAVA},
    {"transient",   TRANSIENT       ,LANG_JAVA},
    {"try",         TRY             ,LANG_JAVA},

    {"true",        TRUE_LITERAL    ,LANG_JAVA},
    {"false",       FALSE_LITERAL   ,LANG_JAVA},
    {"null",        NULL_LITERAL    ,LANG_JAVA},

    {"bool",        BOOL            ,LANG_C},

    {"const_cast",          CONST_CAST      ,},
    {"delete",              DELETE          ,},
    {"dynamic_cast",        DYNAMIC_CAST    ,},
    {"explicit",            EXPLICIT        ,},
    {"friend",              FRIEND          ,},
    {"mutable",             MUTABLE         ,},
    {"namespace",           NAMESPACE       ,},
    {"operator",            OPERATOR        ,},
    {"reinterpret_cast",    REINTERPRET_CAST,},
    {"static_cast",         STATIC_CAST     ,},
    {"template",            TEMPLATE        ,},
    {"typeid",              TYPEID          ,},
    {"typename",            TYPENAME        ,},
    {"using",               USING           ,},
    {"virtual",             VIRTUAL         ,},
    {"wchar_t",             WCHAR_T         ,},

    {"break",       BREAK           ,LANG_C | LANG_YACC | LANG_JAVA},
    {"case",        CASE            ,LANG_C | LANG_YACC | LANG_JAVA},
    {"char",        CHAR            ,LANG_C | LANG_YACC | LANG_JAVA},
    {"const",       CONST           ,LANG_C | LANG_YACC | LANG_JAVA},
    {"continue",    CONTINUE        ,LANG_C | LANG_YACC | LANG_JAVA},
    {"default",     DEFAULT         ,LANG_C | LANG_YACC | LANG_JAVA},
    {"do",          DO              ,LANG_C | LANG_YACC | LANG_JAVA},
    {"double",      DOUBLE          ,LANG_C | LANG_YACC | LANG_JAVA},
    {"else",        ELSE            ,LANG_C | LANG_YACC | LANG_JAVA},
    {"float",       FLOAT           ,LANG_C | LANG_YACC | LANG_JAVA},
    {"for",         FOR             ,LANG_C | LANG_YACC | LANG_JAVA},
    {"goto",        GOTO            ,LANG_C | LANG_YACC | LANG_JAVA},
    {"if",          IF              ,LANG_C | LANG_YACC | LANG_JAVA},
    {"int",         INT             ,LANG_C | LANG_YACC | LANG_JAVA},
    {"long",        LONG            ,LANG_C | LANG_YACC | LANG_JAVA},
    {"return",      RETURN          ,LANG_C | LANG_YACC | LANG_JAVA},
    {"short",       SHORT           ,LANG_C | LANG_YACC | LANG_JAVA},
    {"static",      STATIC          ,LANG_C | LANG_YACC | LANG_JAVA},
    {"switch",      SWITCH          ,LANG_C | LANG_YACC | LANG_JAVA},
    {"void",        VOID            ,LANG_C | LANG_YACC | LANG_JAVA},
    {"volatile",    VOLATILE        ,LANG_C | LANG_YACC | LANG_JAVA},
    {"while",       WHILE           ,LANG_C | LANG_YACC | LANG_JAVA},

    {"restrict",        RESTRICT        ,LANG_C},
    {"_Atomic",         _ATOMIC         ,LANG_C},
    {"_Bool",           _BOOL           ,LANG_C},
    {"_Noreturn",       _NORETURN       ,LANG_C},
    {"_Thread_local",   _THREADLOCAL    ,LANG_C},

    /*
      {"token",       TOKEN           ,LAN_YACC},
      {"type",        TYPE            ,LAN_YACC},
    */

    {">>>=",        URIGHT_ASSIGN   ,LANG_JAVA},
    {">>>",         URIGHT_OP       ,LANG_JAVA},
    {"...",         ELIPSIS         ,LANG_C},
    {">>=",         RIGHT_ASSIGN    ,LANG_C},
    {"<<=",         LEFT_ASSIGN     ,LANG_C},
    {"+=",      ADD_ASSIGN          ,LANG_C},
    {"-=",      SUB_ASSIGN          ,LANG_C},
    {"*=",      MUL_ASSIGN          ,LANG_C},
    {"/=",      DIV_ASSIGN          ,LANG_C},
    {"%=",      MOD_ASSIGN          ,LANG_C},
    {"&=",      AND_ASSIGN          ,LANG_C},
    {"^=",      XOR_ASSIGN          ,LANG_C},
    {"|=",      OR_ASSIGN           ,LANG_C},
    {">>",      RIGHT_OP            ,LANG_C},
    {"<<",      LEFT_OP             ,LANG_C},
    {"++",      INC_OP              ,LANG_C},
    {"--",      DEC_OP              ,LANG_C},
    {"->",      PTR_OP      ,LANG_C},
    {"->*",     PTRM_OP     ,},
    {".*",      POINTM_OP   ,},
    {"::",      DPOINT      ,},
    {"&&",      AND_OP      ,LANG_C},
    {"||",      OR_OP       ,LANG_C},
    {"<=",      LE_OP       ,LANG_C},
    {">=",      GE_OP       ,LANG_C},
    {"==",      EQ_OP       ,LANG_C},
    {"!=",      NE_OP       ,LANG_C},
    {";",       ';'     ,LANG_C},
    {"{",       '{'     ,LANG_C},
    {"}",       '}'     ,LANG_C},
    {",",       ','     ,LANG_C},
    {":",       ':'     ,LANG_C},
    {"=",       '='     ,LANG_C},
    {"(",       '('     ,LANG_C},
    {")",       ')'     ,LANG_C},
    {"[",       '['     ,LANG_C},
    {"]",       ']'     ,LANG_C},
    {".",       '.'     ,LANG_C},
    {"&",       '&'     ,LANG_C},
    {"!",       '!'     ,LANG_C},
    {"~",       '~'     ,LANG_C},
    {"-",       '-'     ,LANG_C},
    {"+",       '+'     ,LANG_C},
    {"*",       '*'     ,LANG_C},
    {"/",       '/'     ,LANG_C},
    {"%",       '%'     ,LANG_C},
    {"<",       '<'     ,LANG_C},
    {">",       '>'     ,LANG_C},
    {"^",       '^'     ,LANG_C},
    {"|",       '|'     ,LANG_C},
    {"?",       '?'     ,LANG_C},


    {"'CONSTANT'",              CONSTANT            ,LANG_C},
    {"'CONSTANT'",              LONG_CONSTANT       ,LANG_C},
    {"'CONSTANT'",              FLOAT_CONSTANT      ,LANG_C},
    {"'CONSTANT'",              DOUBLE_CONSTANT     ,LANG_C},
    {"'STRING_LITERAL'",        STRING_LITERAL      ,LANG_C},
#ifdef DEBUG
    {"'IDENTIFIER'",            IDENTIFIER          ,LANG_C},
    {"'LINE_TOK'",              LINE_TOK            ,LANG_C},
    {"'BLOCK_MARKER'",          OL_MARKER_TOKEN     ,LANG_C},
    {"#INCLUDE",                CPP_INCLUDE     ,LANG_C},
    {"#DEFINE0",                CPP_DEFINE0     ,LANG_C},
    {"#DEFINE",                 CPP_DEFINE      ,LANG_C},
    {"#IFDEF",                  CPP_IFDEF       ,LANG_C},
    {"#IFNDEF",                 CPP_IFNDEF      ,LANG_C},
    {"#IF",                     CPP_IF          ,LANG_C},
    {"#ELSE",                   CPP_ELSE        ,LANG_C},
    {"#ENDIF",                  CPP_ENDIF       ,LANG_C},
    {"#PRAGMA",                 CPP_PRAGMA      ,LANG_C},
    {"#LINE",                   CPP_LINE        ,LANG_C},
    {"##",                      CPP_COLLATION   ,LANG_C},
#endif

    {NULL,      0       ,LANG_C}         /* sentinel*/
};

S_tokenNameIni s_tokenNameIniTab2[] = {
    {"__const",         CONST           ,LANG_C | LANG_YACC},
    {"__const__",       CONST           ,LANG_C | LANG_YACC},
    {"__signed",        SIGNED          ,LANG_C | LANG_YACC},
    {"__signed__",      SIGNED          ,LANG_C | LANG_YACC},
    {"__inline",        ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__inline__",      ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__volatile",      VOLATILE        ,LANG_C | LANG_YACC},
    {"__volatile__",    VOLATILE        ,LANG_C | LANG_YACC},
    {"__asm",           ASM_KEYWORD     ,LANG_C},
    {"__asm__",         ASM_KEYWORD     ,LANG_C},
    {"__label__",       LABEL           ,LANG_C},
    {"__near",          ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__far",           ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__pascal",        ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"_near",           ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"_far",            ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"_pascal",         ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"_const",          ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__near",          ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__far",           ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__pascal",        ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__cdecl",         ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__fastcall",      ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__stdcall",       ANONYME_MOD     ,LANG_C | LANG_YACC},
    {"__int8",          INT             ,LANG_C | LANG_YACC},
    {"__int16",         INT             ,LANG_C | LANG_YACC},
    {"__int32",         INT             ,LANG_C | LANG_YACC},
    {"__int64",         INT             ,LANG_C | LANG_YACC},
    {NULL,      0       ,LANG_C}         /* sentinel*/
};

S_tokenNameIni s_tokenNameIniTab3[] = {
    {"assert",      ASSERT          ,LANG_JAVA},
    {NULL,          0               ,LANG_C}         /* sentinel*/
};


S_typeCharCodeIni s_baseTypeCharCodesIniTab[] = {
    {TypeByte,      'B'},
    {TypeChar,      'C'},
    {TypeDouble,    'D'},
    {TypeFloat,     'F'},
    {TypeInt,       'I'},
    {TypeLong,      'J'},
    {TypeShort,     'S'},
    {TypeBoolean,   'Z'},
    {TypeVoid,      'V'},
    {TypeError,     'E'},
    {TypeNull,      JAVA_NULL_CODE},    /* this is just my in(con)vention */
    {-1,            ' '},
};

S_javaTypePCTIConvertIni s_javaTypePCTIConvertIniTab[] = {
    {TypeByte,      PCTIndexByte},
    {TypeShort,     PCTIndexShort},
    {TypeChar,      PCTIndexChar},
    {TypeInt,       PCTIndexInt},
    {TypeLong,      PCTIndexLong},
    {TypeFloat,     PCTIndexFloat},
    {TypeDouble,    PCTIndexDouble},
    {-1,-1},
};

char *s_extractStorageName[MAX_STORAGE];

S_int2StringTab s_extractStoragesNamesInitTab[] = {
    {StorageExtern,     "extern "},
    {StorageStatic,     "static "},
    {StorageRegister,   "register "},
    {StorageTypedef,    "typedef "},
    {-1, NULL}
};

S_int2StringTab s_typeNamesInitTab[] = {
    {TypeDefault ,          "Default"},
    {TypeChar ,             "char"},
    {TypeUnsignedChar ,     "unsigned char"},
    {TypeSignedChar ,       "signed char"},
    {TypeInt ,              "int"},
    {TypeUnsignedInt ,      "unsigned int"},
    {TypeSignedInt ,        "signed int"},
    {TypeShortInt ,         "short int"},
    {TypeShortUnsignedInt , "short unsigned int"},
    {TypeShortSignedInt ,   "short signed int"},
    {TypeLongInt ,          "long int"},
    {TypeLongUnsignedInt ,  "long unsigned int"},
    {TypeLongSignedInt ,    "long signed int"},
    {TypeFloat ,            "float"},
    {TypeDouble ,           "double"},
    {TypeStruct,            "struct"},
    {TypeUnion,             "union"},
    {TypeEnum ,             "enum"},
    {TypeVoid ,             "void"},
    {TypePointer,           "Pointer"},
    {TypeArray,             "Array"},
    {TypeFunction,          "Function"},
    {TypeAnonymousField,    "AnonymousField"},
    {TypeError,             "Error"},
    {TypeCppIfElse,         "#if-else-fi"},
    {TypeCppInclude,        "#include"},
    {TypeCppCollate,        "##"},

    {TmodLong,              "long"},
    {TmodShort,             "short"},
    {TmodSigned,            "signed"},
    {TmodUnsigned,          "unsigned"},
    {TmodShortSigned,       "short signed"},
    {TmodShortUnsigned,     "short unsigned"},
    {TmodLongSigned,        "long signed"},
    {TmodLongUnsigned,      "long unsigned"},

    {TypeElipsis,           "elipsis"},

    {TypeByte,              "byte"},
    {TypeShort,             "short"},
    {TypeLong,              "long"},
    {TypeBoolean,           "boolean"},
    {TypeNull,              "null"},
    {TypeOverloadedFunction,    "OverloadedFunction"},

    {TypeWchar_t,           "wchar_t"},

    {TypeLabel,             "label"},
    {TypeKeyword,           "keyword"},
    {TypeToken,             "token"},
    {TypeMacro,             "macro"},
    {TypeMacroArg,          "macro argument"},
    {TypeUndefMacro,        "Undefined Macro"},
    {TypePackage,           "package"},
    {TypeYaccSymbol,        "yacc symbol"},
    {TypeCppCollate,        "Cpp##sym"},
    {TypeSpecialComplet,    "(Completion Wizard)"},
    {TypeInheritedFullMethod,   "(Override Wizard)"},
    {TypeNonImportedClass,  "fully qualified name"},
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
    TypeAnonymousField ,
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
    -1,
};
