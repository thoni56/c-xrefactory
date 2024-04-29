#include "globals.h"

#include "head.h"
#include "proto.h"
#include "parsers.h"
#include "usage.h"
#include "options.h"

#include "protocol.h"


bool fileAbortEnabled;

int s_lastReturnedLexem;

unsigned s_recFindCl = 1;

CurrentlyParsedClassInfo parsedClassInfo;
CurrentlyParsedClassInfo parsedClassInfoInit = {0,};

CurrentlyParsedInfo parsedInfo;


/* Variables for capturing parameter positions */
bool parameterListIsVoid; /* Is what looks like a parameter declaration actually "void" */
int parameterCount;
Position parameterPosition;
Position parameterBeginPosition;
Position parameterEndPosition;


Position s_primaryStartPosition;
Position s_staticPrefixStartPosition;

Id yyIdBuffer[YYIDBUFFER_SIZE];
int yyIdBufferIndex = 0;

TypeModifier *s_structRecordCompletionType;
TypeModifier *s_upLevelFunctionCompletionType;

char *inputFileName="";
Completions collectedCompletions;

/* **************** cached symbols ********************** */

Position s_olcxByPassPos;
Position cxRefPosition;         /* Maybe the position that we consider us "on"? */

time_t fileProcessingStartTime;

Language currentLanguage;
int currentPass;
int maxPasses;

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
//  Emacs, in c-xref.el !!!!!!!!!
int s_refListFilters[MAX_REF_LIST_FILTER_LEVEL] = {
    UsageMaxOLUsages,
    UsageUsed,
    UsageAddrUsed,
    UsageLvalUsed,
};

char *s_cppVarArgsName = "__VA_ARGS__";
StringList *javaClassPaths;
char *javaSourcePaths;


/* ********* vars for on-line additions after EOF ****** */

bool olstringFound = false;
bool olstringServed = false;
UsageKind olstringUsageKind = 0;
char *olstringInMacroBody = NULL;
int s_olMacro2PassFile;

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
