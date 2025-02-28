/*
  Our yacc does not support %expect, but this should be it for the C grammar

  %expect 7
  %expect-rr 34

../../../byacc-1.9/yacc: 7 shift/reduce conflicts, 28 reduce/reduce conflicts.

*/

%{

#define c_yylex yylex

#include "c_parser.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ast.h"
#include "caching.h"
#include "commons.h"
#include "complete.h"
#include "cxref.h"
#include "extract.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "id.h"
#include "list.h"
#include "log.h"
#include "options.h"
#include "semact.h"
#include "stackmemory.h"
#include "symbol.h"
#include "yylex.h"

#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define c_yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

static int savedWorkMemoryIndex = 0;

%}

/* Token definitions *must* be the same in all parsers. The following
   is a marker, it must be the same as in the Makefile check */
/* START OF COMMON TOKEN DEFINITIONS */

/* ************************* SPECIALS ****************************** */

%token TYPE_NAME

/* ************************* OPERATORS ****************************** */
/* common */
%token INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

/* c-only */
%token PTR_OP ELLIPSIS

/* yacc-only */
%token YACC_PERC YACC_DPERC

/* ************************** KEYWORDS ******************************** */

%token STATIC BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE
%token FLOAT FOR GOTO IF INT LONG RETURN SHORT SWITCH VOID VOLATILE WHILE

/* c-special */
%token TYPEDEF EXTERN AUTO REGISTER SIGNED UNSIGNED STRUCT UNION ENUM
%token SIZEOF RESTRICT _ATOMIC _BOOL _THREADLOCAL _NORETURN
%token INLINE ASM_KEYWORD
/* hmm */
%token ANONYMOUS_MODIFIER

%token TRUE_LITERAL FALSE_LITERAL

/* yacc-special */
%token TOKEN TYPE

/* gcc specials */
%token LABEL

/* ******************** COMPLETION SPECIAL TOKENS ******************** */

%token COMPLETE_FOR_STATEMENT1 COMPLETE_FOR_STATEMENT2

/* c-only */
%token COMPLETE_TYPE_NAME
%token COMPLETE_STRUCT_NAME COMPLETE_STRUCT_MEMBER_NAME COMPLETE_UP_FUN_PROFILE
%token COMPLETE_ENUM_NAME COMPLETE_LABEL_NAME COMPLETE_OTHER_NAME

/* yacc-special */
%token COMPLETE_YACC_LEXEM_NAME

/* ************************** CPP-TOKENS ****************************** */
/* c-only */
%token CPP_TOKENS_START
%token CPP_INCLUDE CPP_INCLUDE_NEXT CPP_DEFINE CPP_IFDEF CPP_IFNDEF CPP_IF CPP_ELSE CPP_ENDIF
%token CPP_ELIF CPP_UNDEF
%token CPP_PRAGMA CPP_LINE
%token CPP_DEFINE0       /* macro with no argument */
%token CPP_TOKENS_END

%token CPP_COLLATION     /* ## in macro body */
%token CPP_DEFINED_OP    /* defined(xxx) in #if */

/* ******************************************************************** */
/* special token signaling end of input */
%token EOI_TOKEN

/* ******************************************************************** */
%token OL_MARKER_TOKEN OL_MARKER_TOKEN1 OL_MARKER_TOKEN2

/* ******************************************************************** */
%token TMP_TOKEN1 TMP_TOKEN2

/* ************************ MULTI TOKENS START ************************ */
%token MULTI_TOKENS_START

/* commons */
%token IDENTIFIER CONSTANT LONG_CONSTANT
%token FLOAT_CONSTANT DOUBLE_CONSTANT
%token STRING_LITERAL CHAR_LITERAL
%token LINE_TOKEN
%token IDENT_TO_COMPLETE        /* identifier under cursor */

/* c-only */
%token CPP_MACRO_ARGUMENT IDENT_NO_CPP_EXPAND

/* ******************************************************************
 * LAST_TOKEN is to dimension the lexemEnum arrays, should always be
 * the last token, duh! If not, there is a token that is not explicitly
 * declared above. */

%token LAST_TOKEN

/* END OF COMMON TOKEN DEFINITIONS */
/* Token definitions *must* be the same in all parsers. The above
   is a marker, it must be the same as in the Makefile check */

%union {
#include "yystype.h"
}


%type <ast_id> IDENTIFIER identifier struct_identifier enum_identifier
%type <ast_id> field_identifier STRUCT UNION struct_or_union
%type <ast_id> user_defined_type TYPE_NAME
%type <ast_id> designator, designator_list
%type <ast_idList> designation_opt, initializer, initializer_list, eq_initializer_opt
%type <ast_integer> assignment_operator
%type <ast_integer> pointer CONSTANT _ncounter_ _nlabel_ _ngoto_ _nfork_
%type <ast_unsigned> storage_class_specifier type_specifier1
%type <ast_unsigned> type_modality_specifier Save_index
%type <ast_symbol> init_declarator declarator declarator2 struct_declarator
%type <ast_symbol> type_specifier_list type_mod_specifier_list
%type <ast_symbol> type_specifier_list0
%type <ast_symbol> top_init_declarations
%type <ast_symbol> declaration_specifiers init_declarations
%type <ast_symbol> declaration_modality_specifiers declaration_specifiers0
%type <ast_symbol> enumerator parameter_declaration
%type <ast_symbol> function_head_declaration function_definition_head
%type <ast_symbol> struct_declaration_list struct_declaration
%type <ast_symbol> struct_declarator_list
%type <ast_symbolList> enumerator_list enumerator_list_comma
%type <ast_symbol> fun_arg_init_declarations fun_arg_declaration
%type <ast_symbolPositionListPair> parameter_list parameter_type_list
%type <ast_symbolPositionListPair> parameter_identifier_list identifier_list
%type <ast_positionList> argument_expr_list argument_expr_list_opt

%type <ast_typeModifiers> type_specifier2
%type <ast_typeModifiers> struct_or_union_specifier struct_or_union_define_specifier
%type <ast_typeModifiers> enum_specifier enum_define_specifier
%type <ast_typeModifiers> type_name abstract_declarator abstract_declarator2

%type <ast_expressionType> primary_expr postfix_expr unary_expr cast_expr compound_literal
%type <ast_expressionType> multiplicative_expr additive_expr shift_expr
%type <ast_expressionType> relational_expr equality_expr and_expr exclusive_or_expr
%type <ast_expressionType> inclusive_or_expr logical_and_expr logical_or_expr
%type <ast_expressionType> conditional_expr assignment_expr expr maybe_expr

%type <ast_position> STRING_LITERAL '(' ',' ')'

%start file
%%

primary_expr
    : IDENTIFIER            {
        Symbol *symbol = $1.data->symbol;
        if (symbol != NULL && symbol->type == TypeDefault) {
            assert(symbol->storage != StorageTypedef);
            $$.data.typeModifier = symbol->u.typeModifier;
            assert(options.mode);
            $$.data.reference = addCxReference(symbol, $1.data->position, UsageUsed, NO_FILE_NUMBER);
        } else {
            /* implicit function declaration */
            TypeModifier *modifier;
            Symbol *newSymbol;
            Symbol *definitionSymbol;

            modifier = newTypeModifier(TypeInt, NULL, NULL);
            $$.data.typeModifier = newFunctionTypeModifier(NULL, NULL, modifier);

            newSymbol = newSymbolAsType($1.data->name, $1.data->name, $1.data->position, $$.data.typeModifier);
            newSymbol->storage = StorageExtern;

            definitionSymbol = addNewSymbolDefinition(symbolTable, inputFileName, newSymbol, StorageExtern, UsageUsed);
            $$.data.reference = addCxReference(definitionSymbol, $1.data->position, UsageUsed, NO_FILE_NUMBER);
        }
    }
    | CHAR_LITERAL          { $$.data.typeModifier = newSimpleTypeModifier(TypeInt); $$.data.reference = NULL;}
    | CONSTANT              { $$.data.typeModifier = newSimpleTypeModifier(TypeInt); $$.data.reference = NULL;}
    | LONG_CONSTANT         { $$.data.typeModifier = newSimpleTypeModifier(TypeLong); $$.data.reference = NULL;}
    | FLOAT_CONSTANT        { $$.data.typeModifier = newSimpleTypeModifier(TypeFloat); $$.data.reference = NULL;}
    | DOUBLE_CONSTANT       { $$.data.typeModifier = newSimpleTypeModifier(TypeDouble); $$.data.reference = NULL;}
    | string_literals       {
        TypeModifier *modifier;
        modifier = newSimpleTypeModifier(TypeChar);
        $$.data.typeModifier = newPointerTypeModifier(modifier);
        $$.data.reference = NULL;
    }
    | '(' expr ')'          {
        $$.data = $2.data;
    }
    | '(' compound_statement ')'            {       /* GNU's shit */
        $$.data.typeModifier = &errorModifier;
        $$.data.reference = NULL;
    }
    | COMPLETE_OTHER_NAME      { assert(0); /* token never used */ }
    ;

string_literals
    : STRING_LITERAL
    | STRING_LITERAL string_literals
    ;

postfix_expr
    : primary_expr                              /*& { $$.data = $1.data; } &*/
    | postfix_expr '[' expr ']'                 {
        if ($1.data.typeModifier->type==TypePointer || $1.data.typeModifier->type==TypeArray) $$.data.typeModifier=$1.data.typeModifier->next;
        else if ($3.data.typeModifier->type==TypePointer || $3.data.typeModifier->type==TypeArray) $$.data.typeModifier=$3.data.typeModifier->next;
        else $$.data.typeModifier = &errorModifier;
        $$.data.reference = NULL;
        assert($$.data.typeModifier);
    }
/*
  Replaced by optional arguments in "postfix_expr '(' argument_expr_list_opt ')' ...
    | postfix_expr '(' ')'                      {
        if ($1.d.typeModifier->kind==TypeFunction) {
            $$.d.typeModifier=$1.d.typeModifier->next;
            handleInvocationParamPositions($1.d.reference, &$2.d, NULL, &$3.d, 0);
        } else {
            $$.d.typeModifier = &s_errorModifier;
        }
        $$.d.reference = NULL;
        assert($$.d.typeModifier);
    }
*/
    | postfix_expr
            {
                $<typeModifier>$ = upLevelFunctionCompletionType;
                upLevelFunctionCompletionType = $1.data.typeModifier;
            }
      '(' argument_expr_list_opt ')'    {
        upLevelFunctionCompletionType = $<typeModifier>2;
        if ($1.data.typeModifier->type==TypeFunction) {
            $$.data.typeModifier=$1.data.typeModifier->next;
            if ($4.data == NULL) {
                handleInvocationParamPositions($1.data.reference, $3.data, NULL, $5.data, 0);
            } else {
                handleInvocationParamPositions($1.data.reference, $3.data, $4.data->next, $5.data, 1);
            }
        } else {
            $$.data.typeModifier = &errorModifier;
        }
        $$.data.reference = NULL;
        assert($$.data.typeModifier);
    }
    | postfix_expr {setDirectStructureCompletionType($1.data.typeModifier);} '.' field_identifier        {
        Symbol *rec=NULL;
        $$.data.reference = findStructureFieldFromType($1.data.typeModifier, $4.data, &rec);
        assert(rec);
        $$.data.typeModifier = rec->u.typeModifier;
        assert($$.data.typeModifier);
    }
    | postfix_expr {setIndirectStructureCompletionType($1.data.typeModifier);} PTR_OP field_identifier   {
        Symbol *rec=NULL;
        $$.data.reference = NULL;
        if ($1.data.typeModifier->type==TypePointer || $1.data.typeModifier->type==TypeArray) {
            $$.data.reference = findStructureFieldFromType($1.data.typeModifier->next, $4.data, &rec);
            assert(rec);
            $$.data.typeModifier = rec->u.typeModifier;
        } else $$.data.typeModifier = &errorModifier;
        assert($$.data.typeModifier);
    }
    | postfix_expr INC_OP                       {
        $$.data.typeModifier = $1.data.typeModifier;
        resetReferenceUsage($1.data.reference, UsageAddrUsed);
    }
    | postfix_expr DEC_OP                       {
        $$.data.typeModifier = $1.data.typeModifier;
        resetReferenceUsage($1.data.reference, UsageAddrUsed);
    }
    | compound_literal
    ;

compound_literal                /* Added in C99 */
    : '(' type_name ')' '{' initializer_list optional_comma '}'     {
        IdList *idList;
        for (idList = $5.data; idList != NULL; idList = idList->next) {
            Symbol *rec=NULL;
            (void) findStructureFieldFromType($2.data, &idList->id, &rec);
        }
        $$.data.typeModifier = $2.data;
        $$.data.reference = NULL;
    }
    ;

optional_comma
    :
    | ','
    ;

field_identifier
    : identifier                /*& { $$.data = $1.data; } &*/
    | COMPLETE_STRUCT_MEMBER_NAME     { assert(0); /* token never used */ }
    ;

argument_expr_list_opt
    :                                           {
        $$.data = NULL;
    }
    |   argument_expr_list          {
            $$.data = newPositionList(noPosition, $1.data);
        }
    ;

argument_expr_list
    : assignment_expr                           {
        $$.data = NULL;
    }
    | argument_expr_list ',' assignment_expr    {
        $$.data = $1.data;
        appendPositionToList(&$$.data, $2.data);
    }
    | COMPLETE_UP_FUN_PROFILE                          {/* never used */}
    | argument_expr_list ',' COMPLETE_UP_FUN_PROFILE   {/* never used */}
    ;

unary_expr
    : postfix_expr                  /*& { $$.data = $1.data; } &*/
    | INC_OP unary_expr             {
        $$.data.typeModifier = $2.data.typeModifier;
        resetReferenceUsage($2.data.reference, UsageAddrUsed);
    }
    | DEC_OP unary_expr             {
        $$.data.typeModifier = $2.data.typeModifier;
        resetReferenceUsage($2.data.reference, UsageAddrUsed);
    }
    | unary_operator cast_expr      {
        $$.data.typeModifier = $2.data.typeModifier;
        $$.data.reference = NULL;
    }
    | '&' cast_expr                 {
        $$.data.typeModifier = newPointerTypeModifier($2.data.typeModifier);
        resetReferenceUsage($2.data.reference, UsageAddrUsed);
        $$.data.reference = NULL;
    }
    | '*' cast_expr                 {
        if ($2.data.typeModifier->type==TypePointer || $2.data.typeModifier->type==TypeArray) $$.data.typeModifier=$2.data.typeModifier->next;
        else $$.data.typeModifier = &errorModifier;
        assert($$.data.typeModifier);
        $$.data.reference = NULL;
    }
    | SIZEOF unary_expr             {
        $$.data.typeModifier = newSimpleTypeModifier(TypeInt);
        $$.data.reference = NULL;
    }
    | SIZEOF '(' type_name ')'      {
        $$.data.typeModifier = newSimpleTypeModifier(TypeInt);
        $$.data.reference = NULL;
    }
    /* yet another GCC ext. */
    | AND_OP identifier     {
        labelReference($2.data, UsageLvalUsed);
    }
    ;

unary_operator
    : '+'
    | '-'
    | '~'
    | '!'
    ;

cast_expr
    : unary_expr                        /*& { $$.data = $1.data; } &*/
    | '(' type_name ')' cast_expr       {
        $$.data.typeModifier = $2.data;
        $$.data.reference = $4.data.reference;
    }
    ;

multiplicative_expr
    : cast_expr                         /*& { $$.data = $1.data; } &*/
    | multiplicative_expr '*' cast_expr {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    | multiplicative_expr '/' cast_expr {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    | multiplicative_expr '%' cast_expr {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

additive_expr
    : multiplicative_expr                       /*& { $$.data = $1.data; } &*/
    | additive_expr '+' multiplicative_expr     {
        if ($3.data.typeModifier->type==TypePointer || $3.data.typeModifier->type==TypeArray) $$.data.typeModifier = $3.data.typeModifier;
        else $$.data.typeModifier = $1.data.typeModifier;
        $$.data.reference = NULL;
    }
    | additive_expr '-' multiplicative_expr     {
        if ($3.data.typeModifier->type==TypePointer || $3.data.typeModifier->type==TypeArray) $$.data.typeModifier = $3.data.typeModifier;
        else $$.data.typeModifier = $1.data.typeModifier;
        $$.data.reference = NULL;
    }
    ;

shift_expr
    : additive_expr                             /*& { $$.data = $1.data; } &*/
    | shift_expr LEFT_OP additive_expr          {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    | shift_expr RIGHT_OP additive_expr         {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

relational_expr
    : shift_expr                                /*& { $$.data = $1.data; } &*/
    | relational_expr '<' shift_expr            {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    | relational_expr '>' shift_expr            {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    | relational_expr LE_OP shift_expr          {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    | relational_expr GE_OP shift_expr          {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

equality_expr
    : relational_expr                           /*& { $$.data = $1.data; } &*/
    | equality_expr EQ_OP relational_expr       {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    | equality_expr NE_OP relational_expr       {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

and_expr
    : equality_expr                             /*& { $$.data = $1.data; } &*/
    | and_expr '&' equality_expr                {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

exclusive_or_expr
    : and_expr                                  /*& { $$.data = $1.data; } &*/
    | exclusive_or_expr '^' and_expr            {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

inclusive_or_expr
    : exclusive_or_expr                         /*& { $$.data = $1.data; } &*/
    | inclusive_or_expr '|' exclusive_or_expr   {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

logical_and_expr
    : inclusive_or_expr                         /*& { $$.data = $1.data; } &*/
    | logical_and_expr AND_OP inclusive_or_expr {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

logical_or_expr
    : logical_and_expr                          /*& { $$.data = $1.data; } &*/
    | logical_or_expr OR_OP logical_and_expr    {
        $$.data.typeModifier = &defaultIntModifier;
        $$.data.reference = NULL;
    }
    ;

conditional_expr
    : logical_or_expr                                           /*& { $$.data = $1.data; } &*/
    | logical_or_expr '?' logical_or_expr ':' conditional_expr  {
        $$.data.typeModifier = $3.data.typeModifier;
        $$.data.reference = NULL;
    }
    /* GNU Conditional operator (a?:b meaning if a then a else b) */
    | logical_or_expr '?' ':' conditional_expr  {
        $$.data.typeModifier = $4.data.typeModifier;
        $$.data.reference = NULL;
    }
    ;

assignment_expr
    : conditional_expr                                  /*& { $$.data = $1.data; } &*/
    | unary_expr assignment_operator assignment_expr    {
        if ($1.data.reference != NULL && options.serverOperation == OLO_EXTRACT) {
            Reference *r = duplicateReferenceInCxMemory($1.data.reference);
            $1.data.reference->usage = UsageNone;
            if ($2.data == '=') {
                resetReferenceUsage(r, UsageLvalUsed);
            } else {
                resetReferenceUsage(r, UsageAddrUsed);
            }
        } else {
            if ($2.data == '=') {
                resetReferenceUsage($1.data.reference, UsageLvalUsed);
            } else {
                resetReferenceUsage($1.data.reference, UsageAddrUsed);
            }
        }
        $$.data = $1.data;    /* $$.d.r will be used for FOR completions ! */
    }
    ;

assignment_operator
    : '='                   {$$.data = '=';}
    | MUL_ASSIGN            {$$.data = MUL_ASSIGN;}
    | DIV_ASSIGN            {$$.data = DIV_ASSIGN;}
    | MOD_ASSIGN            {$$.data = MOD_ASSIGN;}
    | ADD_ASSIGN            {$$.data = ADD_ASSIGN;}
    | SUB_ASSIGN            {$$.data = SUB_ASSIGN;}
    | LEFT_ASSIGN           {$$.data = LEFT_ASSIGN;}
    | RIGHT_ASSIGN          {$$.data = RIGHT_ASSIGN;}
    | AND_ASSIGN            {$$.data = AND_ASSIGN;}
    | XOR_ASSIGN            {$$.data = XOR_ASSIGN;}
    | OR_ASSIGN             {$$.data = OR_ASSIGN;}
    ;

expr
    : assignment_expr                           /*& { $$.data = $1.data; } &*/
    | expr ',' assignment_expr                  {
        $$.data.typeModifier = $3.data.typeModifier;
        $$.data.reference = NULL;
    }
    ;

constant_expr
    : conditional_expr
    ;

Save_index
    :    {
        $$.data = savedWorkMemoryIndex;
    }
    ;

declaration
    : Save_index declaration_specifiers ';'     { savedWorkMemoryIndex = $1.data; }
    | Save_index init_declarations ';'          { savedWorkMemoryIndex = $1.data; }
    | error
        {
#if YYDEBUG
            char buffer[1000];
            sprintf(buffer, "DEBUG: error parsing declaration, near '%s'", yytext);
            yyerror(buffer);
#endif
        }
    ;

init_declarations
    : declaration_specifiers init_declarator eq_initializer_opt {
        $$.data = $1.data;
        addNewDeclaration(symbolTable, $1.data, $2.data, $3.data, StorageAuto);
    }
    | init_declarations ',' init_declarator eq_initializer_opt  {
        $$.data = $1.data;
        addNewDeclaration(symbolTable, $1.data, $3.data, $4.data, StorageAuto);
    }
    | error                                             {
        /* $$.d = &s_errorSymbol; */
        $$.data = typeSpecifier2(&errorModifier);
#if YYDEBUG
        char buffer[1000];
        sprintf(buffer, "DEBUG: error parsing init_declarations, near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    ;

declaration_specifiers
    : declaration_modality_specifiers
    | declaration_specifiers0
    ;

user_defined_type
    : TYPE_NAME                                             {
        int usage;
        $$.data = $1.data;
        assert(options.mode);
        assert($1.data);
        assert($1.data->symbol);
        if (nestingLevel() == 0)
            usage = USAGE_TOP_LEVEL_USED;
        else
            usage = UsageUsed;
        addCxReference($1.data->symbol, $1.data->position,usage,NO_FILE_NUMBER);
    }
    ;

declaration_specifiers0
    : user_defined_type                                     {
        assert($1.data);
        assert($1.data->symbol);
        $$.data = typeSpecifier2($1.data->symbol->u.typeModifier);
    }
    | type_specifier1                                       {
        $$.data  = typeSpecifier1($1.data);
    }
    | type_specifier2                                       {
        $$.data  = typeSpecifier2($1.data);
    }
    | declaration_modality_specifiers  user_defined_type    {
        assert($2.data);
        assert($2.data->symbol);
        $$.data = $1.data;
        declTypeSpecifier2($1.data,$2.data->symbol->u.typeModifier);
    }
    | declaration_modality_specifiers type_specifier1       {
        $$.data = $1.data;
        declTypeSpecifier1($1.data,$2.data);
    }
    | declaration_modality_specifiers type_specifier2       {
        $$.data = $1.data;
        declTypeSpecifier2($1.data,$2.data);
    }
    | declaration_specifiers0 type_modality_specifier       {
        $$.data = $1.data;
        declTypeSpecifier1($1.data,$2.data);
    }
    | declaration_specifiers0 type_specifier1               {
        $$.data = $1.data;
        declTypeSpecifier1($1.data,$2.data);
    }
    | declaration_specifiers0 type_specifier2               {
        $$.data = $1.data;
        declTypeSpecifier2($1.data,$2.data);
    }
    | declaration_specifiers0 storage_class_specifier       {
        $$.data = $1.data;
        $$.data->storage = $2.data;
    }
    | declaration_specifiers0 function_specifier            {
        $$.data = $1.data;
    }
    | COMPLETE_TYPE_NAME                                       {
        assert(0);
    }
    | declaration_modality_specifiers COMPLETE_TYPE_NAME       {
        assert(0); /* token never used */
    }
    ;

declaration_modality_specifiers
    : storage_class_specifier                               {
        $$.data  = typeSpecifier1(TypeDefault);
        $$.data->storage = $1.data;
    }
    | declaration_modality_specifiers storage_class_specifier       {
        $$.data = $1.data;
        $$.data->storage = $2.data;
    }
    | type_modality_specifier                               {
        $$.data  = typeSpecifier1($1.data);
    }
    | declaration_modality_specifiers type_modality_specifier       {
        declTypeSpecifier1($1.data, $2.data);
    }
    | function_specifier                                    {
        $$.data = typeSpecifier1(TypeDefault);
    }
    | declaration_modality_specifiers function_specifier            {
        $$.data = $1.data;
    }
    ;


/* a gcc extension ? */
asm_opt
    :
    |   ASM_KEYWORD '(' string_literals ')'
    ;

eq_initializer_opt
    :                   {
        $$.data = NULL;
    }
    | '=' initializer   {
        $$.data = $2.data;
    }
    ;

init_declarator
    : declarator asm_opt /* eq_initializer_opt   { $$.d = $1.d; } */
    ;

storage_class_specifier
    : TYPEDEF       { $$.data = StorageTypedef; }
    | EXTERN        { $$.data = StorageExtern; }
    | STATIC        { $$.data = StorageStatic; }
    | _THREADLOCAL  { $$.data = StorageThreadLocal; }
    | AUTO          { $$.data = StorageAuto; }
    | REGISTER      { $$.data = StorageAuto; }
    ;

type_modality_specifier
    : CONST         { $$.data = TypeDefault; }
    | RESTRICT      { $$.data = TypeDefault; }
    | VOLATILE      { $$.data = TypeDefault; }
    | _ATOMIC       { $$.data = TypeDefault; }
    | ANONYMOUS_MODIFIER   { $$.data = TypeDefault; }
    ;

type_modality_specifier_opt
    :
    | type_modality_specifier
    ;

type_specifier1
    : CHAR      { $$.data = TypeChar; }
    | SHORT     { $$.data = TmodShort; }
    | INT       { $$.data = TypeInt; }
    | LONG      { $$.data = TmodLong; }
    | SIGNED    { $$.data = TmodSigned; }
    | UNSIGNED  { $$.data = TmodUnsigned; }
    | FLOAT     { $$.data = TypeFloat; }
    | DOUBLE    { $$.data = TypeDouble; }
    | VOID      { $$.data = TypeVoid; }
    | _BOOL     { $$.data = TypeBoolean; }
    ;

type_specifier2
    : struct_or_union_specifier     /*& { $$.data = $1.data; } &*/
    | enum_specifier                /*& { $$.data = $1.data; } &*/
    ;

function_specifier
    : INLINE
    | _NORETURN
    ;

struct_or_union_specifier
    : struct_or_union struct_identifier                             {
        int usage;
        if (nestingLevel() == 0)
            usage = USAGE_TOP_LEVEL_USED;
        else
            usage = UsageUsed;
        $$.data = simpleStructOrUnionSpecifier($1.data, $2.data, usage);
    }
    | struct_or_union_define_specifier '{' struct_declaration_list '}'{
        assert($1.data && $1.data->u.t);
        $$.data = $1.data;
        specializeStructOrUnionDef($$.data->u.t, $3.data);
    }
    | struct_or_union_define_specifier '{' '}'                      {
        $$.data = $1.data;
    }
    ;

struct_or_union_define_specifier
    : struct_or_union struct_identifier                             {
        $$.data = simpleStructOrUnionSpecifier($1.data, $2.data, UsageDefined);
    }
    | struct_or_union                                               {
        $$.data = createNewAnonymousStructOrUnion($1.data);
    }
    ;

struct_identifier
    : identifier            /*& { $$.data = $1.data; } &*/
    | COMPLETE_STRUCT_NAME     { assert(0); /* token never used */ }
    ;

struct_or_union
    : STRUCT        { $$.data = $1.data; }
    | UNION         { $$.data = $1.data; }
    ;

struct_declaration_list
    : struct_declaration                                /*& { $$.data = $1.data; } &*/
    | struct_declaration_list struct_declaration        {
        if ($1.data == &errorSymbol || $1.data->type==TypeError) {
            $$.data = $2.data;
        } else if ($2.data == &errorSymbol || $1.data->type==TypeError)  {
            $$.data = $1.data;
        } else {
            $$.data = $1.data;
            LIST_APPEND(Symbol, $$.data, $2.data);
        }
    }
    ;

struct_declaration
    : Save_index type_specifier_list struct_declarator_list ';'     {
        assert($2.data && $3.data);
        for (Symbol *symbol=$3.data; symbol!=NULL; symbol=symbol->next) {
            completeDeclarator($2.data, symbol);
        }
        $$.data = $3.data;
        savedWorkMemoryIndex = $1.data;
    }
    | error                                             {
        $$.data = newSymbolAsCopyOf(&errorSymbol);
#if YYDEBUG
        char buffer[1000];
        sprintf(buffer, "DEBUG: error parsing struct_declaration near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    ;

struct_declarator_list
    : struct_declarator                                 {
        $$.data = $1.data;
        assert($$.data->next == NULL);
    }
    | struct_declarator_list ',' struct_declarator      {
        $$.data = $1.data;
        assert($3.data->next == NULL);
        LIST_APPEND(Symbol, $$.data, $3.data);
    }
    ;

struct_declarator
    : /* gcc extension allow empty field */ {
        $$.data = createEmptyField();
    }
    | ':' constant_expr             {
        $$.data = createEmptyField();
    }
    | declarator                    /*& { $$.data = $1.data; } &*/
    | declarator ':' constant_expr  /*& { $$.data = $1.data; } &*/
    ;

enum_specifier
    : ENUM enum_identifier                                  {
        Usage usage;
        if (nestingLevel() == 0)
            usage = USAGE_TOP_LEVEL_USED;
        else
            usage = UsageUsed;
        $$.data = simpleEnumSpecifier($2.data, usage);
    }
    | enum_define_specifier '{' enumerator_list_comma '}'       {
        assert($1.data && $1.data->type == TypeEnum && $1.data->u.t);
        $$.data = $1.data;
        if ($$.data->u.t->u.enums==NULL) {
            $$.data->u.t->u.enums = $3.data;
            addToFrame(setToNull, &($$.data->u.t->u.enums));
        }
    }
    | ENUM '{' enumerator_list_comma '}'                        {
        $$.data = createNewAnonymousEnum($3.data);
    }
    ;

enum_define_specifier
    : ENUM enum_identifier                                  {
        $$.data = simpleEnumSpecifier($2.data, UsageDefined);
    }
    ;

enum_identifier
    : identifier            /*& { $$.data = $1.data; } &*/
    | COMPLETE_ENUM_NAME       { assert(0); /* token never used */ }
    ;

enumerator_list_comma
    : enumerator_list               /*& { $$.data = $1.data; } &*/
    | enumerator_list ','           /*& { $$.data = $1.data; } &*/
    ;

enumerator_list
    : enumerator                            {
        $$.data = createDefinitionList($1.data);
    }
    | enumerator_list ',' enumerator        {
        $$.data = $1.data;
        LIST_APPEND(SymbolList, $$.data, createDefinitionList($3.data));
    }
    ;

enumerator
    : identifier                            {
        $$.data = createSimpleDefinition(StorageConstant,TypeInt,$1.data);
        addNewSymbolDefinition(symbolTable, inputFileName, $$.data, StorageConstant, UsageDefined);
    }
    | identifier '=' constant_expr          {
        $$.data = createSimpleDefinition(StorageConstant,TypeInt,$1.data);
        addNewSymbolDefinition(symbolTable, inputFileName, $$.data, StorageConstant, UsageDefined);
    }
    | error                                 {
        $$.data = newSymbolAsCopyOf(&errorSymbol);
#if YYDEBUG
        char buffer[1000];
        sprintf(buffer, "DEBUG: error parsing enumerator near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    | COMPLETE_OTHER_NAME      { assert(0); /* token never used */ }
    ;

declarator
    : declarator2                                       /*& { $$.data = $1.data; } &*/
    | pointer declarator2                               {
        $$.data = $2.data;
        assert($$.data->npointers == 0);
        $$.data->npointers = $1.data;
    }
    ;

declarator2
    : identifier                                        {
        $$.data = newSymbol($1.data->name, $1.data->name, $1.data->position);
    }
    | '(' declarator ')'                                {
        $$.data = $2.data;
        unpackPointers($$.data);
    }
    | declarator2 '[' ']'                               {
        assert($1.data);
        $$.data = $1.data;
        addComposedTypeToSymbol($$.data, TypeArray);
    }
    | declarator2 '[' constant_expr ']'                 {
        assert($1.data);
        $$.data = $1.data;
        addComposedTypeToSymbol($$.data, TypeArray);
    }
    | declarator2 '(' ')'                               {
        TypeModifier *modifier;
        assert($1.data);
        $$.data = $1.data;
        modifier = addComposedTypeToSymbol($$.data, TypeFunction);
        initFunctionTypeModifier(&modifier->u.f , NULL);
        handleDeclaratorParamPositions($1.data, $2.data, NULL, $3.data, false, false);
    }
    | declarator2 '(' parameter_type_list ')'           {
        TypeModifier *modifier;
        assert($1.data);
        $$.data = $1.data;
        modifier = addComposedTypeToSymbol($$.data, TypeFunction);
        initFunctionTypeModifier(&modifier->u.f , $3.data.symbol);
        bool isVoid = $3.data.symbol->u.typeModifier->type == TypeVoid;
        handleDeclaratorParamPositions($1.data, $2.data, $3.data.positionList, $4.data, true, isVoid);
    }
    | declarator2 '(' parameter_identifier_list ')'     {
        TypeModifier *modifier;
        assert($1.data);
        $$.data = $1.data;
        modifier = addComposedTypeToSymbol($$.data, TypeFunction);
        initFunctionTypeModifier(&modifier->u.f , $3.data.symbol);
        handleDeclaratorParamPositions($1.data, $2.data, $3.data.positionList, $4.data, true, false);
    }
    | COMPLETE_OTHER_NAME      { assert(0); /* token never used */ }
    ;

pointer
    : '*'                                   {
        $$.data = 1;
    }
    | '*' type_mod_specifier_list               {
        $$.data = 1;
    }
    | '*' pointer                           {
        $$.data = $2.data+1;
    }
    | '*' type_mod_specifier_list pointer       {
        $$.data = $3.data+1;
    }
    ;

type_mod_specifier_list
    : type_modality_specifier                                   {
        $$.data  = typeSpecifier1($1.data);
    }
    | type_mod_specifier_list type_modality_specifier           {
        declTypeSpecifier1($1.data, $2.data);
    }
    ;

/*
type_specifier_list
    : type_specifier1                                   {
        $$.d  = typeSpecifier1($1.d);
    }
    | type_specifier2                                   {
        $$.d  = typeSpecifier2($1.d);
    }
    | type_specifier_list type_specifier1               {
        declTypeSpecifier1($1.d, $2.d);
    }
    | type_specifier_list type_specifier2               {
        declTypeSpecifier2($1.d, $2.d);
    }
    ;
*/

type_specifier_list
    : type_mod_specifier_list                       /*& { $$.data = $1.data; } &*/
    | type_specifier_list0                          /*& { $$.data = $1.data; } &*/
    ;

type_specifier_list0
    : user_defined_type                                     {
        assert($1.data);
        assert($1.data->symbol);
        assert($1.data->symbol);
        $$.data = typeSpecifier2($1.data->symbol->u.typeModifier);
    }
    | type_specifier1                                       {
        $$.data  = typeSpecifier1($1.data);
    }
    | type_specifier2                                       {
        $$.data  = typeSpecifier2($1.data);
    }
    | type_mod_specifier_list user_defined_type             {
        assert($2.data);
        assert($2.data->symbol);
        assert($2.data->symbol);
        $$.data = $1.data;
        declTypeSpecifier2($1.data,$2.data->symbol->u.typeModifier);
    }
    | type_mod_specifier_list type_specifier1       {
        $$.data = $1.data;
        declTypeSpecifier1($1.data,$2.data);
    }
    | type_mod_specifier_list type_specifier2       {
        $$.data = $1.data;
        declTypeSpecifier2($1.data,$2.data);
    }
    | type_specifier_list0 type_modality_specifier      {
        $$.data = $1.data;
        declTypeSpecifier1($1.data,$2.data);
    }
    | type_specifier_list0 type_specifier1              {
        $$.data = $1.data;
        declTypeSpecifier1($1.data,$2.data);
    }
    | type_specifier_list0 type_specifier2              {
        $$.data = $1.data;
        declTypeSpecifier2($1.data,$2.data);
    }
    | COMPLETE_TYPE_NAME                                       {
        assert(0);
    }
    | type_mod_specifier_list COMPLETE_TYPE_NAME       {
        assert(0); /* token never used */
    }
    ;

parameter_identifier_list
    : identifier_list                           /*& { $$.data = $1.data; } &*/
    | identifier_list ',' ELLIPSIS               {
        Symbol *symbol;
        Position pos = makePosition(-1, 0, 0);

        symbol = newSymbol("", "", pos);
        symbol->type = TypeElipsis;
        $$.data = $1.data;

        LIST_APPEND(Symbol, $$.data.symbol, symbol);
        appendPositionToList(&$$.data.positionList, $2.data);
    }
    ;

identifier_list
    : IDENTIFIER                                {
        Symbol *symbol;
        symbol = newSymbol($1.data->name, $1.data->name, $1.data->position);
        $$.data.symbol = symbol;
        $$.data.positionList = NULL;
    }
    | identifier_list ',' identifier            {
        Symbol *symbol;
        symbol = newSymbol($3.data->name, $3.data->name, $3.data->position);
        $$.data = $1.data;
        LIST_APPEND(Symbol, $$.data.symbol, symbol);
        appendPositionToList(&$$.data.positionList, $2.data);
    }
    | COMPLETE_OTHER_NAME      { assert(0); /* token never used */ }
    ;

parameter_type_list
    : parameter_list                    /*& { $$.data = $1.data; } &*/
    | parameter_list ',' ELLIPSIS                {
        Symbol *symbol;
        Position position = makePosition(-1, 0, 0);

        symbol = newSymbol("", "", position);
        symbol->type = TypeElipsis;
        $$.data = $1.data;

        LIST_APPEND(Symbol, $$.data.symbol, symbol);
        appendPositionToList(&$$.data.positionList, $2.data);
    }
    ;

parameter_list
    : parameter_declaration                         {
        $$.data.symbol = $1.data;
        $$.data.positionList = NULL;
    }
    | parameter_list ',' parameter_declaration      {
        $$.data = $1.data;
        LIST_APPEND(Symbol, $1.data.symbol, $3.data);
        appendPositionToList(&$$.data.positionList, $2.data);
    }
    ;


parameter_declaration
    : declaration_specifiers declarator         {
        completeDeclarator($1.data, $2.data);
        $$.data = $2.data;
    }
    | type_name                                 {
        $$.data = newSymbolAsType(NULL, NULL, noPosition, $1.data);
    }
    | error                                     {
        $$.data = newSymbolAsCopyOf(&errorSymbol);
#if YYDEBUG
        char buffer[1000];
        sprintf(buffer, "DEBUG: error parsing parameter_declaration near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    ;

type_name
    : declaration_specifiers                        {
        $$.data = $1.data->u.typeModifier;
    }
    | declaration_specifiers abstract_declarator    {
        $$.data = $2.data;
        LIST_APPEND(TypeModifier, $$.data, $1.data->u.typeModifier);
    }
    ;

abstract_declarator
    : pointer                               {
        int i;
        $$.data = newPointerTypeModifier(NULL);
        for(i=1; i<$1.data; i++) appendComposedType(&($$.data), TypePointer);
    }
    | abstract_declarator2                  {
        $$.data = $1.data;
    }
    | pointer abstract_declarator2          {
        int i;
        $$.data = $2.data;
        for(i=0; i<$1.data; i++) appendComposedType(&($$.data), TypePointer);
    }
    ;

abstract_declarator2
    : '(' abstract_declarator ')'           {
        $$.data = $2.data;
    }
    | '[' ']'                               {
        $$.data = newArrayTypeModifier();
    }
    | '[' constant_expr ']'                 {
        $$.data = newArrayTypeModifier();
    }
    | abstract_declarator2 '[' ']'          {
        $$.data = $1.data;
        appendComposedType(&($$.data), TypeArray);
    }
    | abstract_declarator2 '[' constant_expr ']'    {
        $$.data = $1.data;
        appendComposedType(&($$.data), TypeArray);
    }
    | '(' ')'                                       {
        $$.data = newFunctionTypeModifier(NULL, NULL, NULL);
    }
    | '(' parameter_type_list ')'                   {
        $$.data = newFunctionTypeModifier($2.data.symbol, NULL, NULL);
    }
    | abstract_declarator2 '(' ')'                  {
        TypeModifier *modifier;
        $$.data = $1.data;
        modifier = appendComposedType(&($$.data), TypeFunction);
        initFunctionTypeModifier(&modifier->u.f , NULL);
    }
    | abstract_declarator2 '(' parameter_type_list ')'          {
        TypeModifier *modifier;
        $$.data = $1.data;
        modifier = appendComposedType(&($$.data), TypeFunction);
        initFunctionTypeModifier(&modifier->u.f , NULL);
    }
    ;

initializer
    : assignment_expr       {
        $$.data = NULL;
    }
      /* it is enclosed because on linux kernel it overflows memory */
    | '{' initializer_list '}'  {
        $$.data = $2.data;
    }
    | '{' initializer_list ',' '}'  {
        $$.data = $2.data;
    }
    | error             {
        $$.data = NULL;
#if YYDEBUG
        char buffer[1000];
        sprintf(buffer, "DEBUG: error parsing initializer, near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    ;

initializer_list
    : Save_index designation_opt Start_block initializer End_block {
        $$.data = $2.data;
        savedWorkMemoryIndex = $1.data;
    }
    | initializer_list ',' Save_index designation_opt Start_block initializer End_block {
        LIST_APPEND(IdList, $1.data, $4.data);
        savedWorkMemoryIndex = $3.data;
    }
    ;

designation_opt
    :                           {
        $$.data = NULL;
    }
    | designator_list '='       {
        $$.data = stackMemoryAlloc(sizeof(IdList));
        *($$.data) = makeIdList(*$1.data, NULL);
    }
    ;

designator_list
    : designator                    {
        $$.data = $1.data;
    }
    | designator_list designator    {
        LIST_APPEND(Id, $1.data, $2.data);
    }
    ;

designator
    : '[' constant_expr ']'     {
        $$.data = stackMemoryAlloc(sizeof(Id));
        *($$.data) = makeId("", NULL, noPosition);
    }
    | '.' field_identifier    {
        $$.data = stackMemoryAlloc(sizeof(Id));
        *($$.data) = *($2.data);
    }
    ;

statement
    : Save_index labeled_statement      {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index compound_statement     {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index expression_statement       {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index selection_statement        {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index iteration_statement        {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index jump_statement     {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index asm_statement      {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index error  {
        savedWorkMemoryIndex = $1.data;
    }
    ;

label
    :   label_def_name ':'
    |   CASE constant_expr ':' {
            generateSwitchCaseFork(false);
    }
    |   CASE constant_expr ELLIPSIS constant_expr ':' {
            generateSwitchCaseFork(false);
    }
    |   DEFAULT ':' {
            generateSwitchCaseFork(false);
    }
    ;

labeled_statement
    : label statement
    ;

label_def_name
    : identifier            {
        labelReference($1.data,UsageDefined);
    }
    | COMPLETE_LABEL_NAME      { assert(0); /* token never used */ }
    ;

label_name
    : identifier            {
        labelReference($1.data,UsageUsed);
    }
    | COMPLETE_LABEL_NAME      { assert(0); /* token never used */ }
    ;

compound_statement
    : '{' '}'
    | '{' Start_block label_decls_opt statement_list End_block '}'
/*&
    | '{' Start_block label_decls_opt declaration_list End_block '}'
    | '{' Start_block label_decls_opt declaration_list statement_list End_block '}'
&*/
    ;

label_decls_opt
    :
    |   label_decls
    ;

label_decls
    :   LABEL identifier {
        labelReference($2.data,UsageDeclared);
    }
    |   label_decls LABEL identifier {
        labelReference($3.data,UsageDeclared);
    }
    ;

/*&
declaration_list
    : declaration
    | declaration_list declaration
    ;
&*/

/* allowing declarations inside statements makes better error recovery.
   If an error occurs in one of early declarations, this worked only
   because of some strange recovering
 */

statement_list
    : statement
    | statement_list statement
    | declaration
    | statement_list declaration
    ;

maybe_expr
    :                   { $$.data.typeModifier = NULL; $$.data.reference = NULL; }
    | expr              { $$.data = $1.data; }
    ;

expression_statement
    : maybe_expr ';'
    ;


_ncounter_:  {$$.data = nextGeneratedLocalSymbol();}
    ;

_nlabel_:   {$$.data = nextGeneratedLabelSymbol();}
    ;

_ngoto_:    {$$.data = nextGeneratedGotoSymbol();}
    ;

_nfork_:    {$$.data = nextGeneratedForkSymbol();}
    ;

selection_statement
    : IF '(' expr ')' _nfork_ statement                     {
        generateInternalLabelReference($5.data, UsageDefined);
    }
    | IF '(' expr ')' _nfork_ statement ELSE _ngoto_ {
        generateInternalLabelReference($5.data, UsageDefined);
    }   statement                               {
        generateInternalLabelReference($8.data, UsageDefined);
    }
    | SWITCH '(' expr ')' /*5*/ _ncounter_  {/*6*/
        $<symbol>$ = addContinueBreakLabelSymbol(1000*$5.data, SWITCH_LABEL_NAME);
    } {/*7*/
        $<symbol>$ = addContinueBreakLabelSymbol($5.data, BREAK_LABEL_NAME);
        generateInternalLabelReference($5.data, UsageFork);
    } statement                 {
        generateSwitchCaseFork(true);
        deleteContinueBreakSymbol($<symbol>7);
        deleteContinueBreakSymbol($<symbol>6);
        generateInternalLabelReference($5.data, UsageDefined);
    }
    ;

for1maybe_expr
    : maybe_expr            {completionTypeForForStatement=$1.data;}
    ;

iteration_statement
    : WHILE _nlabel_ '(' expr ')' /*6*/ _nfork_
    {/*7*/
        $<symbol>$ = addContinueBreakLabelSymbol($2.data, CONTINUE_LABEL_NAME);
    } {/*8*/
        $<symbol>$ = addContinueBreakLabelSymbol($6.data, BREAK_LABEL_NAME);
    } statement                 {
        deleteContinueBreakSymbol($<symbol>8);
        deleteContinueBreakSymbol($<symbol>7);
        generateInternalLabelReference($2.data, UsageUsed);
        generateInternalLabelReference($6.data, UsageDefined);
    }

    | DO _nlabel_ _ncounter_ _ncounter_ { /*5*/
        $<symbol>$ = addContinueBreakLabelSymbol($3.data, CONTINUE_LABEL_NAME);
    } {/*6*/
        $<symbol>$ = addContinueBreakLabelSymbol($4.data, BREAK_LABEL_NAME);
    } statement WHILE {
        deleteContinueBreakSymbol($<symbol>6);
        deleteContinueBreakSymbol($<symbol>5);
        generateInternalLabelReference($3.data, UsageDefined);
    } '(' expr ')' ';'          {
        generateInternalLabelReference($2.data, UsageFork);
        generateInternalLabelReference($4.data, UsageDefined);
    }

    | FOR '(' for1maybe_expr ';'
            /*5*/ _nlabel_  maybe_expr ';'  /*8*/_ngoto_
            /*9*/ _nlabel_  maybe_expr ')' /*12*/ _nfork_
        { /*13*/
            generateInternalLabelReference($5.data, UsageUsed);
            generateInternalLabelReference($8.data, UsageDefined);
            $<symbol>$ = addContinueBreakLabelSymbol($9.data, CONTINUE_LABEL_NAME);
        }
        { /*14*/
            $<symbol>$ = addContinueBreakLabelSymbol($12.data, BREAK_LABEL_NAME);
        }
            statement
        {
            deleteContinueBreakSymbol($<symbol>14);
            deleteContinueBreakSymbol($<symbol>13);
            generateInternalLabelReference($9.data, UsageUsed);
            generateInternalLabelReference($12.data, UsageDefined);
        }

    | FOR '(' init_declarations ';'
            /*5*/ _nlabel_  maybe_expr ';'  /*8*/_ngoto_
            /*9*/ _nlabel_  maybe_expr ')' /*12*/ _nfork_
        { /*13*/
            generateInternalLabelReference($5.data, UsageUsed);
            generateInternalLabelReference($8.data, UsageDefined);
            $<symbol>$ = addContinueBreakLabelSymbol($9.data, CONTINUE_LABEL_NAME);
        }
        { /*14*/
            $<symbol>$ = addContinueBreakLabelSymbol($12.data, BREAK_LABEL_NAME);
        }
            statement
        {
            deleteContinueBreakSymbol($<symbol>14);
            deleteContinueBreakSymbol($<symbol>13);
            generateInternalLabelReference($9.data, UsageUsed);
            generateInternalLabelReference($12.data, UsageDefined);
        }

    | FOR '(' for1maybe_expr ';' COMPLETE_FOR_STATEMENT1
    | FOR '(' for1maybe_expr ';' _nlabel_  maybe_expr ';' COMPLETE_FOR_STATEMENT2
    | FOR '(' init_declarations ';' COMPLETE_FOR_STATEMENT1
    | FOR '(' init_declarations ';' _nlabel_  maybe_expr ';' COMPLETE_FOR_STATEMENT2
    ;

jump_statement
    : GOTO label_name ';'
    | CONTINUE ';'          {
        generateContinueBreakReference(CONTINUE_LABEL_NAME);
    }
    | BREAK ';'             {
        generateContinueBreakReference(BREAK_LABEL_NAME);
    }
    | RETURN ';'            {
        generateInternalLabelReference(-1, UsageUsed);
    }
    | RETURN expr ';'       {
        generateInternalLabelReference(-1, UsageUsed);
    }
    ;

_bef_:          {
        actionsBeforeAfterExternalDefinition();
    }
    ;

/* ****************** following is some gcc asm support ************ */
/* it is not exactly as in gcc, but I hope it is suf. general */

gcc_asm_symbolic_name_opt
    :
    |   '[' IDENTIFIER ']'
    ;

gcc_asm_item_opt
    :
    |   gcc_asm_symbolic_name_opt IDENTIFIER
    |   gcc_asm_symbolic_name_opt IDENTIFIER '(' expr ')'
    |   gcc_asm_symbolic_name_opt string_literals
    |   gcc_asm_symbolic_name_opt string_literals '(' expr ')'
    ;

gcc_asm_item_list
    :   gcc_asm_item_opt
    |   gcc_asm_item_list ',' gcc_asm_item_opt
    ;

gcc_asm_oper
    :   ':' gcc_asm_item_list
    |   gcc_asm_oper ':' gcc_asm_item_list
    ;

asm_statement
    :   ASM_KEYWORD type_modality_specifier_opt '(' expr ')' ';'
    |   ASM_KEYWORD type_modality_specifier_opt '(' expr gcc_asm_oper ')' ';'
    ;

/* *********************************************************************** */

file
    : _bef_
    | _bef_ cached_external_definition_list _bef_
    ;

cached_external_definition_list
    : external_definition               {
        if (includeStack.pointer == 0) {
            placeCachePoint(true);
        }
    }
    | cached_external_definition_list _bef_ external_definition {
        if (includeStack.pointer == 0) {
            placeCachePoint(true);
        }
    }
    | error
    ;

external_definition
    : Save_index declaration_specifiers ';'     {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index top_init_declarations ';'      {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index function_definition_head {
        Symbol *symbol;
        int i;
        assert($2.data);
        // I think that due to the following line sometimes
        // storage was not extern, see 'addNewSymbolDef'
        /*& if ($2.data->storage == StorageDefault) $2.data->storage = StorageExtern; &*/
        // TODO!!!, here you should check if there is previous declaration of
        // the function, if yes and is declared static, make it static!
        addNewSymbolDefinition(symbolTable, inputFileName, $2.data, StorageExtern, UsageDefined);
        savedWorkMemoryIndex = $1.data;
        beginBlock();
        counters.localVar = 0;
        assert($2.data->u.typeModifier && $2.data->u.typeModifier->type == TypeFunction);
        parsedInfo.function = $2.data;
        generateInternalLabelReference(-1, UsageDefined);
        for (symbol=$2.data->u.typeModifier->u.f.args, i=1; symbol!=NULL; symbol=symbol->next,i++) {
            if (symbol->type == TypeElipsis)
                continue;
            if (symbol->u.typeModifier == NULL)
                symbol->u.typeModifier = &defaultIntModifier;
            addFunctionParameterToSymTable(symbolTable, $2.data, symbol, i);
        }
    } compound_statement {
        endBlock();
        parsedInfo.function = NULL;
    }
    | Save_index EXTERN STRING_LITERAL  external_definition {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index EXTERN STRING_LITERAL  '{' cached_external_definition_list '}' {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index ASM_KEYWORD '(' expr ')' ';'       {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index error compound_statement       {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index error      {
        savedWorkMemoryIndex = $1.data;
    }
    | Save_index ';'        {  /* empty external definition */
        savedWorkMemoryIndex = $1.data;
    }
    ;

top_init_declarations
    : declaration_specifiers init_declarator eq_initializer_opt {
        $$.data = $1.data;
        addNewDeclaration(symbolTable, $1.data, $2.data, $3.data, StorageExtern);
    }
    | init_declarator eq_initializer_opt                        {
        $$.data = & defaultIntDefinition;
        addNewDeclaration(symbolTable, $$.data, $1.data, $2.data, StorageExtern);
    }
    | top_init_declarations ',' init_declarator eq_initializer_opt          {
        $$.data = $1.data;
        addNewDeclaration(symbolTable, $1.data, $3.data, $4.data, StorageExtern);
    }
    | error                                                     {
        /* $$.d = &s_errorSymbol; */
        $$.data = typeSpecifier2(&errorModifier);
    }
    ;

function_definition_head
    : function_head_declaration                         /*& { $$.data = $1.data; } &*/
    | function_definition_head fun_arg_declaration      {
        assert($1.data->u.typeModifier && $1.data->u.typeModifier->type == TypeFunction);
        Result r = mergeArguments($1.data->u.typeModifier->u.f.args, $2.data);
        if (r == RESULT_ERR) YYERROR;
        $$.data = $1.data;
    }
    ;

fun_arg_declaration
    : declaration_specifiers ';'                                {
        $$.data = NULL;
    }
    | declaration_specifiers fun_arg_init_declarations ';'      {
        Symbol *symbol;
        assert($1.data && $2.data);
        for(symbol=$2.data; symbol!=NULL; symbol=symbol->next) {
            completeDeclarator($1.data, symbol);
        }
        $$.data = $2.data;
    }
    ;

fun_arg_init_declarations
    : init_declarator eq_initializer_opt            {
        $$.data = $1.data;
    }
    | fun_arg_init_declarations ',' init_declarator eq_initializer_opt              {
        $$.data = $1.data;
        LIST_APPEND(Symbol, $$.data, $3.data);
    }
    | fun_arg_init_declarations ',' error                       {
        $$.data = $1.data;
    }
    ;

function_head_declaration
    : declarator                            {
        completeDeclarator(&defaultIntDefinition, $1.data);
        assert($1.data && $1.data->u.typeModifier);
        if ($1.data->u.typeModifier->type != TypeFunction) YYERROR;
        $$.data = $1.data;
    }
    | declaration_specifiers declarator     {
        completeDeclarator($1.data, $2.data);
        assert($2.data && $2.data->u.typeModifier);
        if ($2.data->u.typeModifier->type != TypeFunction) YYERROR;
        $$.data = $2.data;
    }
    ;


Start_block:    { beginBlock(); }
    ;

End_block:     { endBlock(); }
    ;

identifier
    : IDENTIFIER    /*& { $$.data = $1.data; } &*/
    | TYPE_NAME     /*& { $$.data = $1.data; } &*/
    ;

%%

static CompletionFunctionsTable specialCompletionsCollectorsTable[]  = {
    {COMPLETE_FOR_STATEMENT1,    collectForStatementCompletions1},
    {COMPLETE_FOR_STATEMENT2,    collectForStatementCompletions2},
    {COMPLETE_UP_FUN_PROFILE,    completeUpFunProfile},
    {0,NULL}
};

static CompletionFunctionsTable completionsCollectorsTable[]  = {
    {COMPLETE_TYPE_NAME,          collectTypesCompletions},
    {COMPLETE_STRUCT_NAME,        collectStructsCompletions},
    {COMPLETE_STRUCT_MEMBER_NAME, collectStructMemberCompletions},
    {COMPLETE_ENUM_NAME,          collectEnumsCompletions},
    {COMPLETE_LABEL_NAME,         collectLabelsCompletions},
    {COMPLETE_OTHER_NAME,         collectOthersCompletions},
    {0,NULL}
};


/* This needs to reside inside parser because of macro transformation of yy-variables */
static bool exists_valid_parser_action_on(int token) {
    int yyn1, yyn2;
    bool shift_action = (yyn1 = yysindex[lastyystate]) && (yyn1 += token) >= 0 &&
        yyn1 <= YYTABLESIZE && yycheck[yyn1] == token;
    bool reduce_action = (yyn2 = yyrindex[lastyystate]) && (yyn2 += token) >= 0 &&
        yyn2 <= YYTABLESIZE && yycheck[yyn2] == token;
    bool valid = shift_action || reduce_action;

    return valid;
}


static bool runCompletionsCollectorsIn(CompletionFunctionsTable *completionsTable) {
    int token;
    for (int i=0; (token=completionsTable[i].token) != 0; i++) {
        log_trace("trying token %d", tokenNamesTable[token]);
        if (exists_valid_parser_action_on(token)) {
            log_trace("completing %d==%s in state %d", i, tokenNamesTable[token], lastyystate);
            (*completionsTable[i].fun)(&collectedCompletions);
            if (collectedCompletions.abortFurtherCompletions)
                return false;
        }
    }
    return true;
}


/* These are similar in the two parsers, except that we have made macro renaming of the
   YACC variables (so they are actually called something different) so that we can have
   multiple parsers linked together. Therefore it is not straight forward to refactor
   out commonalities. */
void makeCCompletions(char *string, int len, Position position) {
    CompletionLine completionLine;

    log_trace("completing \"%s\"", string);
    strncpy(collectedCompletions.idToProcess, string, MAX_FUNCTION_NAME_LENGTH);
    collectedCompletions.idToProcess[MAX_FUNCTION_NAME_LENGTH-1] = 0;
    initCompletions(&collectedCompletions, len, position);

    /* special wizard completions */
    if (!runCompletionsCollectorsIn(specialCompletionsCollectorsTable))
        return;

    /* If there is a wizard completion, RETURN now */
    if (collectedCompletions.alternativeCount != 0)
        return;

    /* basic language tokens */
    if (!runCompletionsCollectorsIn(completionsCollectorsTable))
        return;

    /* basic language tokens */
    for (int token=0; token<LAST_TOKEN; token++) {
        if (token == IDENTIFIER)
            continue;
        if (exists_valid_parser_action_on(token)) {
            if (tokenNamesTable[token] != NULL) {
                if (isalpha(*tokenNamesTable[token]) || *tokenNamesTable[token]=='_') {
                    completionLine = makeCompletionLine(tokenNamesTable[token], NULL, TypeKeyword, 0, NULL);
                } else {
                    completionLine = makeCompletionLine(tokenNamesTable[token], NULL, TypeToken, 0, NULL);
                }
                log_trace("completing %d==%s(%s) in state %d", token, tokenNamesTable[token], tokenNamesTable[token], lastyystate);
                processName(tokenNamesTable[token], &completionLine, false, &collectedCompletions);
            }
        }
    }
}
