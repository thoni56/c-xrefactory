#ifndef GLOBALS_H_INCLUDED
#define GLOBALS_H_INCLUDED

#include "constants.h"
#include "symboltable.h"
#include "stringlist.h"
#include "proto.h"


/* ***************** unique counters  *********************** */
typedef struct counters {
    int localSym;
    int localVar;
    int anonymousClassCounter;
} Counters;


extern char cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
extern char s_base[MAX_FILE_NAME_SIZE];
extern char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

extern bool fileAbortEnabled;

extern int s_lastReturnedLexem;
extern Position s_spp[SPP_MAX];

// !!! if changing this, change also s_noRef!!!
#define NO_USAGE (Usage){UsageNone, 0}

extern int progressFactor;
extern int progressOffset;

extern int      cxResizingBlocked;
extern Counters counters;
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
extern Id yyIdBuffer[YYIDBUFFER_SIZE];
extern int yyIdBufferIndex;

extern char *s_cppVarArgsName;
extern char defaultClassPath[];
extern Id javaAnonymousClassName;
extern StringList *javaClassPaths;
extern char *javaSourcePaths;
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

extern FILE *communicationChannel;

extern int s_javaPreScanOnly;

extern S_currentlyParsedCl s_cp;
extern S_currentlyParsedCl s_cpInit;
extern S_currentlyParsedStatics s_cps;
extern S_currentlyParsedStatics s_cpsInit;

extern TypeModifier defaultIntModifier;
extern Symbol s_defaultIntDefinition;
extern TypeModifier s_defaultPackedTypeModifier;
extern TypeModifier s_defaultVoidModifier;
extern Symbol s_defaultVoidDefinition;
extern TypeModifier s_errorModifier;
extern Symbol s_errorSymbol;
extern Position noPosition;

extern uchar typeLongChange[MAX_TYPE];
extern uchar typeShortChange[MAX_TYPE];
extern uchar typeSignedChange[MAX_TYPE];
extern uchar typeUnsignedChange[MAX_TYPE];

extern TypeModifier *s_structRecordCompletionType;
extern TypeModifier *s_upLevelFunctionCompletionType;
extern ExprTokenType s_forCompletionType;
extern TypeModifier *s_javaCompletionLastPrimary;
extern char *tokenNamesTable[];
extern int tokenNameLengthsTable[];
extern TypeModifier * s_preCreatedTypesTable[MAX_TYPE];
extern TypeModifier * s_preCrPtr1TypesTab[MAX_TYPE];
extern TypeModifier * s_preCrPtr2TypesTab[MAX_TYPE];

extern char javaBaseTypeCharCodes[MAX_TYPE];
extern int javaCharCodeBaseTypes[MAX_CHARS];
extern char javaTypePCTIConvert[MAX_TYPE];
extern char s_javaPrimitiveWideningConversions[MAX_PCTIndex-1][MAX_PCTIndex-1];

extern Position s_olcxByPassPos;
extern Position s_cxRefPos;
extern int s_cxRefFlag;

extern FILE *inputFile;

extern int inputFileNumber;
extern int olOriginalFileIndex;     /* number of original file */
extern int olOriginalComFileNumber;  /* number of original communication file */
extern int olStringSecondProcessing; /* am I in macro body pass ? */

extern char *storageNamesTable[MAX_STORAGE_NAMES];

extern char *s_editCommunicationString;

extern jmp_buf cxmemOverflow;

extern char *inputFilename;

extern time_t fileProcessingStartTime;

extern Language currentLanguage;
extern int currentPass;
extern int maxPasses;

extern Completions s_completions;
extern unsigned s_menuFilterOoBits[MAX_MENU_FILTER_LEVEL];
extern int s_refListFilters[MAX_REF_LIST_FILTER_LEVEL];

/* ********* vars for on-line additions after EOF ****** */

extern char s_olstring[MAX_FUN_NAME_SIZE];
extern bool s_olstringFound;
extern bool s_olstringServed;
extern int s_olstringUsage;
extern char *s_olstringInMbody;
extern int s_olMacro2PassFile;

/* **************** variables due to cpp **************** */

extern Access javaRequiredAccessibilityTable[MAX_REQUIRED_ACCESS+1];

extern char *s_javaThisPackageName;

#endif
