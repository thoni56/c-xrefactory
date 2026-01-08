#include "init.h"

#include <ctype.h>
#include <string.h>

#include "commons.h"
#include "globals.h"
#include "lexem.h"
#include "log.h"
#include "storage.h"
#include "symboltable.h"
#include "typemodifier.h"


/* *********************************************************************** */
/* ********************* TABLES TO INIT TABLES *************************** */
/* *********************************************************************** */

typedef struct tokenNameIni {
    char        *name;
    int         token;
    unsigned    languages;
} TokenNamesInitTable;


/* Order among synonyms/aliases is important, the last one will be shown as the
 * alternative when completing. This table includes the canonical token names. */
static TokenNamesInitTable tokenNamesCanonical[] = {
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

    {"true",        TRUE_LITERAL,	LANG_C | LANG_YACC},
    {"false",       FALSE_LITERAL,	LANG_C | LANG_YACC},
    {"break",       BREAK,			LANG_C | LANG_YACC},
    {"case",        CASE,			LANG_C | LANG_YACC},
    {"char",        CHAR,			LANG_C | LANG_YACC},
    {"const",       CONST,			LANG_C | LANG_YACC},
    {"continue",    CONTINUE,		LANG_C | LANG_YACC},
    {"default",     DEFAULT,		LANG_C | LANG_YACC},
    {"do",          DO,				LANG_C | LANG_YACC},
    {"double",      DOUBLE,			LANG_C | LANG_YACC},
    {"else",        ELSE,			LANG_C | LANG_YACC},
    {"float",       FLOAT,			LANG_C | LANG_YACC},
    {"for",         FOR,			LANG_C | LANG_YACC},
    {"goto",        GOTO,			LANG_C | LANG_YACC},
    {"if",          IF,				LANG_C | LANG_YACC},
    {"int",         INT,			LANG_C | LANG_YACC},
    {"long",        LONG,			LANG_C | LANG_YACC},
    {"return",      RETURN,			LANG_C | LANG_YACC},
    {"short",       SHORT,			LANG_C | LANG_YACC},
    {"static",      STATIC,			LANG_C | LANG_YACC},
    {"switch",      SWITCH,			LANG_C | LANG_YACC},
    {"void",        VOID,			LANG_C | LANG_YACC},
    {"volatile",    VOLATILE,		LANG_C | LANG_YACC},
    {"while",       WHILE,			LANG_C | LANG_YACC},

    {"restrict",    RESTRICT,		LANG_C},

    {"bool",        _BOOL,			LANG_C},
    {"_Atomic",     _ATOMIC,		LANG_C},
    {"_Noreturn",   _NORETURN,		LANG_C},
    {"_Thread_local", _THREADLOCAL,	LANG_C},
    {"static_assert", _STATIC_ASSERT, LANG_C},

    /* Operators... */
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

/* ... and here are some synonyms: */
static TokenNamesInitTable tokenNamesAliases[] = {
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
    {"__restrict",      RESTRICT,           LANG_C},
    {"__restrict__",    RESTRICT,           LANG_C},
    {"_Bool",           _BOOL,              LANG_C},
    {"_Static_assert",  _STATIC_ASSERT,     LANG_C},
    {NULL,              0,					LANG_C}         /* sentinel*/
};


static int builtinTypesInitTable[] = {
    TypeDefault,
    TypeChar,
    TypeUnsignedChar,
    TypeSignedChar,
    TypeInt,
    TypeUnsignedInt,
    TypeSignedInt,
    TypeShortInt,
    TypeShortUnsignedInt,
    TypeShortSignedInt,
    TypeLongInt,
    TypeLongUnsignedInt,
    TypeLongSignedInt,
    TypeFloat,
    TypeDouble,
    TypeLong,
    TypeBool,
    TypeNull,
    TypeVoid,
    TypeError,
    TypeAnonymousField,
    /* MODIFIERS: */
    TmodLong,
    TmodShort,
    TmodSigned,
    TmodUnsigned,
    TmodShortSigned,
    TmodShortUnsigned,
    TmodLongSigned,
    TmodLongUnsigned,
    -1,
};

typedef struct int2StringTable {
    int     i;
    char    *string;
} Int2StringDictionary;


static Int2StringDictionary typeNamesInitTable[] = {
    {TypeDefault,           "Default"},
    {TypeChar,              "char"},
    {TypeUnsignedChar,      "unsigned char"},
    {TypeSignedChar,        "signed char"},
    {TypeInt,               "int"},
    {TypeUnsignedInt,       "unsigned int"},
    {TypeSignedInt,         "signed int"},
    {TypeShortInt,          "short int"},
    {TypeShortUnsignedInt,  "short unsigned int"},
    {TypeShortSignedInt,    "short signed int"},
    {TypeLongInt,           "long int"},
    {TypeLongUnsignedInt,   "long unsigned int"},
    {TypeLongSignedInt,     "long signed int"},
    {TypeFloat,             "float"},
    {TypeDouble,            "double"},
    {TypeStruct,            "struct"},
    {TypeUnion,             "union"},
    {TypeEnum,              "enum"},
    {TypeVoid,              "void"},
    {TypePointer,           "Pointer"},
    {TypeArray,             "Array"},
    {TypeFunction,          "Function"},
    {TypeAnonymousField,    "AnonymousField"},
    {TypeError,             "Error"},
    {TypeCppIfElse,         "#if-else-fi"},
    {TypeCppInclude,        "#include"},
    {TypeCppCollate,        "##"},

    {TmodLong,              "long"},
    {TmodShort,             "short"},
    {TmodSigned,            "signed"},
    {TmodUnsigned,          "unsigned"},
    {TmodShortSigned,       "short signed"},
    {TmodShortUnsigned,     "short unsigned"},
    {TmodLongSigned,        "long signed"},
    {TmodLongUnsigned,      "long unsigned"},

    {TypeElipsis,           "elipsis"},

    {TypeLong,              "long"},
    {TypeBool,              "boolean"},
    {TypeNull,              "null"},

    {TypeLabel,             "label"},
    {TypeKeyword,           "keyword"},
    {TypeToken,             "token"},
    {TypeMacro,             "macro"},
    {TypeMacroArg,          "macro argument"},
    {TypeCppUndefinedMacro, "Undefined Macro"},
    {TypeYaccSymbol,        "yacc symbol"},
    {TypeCppCollate,        "Cpp##sym"},
    {TypeSpecialComplete,    "(Completion Wizard)"},
    {TypeInheritedFullMethod,   "(Override Wizard)"},
    {TypeNonImportedClass,  "fully qualified name"},
    {-1,					NULL}
};


static Int2StringDictionary storageNamesInitTable[] = {
    {StorageExtern,     "extern "},
    {StorageStatic,     "static "},
    {StorageRegister,   "register "},
    {StorageTypedef,    "typedef "},
    {-1,                NULL}
};


static void initTokensFromTable(TokenNamesInitTable *tokenNamesInitTable) {
    char *name;
    int token, languages;
    Symbol *symbol;

    for(int i=0; tokenNamesInitTable[i].name!=NULL; i++) {
        name = tokenNamesInitTable[i].name;
        token = tokenNamesInitTable[i].token;
        languages = tokenNamesInitTable[i].languages;
        tokenNamesTable[token] = name;
        /* NOTE only tokens that are actually initialized have a
         * length, the rest have zero, so we can't replace this
         * strlen() with strlen()s of the tokenNamesTable entry */
        tokenNameLengthsTable[token] = strlen(name);
        if ((isalpha(*name) || *name=='_') && (languages & currentLanguage)) {
            /* looks like a keyword */
            log_debug("adding keyword '%s' to symbol table", name);
            symbol = newSymbolAsKeyword(name, name, NO_POSITION, token);
            symbol->type = TypeKeyword;
            symbol->storage = StorageDefault;
            symbolTableAdd(symbolTable, symbol);
        }
    }
}


void initTokenNamesTables(void) {

    initTokensFromTable(tokenNamesAliases);

    /* Canonical token names last, because we wish to show them when completing */
    initTokensFromTable(tokenNamesCanonical);

    /* and add the 'defined' keyword for #if */
    Symbol *symbolP = newSymbol("defined", NO_POSITION);
    symbolP->type = TypeCppDefinedOp;
    symbolP->storage = StorageDefault;
    symbolTableAdd(symbolTable, symbolP);

    /* and add the '__has_include' keyword for #if */
    symbolP = newSymbol("__has_include", NO_POSITION);
    symbolP->type = TypeCppHasIncludeOp;
    symbolP->storage = StorageDefault;
    symbolTableAdd(symbolTable, symbolP);

    /* and add the '__has_include_next' keyword for #if */
    symbolP = newSymbol("__has_include_next", NO_POSITION);
    symbolP->type = TypeCppHasIncludeNextOp;
    symbolP->storage = StorageDefault;
    symbolTableAdd(symbolTable, symbolP);
}

void initTypeNames(void) {
    for (int i=0; typeNamesInitTable[i].i != -1; i++) {
        Int2StringDictionary *s = &typeNamesInitTable[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        typeNamesTable[s->i] = s->string;
    }
}

void initStorageNames(void) {
    for (int i=0; i<STORAGE_ENUMS_MAX; i++)
        storageNamesTable[i]="";
    for (int i=0; storageNamesInitTable[i].i != -1; i++) {
        Int2StringDictionary *s = &storageNamesInitTable[i];
        assert(s->i >= 0 && s->i < MAX_TYPE);
        storageNamesTable[s->i] = s->string;
    }
}


void initArchaicTypes(void) {
    initTypeModifier(&defaultIntModifier, TypeInt);
    fillSymbolWithTypeModifier(&defaultIntDefinition, NULL, NULL, NO_POSITION, &defaultIntModifier);

    initTypeModifier(&defaultPackedTypeModifier, TypePackedType);

    initTypeModifier(&defaultVoidModifier,TypeVoid);
    fillSymbolWithTypeModifier(&defaultVoidDefinition, NULL, NULL, NO_POSITION, &defaultVoidModifier);

    initTypeModifier(&errorModifier, TypeError);
    fillSymbolWithTypeModifier(&errorSymbol, "__ERROR__", "__ERROR__", NO_POSITION, &errorModifier);
    errorSymbol.type = TypeError;
    errorSymbol.storage = StorageDefault;
}

void initBuiltinTypes(void) {
    for (int i=0; i<MAX_TYPE; i++) {
        builtinTypesTable[i] = NULL;
        builtinPtr2TypeTable[i] = NULL;
        builtinPtr2Ptr2TypeTable[i] = NULL;
    }

    for (int i=0; ; i++) {
        int t = builtinTypesInitTable[i];
        if (t<0)
            break;

        /* pre-create X */
        assert(t>=0 && t<MAX_TYPE);
        builtinTypesTable[t] = newTypeModifier(t, NULL, NULL);

        /* pre-create *X */
        builtinPtr2TypeTable[t] = newTypeModifier(TypePointer, NULL, builtinTypesTable[t]);

        /* pre-create **X */
        builtinPtr2Ptr2TypeTable[t] = newTypeModifier(TypePointer, NULL, builtinPtr2TypeTable[t]);
    }
}
