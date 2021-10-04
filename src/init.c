#include "init.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "parsers.h"
#include "protocol.h"
#include "misc.h"
#include "symbol.h"
#include "classfilereader.h"
#include "log.h"


/* *********************************************************************** */
/* ********************* TABLES TO INIT TABLES *************************** */
/* *********************************************************************** */

typedef struct tokenNameIni {
    char        *name;
    int         token;
    unsigned    languages;
} TokenNamesInitTable;

static TokenNamesInitTable tokenNameInitTable1[] = {
    {"asm",         ASM_KEYWORD,	LANG_C | LANG_YACC},
    {"auto",        AUTO,			LANG_C | LANG_YACC},
    {"enum",        ENUM,			LANG_C | LANG_YACC},
    {"extern",      EXTERN,			LANG_C | LANG_YACC},
    {"inline",      INLINE,			LANG_C | LANG_YACC},
    {"register",    REGISTER,		LANG_C | LANG_YACC},
    {"signed",      SIGNED,			LANG_C | LANG_YACC},
    {"sizeof",      SIZEOF,			LANG_C | LANG_YACC},
    {"struct",      STRUCT,			LANG_C | LANG_YACC},
    {"typedef",     TYPEDEF,		LANG_C | LANG_YACC},
    {"union",       UNION,			LANG_C | LANG_YACC},
    {"unsigned",    UNSIGNED,		LANG_C | LANG_YACC},

    {"abstract",    ABSTRACT,		LANG_JAVA},
    {"boolean",     BOOLEAN,		LANG_JAVA},
    {"byte",        BYTE,			LANG_JAVA},
    {"catch",       CATCH,			LANG_JAVA},
    {"class",       CLASS,			LANG_JAVA},
    {"extends",     EXTENDS,		LANG_JAVA},
    {"final",       FINAL,			LANG_JAVA},
    {"finally",     FINALLY,		LANG_JAVA},
    {"implements",  IMPLEMENTS,		LANG_JAVA},
    {"import",      IMPORT,			LANG_JAVA},
    {"instanceof",  INSTANCEOF,		LANG_JAVA},
    {"interface",   INTERFACE,		LANG_JAVA},
    {"native",      NATIVE,			LANG_JAVA},
    {"new",         NEW,			LANG_JAVA},
    {"package",     PACKAGE,		LANG_JAVA},
    {"private",     PRIVATE,		LANG_JAVA},
    {"protected",   PROTECTED,		LANG_JAVA},
    {"public",      PUBLIC,			LANG_JAVA},
    {"super",       SUPER,			LANG_JAVA},

    {"synchronized",SYNCHRONIZED,	LANG_JAVA},
    {"strictfp",    STRICTFP,		LANG_JAVA},
    {"this",        THIS,			LANG_JAVA},
    {"throw",       THROW,			LANG_JAVA},
    {"throws",      THROWS,			LANG_JAVA},
    {"transient",   TRANSIENT,		LANG_JAVA},
    {"try",         TRY,			LANG_JAVA},

    {"true",        TRUE_LITERAL,	LANG_JAVA},
    {"false",       FALSE_LITERAL,	LANG_JAVA},
    {"null",        NULL_LITERAL,	LANG_JAVA},

    {"bool",        BOOL,			LANG_C},

    {"const_cast",  CONST_CAST,		},
    {"delete",      DELETE,			},
    {"dynamic_cast",DYNAMIC_CAST,	},
    {"explicit",    EXPLICIT,		},
    {"friend",      FRIEND,			},
    {"mutable",     MUTABLE,		},
    {"namespace",   NAMESPACE,		},
    {"operator",    OPERATOR,		},
    {"reinterpret_cast",    REINTERPRET_CAST,},
    {"static_cast", STATIC_CAST,	},
    {"template",    TEMPLATE,		},
    {"typeid",      TYPEID,			},
    {"typename",    TYPENAME,		},
    {"using",       USING,			},
    {"virtual",     VIRTUAL,		},
    {"wchar_t",     WCHAR_T,		},

    {"break",       BREAK,			LANG_C | LANG_YACC | LANG_JAVA},
    {"case",        CASE,			LANG_C | LANG_YACC | LANG_JAVA},
    {"char",        CHAR,			LANG_C | LANG_YACC | LANG_JAVA},
    {"const",       CONST,			LANG_C | LANG_YACC | LANG_JAVA},
    {"continue",    CONTINUE,		LANG_C | LANG_YACC | LANG_JAVA},
    {"default",     DEFAULT,		LANG_C | LANG_YACC | LANG_JAVA},
    {"do",          DO,				LANG_C | LANG_YACC | LANG_JAVA},
    {"double",      DOUBLE,			LANG_C | LANG_YACC | LANG_JAVA},
    {"else",        ELSE,			LANG_C | LANG_YACC | LANG_JAVA},
    {"float",       FLOAT,			LANG_C | LANG_YACC | LANG_JAVA},
    {"for",         FOR,			LANG_C | LANG_YACC | LANG_JAVA},
    {"goto",        GOTO,			LANG_C | LANG_YACC | LANG_JAVA},
    {"if",          IF,				LANG_C | LANG_YACC | LANG_JAVA},
    {"int",         INT,			LANG_C | LANG_YACC | LANG_JAVA},
    {"long",        LONG,			LANG_C | LANG_YACC | LANG_JAVA},
    {"return",      RETURN,			LANG_C | LANG_YACC | LANG_JAVA},
    {"short",       SHORT,			LANG_C | LANG_YACC | LANG_JAVA},
    {"static",      STATIC,			LANG_C | LANG_YACC | LANG_JAVA},
    {"switch",      SWITCH,			LANG_C | LANG_YACC | LANG_JAVA},
    {"void",        VOID,			LANG_C | LANG_YACC | LANG_JAVA},
    {"volatile",    VOLATILE,		LANG_C | LANG_YACC | LANG_JAVA},
    {"while",       WHILE,			LANG_C | LANG_YACC | LANG_JAVA},

    {"restrict",    RESTRICT,		LANG_C},
    {"_Atomic",     _ATOMIC,		LANG_C},
    {"_Bool",       _BOOL,			LANG_C},
    {"_Noreturn",   _NORETURN,		LANG_C},
    {"_Thread_local", _THREADLOCAL,	LANG_C},

    /*
      {"token",       TOKEN,		LAN_YACC},
      {"type",        TYPE,			LAN_YACC},
    */

    {">>>=",        URIGHT_ASSIGN,	LANG_JAVA},
    {">>>",         URIGHT_OP,		LANG_JAVA},
    {"...",         ELLIPSIS,		LANG_C},
    {">>=",         RIGHT_ASSIGN,	LANG_C},
    {"<<=",         LEFT_ASSIGN,	LANG_C},
    {"+=",      ADD_ASSIGN,			LANG_C},
    {"-=",      SUB_ASSIGN,			LANG_C},
    {"*=",      MUL_ASSIGN,			LANG_C},
    {"/=",      DIV_ASSIGN,			LANG_C},
    {"%=",      MOD_ASSIGN,			LANG_C},
    {"&=",      AND_ASSIGN,			LANG_C},
    {"^=",      XOR_ASSIGN,			LANG_C},
    {"|=",      OR_ASSIGN,			LANG_C},
    {">>",      RIGHT_OP,			LANG_C},
    {"<<",      LEFT_OP,			LANG_C},
    {"++",      INC_OP,				LANG_C},
    {"--",      DEC_OP,				LANG_C},
    {"->",      PTR_OP,				LANG_C},
    {"->*",     PTRM_OP,			},
    {".*",      POINTM_OP,			},
    {"::",      DPOINT,				},
    {"&&",      AND_OP,				LANG_C},
    {"||",      OR_OP,				LANG_C},
    {"<=",      LE_OP,				LANG_C},
    {">=",      GE_OP,				LANG_C},
    {"==",      EQ_OP,				LANG_C},
    {"!=",      NE_OP,				LANG_C},
    {";",       ';',				LANG_C},
    {"{",       '{',				LANG_C},
    {"}",       '}',				LANG_C},
    {",",       ',',				LANG_C},
    {":",       ':',				LANG_C},
    {"=",       '=',				LANG_C},
    {"(",       '(',				LANG_C},
    {")",       ')',				LANG_C},
    {"[",       '[',				LANG_C},
    {"]",       ']',				LANG_C},
    {".",       '.',				LANG_C},
    {"&",       '&',				LANG_C},
    {"!",       '!',				LANG_C},
    {"~",       '~',				LANG_C},
    {"-",       '-',				LANG_C},
    {"+",       '+',				LANG_C},
    {"*",       '*',				LANG_C},
    {"/",       '/',				LANG_C},
    {"%",       '%',				LANG_C},
    {"<",       '<',				LANG_C},
    {">",       '>',				LANG_C},
    {"^",       '^',				LANG_C},
    {"|",       '|',				LANG_C},
    {"?",       '?',				LANG_C},


    {"'CONSTANT'",        CONSTANT,			LANG_C},
    {"'CONSTANT'",        LONG_CONSTANT,	LANG_C},
    {"'CONSTANT'",        FLOAT_CONSTANT,	LANG_C},
    {"'CONSTANT'",        DOUBLE_CONSTANT,	LANG_C},
    {"'STRING_LITERAL'",  STRING_LITERAL,	LANG_C},
    {"'IDENTIFIER'",      IDENTIFIER,		LANG_C},
    {"'LINE_TOKEN'",      LINE_TOKEN,		LANG_C},
    {"'BLOCK_MARKER'",    OL_MARKER_TOKEN,	LANG_C},
    {"#INCLUDE",          CPP_INCLUDE,		LANG_C},
    {"#DEFINE0",          CPP_DEFINE0,		LANG_C},
    {"#DEFINE",           CPP_DEFINE,		LANG_C},
    {"#IFDEF",            CPP_IFDEF,		LANG_C},
    {"#IFNDEF",           CPP_IFNDEF,		LANG_C},
    {"#IF",               CPP_IF,			LANG_C},
    {"#ELSE",             CPP_ELSE,			LANG_C},
    {"#ENDIF",            CPP_ENDIF,		LANG_C},
    {"#PRAGMA",           CPP_PRAGMA,		LANG_C},
    {"#LINE",             CPP_LINE,			LANG_C},
    {"##",                CPP_COLLATION,	LANG_C},
    {NULL,                0,                LANG_C}         /* sentinel*/
};

static TokenNamesInitTable tokenNameInitTable2[] = {
    {"__const",         CONST,				LANG_C | LANG_YACC},
    {"__const__",       CONST,				LANG_C | LANG_YACC},
    {"__signed",        SIGNED,				LANG_C | LANG_YACC},
    {"__signed__",      SIGNED,				LANG_C | LANG_YACC},
    {"__inline",        ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__inline__",      ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__volatile",      VOLATILE,			LANG_C | LANG_YACC},
    {"__volatile__",    VOLATILE,			LANG_C | LANG_YACC},
    {"__asm",           ASM_KEYWORD,		LANG_C},
    {"__asm__",         ASM_KEYWORD,		LANG_C},
    {"__label__",       LABEL,				LANG_C},
    {"__near",          ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__far",           ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__pascal",        ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"_near",           ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"_far",            ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"_pascal",         ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"_const",          ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__near",          ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__far",           ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__pascal",        ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__cdecl",         ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__fastcall",      ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__stdcall",       ANONYMOUS_MODIFIER,	LANG_C | LANG_YACC},
    {"__int8",          INT,				LANG_C | LANG_YACC},
    {"__int16",         INT,				LANG_C | LANG_YACC},
    {"__int32",         INT,				LANG_C | LANG_YACC},
    {"__int64",         INT,				LANG_C | LANG_YACC},
    {NULL,              0,					LANG_C}         /* sentinel*/
};

static TokenNamesInitTable tokenNameInitTable3[] = {
    {"assert",          ASSERT,				LANG_JAVA},
    {NULL,              0,					LANG_C}         /* sentinel*/
};


static char *autoDetectJavaVersion(void) {
    int i;
    char *res;
    // O.K. pass jars and look for rt.jar
    for(i=0; i<MAX_JAVA_ZIP_ARCHIVES && zipArchiveTable[i].fn[0]!=0; i++) {
        if (stringContainsSubstring(zipArchiveTable[i].fn, "rt.jar")) {
            // I got it, detect java version
            if (stringContainsSubstring(zipArchiveTable[i].fn, "1.4")) {
                res = JAVA_VERSION_1_4;
                goto fini;
            }
        }
    }
    res = JAVA_VERSION_1_3;
 fini:
    return res;
}

static void initTokensFromTable(TokenNamesInitTable *tokenNamesInitTable) {
    char *name;
    int token, languages;
    Symbol *symbol;

    for(int i=0; tokenNamesInitTable[i].name!=NULL; i++) {
        name = tokenNamesInitTable[i].name;
        token = tokenNamesInitTable[i].token;
        languages = tokenNamesInitTable[i].languages;
        tokenNamesTable[token] = name;
        /* NOTE only tokens that are actually are initialized have a
         * length, the rest have zero, so we can't replace this
         * strlen() with strlen()s of the tokenNamesTable entry */
        tokenNameLengthsTable[token] = strlen(name);
        if ((isalpha(*name) || *name=='_') && (languages & s_language)) {
            /* looks like a keyword */
            log_trace("adding keyword '%s' to symbol table", name);
            symbol = newSymbolAsKeyword(name, name, s_noPos, token);
            fillSymbolBits(&symbol->bits, AccessDefault, TypeKeyword, StorageNone);
            symbolTableAdd(symbolTable, symbol);
        }
    }
}


void initTokenNamesTables(void) {
    char *javaVersion;
    Symbol *symbolP;

    static bool messageWritten = false;

    if (!options.strictAnsi) {
        initTokensFromTable(tokenNameInitTable2);
    }
    javaVersion = options.javaVersion;
    if (strcmp(javaVersion, JAVA_VERSION_AUTO)==0)
        javaVersion = autoDetectJavaVersion();

    if (options.taskRegime!=RegimeEditServer && !messageWritten) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"java version == %s", javaVersion);
        infoMessage(tmpBuff);
        messageWritten = true;
    }

    if (strcmp(javaVersion, JAVA_VERSION_1_4)==0) {
        initTokensFromTable(tokenNameInitTable3);
    }

    /* regular tokentab at last, because we wish to have correct names */
    initTokensFromTable(tokenNameInitTable1);

    /* and add the 'defined' keyword for #if */
    symbolP = newSymbol("defined", "defined", s_noPos);
    fillSymbolBits(&symbolP->bits, AccessDefault, TypeDefinedOp, StorageNone);
    symbolTableAdd(symbolTable, symbolP);
}

void initJavaTypePCTIConvertIniTab(void) {
    int                         i;
    S_javaTypePCTIConvertIni    *s;
    for (i=0; s_javaTypePCTIConvertIniTab[i].symType != -1; i++) {
        s = &s_javaTypePCTIConvertIniTab[i];
        assert(s->symType >= 0 && s->symType < MAX_TYPE);
        s_javaTypePCTIConvert[s->symType] = s->PCTIndex;
    }
}

void initTypeCharCodeTab(void) {
    S_typeCharCodeIni *s;

    for (int i=0; s_baseTypeCharCodesIniTab[i].symType != -1; i++) {
        s = &s_baseTypeCharCodesIniTab[i];
        assert(s->symType >= 0 && s->symType < MAX_TYPE);
        s_javaBaseTypeCharCodes[s->symType] = s->code;
        assert(s->code >= 0 && s->code < MAX_CHARS);
        s_javaCharCodeBaseTypes[s->code] = s->symType;
    }
}

void initTypeNames(void) {
    Int2StringTable *s;

    for (int i=0; typeNamesInitTable[i].i != -1; i++) {
        s = &typeNamesInitTable[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        typeNamesTable[s->i] = s->string;
    }
}

void initStorageNames(void) {
    Int2StringTable      *s;

    for (int i=0; i<MAX_STORAGE_NAMES; i++)
        storageNamesTable[i]="";
    for (int i=0; storageNamesInitTable[i].i != -1; i++) {
        s = &storageNamesInitTable[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        storageNamesTable[s->i] = s->string;
    }
}


void initArchaicTypes(void) {
    /* ******* some defaults and built-ins initialisations ********* */

    initTypeModifier(&s_defaultIntModifier, TypeInt);
    fillSymbolWithType(&s_defaultIntDefinition, NULL, NULL, s_noPos, &s_defaultIntModifier);

    initTypeModifier(&s_defaultPackedTypeModifier, TypePackedType);

    initTypeModifier(&s_defaultVoidModifier,TypeVoid);
    fillSymbolWithType(&s_defaultVoidDefinition, NULL, NULL, s_noPos, &s_defaultVoidModifier);

    initTypeModifier(&s_errorModifier, TypeError);
    fillSymbolWithType(&s_errorSymbol, "__ERROR__", "__ERROR__", s_noPos, &s_errorModifier);
    fillSymbolBits(&s_errorSymbol.bits, AccessDefault, TypeError, StorageNone);
}

void initPreCreatedTypes(void) {
    int i,t;

    for(i=0; i<MAX_TYPE; i++) {
        s_preCreatedTypesTable[i] = NULL;
        s_preCrPtr1TypesTab[i] = NULL;
    }
    for(i=0; ; i++) {
        t = preCreatedTypesInitTable[i];
        if (t<0) break;
        /* pre-create X */
        assert(t>=0 && t<MAX_TYPE);
        s_preCreatedTypesTable[t] = newTypeModifier(t, NULL, NULL);
        /* pre-create *X */
        s_preCrPtr1TypesTab[t] = newTypeModifier(TypePointer, NULL, s_preCreatedTypesTable[t]);
        /* pre-create **X */
        s_preCrPtr2TypesTab[t] = newTypeModifier(TypePointer, NULL, s_preCrPtr1TypesTab[t]);
    }
}
