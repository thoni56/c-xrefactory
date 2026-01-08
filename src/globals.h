#ifndef GLOBALS_H_INCLUDED
#define GLOBALS_H_INCLUDED

#include <stdio.h>
#include <setjmp.h>
#include "constants.h"
#include "completion.h"

#include "head.h"
#include "id.h"
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

extern FILE *outputFile;

extern ParsedInfo parsedInfo;

extern TypeModifier defaultIntModifier;
extern Symbol defaultIntDefinition;
extern TypeModifier defaultPackedTypeModifier;
extern TypeModifier defaultVoidModifier;
extern Symbol defaultVoidDefinition;
extern TypeModifier errorModifier;
extern Symbol errorSymbol;

extern uchar typeLongChange[MAX_TYPE];
extern uchar typeShortChange[MAX_TYPE];
extern uchar typeSignedChange[MAX_TYPE];
extern uchar typeUnsignedChange[MAX_TYPE];

extern TypeModifier *structMemberCompletionType;
extern TypeModifier *upLevelFunctionCompletionType;
extern char *tokenNamesTable[];
extern int tokenNameLengthsTable[];

extern TypeModifier *builtinTypesTable[MAX_TYPE];
extern TypeModifier *builtinPtr2TypeTable[MAX_TYPE];
extern TypeModifier *builtinPtr2Ptr2TypeTable[MAX_TYPE];

extern int requestFileNumber;

extern char *storageNamesTable[STORAGE_ENUMS_MAX];

extern jmp_buf errorLongJumpBuffer;

extern char *inputFileName;

extern time_t fileProcessingStartTime;


#define LANGUAGE(langBit) ((currentLanguage & (langBit)) != 0)

extern Language currentLanguage;
extern int currentPass;
extern int maxPasses;

extern Completions collectedCompletions;


/* ********* vars for on-line additions after EOF ****** */

extern bool completionPositionFound;
extern bool completionStringServed;
extern Usage olstringUsage;
extern char *completionStringInMacroBody;
extern int fileToParseForMacroExpansion;

#endif
