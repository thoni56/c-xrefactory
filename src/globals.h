#ifndef GLOBALS_H_INCLUDED
#define GLOBALS_H_INCLUDED

#include <stdio.h>
#include <setjmp.h>
#include "constants.h"
#include "completion.h"
#include "head.h"
#include "proto.h"
#include "usage.h"


typedef enum {
    IPP_FUNCTION_BEGIN,
    IPP_FUNCTION_END,
    IPP_MAX
} ImportantParsedPositionKind;

extern Position parsedPositions[IPP_MAX];


extern char cwd[MAX_FILE_NAME_SIZE]; /* current working directory */
extern char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

extern bool fileAbortEnabled;

extern bool cxResizingBlocked;
extern unsigned memberFindCount;

extern FILE *errOut;


/* Variables for capturing parameter positions */
extern bool parameterListIsVoid; /* Is what looks like a parameter declaration actually "void" */
extern int parameterCount;
extern Position parameterPosition;
extern Position parameterBeginPosition;
extern Position parameterEndPosition;

extern Position primaryStartPosition;
extern Position staticPrefixStartPosition;
extern Id yyIdBuffer[YYIDBUFFER_SIZE];
extern int yyIdBufferIndex;

extern char *cppVarArgsName;
extern char *javaSourcePaths;

extern FILE *communicationChannel;

extern ParsedInfo parsedInfo;

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

extern TypeModifier *structMemberCompletionType;
extern TypeModifier *upLevelFunctionCompletionType;
extern char *tokenNamesTable[];
extern int tokenNameLengthsTable[];

extern TypeModifier *preCreatedTypesTable[MAX_TYPE];
extern TypeModifier *preCreatedPtr2TypeTable[MAX_TYPE];
extern TypeModifier *preCreatedPtr2Ptr2TypeTable[MAX_TYPE];

extern int javaCharCodeBaseTypes[MAX_CHARS];

extern Position olcxByPassPos;
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
extern unsigned menuFilterOoBits[MAX_MENU_FILTER_LEVEL];
extern int refListFilters[MAX_REF_LIST_FILTER_LEVEL];

/* ********* vars for on-line additions after EOF ****** */

extern bool olstringFound;
extern bool olstringServed;
extern Usage olstringUsage;
extern char *olstringInMacroBody;
extern int olMacro2PassFile;

#endif
