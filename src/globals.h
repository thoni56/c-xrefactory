#ifndef GLOBALS_H
#define GLOBALS_H

#include "constants.h"
#include "symboltable.h"
#include "filetab.h"


typedef struct int2StringTab {
    int     i;
    char    *s;
} S_int2StringTab;

typedef struct stringList {
    char *d;
    struct stringList *next;
} S_stringList;

typedef struct tokenNameIni {
    char        *name;
    int         token;
    unsigned    languages;
} S_tokenNameIni;


extern char s_cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
extern char s_base[MAX_FILE_NAME_SIZE];
extern char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

extern int s_fileAbortionEnabled;

#ifdef OLCX_MEMORY_CHECK
extern void *s_olcx_check_array[OLCX_CHECK_ARRAY_SIZE];
extern int s_olcx_check_array_sizes[OLCX_CHECK_ARRAY_SIZE];
extern int s_olcx_check_arrayi;
#endif
extern bool s_wildcardSearch;
extern int s_lastReturnedLexem;
extern Position s_spp[SPP_MAX];
extern UsageBits s_noUsage;

extern int s_progressFactor;
extern int s_progressOffset;

extern z_stream s_defaultZStream;

extern time_t s_expTime;

extern int s_cxResizingBlocked;
extern S_counters s_count;
extern unsigned s_recFindCl;

extern FILE *errOut;
extern FILE *dumpOut;

extern char s_olSymbolType[COMPLETION_STRING_SIZE];
extern char s_olSymbolClassType[COMPLETION_STRING_SIZE];
extern Position s_paramPosition;
extern Position s_paramBeginPosition;
extern Position s_paramEndPosition;
extern Position s_primaryStartPosition;
extern Position s_staticPrefixStartPosition;
extern Id s_yyIdentBuf[YYBUFFERED_ID_INDEX];
extern int s_yyIdentBufi;

extern char *s_cppVarArgsName;
extern char s_defaultClassPath[];
extern Id s_javaAnonymousClassName;
extern Id s_javaConstructorName;
extern S_stringList *s_javaClassPaths;
extern char *s_javaSourcePaths;
extern IdList *s_javaDefaultPackageName;
extern IdList *s_javaLangName;
extern IdList *s_javaLangStringName;
extern IdList *s_javaLangCloneableName;
extern IdList *s_javaIoSerializableName;
extern IdList *s_javaLangClassName;
extern IdList *s_javaLangObjectName;
extern char *s_javaLangObjectLinkName;

extern Symbol s_javaArrayObjectSymbol;
extern Symbol *s_javaStringSymbol;
extern Symbol *s_javaObjectSymbol;
extern Symbol *s_javaCloneableSymbol;
extern Symbol *s_javaIoSerializableSymbol;
extern TypeModifier s_javaStringModifier;
extern TypeModifier s_javaClassModifier;
extern TypeModifier s_javaObjectModifier;

extern FILE *cxOut;
extern FILE *ccOut;

extern int s_javaPreScanOnly;

extern S_currentlyParsedCl s_cp;
extern S_currentlyParsedCl s_cpInit;
extern S_currentlyParsedStatics s_cps;
extern S_currentlyParsedStatics s_cpsInit;

extern S_fileTab s_fileTab;

extern TypeModifier s_defaultIntModifier;
extern Symbol s_defaultIntDefinition;
extern TypeModifier s_defaultPackedTypeModifier;
extern TypeModifier s_defaultVoidModifier;
extern Symbol s_defaultVoidDefinition;
extern TypeModifier s_errorModifier;
extern Symbol s_errorSymbol;
extern struct stat s_noStat;
extern Position s_noPos;
extern Reference s_noRef;

extern uchar typeLongChange[MAX_TYPE];
extern uchar typeShortChange[MAX_TYPE];
extern uchar typeSignedChange[MAX_TYPE];
extern uchar typeUnsignedChange[MAX_TYPE];

extern TypeModifier *s_structRecordCompletionType;
extern TypeModifier *s_upLevelFunctionCompletionType;
extern S_exprTokenType s_forCompletionType;
extern TypeModifier *s_javaCompletionLastPrimary;
extern char *s_tokenName[];
extern int s_tokenLength[];
extern S_tokenNameIni s_tokenNameIniTab[];
extern S_tokenNameIni s_tokenNameIniTab2[];
extern S_tokenNameIni s_tokenNameIniTab3[];
extern int s_preCrTypesIniTab[];
extern TypeModifier * s_preCreatedTypesTable[MAX_TYPE];
extern TypeModifier * s_preCrPtr1TypesTab[MAX_TYPE];
extern TypeModifier * s_preCrPtr2TypesTab[MAX_TYPE];

extern char s_javaBaseTypeCharCodes[MAX_TYPE];
extern int s_javaCharCodeBaseTypes[MAX_CHARS];
extern char s_javaTypePCTIConvert[MAX_TYPE];
extern S_javaTypePCTIConvertIni s_javaTypePCTIConvertIniTab[];
extern char s_javaPrimitiveWideningConversions[MAX_PCTIndex-1][MAX_PCTIndex-1];

extern S_typeCharCodeIni s_baseTypeCharCodesIniTab[];

extern Position s_olcxByPassPos;
extern Position s_cxRefPos;
extern int s_cxRefFlag;

extern FILE *inputFile;

extern int s_input_file_number;
extern int s_olStringSecondProcessing;  /* am I in macro body pass ? */
extern int s_olOriginalFileNumber;      /* original file name */
extern int s_olOriginalComFileNumber;	/* original communication file */
extern int s_noneFileIndex;

extern S_int2StringTab s_typeNamesInitTab[];
extern char *s_extractStorageName[MAX_STORAGE];
extern S_int2StringTab s_extractStoragesNamesInitTab[];

extern char *s_editCommunicationString;

extern time_t s_expiration;

extern char tmpMemory[SIZE_TMP_MEM];
extern char ppmMemory[SIZE_ppmMemory];
extern int ppmMemoryi;

extern int olcxMemoryAllocatedBytes;


extern jmp_buf cxmemOverflow;

extern char *s_input_file_name;

extern time_t s_fileProcessStartTime;

extern Language s_language;
extern int s_currCppPass;
extern int s_cppPassMax;

extern Completions s_completions;
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

extern bool s_ifEvaluation;		/* flag for yylex, to not filter '\n' */

extern S_options s_opt;			// current options
extern S_options s_ropt;		// xref -refactory command line options
extern S_options s_cachedOptions;
extern int s_javaRequiredeAccessibilitiesTable[MAX_REQUIRED_ACCESS+1];
extern S_options s_initOpt;

extern char *s_javaThisPackageName;

#endif
