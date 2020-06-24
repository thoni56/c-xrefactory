#include "globals.h"

#include "stdinc.h"
#include "head.h"
#include "proto.h"
#include "parsers.h"
#include "usage.h"

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

Counters counters;
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

char ppmMemory[SIZE_ppmMemory];
char mbMemory[SIZE_mbMemory];

int olcxMemoryAllocatedBytes;

bool s_ifEvaluation = false;     /* flag for yylex, to not filter '\n' TODO: move, duh!*/

Position s_olcxByPassPos;
Position s_cxRefPos;
int s_cxRefFlag=0;

time_t s_fileProcessStartTime;

Language s_language;
int s_currCppPass;
int s_cppPassMax;

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

S_options options;        // current options
S_options s_ropt;       // xref -refactory command line options

char *s_javaThisPackageName = "";

/* ************************************************************* */
/* ************* constant during the program ******************* */
/* *********** once, the file starts to be parsed ************** */
/* ************************************************************* */

FILE *cxOut;
FILE *inputFile;
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
    AccessPublic,
    AccessProtected,
    AccessDefault,
    AccessPrivate,
};

S_options s_initOpt = {
                       /* GENERAL */
    0,                  // exit
    "gcc",              // path to compiler to use for auto-discovering compiler and defines
    MULE_DEFAULT,       // encoding
    0,                  // completeParenthesis
    NID_IMPORT_ON_DEMAND,   // defaultAddImportStrategy
    0,                  // referenceListWithoutSource
    1,                  // completionOverloadWizardDeep
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
#if defined (__WIN32__)
    "c;C",              // cFilesSuffixes
    "java;JAV",         // javaFilesSuffixes
    "C;cpp;CC;cc",      // c++FilesSuffixes
#else
    "c:C",              // cFilesSuffixes
    "java",             // javaFilesSuffixes
    "C:cpp:CC:cc",      // c++FilesSuffixes
#endif
    1,                  // fileNamesCaseSensitive
    TSS_FULL_SEARCH,    // search Tag file specifics
    JAVA_VERSION_AUTO,  // java version
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

    /* HTML */
    "",                         // htmlroot
    NULL,                       // htmlLineNumColor
    80,                         // htmlcxlinelen
    "java.applet:java.awt:java.beans:java.io:java.lang:java.math:java.net:java.rmi:java.security:java.sql:java.text:java.util:javax.accessibility:javax.swing:org.omg.CORBA:org.omg.CosNaming",     // -htmljavadocavailable
    ":",                        // htmlLineNumLabel
    0,                          // html cut suffix
    0,                          // htmlGenJdkDocLinks
    NULL,                       // htmlJdkDocUrl - "http://java.sun.com/j2se/1.3/docs/api",
    false,                      // htmlDirectX
    "",                         // htmlLinkSuffix
    false,                      // htmlNoUnderline
    "navy",                     // htmlLinkColor
    "",                         // htmlCutPath
    0,                          // htmlCutPathLen
    0,                          /* htmlLineNums */
    false,                      /* htmlNoColors */
    NULL,                       /* htmlZipCommand */
    false,                      /* htmlFunSeparate */
    false,                      /* htmlglobalx */
    false,                      /* htmllocalx */
    {0, },                      // -cutpaths

    /* JAVA: */
    "",                         // javaDocPath
    0,          // allowPackagesOnCl
    NULL,       // sourcepath
    "/tmp",     // jdocTmpDir

    /* MIXED THINGS... */
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
    (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE), // ooChecksBits
    1,          // cxMemoryFactor
    1,
    0,
    NULL,
    "0:3",      // olcxlccursor
    "",         /* olcxSearchString */
    79,         /* olineLen */
    "*_",       /* olExtractAddrParPrefix */
    0,          // extractMode, must be zero
    //& "__newFunction__",  // extractName
    MAX_COMPLETIONS,
    0,
    0,
    "",
    true,                       // recursively dirs
    "",                         // default classpath
    8,                          /* tabulator */
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

    /* CXREF options  */
    0,                          /* show err */
    true,                       /* brief_cxref */
    UP_CREATE,                  /* create, update */
    false,                      // updateOnlyModifiedFiles
    NULL,                       /* last_message */
    0,                          /* refnum */

    // all the rest is initialized to zeros
    {0, },                      // get/set end

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
    {-1,			-1},
};

char *s_extractStorageName[MAX_STORAGE];

Int2StringTable s_extractStoragesNamesInitTab[] = {
    {StorageExtern,     "extern "},
    {StorageStatic,     "static "},
    {StorageRegister,   "register "},
    {StorageTypedef,    "typedef "},
    {-1,                NULL}
};

Int2StringTable typeNamesInitTable[] = {
    {TypeDefault,           "Default"},
    {TypeChar,              "char"},
    {TypeUnsignedChar,      "unsigned char"},
    {TypeSignedChar,        "signed char"},
    {TypeInt,               "int"},
    {TypeUnsignedInt,       "unsigned int"},
    {TypeSignedInt,         "signed int"},
    {TypeShortInt,          "short int"},
    {TypeShortUnsignedInt,  "short unsigned int"},
    {TypeShortSignedInt,    "short signed int"},
    {TypeLongInt,           "long int"},
    {TypeLongUnsignedInt,   "long unsigned int"},
    {TypeLongSignedInt,     "long signed int"},
    {TypeFloat,             "float"},
    {TypeDouble,            "double"},
    {TypeStruct,            "struct"},
    {TypeUnion,             "union"},
    {TypeEnum,              "enum"},
    {TypeVoid,              "void"},
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
    {-1,					NULL}
};

int preCreatedTypesInitTable[] = {
    TypeDefault,
    TypeChar,
    TypeUnsignedChar,
    TypeSignedChar,
    TypeInt,
    TypeUnsignedInt,
    TypeSignedInt,
    TypeShortInt,
    TypeShortUnsignedInt,
    TypeShortSignedInt,
    TypeLongInt,
    TypeLongUnsignedInt,
    TypeLongSignedInt,
    TypeFloat,
    TypeDouble,
    TypeVoid,
    TypeError,
    TypeAnonymousField,
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
