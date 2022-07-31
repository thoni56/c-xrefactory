#include "globals.h"

#include "stdinc.h"
#include "head.h"
#include "proto.h"
#include "parsers.h"
#include "usage.h"
#include "options.h"

#include "protocol.h"


bool fileAbortEnabled;

int s_lastReturnedLexem;

Position parsedPositions[SPP_MAX];

Counters counters;
unsigned s_recFindCl = 1;

CurrentlyParsedClassInfo parsedClassInfo;
CurrentlyParsedClassInfo parsedClassInfoInit = {0,};

CurrentlyParsedInfo parsedInfo;

bool javaPreScanOnly = false;


/* *********** symbols excluded from cache ************** */

char s_olSymbolType[COMPLETION_STRING_SIZE];
char s_olSymbolClassType[COMPLETION_STRING_SIZE];

Position parameterPosition;
Position parameterBeginPosition;
Position parameterEndPosition;
Position s_primaryStartPosition;
Position s_staticPrefixStartPosition;

Id yyIdBuffer[YYIDBUFFER_SIZE];
int yyIdBufferIndex = 0;

TypeModifier *s_structRecordCompletionType;
TypeModifier *s_upLevelFunctionCompletionType;
ExprTokenType s_forCompletionType;
TypeModifier *s_javaCompletionLastPrimary;

struct yyGlobalState *s_yygstate;
struct yyGlobalState *s_initYygstate;

char *inputFileName="";
Completions collectedCompletions;

/* **************** cached symbols ********************** */

Position s_olcxByPassPos;
Position cxRefPosition;

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
char defaultClassPath[] = ".";
StringList *javaClassPaths;
char *javaSourcePaths;

Id javaAnonymousClassName = {"{Anonymous}", NULL, {-1,0,0}};

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
bool olstringFound = false;
bool olstringServed = false;
int s_olstringUsage = 0;
char *s_olstringInMbody = NULL;
int s_olMacro2PassFile;

/* ******************* yytext for yacc ****************** */
char *yytext;

char *s_javaThisPackageName = "";

/* ************************************************************* */
/* ************* constant during the program ******************* */
/* *********** once, the file starts to be parsed ************** */
/* ************************************************************* */

FILE *communicationChannel=NULL;
FILE *errOut=NULL;

bool cxResizingBlocked = false;

int inputFileNumber          = -1;
int olOriginalFileIndex     = -1;
int olOriginalComFileNumber  = -1;

TypeModifier defaultIntModifier;
Symbol defaultIntDefinition;
TypeModifier defaultPackedTypeModifier;
TypeModifier defaultVoidModifier;
Symbol defaultVoidDefinition;
TypeModifier errorModifier;
Symbol errorSymbol;
Position noPosition = {-1, 0, 0};

Symbol s_javaArrayObjectSymbol;
Symbol *s_javaStringSymbol;
Symbol *s_javaCloneableSymbol;
Symbol *s_javaIoSerializableSymbol;
Symbol *s_javaObjectSymbol;
TypeModifier s_javaStringModifier;
TypeModifier s_javaClassModifier;
TypeModifier s_javaObjectModifier;

char cwd[MAX_FILE_NAME_SIZE]; /* current working directory */

char ppcTmpBuff[MAX_PPC_RECORD_SIZE];

jmp_buf cxmemOverflow;

/* ********************************************************************** */
/*                            real constants                              */
/* ********************************************************************** */

Access javaRequiredAccessibilityTable[MAX_REQUIRED_ACCESS+1] = {
    AccessPublic,
    AccessProtected,
    AccessDefault,
    AccessPrivate,
};

char *s_editCommunicationString = "C@$./@mVeDitznAC";

uchar typeShortChange[MAX_TYPE];
uchar typeLongChange[MAX_TYPE];
uchar typeSignedChange[MAX_TYPE];
uchar typeUnsignedChange[MAX_TYPE];

char javaBaseTypeCharCodes[MAX_TYPE];
int javaCharCodeBaseTypes[MAX_CHARS];

char javaTypePCTIConvert[MAX_TYPE];

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

/* These should go together in a struct. Also the lengths are not just
 * the lengths of the names, some slots do not have names, so they are
 * NULL and the length is zero. */
char *tokenNamesTable[LAST_TOKEN];
int tokenNameLengthsTable[LAST_TOKEN];

TypeModifier *preCreatedTypesTable[MAX_TYPE];
TypeModifier *preCreatedPtr2TypeTable[MAX_TYPE];
TypeModifier *preCreatedPtr2Ptr2TypeTable[MAX_TYPE];

char *storageNamesTable[MAX_STORAGE_NAMES];
