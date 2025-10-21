#include "globals.h"

#include "head.h"
#include "proto.h"
#include "usage.h"
#include "lexem.h"


bool fileAbortEnabled;

unsigned memberFindCount = 1;


ParsedInfo parsedInfo;

Position parsedPositions[IPP_MAX];


/* Variables for capturing parameter positions */
bool parameterListIsVoid; /* Is what looks like a parameter declaration actually "void" */
int parameterCount;
Position parameterPosition;
Position parameterBeginPosition;
Position parameterEndPosition;


Position primaryStartPosition;
Position staticPrefixStartPosition;

Id yyIdBuffer[YYIDBUFFER_SIZE];
int yyIdBufferIndex = 0;

TypeModifier *structMemberCompletionType;
TypeModifier *upLevelFunctionCompletionType;

char *inputFileName="";
Completions collectedCompletions;

/* **************** cached symbols ********************** */

Position cxRefPosition;         /* Maybe the position that we consider us "on"? */

time_t fileProcessingStartTime;

Language currentLanguage;
int currentPass;
int maxPasses;

char *cppVarArgsName = "__VA_ARGS__";


/* ********* vars for on-line additions after EOF ****** */

bool completionPositionFound = false;
bool completionStringServed = false;
Usage olstringUsage = 0;
char *completionStringInMacroBody = NULL;
int olMacro2PassFile;

/* ******************* yytext for yacc ****************** */
char *yytext;


/* ************************************************************* */
/* ************* constant during the program ******************* */
/* *********** once, the file starts to be parsed ************** */
/* ************************************************************* */

FILE *outputFile=NULL;
FILE *errOut=NULL;

bool cxResizingBlocked = false;

int inputFileNumber          = -1;
int originalFileNumber     = -1;
int originalCommandLineFileNumber  = -1;

TypeModifier defaultIntModifier;
Symbol defaultIntDefinition;
TypeModifier defaultPackedTypeModifier;
TypeModifier defaultVoidModifier;
Symbol defaultVoidDefinition;
TypeModifier errorModifier;
Symbol errorSymbol;
Position noPosition = {-1, 0, 0};


char cwd[MAX_FILE_NAME_SIZE]; /* current working directory */

char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

jmp_buf errorLongJumpBuffer;

/* ********************************************************************** */
/*                            real constants                              */
/* ********************************************************************** */

uchar typeShortChange[MAX_TYPE];
uchar typeLongChange[MAX_TYPE];
uchar typeSignedChange[MAX_TYPE];
uchar typeUnsignedChange[MAX_TYPE];


/* These should go together in a struct. Also the lengths are not just
 * the lengths of the names, some slots do not have names, so they are
 * NULL and the length is zero. */
char *tokenNamesTable[LAST_TOKEN];
int tokenNameLengthsTable[LAST_TOKEN];

TypeModifier *preCreatedTypesTable[MAX_TYPE];
TypeModifier *preCreatedPtr2TypeTable[MAX_TYPE];
TypeModifier *preCreatedPtr2Ptr2TypeTable[MAX_TYPE];

char *storageNamesTable[MAX_STORAGE_NAMES];
