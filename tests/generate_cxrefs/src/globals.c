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

Position olcxByPassPos;
Position cxRefPosition;         /* Maybe the position that we consider us "on"? */

time_t fileProcessingStartTime;

Language currentLanguage;
int currentPass;
int maxPasses;

unsigned menuFilterOoBits[MAX_MENU_FILTER_LEVEL] = {
    (OOC_VIRT_ANY | OOC_PROFILE_ANY),
    //& (OOC_VIRT_RELATED | OOC_PROFILE_ANY),
    (OOC_VIRT_ANY | OOC_PROFILE_APPLICABLE),
    (OOC_VIRT_SUBCLASS_OF_RELATED | OOC_PROFILE_APPLICABLE),
    //& (OOC_VIRT_APPLICABLE | OOC_PROFILE_APPLICABLE),
    //& (OOC_VIRT_SAME_FUN_CLASS | OOC_PROFILE_APPLICABLE),
    //& (OOC_VIRT_SAME_APPL_FUN_CLASS | OOC_PROFILE_APPLICABLE),
};

// !!!! if you modify this, you will need to modify level# for
//  Emacs, in c-xref.el !!!!!!!!!
int refListFilters[MAX_REF_LIST_FILTER_LEVEL] = {
    UsageMaxOLUsages,
    UsageUsed,
    UsageAddrUsed,
    UsageLvalUsed,
};

char *cppVarArgsName = "__VA_ARGS__";
char *javaSourcePaths;


/* ********* vars for on-line additions after EOF ****** */

bool olstringFound = false;
bool olstringServed = false;
UsageKind olstringUsageKind = 0;
char *olstringInMacroBody = NULL;
int olMacro2PassFile;

/* ******************* yytext for yacc ****************** */
char *yytext;


/* ************************************************************* */
/* ************* constant during the program ******************* */
/* *********** once, the file starts to be parsed ************** */
/* ************************************************************* */

FILE *communicationChannel=NULL;
FILE *errOut=NULL;

bool cxResizingBlocked = false;

int inputFileNumber          = -1;
int olOriginalFileNumber     = -1;
int olOriginalComFileNumber  = -1;

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

jmp_buf cxmemOverflow;

/* ********************************************************************** */
/*                            real constants                              */
/* ********************************************************************** */

uchar typeShortChange[MAX_TYPE];
uchar typeLongChange[MAX_TYPE];
uchar typeSignedChange[MAX_TYPE];
uchar typeUnsignedChange[MAX_TYPE];

int javaCharCodeBaseTypes[MAX_CHARS];


/* These should go together in a struct. Also the lengths are not just
 * the lengths of the names, some slots do not have names, so they are
 * NULL and the length is zero. */
char *tokenNamesTable[LAST_TOKEN];
int tokenNameLengthsTable[LAST_TOKEN];

TypeModifier *preCreatedTypesTable[MAX_TYPE];
TypeModifier *preCreatedPtr2TypeTable[MAX_TYPE];
TypeModifier *preCreatedPtr2Ptr2TypeTable[MAX_TYPE];

char *storageNamesTable[MAX_STORAGE_NAMES];
