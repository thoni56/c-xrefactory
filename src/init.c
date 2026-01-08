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
} TokenNamesInitTable;


/* Order among synonyms/aliases is important, the last one will be shown as the
 * alternative when completing. This table includes the canonical token names. */
static TokenNamesInitTable tokenNamesCanonical[] = {
    {"asm",              ASM_KEYWORD},
    {"auto",             AUTO},
    {"enum",             ENUM},
    {"extern",           EXTERN},
    {"inline",           INLINE},
    {"register",         REGISTER},
    {"signed",           SIGNED},
    {"sizeof",           SIZEOF},
    {"struct",           STRUCT},
    {"typedef",          TYPEDEF},
    {"union",            UNION},
    {"unsigned",         UNSIGNED},

    {"true",             TRUE_LITERAL},
    {"false",            FALSE_LITERAL},
    {"break",            BREAK},
    {"case",             CASE},
    {"char",             CHAR},
    {"const",            CONST},
    {"continue",         CONTINUE},
    {"default",          DEFAULT},
    {"do",               DO},
    {"double",           DOUBLE},
    {"else",             ELSE},
    {"float",            FLOAT},
    {"for",              FOR},
    {"goto",             GOTO},
    {"if",               IF},
    {"int",              INT},
    {"long",             LONG},
    {"return",           RETURN},
    {"short",            SHORT},
    {"static",           STATIC},
    {"switch",           SWITCH},
    {"void",             VOID},
    {"volatile",         VOLATILE},
    {"while",            WHILE},

    {"restrict",         RESTRICT},

    {"bool",             _BOOL},
    {"_Atomic",          _ATOMIC},
    {"_Noreturn",        _NORETURN},
    {"_Thread_local",    _THREADLOCAL},
    {"static_assert",    _STATIC_ASSERT},

    /* Operators... */
    {"...",              ELLIPSIS},
    {">>=",              RIGHT_ASSIGN},
    {"<<=",              LEFT_ASSIGN},
    {"+=",               ADD_ASSIGN},
    {"-=",               SUB_ASSIGN},
    {"*=",               MUL_ASSIGN},
    {"/=",               DIV_ASSIGN},
    {"%=",               MOD_ASSIGN},
    {"&=",               AND_ASSIGN},
    {"^=",               XOR_ASSIGN},
    {"|=",               OR_ASSIGN},
    {">>",               RIGHT_OP},
    {"<<",               LEFT_OP},
    {"++",               INC_OP},
    {"--",               DEC_OP},
    {"->",               PTR_OP},
    {"&&",               AND_OP},
    {"||",               OR_OP},
    {"<=",               LE_OP},
    {">=",               GE_OP},
    {"==",               EQ_OP},
    {"!=",               NE_OP},
    {";",                ';'},
    {"{",                '{'},
    {"}",                '}'},
    {",",                ','},
    {":",                ':'},
    {"=",                '='},
    {"(",                '('},
    {")",                ')'},
    {"[",                '['},
    {"]",                ']'},
    {".",                '.'},
    {"&",                '&'},
    {"!",                '!'},
    {"~",                '~'},
    {"-",                '-'},
    {"+",                '+'},
    {"*",                '*'},
    {"/",                '/'},
    {"%",                '%'},
    {"<",                '<'},
    {">",                '>'},
    {"^",                '^'},
    {"|",                '|'},
    {"?",                '?'},

    {"'CONSTANT'",       CONSTANT},
    {"'CONSTANT'",       LONG_CONSTANT},
    {"'CONSTANT'",       FLOAT_CONSTANT},
    {"'CONSTANT'",       DOUBLE_CONSTANT},
    {"'STRING_LITERAL'", STRING_LITERAL},
    {"'IDENTIFIER'",     IDENTIFIER},
    {"'LINE_TOKEN'",     LINE_TOKEN},
    {"'BLOCK_MARKER'",   OL_MARKER_TOKEN},
    {"#INCLUDE",         CPP_INCLUDE},
    {"#DEFINE0",         CPP_DEFINE0},
    {"#DEFINE",          CPP_DEFINE},
    {"#IFDEF",           CPP_IFDEF},
    {"#IFNDEF",          CPP_IFNDEF},
    {"#IF",              CPP_IF},
    {"#ELSE",            CPP_ELSE},
    {"#ENDIF",           CPP_ENDIF},
    {"#PRAGMA",          CPP_PRAGMA},
    {"#LINE",            CPP_LINE},
    {"##",               CPP_COLLATION},
    {NULL,               0}
};

/* ... and here are some synonyms: */
static TokenNamesInitTable tokenNamesAliases[] = {
    {"__const",         CONST},
    {"__const__",       CONST},
    {"__signed",        SIGNED},
    {"__signed__",      SIGNED},
    {"__inline",        ANONYMOUS_MODIFIER},
    {"__inline__",      ANONYMOUS_MODIFIER},
    {"__volatile",      VOLATILE},
    {"__volatile__",    VOLATILE},
    {"__asm",           ASM_KEYWORD},
    {"__asm__",         ASM_KEYWORD},
    {"__label__",       LABEL},
    {"__restrict",      RESTRICT},
    {"__restrict__",    RESTRICT},
    {"_Bool",           _BOOL},
    {"_Static_assert",  _STATIC_ASSERT},
    {NULL,              0}
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

    for(int i=0; tokenNamesInitTable[i].name!=NULL; i++) {
        char *name = tokenNamesInitTable[i].name;
        int token = tokenNamesInitTable[i].token;
        tokenNamesTable[token] = name;
        /* NOTE only tokens that are actually initialized have a
         * length, the rest have zero, so we can't replace this
         * strlen() with strlen()s of the tokenNamesTable entry */
        tokenNameLengthsTable[token] = strlen(name);
        if (isalpha(*name) || *name=='_') {
            /* looks like a keyword */
            log_debug("adding keyword '%s' to symbol table", name);
            Symbol *symbol = newSymbolAsKeyword(name, name, NO_POSITION, token);
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
