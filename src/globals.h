#ifndef GLOBALS_H
#define GLOBALS_H

#include "constants.h"
#include "proto.h"
#include "symtab.h"
#include "filetab.h"


extern char s_cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
extern char s_base[MAX_FILE_NAME_SIZE];
extern char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

extern char *s_debugSurveyPointer;
extern int s_fileAbortionEnabled;

#ifdef OLCX_MEMORY_CHECK
extern void *s_olcx_check_array[OLCX_CHECK_ARRAY_SIZE];
extern int s_olcx_check_array_sizes[OLCX_CHECK_ARRAY_SIZE];
extern int s_olcx_check_arrayi;
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
extern S_fileTab s_fileTab;

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

extern int olcxMemoryAllocatedBytes;


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
extern int macroStackIndex;

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

#endif
