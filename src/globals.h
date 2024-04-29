#ifndef GLOBALS_H_INCLUDED
#define GLOBALS_H_INCLUDED

#include "constants.h"
#include "completion.h"
#include "symboltable.h"
#include "stringlist.h"
#include "proto.h"
#include "usage.h"


extern char cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
extern char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

extern bool fileAbortEnabled;

extern int s_lastReturnedLexem;
extern Position parsedPositions[SPP_MAX];

extern bool cxResizingBlocked;
extern unsigned s_recFindCl;

extern FILE *errOut;


/* Variables for capturing parameter positions */
extern bool parameterListIsVoid; /* Is what looks like a parameter declaration actually "void" */
extern int parameterCount;
extern Position parameterPosition;
extern Position parameterBeginPosition;
extern Position parameterEndPosition;

extern Position s_primaryStartPosition;
extern Position s_staticPrefixStartPosition;
extern Id yyIdBuffer[YYIDBUFFER_SIZE];
extern int yyIdBufferIndex;

extern char *s_cppVarArgsName;
extern StringList *javaClassPaths;
extern char *javaSourcePaths;
extern IdList *s_javaLangName;
extern IdList *s_javaLangStringName;
extern IdList *s_javaLangCloneableName;
extern IdList *s_javaIoSerializableName;
extern IdList *s_javaLangClassName;
extern IdList *s_javaLangObjectName;

extern FILE *communicationChannel;

extern bool javaPreScanOnly;

extern CurrentlyParsedClassInfo parsedClassInfo;
extern CurrentlyParsedClassInfo parsedClassInfoInit;
extern CurrentlyParsedInfo parsedInfo;

extern TypeModifier defaultIntModifier;
extern Symbol defaultIntDefinition;
extern TypeModifier defaultPackedTypeModifier;
extern TypeModifier defaultVoidModifier;
extern Symbol defaultVoidDefinition;
extern TypeModifier errorModifier;
extern Symbol errorSymbol;
extern Position noPosition;

extern uchar typeLongChange[MAX_TYPE];
extern uchar typeShortChange[MAX_TYPE];
extern uchar typeSignedChange[MAX_TYPE];
extern uchar typeUnsignedChange[MAX_TYPE];

extern TypeModifier *s_structRecordCompletionType;
extern TypeModifier *s_upLevelFunctionCompletionType;
extern char *tokenNamesTable[];
extern int tokenNameLengthsTable[];

extern TypeModifier *preCreatedTypesTable[MAX_TYPE];
extern TypeModifier *preCreatedPtr2TypeTable[MAX_TYPE];
extern TypeModifier *preCreatedPtr2Ptr2TypeTable[MAX_TYPE];

extern int javaCharCodeBaseTypes[MAX_CHARS];

extern Position s_olcxByPassPos;
extern Position cxRefPosition;

extern int inputFileNumber;
extern int olOriginalFileNumber;     /* number of original file */
extern int olOriginalComFileNumber;  /* number of original communication file */

extern char *storageNamesTable[MAX_STORAGE_NAMES];

extern jmp_buf cxmemOverflow;

extern char *inputFileName;

extern time_t fileProcessingStartTime;


#define LANGUAGE(langBit) ((currentLanguage & (langBit)) != 0)

extern Language currentLanguage;
extern int currentPass;
extern int maxPasses;

extern Completions collectedCompletions;
extern unsigned s_menuFilterOoBits[MAX_MENU_FILTER_LEVEL];
extern int s_refListFilters[MAX_REF_LIST_FILTER_LEVEL];

/* ********* vars for on-line additions after EOF ****** */

extern bool olstringFound;
extern bool olstringServed;
extern UsageKind olstringUsageKind;
extern char *olstringInMacroBody;
extern int s_olMacro2PassFile;

/* **************** variables due to cpp **************** */

extern char *s_javaThisPackageName;

#endif
