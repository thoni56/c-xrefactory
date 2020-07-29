/*
  Our yacc does not support %expect, but this should be it for the C grammar

  %expect 7
  %expect-rr 34

../../../byacc-1.9/yacc: 7 shift/reduce conflicts, 28 reduce/reduce conflicts.

*/

%{

#define c_yylex yylex

#include "c_parser.x"

#include "globals.h"
#include "misc.h"
#include "html.h"
#include "yylex.h"
#include "commons.h"
#include "caching.h"
#include "complete.h"
#include "extract.h"
#include "semact.h"
#include "cxref.h"
#include "symbol.h"
#include "list.h"
#include "filedescriptor.h"
#include "id.h"

#include "log.h"

#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define c_yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

#define SetDirectStructureCompletionType(xxx) {\
    assert(options.taskRegime);\
    if (options.taskRegime == RegimeEditServer) {\
        s_structRecordCompletionType = xxx;\
        assert(s_structRecordCompletionType);\
    }\
}
#define SetIndirectStructureCompletionType(xxx) {\
    assert(options.taskRegime);\
    if (options.taskRegime == RegimeEditServer) {\
        if (xxx->kind==TypePointer || xxx->kind==TypeArray) {\
            s_structRecordCompletionType = xxx->next;\
            assert(s_structRecordCompletionType);\
        } else s_structRecordCompletionType = &s_errorModifier;\
    }\
}

#define AddComposedType(ddd, ttt) appendComposedType(&ddd->u.type, ttt)

%}

/* Token definitions *must* be the same in all parsers. The following
   is a marker, it must be the same as in the Makefile check */
/* START OF COMMON TOKEN DEFINITIONS */

/* ************************* SPECIALS ****************************** */
/* c+c++ */

%token TYPE_NAME

/* c++-only */
%token CLASS_NAME TEMPLATE_NAME
%token CONVERSION_OP_ID_PREFIX OPERATOR_IDENT

/* ************************* OPERATORS ****************************** */
/* common */
%token INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

/* c-only */
%token PTR_OP ELIPSIS

/* java -only */
%token URIGHT_OP URIGHT_ASSIGN

/* yacc-only */
%token YACC_PERC YACC_DPERC

/* c++-only */
%token DPOINT POINTM_OP PTRM_OP

/* ************************** KEYWORDS ******************************** */

%token STATIC BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE
%token FLOAT FOR GOTO IF INT LONG RETURN SHORT SWITCH VOID VOLATILE WHILE

/* c-special */
%token TYPEDEF EXTERN AUTO REGISTER SIGNED UNSIGNED STRUCT UNION ENUM
%token SIZEOF RESTRICT _ATOMIC _BOOL _THREADLOCAL _NORETURN
/* hmm */
%token ANONYME_MOD

/* java-special */
%token ABSTRACT BOOLEAN BYTE CATCH CLASS EXTENDS FINAL FINALLY
%token IMPLEMENTS IMPORT INSTANCEOF INTERFACE NATIVE NEW
%token PACKAGE PRIVATE PROTECTED PUBLIC SUPER
%token SYNCHRONIZED THIS THROW THROWS TRANSIENT TRY
%token TRUE_LITERAL FALSE_LITERAL NULL_LITERAL
%token STRICTFP ASSERT

/* c++-special */
%token FRIEND OPERATOR NAMESPACE TEMPLATE DELETE MUTABLE EXPLICIT
%token WCHAR_T BOOL USING ASM_KEYWORD EXPORT VIRTUAL INLINE TYPENAME
%token DYNAMIC_CAST STATIC_CAST REINTERPRET_CAST CONST_CAST TYPEID

/* yacc-special */
%token TOKEN TYPE

/* gcc specials */
%token LABEL

/* ******************** COMPLETION SPECIAL TOKENS ******************** */

%token COMPL_FOR_SPECIAL1 COMPL_FOR_SPECIAL2
%token COMPL_THIS_PACKAGE_SPECIAL

/* c-only */
%token COMPL_TYPE_NAME
%token COMPL_STRUCT_NAME COMPL_STRUCT_REC_NAME COMPL_UP_FUN_PROFILE
%token COMPL_ENUM_NAME COMPL_LABEL_NAME COMPL_OTHER_NAME

/* java-only */
%token COMPL_CLASS_DEF_NAME COMPL_FULL_INHERITED_HEADER
%token COMPL_TYPE_NAME0
%token COMPL_TYPE_NAME1
%token COMPL_PACKAGE_NAME0 COMPL_EXPRESSION_NAME0 COMPL_METHOD_NAME0
%token COMPL_PACKAGE_NAME1 COMPL_EXPRESSION_NAME1 COMPL_METHOD_NAME1
%token COMPL_CONSTRUCTOR_NAME0 COMPL_CONSTRUCTOR_NAME1
%token COMPL_CONSTRUCTOR_NAME2 COMPL_CONSTRUCTOR_NAME3
%token COMPL_STRUCT_REC_PRIM COMPL_STRUCT_REC_SUPER COMPL_QUALIF_SUPER
%token COMPL_SUPER_CONSTRUCTOR1 COMPL_SUPER_CONSTRUCTOR2
%token COMPL_THIS_CONSTRUCTOR COMPL_IMPORT_SPECIAL
%token COMPL_VARIABLE_NAME_HINT COMPL_CONSTRUCTOR_HINT
%token COMPL_METHOD_PARAM1 COMPL_METHOD_PARAM2 COMPL_METHOD_PARAM3

/* yacc-special */
%token COMPL_YACC_LEXEM_NAME

/* ************************** CPP-TOKENS ****************************** */
/* c-only */
%token CPP_TOKENS_START
%token CPP_INCLUDE CPP_DEFINE CPP_IFDEF CPP_IFNDEF CPP_IF CPP_ELSE CPP_ENDIF
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
/* special tokens used for nontrivial caching !!!! not used */
%token CACHING1_TOKEN

/* ******************************************************************** */
%token OL_MARKER_TOKEN OL_MARKER_TOKEN1 OL_MARKER_TOKEN2

/* ******************************************************************** */
%token TMP_TOKEN1 TMP_TOKEN2
%token CCC_OPER_PARENTHESIS CCC_OPER_BRACKETS

/* ************************ MULTI TOKENS START ************************ */
%token MULTI_TOKENS_START

/* commons */
%token IDENTIFIER CONSTANT LONG_CONSTANT
%token FLOAT_CONSTANT DOUBLE_CONSTANT
%token STRING_LITERAL
%token LINE_TOK
%token IDENT_TO_COMPLETE        /* identifier under cursor */

/* c-only */
%token CPP_MAC_ARG IDENT_NO_CPP_EXPAND

/* java-only */
%token CHAR_LITERAL

/* c++-only */

/* ****************************************************************** */

%token LAST_TOKEN

/* END OF COMMON TOKEN DEFINITIONS */
/* Token definitions *must* be the same in all parsers. The above
   is a marker, it must be the same as in the Makefile check */

%union {
#include "yystype.h"
}


%type <ast_id> IDENTIFIER identifier struct_identifier enum_identifier
%type <ast_id> str_rec_identifier STRUCT UNION struct_or_union
%type <ast_id> user_defined_type TYPE_NAME
%type <ast_id> designator, designator_list
%type <ast_idList> designation_opt, initializer, initializer_list, eq_initializer_opt
%type <ast_integer> assignment_operator
%type <ast_integer> pointer CONSTANT _ncounter_ _nlabel_ _ngoto_ _nfork_
%type <ast_unsigned> storage_class_specifier type_specifier1
%type <ast_unsigned> type_modality_specifier Sv_tmp
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
        Symbol *p;
        Symbol *dd;
        p = $1.d->symbol;
        if (p != NULL && p->bits.symType == TypeDefault) {
            assert(p && p);
            dd = p;
            assert(dd->bits.storage != StorageTypedef);
            $$.d.typeModifier = dd->u.type;
            assert(options.taskRegime);
            $$.d.reference = addCxReference(p, &$1.d->p, UsageUsed, noFileIndex, noFileIndex);
        } else {
            /* implicit function declaration */
            TypeModifier *p;
            Symbol *d;
            Symbol *dd;

            p = newTypeModifier(TypeInt, NULL, NULL);
            $$.d.typeModifier = newFunctionTypeModifier(NULL, NULL, NULL, p);

            d = newSymbolAsType($1.d->name, $1.d->name, $1.d->p, $$.d.typeModifier);
            fillSymbolBits(&d->bits, AccessDefault, TypeDefault, StorageExtern);

            dd = addNewSymbolDef(d, StorageExtern, s_symbolTable, UsageUsed);
            $$.d.reference = addCxReference(dd, &$1.d->p, UsageUsed, noFileIndex, noFileIndex);
        }
    }
    | CHAR_LITERAL          { $$.d.typeModifier = newSimpleTypeModifier(TypeInt); $$.d.reference = NULL;}
    | CONSTANT              { $$.d.typeModifier = newSimpleTypeModifier(TypeInt); $$.d.reference = NULL;}
    | LONG_CONSTANT         { $$.d.typeModifier = newSimpleTypeModifier(TypeLong); $$.d.reference = NULL;}
    | FLOAT_CONSTANT        { $$.d.typeModifier = newSimpleTypeModifier(TypeFloat); $$.d.reference = NULL;}
    | DOUBLE_CONSTANT       { $$.d.typeModifier = newSimpleTypeModifier(TypeDouble); $$.d.reference = NULL;}
    | string_literals       {
        TypeModifier *p;
        p = newSimpleTypeModifier(TypeChar);
        $$.d.typeModifier = newPointerTypeModifier(p);
        $$.d.reference = NULL;
    }
    | '(' expr ')'          {
        $$.d = $2.d;
    }
    | '(' compound_statement ')'            {       /* GNU's shit */
        $$.d.typeModifier = &s_errorModifier;
        $$.d.reference = NULL;
    }
    | COMPL_OTHER_NAME      { assert(0); /* token never used */ }
    ;

string_literals
    : STRING_LITERAL
    | STRING_LITERAL string_literals
    ;

postfix_expr
    : primary_expr                              /*& { $$.d = $1.d; } &*/
    | postfix_expr '[' expr ']'                 {
        if ($1.d.typeModifier->kind==TypePointer || $1.d.typeModifier->kind==TypeArray) $$.d.typeModifier=$1.d.typeModifier->next;
        else if ($3.d.typeModifier->kind==TypePointer || $3.d.typeModifier->kind==TypeArray) $$.d.typeModifier=$3.d.typeModifier->next;
        else $$.d.typeModifier = &s_errorModifier;
        $$.d.reference = NULL;
        assert($$.d.typeModifier);
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
                $<typeModifier>$ = s_upLevelFunctionCompletionType;
                s_upLevelFunctionCompletionType = $1.d.typeModifier;
            }
      '(' argument_expr_list_opt ')'    {
        s_upLevelFunctionCompletionType = $<typeModifier>2;
        if ($1.d.typeModifier->kind==TypeFunction) {
            $$.d.typeModifier=$1.d.typeModifier->next;
            if ($4.d==NULL) {
                handleInvocationParamPositions($1.d.reference, &$3.d, NULL, &$5.d, 0);
            } else {
                handleInvocationParamPositions($1.d.reference, &$3.d, $4.d->next, &$5.d, 1);
            }
        } else {
            $$.d.typeModifier = &s_errorModifier;
        }
        $$.d.reference = NULL;
        assert($$.d.typeModifier);
    }
    | postfix_expr {SetDirectStructureCompletionType($1.d.typeModifier);} '.' str_rec_identifier        {
        Symbol *rec=NULL;
        $$.d.reference = findStructureFieldFromType($1.d.typeModifier, $4.d, &rec, CLASS_TO_ANY);
        assert(rec);
        $$.d.typeModifier = rec->u.type;
        assert($$.d.typeModifier);
    }
    | postfix_expr {SetIndirectStructureCompletionType($1.d.typeModifier);} PTR_OP str_rec_identifier   {
        Symbol *rec=NULL;
        $$.d.reference = NULL;
        if ($1.d.typeModifier->kind==TypePointer || $1.d.typeModifier->kind==TypeArray) {
            $$.d.reference = findStructureFieldFromType($1.d.typeModifier->next, $4.d, &rec, CLASS_TO_ANY);
            assert(rec);
            $$.d.typeModifier = rec->u.type;
        } else $$.d.typeModifier = &s_errorModifier;
        assert($$.d.typeModifier);
    }
    | postfix_expr INC_OP                       {
        $$.d.typeModifier = $1.d.typeModifier;
        RESET_REFERENCE_USAGE($1.d.reference, UsageAddrUsed);
    }
    | postfix_expr DEC_OP                       {
        $$.d.typeModifier = $1.d.typeModifier;
        RESET_REFERENCE_USAGE($1.d.reference, UsageAddrUsed);
    }
    | compound_literal
    ;

compound_literal                /* Added in C99 */
    : '(' type_name ')' '{' initializer_list optional_comma '}'     {
        for (IdList *idList = $5.d; idList != NULL; idList = idList->next) {
            Symbol *rec=NULL;
            (void) findStructureFieldFromType($2.d, &idList->id, &rec, CLASS_TO_ANY);
        }
        $$.d.typeModifier = $2.d;
        $$.d.reference = NULL;
    }
    ;

optional_comma
    :
    | ','
    ;

str_rec_identifier
    : identifier                /*& { $$.d = $1.d; } &*/
    | COMPL_STRUCT_REC_NAME     { assert(0); /* token never used */ }
    ;

argument_expr_list_opt
    :                                           {
        $$.d = NULL;
    }
    |   argument_expr_list          {
            $$.d = StackMemoryAlloc(PositionList);
            fillPositionList($$.d, s_noPos, $1.d);
        }
    ;

argument_expr_list
    : assignment_expr                           {
        $$.d = NULL;
    }
    | argument_expr_list ',' assignment_expr    {
        $$.d = $1.d;
        appendPositionToList(&$$.d, &$2.d);
    }
    | COMPL_UP_FUN_PROFILE                          {/* never used */}
    | argument_expr_list ',' COMPL_UP_FUN_PROFILE   {/* never used */}
    ;

unary_expr
    : postfix_expr                  /*& { $$.d = $1.d; } &*/
    | INC_OP unary_expr             {
        $$.d.typeModifier = $2.d.typeModifier;
        RESET_REFERENCE_USAGE($2.d.reference, UsageAddrUsed);
    }
    | DEC_OP unary_expr             {
        $$.d.typeModifier = $2.d.typeModifier;
        RESET_REFERENCE_USAGE($2.d.reference, UsageAddrUsed);
    }
    | unary_operator cast_expr      {
        $$.d.typeModifier = $2.d.typeModifier;
        $$.d.reference = NULL;
      }
    | '&' cast_expr                 {
        $$.d.typeModifier = newPointerTypeModifier($2.d.typeModifier);
        RESET_REFERENCE_USAGE($2.d.reference, UsageAddrUsed);
        $$.d.reference = NULL;
    }
    | '*' cast_expr                 {
        if ($2.d.typeModifier->kind==TypePointer || $2.d.typeModifier->kind==TypeArray) $$.d.typeModifier=$2.d.typeModifier->next;
        else $$.d.typeModifier = &s_errorModifier;
        assert($$.d.typeModifier);
        $$.d.reference = NULL;
    }
    | SIZEOF unary_expr             {
        $$.d.typeModifier = newSimpleTypeModifier(TypeInt);
        $$.d.reference = NULL;
    }
    | SIZEOF '(' type_name ')'      {
        $$.d.typeModifier = newSimpleTypeModifier(TypeInt);
        $$.d.reference = NULL;
    }
    /* yet another GCC ext. */
    | AND_OP identifier     {
        labelReference($2.d, UsageLvalUsed);
    }
    ;

unary_operator
    : '+'
    | '-'
    | '~'
    | '!'
    ;

cast_expr
    : unary_expr                        /*& { $$.d = $1.d; } &*/
    | '(' type_name ')' cast_expr       {
        $$.d.typeModifier = $2.d;
        $$.d.reference = $4.d.reference;
    }
    ;

multiplicative_expr
    : cast_expr                         /*& { $$.d = $1.d; } &*/
    | multiplicative_expr '*' cast_expr {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    | multiplicative_expr '/' cast_expr {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    | multiplicative_expr '%' cast_expr {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

additive_expr
    : multiplicative_expr                       /*& { $$.d = $1.d; } &*/
    | additive_expr '+' multiplicative_expr     {
        if ($3.d.typeModifier->kind==TypePointer || $3.d.typeModifier->kind==TypeArray) $$.d.typeModifier = $3.d.typeModifier;
        else $$.d.typeModifier = $1.d.typeModifier;
        $$.d.reference = NULL;
    }
    | additive_expr '-' multiplicative_expr     {
        if ($3.d.typeModifier->kind==TypePointer || $3.d.typeModifier->kind==TypeArray) $$.d.typeModifier = $3.d.typeModifier;
        else $$.d.typeModifier = $1.d.typeModifier;
        $$.d.reference = NULL;
    }
    ;

shift_expr
    : additive_expr                             /*& { $$.d = $1.d; } &*/
    | shift_expr LEFT_OP additive_expr          {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    | shift_expr RIGHT_OP additive_expr         {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

relational_expr
    : shift_expr                                /*& { $$.d = $1.d; } &*/
    | relational_expr '<' shift_expr            {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    | relational_expr '>' shift_expr            {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    | relational_expr LE_OP shift_expr          {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    | relational_expr GE_OP shift_expr          {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

equality_expr
    : relational_expr                           /*& { $$.d = $1.d; } &*/
    | equality_expr EQ_OP relational_expr       {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    | equality_expr NE_OP relational_expr       {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

and_expr
    : equality_expr                             /*& { $$.d = $1.d; } &*/
    | and_expr '&' equality_expr                {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

exclusive_or_expr
    : and_expr                                  /*& { $$.d = $1.d; } &*/
    | exclusive_or_expr '^' and_expr            {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

inclusive_or_expr
    : exclusive_or_expr                         /*& { $$.d = $1.d; } &*/
    | inclusive_or_expr '|' exclusive_or_expr   {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

logical_and_expr
    : inclusive_or_expr                         /*& { $$.d = $1.d; } &*/
    | logical_and_expr AND_OP inclusive_or_expr {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

logical_or_expr
    : logical_and_expr                          /*& { $$.d = $1.d; } &*/
    | logical_or_expr OR_OP logical_and_expr    {
        $$.d.typeModifier = &s_defaultIntModifier;
        $$.d.reference = NULL;
    }
    ;

conditional_expr
    : logical_or_expr                                           /*& { $$.d = $1.d; } &*/
    | logical_or_expr '?' logical_or_expr ':' conditional_expr  {
        $$.d.typeModifier = $3.d.typeModifier;
        $$.d.reference = NULL;
    }
    /* another GCC "improvement", grrr */
    | logical_or_expr '?' ':' conditional_expr  {
        $$.d.typeModifier = $4.d.typeModifier;
        $$.d.reference = NULL;
    }
    ;

assignment_expr
    : conditional_expr                                  /*& { $$.d = $1.d; } &*/
    | unary_expr assignment_operator assignment_expr    {
        if ($1.d.reference != NULL && options.server_operation == OLO_EXTRACT) {
            Reference *rr;
            rr = duplicateReference($1.d.reference);
            $1.d.reference->usage = s_noUsage;
            if ($2.d == '=') {
                RESET_REFERENCE_USAGE(rr, UsageLvalUsed);
            } else {
                RESET_REFERENCE_USAGE(rr, UsageAddrUsed);
            }
        } else {
            if ($2.d == '=') {
                RESET_REFERENCE_USAGE($1.d.reference, UsageLvalUsed);
            } else {
                RESET_REFERENCE_USAGE($1.d.reference, UsageAddrUsed);
            }
        }
        $$.d = $1.d;    /* $$.d.r will be used for FOR completions ! */
    }
    ;

assignment_operator
    : '='                   {$$.d = '=';}
    | MUL_ASSIGN            {$$.d = MUL_ASSIGN;}
    | DIV_ASSIGN            {$$.d = DIV_ASSIGN;}
    | MOD_ASSIGN            {$$.d = MOD_ASSIGN;}
    | ADD_ASSIGN            {$$.d = ADD_ASSIGN;}
    | SUB_ASSIGN            {$$.d = SUB_ASSIGN;}
    | LEFT_ASSIGN           {$$.d = LEFT_ASSIGN;}
    | RIGHT_ASSIGN          {$$.d = RIGHT_ASSIGN;}
    | AND_ASSIGN            {$$.d = AND_ASSIGN;}
    | XOR_ASSIGN            {$$.d = XOR_ASSIGN;}
    | OR_ASSIGN             {$$.d = OR_ASSIGN;}
    ;

expr
    : assignment_expr                           /*& { $$.d = $1.d; } &*/
    | expr ',' assignment_expr                  {
        $$.d.typeModifier = $3.d.typeModifier;
        $$.d.reference = NULL;
    }
    ;

constant_expr
    : conditional_expr
    ;

Sv_tmp
    :    {
        $$.d = tmpWorkMemoryi;
    }
    ;

declaration
    : Sv_tmp declaration_specifiers ';'     { tmpWorkMemoryi = $1.d; }
    | Sv_tmp init_declarations ';'          { tmpWorkMemoryi = $1.d; }
    | error
        {
#if YYDEBUG
            char buffer[100];
            sprintf(buffer, "error parsing declaration, near '%s'", yytext);
            yyerror(buffer);
#endif
        }
    ;

init_declarations
    : declaration_specifiers init_declarator eq_initializer_opt {
        $$.d = $1.d;
        addNewDeclaration($1.d, $2.d, $3.d, StorageAuto, s_symbolTable);
    }
    | init_declarations ',' init_declarator eq_initializer_opt  {
        $$.d = $1.d;
        addNewDeclaration($1.d, $3.d, $4.d, StorageAuto, s_symbolTable);
    }
    | error                                             {
        /* $$.d = &s_errorSymbol; */
        $$.d = typeSpecifier2(&s_errorModifier);
#if YYDEBUG
        char buffer[100];
        sprintf(buffer, "error parsing init_declarations, near '%s'", yytext);
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
        $$.d = $1.d;
        assert(options.taskRegime);
        assert($1.d);
        assert($1.d->symbol);
        if (nestingLevel() == 0)
            usage = USAGE_TOP_LEVEL_USED;
        else
            usage = UsageUsed;
        addCxReference($1.d->symbol,&$1.d->p,usage,noFileIndex,noFileIndex);
    }
    ;

declaration_specifiers0
    : user_defined_type                                     {
        assert($1.d);
        assert($1.d->symbol);
        $$.d = typeSpecifier2($1.d->symbol->u.type);
    }
    | type_specifier1                                       {
        $$.d  = typeSpecifier1($1.d);
    }
    | type_specifier2                                       {
        $$.d  = typeSpecifier2($1.d);
    }
    | declaration_modality_specifiers  user_defined_type    {
        assert($2.d);
        assert($2.d->symbol);
        $$.d = $1.d;
        declTypeSpecifier2($1.d,$2.d->symbol->u.type);
    }
    | declaration_modality_specifiers type_specifier1       {
        $$.d = $1.d;
        declTypeSpecifier1($1.d,$2.d);
    }
    | declaration_modality_specifiers type_specifier2       {
        $$.d = $1.d;
        declTypeSpecifier2($1.d,$2.d);
    }
    | declaration_specifiers0 type_modality_specifier       {
        $$.d = $1.d;
        declTypeSpecifier1($1.d,$2.d);
    }
    | declaration_specifiers0 type_specifier1               {
        $$.d = $1.d;
        declTypeSpecifier1($1.d,$2.d);
    }
    | declaration_specifiers0 type_specifier2               {
        $$.d = $1.d;
        declTypeSpecifier2($1.d,$2.d);
    }
    | declaration_specifiers0 storage_class_specifier       {
        $$.d = $1.d;
        $$.d->bits.storage = $2.d;
    }
    | declaration_specifiers0 function_specifier            {
        $$.d = $1.d;
    }
    | COMPL_TYPE_NAME                                       {
        assert(0);
    }
    | declaration_modality_specifiers COMPL_TYPE_NAME       {
        assert(0); /* token never used */
    }
    ;

declaration_modality_specifiers
    : storage_class_specifier                               {
        $$.d  = typeSpecifier1(TypeDefault);
        $$.d->bits.storage = $1.d;
    }
    | declaration_modality_specifiers storage_class_specifier       {
        $$.d = $1.d;
        $$.d->bits.storage = $2.d;
    }
    | type_modality_specifier                               {
        $$.d  = typeSpecifier1($1.d);
    }
    | declaration_modality_specifiers type_modality_specifier       {
        declTypeSpecifier1($1.d, $2.d);
    }
    | function_specifier                                    {
        $$.d = typeSpecifier1(TypeDefault);
    }
    | declaration_modality_specifiers function_specifier            {
        $$.d = $1.d;
    }
    ;


/* a gcc extension ? */
asm_opt
    :
    |   ASM_KEYWORD '(' string_literals ')'
    ;

eq_initializer_opt
    :                   {
        $$.d = NULL;
    }
    | '=' initializer   {
        $$.d = $2.d;
    }
    ;

init_declarator
    : declarator asm_opt /* eq_initializer_opt   { $$.d = $1.d; } */
    ;

storage_class_specifier
    : TYPEDEF       { $$.d = StorageTypedef; }
    | EXTERN        { $$.d = StorageExtern; }
    | STATIC        { $$.d = StorageStatic; }
    | _THREADLOCAL  { $$.d = StorageThreadLocal; }
    | AUTO          { $$.d = StorageAuto; }
    | REGISTER      { $$.d = StorageAuto; }
    ;

type_modality_specifier
    : CONST         { $$.d = TypeDefault; }
    | RESTRICT      { $$.d = TypeDefault; }
    | VOLATILE      { $$.d = TypeDefault; }
    | _ATOMIC       { $$.d = TypeDefault; }
    | ANONYME_MOD   { $$.d = TypeDefault; }
    ;

type_modality_specifier_opt
    :
    | type_modality_specifier
    ;

type_specifier1
    : CHAR      { $$.d = TypeChar; }
    | SHORT     { $$.d = TmodShort; }
    | INT       { $$.d = TypeInt; }
    | LONG      { $$.d = TmodLong; }
    | SIGNED    { $$.d = TmodSigned; }
    | UNSIGNED  { $$.d = TmodUnsigned; }
    | FLOAT     { $$.d = TypeFloat; }
    | DOUBLE    { $$.d = TypeDouble; }
    | VOID      { $$.d = TypeVoid; }
    | _BOOL     { $$.d = TypeBoolean; }
    ;

type_specifier2
    : struct_or_union_specifier     /*& { $$.d = $1.d; } &*/
    | enum_specifier                /*& { $$.d = $1.d; } &*/
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
        $$.d = simpleStrUnionSpecifier($1.d, $2.d, usage);
    }
    | struct_or_union_define_specifier '{' struct_declaration_list '}'{
        assert($1.d && $1.d->u.t);
        $$.d = $1.d;
        specializeStrUnionDef($$.d->u.t, $3.d);
    }
    | struct_or_union_define_specifier '{' '}'                      {
        $$.d = $1.d;
    }
    ;

struct_or_union_define_specifier
    : struct_or_union struct_identifier                             {
        $$.d = simpleStrUnionSpecifier($1.d, $2.d, UsageDefined);
    }
    | struct_or_union                                               {
        $$.d = createNewAnonymousStructOrUnion($1.d);
    }
    ;

struct_identifier
    : identifier            /*& { $$.d = $1.d; } &*/
    | COMPL_STRUCT_NAME     { assert(0); /* token never used */ }
    ;

struct_or_union
    : STRUCT        { $$.d = $1.d; }
    | UNION         { $$.d = $1.d; }
    ;

struct_declaration_list
    : struct_declaration                                /*& { $$.d = $1.d; } &*/
    | struct_declaration_list struct_declaration        {
        if ($1.d == &s_errorSymbol || $1.d->bits.symType==TypeError) {
            $$.d = $2.d;
        } else if ($2.d == &s_errorSymbol || $1.d->bits.symType==TypeError)  {
            $$.d = $1.d;
        } else {
            $$.d = $1.d;
            LIST_APPEND(Symbol, $$.d, $2.d);
        }
    }
    ;

struct_declaration
    : Sv_tmp type_specifier_list struct_declarator_list ';'     {
        Symbol *p;
        assert($2.d && $3.d);
        for(p=$3.d; p!=NULL; p=p->next) {
            completeDeclarator($2.d, p);
        }
        $$.d = $3.d;
        tmpWorkMemoryi = $1.d;
    }
    | error                                             {
        $$.d = newSymbolAsCopyOf(&s_errorSymbol);
#if YYDEBUG
        char buffer[100];
        sprintf(buffer, "DEBUG: error parsing struct_declaration near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    ;

struct_declarator_list
    : struct_declarator                                 {
        $$.d = $1.d;
        assert($$.d->next == NULL);
    }
    | struct_declarator_list ',' struct_declarator      {
        $$.d = $1.d;
        assert($3.d->next == NULL);
        LIST_APPEND(Symbol, $$.d, $3.d);
    }
    ;

struct_declarator
    : /* gcc extension allow empty field */ {
        $$.d = createEmptyField();
    }
    | ':' constant_expr             {
        $$.d = createEmptyField();
    }
    | declarator                    /*& { $$.d = $1.d; } &*/
    | declarator ':' constant_expr  /*& { $$.d = $1.d; } &*/
    ;

enum_specifier
    : ENUM enum_identifier                                  {
        int usage;
        if (nestingLevel() == 0)
            usage = USAGE_TOP_LEVEL_USED;
        else
            usage = UsageUsed;
        $$.d = simpleEnumSpecifier($2.d, usage);
    }
    | enum_define_specifier '{' enumerator_list_comma '}'       {
        assert($1.d && $1.d->kind == TypeEnum && $1.d->u.t);
        $$.d = $1.d;
        if ($$.d->u.t->u.enums==NULL) {
            $$.d->u.t->u.enums = $3.d;
            addToTrail(setToNull, & ($$.d->u.t->u.enums) );
        }
    }
    | ENUM '{' enumerator_list_comma '}'                        {
        $$.d = createNewAnonymousEnum($3.d);
    }
    ;

enum_define_specifier
    : ENUM enum_identifier                                  {
        $$.d = simpleEnumSpecifier($2.d, UsageDefined);
    }
    ;

enum_identifier
    : identifier            /*& { $$.d = $1.d; } &*/
    | COMPL_ENUM_NAME       { assert(0); /* token never used */ }
    ;

enumerator_list_comma
    : enumerator_list               /*& { $$.d = $1.d; } &*/
    | enumerator_list ','           /*& { $$.d = $1.d; } &*/
    ;

enumerator_list
    : enumerator                            {
        $$.d = createDefinitionList($1.d);
    }
    | enumerator_list ',' enumerator        {
        $$.d = $1.d;
        LIST_APPEND(SymbolList, $$.d, createDefinitionList($3.d));
    }
    ;

enumerator
    : identifier                            {
        $$.d = createSimpleDefinition(StorageConstant,TypeInt,$1.d);
        addNewSymbolDef($$.d,StorageConstant, s_symbolTable, UsageDefined);
    }
    | identifier '=' constant_expr          {
        $$.d = createSimpleDefinition(StorageConstant,TypeInt,$1.d);
        addNewSymbolDef($$.d,StorageConstant, s_symbolTable, UsageDefined);
    }
    | error                                 {
        $$.d = newSymbolAsCopyOf(&s_errorSymbol);
#if YYDEBUG
        char buffer[100];
        sprintf(buffer, "DEBUG: error parsing enumerator near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    | COMPL_OTHER_NAME      { assert(0); /* token never used */ }
    ;

declarator
    : declarator2                                       /*& { $$.d = $1.d; } &*/
    | pointer declarator2                               {
        $$.d = $2.d;
        assert($$.d->bits.npointers == 0);
        $$.d->bits.npointers = $1.d;
    }
    ;

declarator2
    : identifier                                        {
        $$.d = newSymbol($1.d->name, $1.d->name, $1.d->p);
    }
    | '(' declarator ')'                                {
        $$.d = $2.d;
        unpackPointers($$.d);
    }
    | declarator2 '[' ']'                               {
        assert($1.d);
        $$.d = $1.d;
        AddComposedType($$.d, TypeArray);
    }
    | declarator2 '[' constant_expr ']'                 {
        assert($1.d);
        $$.d = $1.d;
        AddComposedType($$.d, TypeArray);
    }
    | declarator2 '(' ')'                               {
        TypeModifier *p;
        assert($1.d);
        $$.d = $1.d;
        p = AddComposedType($$.d, TypeFunction);
        initFunctionTypeModifier(&p->u.f , NULL);
        handleDeclaratorParamPositions($1.d, &$2.d, NULL, &$3.d, 0);
    }
    | declarator2 '(' parameter_type_list ')'           {
        TypeModifier *p;
        assert($1.d);
        $$.d = $1.d;
        p = AddComposedType($$.d, TypeFunction);
        initFunctionTypeModifier(&p->u.f , $3.d.s);
        handleDeclaratorParamPositions($1.d, &$2.d, $3.d.p, &$4.d, 1);
    }
    | declarator2 '(' parameter_identifier_list ')'     {
        TypeModifier *p;
        assert($1.d);
        $$.d = $1.d;
        p = AddComposedType($$.d, TypeFunction);
        initFunctionTypeModifier(&p->u.f , $3.d.s);
        handleDeclaratorParamPositions($1.d, &$2.d, $3.d.p, &$4.d, 1);
    }
    | COMPL_OTHER_NAME      { assert(0); /* token never used */ }
    ;

pointer
    : '*'                                   {
        $$.d = 1;
    }
    | '*' type_mod_specifier_list               {
        $$.d = 1;
    }
    | '*' pointer                           {
        $$.d = $2.d+1;
    }
    | '*' type_mod_specifier_list pointer       {
        $$.d = $3.d+1;
    }
    ;

type_mod_specifier_list
    : type_modality_specifier                                   {
        $$.d  = typeSpecifier1($1.d);
    }
    | type_mod_specifier_list type_modality_specifier           {
        declTypeSpecifier1($1.d, $2.d);
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
    : type_mod_specifier_list                       /*& { $$.d = $1.d; } &*/
    | type_specifier_list0                          /*& { $$.d = $1.d; } &*/
    ;

type_specifier_list0
    : user_defined_type                                     {
        assert($1.d);
        assert($1.d->symbol);
        assert($1.d->symbol);
        $$.d = typeSpecifier2($1.d->symbol->u.type);
    }
    | type_specifier1                                       {
        $$.d  = typeSpecifier1($1.d);
    }
    | type_specifier2                                       {
        $$.d  = typeSpecifier2($1.d);
    }
    | type_mod_specifier_list user_defined_type             {
        assert($2.d);
        assert($2.d->symbol);
        assert($2.d->symbol);
        $$.d = $1.d;
        declTypeSpecifier2($1.d,$2.d->symbol->u.type);
    }
    | type_mod_specifier_list type_specifier1       {
        $$.d = $1.d;
        declTypeSpecifier1($1.d,$2.d);
    }
    | type_mod_specifier_list type_specifier2       {
        $$.d = $1.d;
        declTypeSpecifier2($1.d,$2.d);
    }
    | type_specifier_list0 type_modality_specifier      {
        $$.d = $1.d;
        declTypeSpecifier1($1.d,$2.d);
    }
    | type_specifier_list0 type_specifier1              {
        $$.d = $1.d;
        declTypeSpecifier1($1.d,$2.d);
    }
    | type_specifier_list0 type_specifier2              {
        $$.d = $1.d;
        declTypeSpecifier2($1.d,$2.d);
    }
    | COMPL_TYPE_NAME                                       {
        assert(0);
    }
    | type_mod_specifier_list COMPL_TYPE_NAME       {
        assert(0); /* token never used */
    }
    ;

parameter_identifier_list
    : identifier_list                           /*& { $$.d = $1.d; } &*/
    | identifier_list ',' ELIPSIS               {
        Symbol *symbol;
        Position pos;
        fillPosition(&pos, -1, 0, 0);

        symbol = newSymbol("", "", pos);
        fillSymbolBits(&symbol->bits, AccessDefault, TypeElipsis, StorageDefault);
        $$.d = $1.d;

        LIST_APPEND(Symbol, $$.d.s, symbol);
        appendPositionToList(&$$.d.p, &$2.d);
    }
    ;

identifier_list
    : IDENTIFIER                                {
        Symbol *p;
        p = newSymbol($1.d->name, $1.d->name, $1.d->p);
        $$.d.s = p;
        $$.d.p = NULL;
    }
    | identifier_list ',' identifier            {
        Symbol        *p;
        p = newSymbol($3.d->name, $3.d->name, $3.d->p);
        $$.d = $1.d;
        LIST_APPEND(Symbol, $$.d.s, p);
        appendPositionToList(&$$.d.p, &$2.d);
    }
    | COMPL_OTHER_NAME      { assert(0); /* token never used */ }
    ;

parameter_type_list
    : parameter_list                    /*& { $$.d = $1.d; } &*/
    | parameter_list ',' ELIPSIS                {
        Symbol *symbol;
        Position position;
        fillPosition(&position, -1, 0, 0);

        symbol = newSymbol("", "", position);
        fillSymbolBits(&symbol->bits, AccessDefault, TypeElipsis, StorageDefault);
        $$.d = $1.d;

        LIST_APPEND(Symbol, $$.d.s, symbol);
        appendPositionToList(&$$.d.p, &$2.d);
    }
    ;

parameter_list
    : parameter_declaration                         {
        $$.d.s = $1.d;
        $$.d.p = NULL;
    }
    | parameter_list ',' parameter_declaration      {
        $$.d = $1.d;
        LIST_APPEND(Symbol, $1.d.s, $3.d);
        appendPositionToList(&$$.d.p, &$2.d);
    }
    ;


parameter_declaration
    : declaration_specifiers declarator         {
        completeDeclarator($1.d, $2.d);
        $$.d = $2.d;
    }
    | type_name                                 {
        $$.d = newSymbolAsType(NULL, NULL, s_noPos, $1.d);
    }
    | error                                     {
        $$.d = newSymbolAsCopyOf(&s_errorSymbol);
#if YYDEBUG
        char buffer[100];
        sprintf(buffer, "DEBUG: error parsing parameter_declaration near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    ;

type_name
    : declaration_specifiers                        {
        $$.d = $1.d->u.type;
    }
    | declaration_specifiers abstract_declarator    {
        $$.d = $2.d;
        LIST_APPEND(TypeModifier, $$.d, $1.d->u.type);
    }
    ;

abstract_declarator
    : pointer                               {
        int i;
        $$.d = newPointerTypeModifier(NULL);
        for(i=1; i<$1.d; i++) appendComposedType(&($$.d), TypePointer);
    }
    | abstract_declarator2                  {
        $$.d = $1.d;
    }
    | pointer abstract_declarator2          {
        int i;
        $$.d = $2.d;
        for(i=0; i<$1.d; i++) appendComposedType(&($$.d), TypePointer);
    }
    ;

abstract_declarator2
    : '(' abstract_declarator ')'           {
        $$.d = $2.d;
    }
    | '[' ']'                               {
        $$.d  = newArrayTypeModifier();
    }
    | '[' constant_expr ']'                 {
        $$.d = newArrayTypeModifier();
    }
    | abstract_declarator2 '[' ']'          {
        $$.d = $1.d;
        appendComposedType(&($$.d), TypeArray);
    }
    | abstract_declarator2 '[' constant_expr ']'    {
        $$.d = $1.d;
        appendComposedType(&($$.d), TypeArray);
    }
    | '(' ')'                                       {
        $$.d = newFunctionTypeModifier(NULL, NULL, NULL, NULL);
    }
    | '(' parameter_type_list ')'                   {
        $$.d = newFunctionTypeModifier($2.d.s, NULL, NULL, NULL);
    }
    | abstract_declarator2 '(' ')'                  {
        TypeModifier *p;
        $$.d = $1.d;
        p = appendComposedType(&($$.d), TypeFunction);
        initFunctionTypeModifier(&p->u.f , NULL);
    }
    | abstract_declarator2 '(' parameter_type_list ')'          {
        TypeModifier *p;
        $$.d = $1.d;
        p = appendComposedType(&($$.d), TypeFunction);
        // I think there should be the following, but in abstract
        // declarator it does not matter
        /*& initFunctionTypeModifier(&p->u.f , $3.d.s); &*/
        initFunctionTypeModifier(&p->u.f , NULL);
    }
    ;

initializer
    : assignment_expr       {
        $$.d = NULL;
    }
      /* it is enclosed because on linux kernel it overflows memory */
    | '{' initializer_list '}'  {
        $$.d = $2.d;
    }
    | '{' initializer_list ',' '}'  {
        $$.d = $2.d;
    }
    | error             {
        $$.d = NULL;
#if YYDEBUG
        char buffer[100];
        sprintf(buffer, "error parsing initializer, near '%s'", yytext);
        yyerror(buffer);
#endif
    }
    ;

initializer_list
    : Sv_tmp designation_opt Start_block initializer Stop_block {
        $$.d = $2.d;
        tmpWorkMemoryi = $1.d;
    }
    | initializer_list ',' Sv_tmp designation_opt Start_block initializer Stop_block {
        LIST_APPEND(IdList, $1.d, $4.d);
        tmpWorkMemoryi = $3.d;
    }
    ;

designation_opt
    :                           {
        $$.d = NULL;
    }
    | designator_list '='       {
        $$.d = StackMemoryAlloc(IdList);
        fillIdList($$.d, *$1.d, $1.d->name, TypeDefault, NULL);
    }
    ;

designator_list
    : designator                    {
        $$.d = $1.d;
    }
    | designator_list designator    {
        LIST_APPEND(Id, $1.d, $2.d);
    }
    ;

designator
    : '[' constant_expr ']'     {
        $$.d = StackMemoryAlloc(Id);
        fillId($$.d, "", NULL, s_noPos);
    }
    | '.' str_rec_identifier    {
        $$.d = StackMemoryAlloc(Id);
        *($$.d) = *($2.d);
    }
    ;

statement
    : Sv_tmp labeled_statement      {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp compound_statement     {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp expression_statement       {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp selection_statement        {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp iteration_statement        {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp jump_statement     {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp asm_statement      {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp error  {
        tmpWorkMemoryi = $1.d;
    }
    ;

label
    :   label_def_name ':'
    |   CASE constant_expr ':' {
            generateSwitchCaseFork(false);
    }
    |   CASE constant_expr ELIPSIS constant_expr ':' {
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
        labelReference($1.d,UsageDefined);
    }
    | COMPL_LABEL_NAME      { assert(0); /* token never used */ }
    ;

label_name
    : identifier            {
        labelReference($1.d,UsageUsed);
    }
    | COMPL_LABEL_NAME      { assert(0); /* token never used */ }
    ;

compound_statement
    : '{' '}'
    | '{' Start_block label_decls_opt statement_list Stop_block '}'
/*&
    | '{' Start_block label_decls_opt declaration_list Stop_block '}'
    | '{' Start_block label_decls_opt declaration_list statement_list Stop_block '}'
&*/
    ;

label_decls_opt
    :
    |   label_decls
    ;

label_decls
    :   LABEL identifier {
        labelReference($2.d,UsageDeclared);
    }
    |   label_decls LABEL identifier {
        labelReference($3.d,UsageDeclared);
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
    :                   { $$.d.typeModifier = NULL; $$.d.reference = NULL; }
    | expr              { $$.d = $1.d; }
    ;

expression_statement
    : maybe_expr ';'
    ;


_ncounter_:  {EXTRACT_COUNTER_SEMACT($$.d);}
    ;

_nlabel_:   {EXTRACT_LABEL_SEMACT($$.d);}
    ;

_ngoto_:    {EXTRACT_GOTO_SEMACT($$.d);}
    ;

_nfork_:    {EXTRACT_FORK_SEMACT($$.d);}
    ;

selection_statement
    : IF '(' expr ')' _nfork_ statement                     {
        generateInternalLabelReference($5.d, UsageDefined);
    }
    | IF '(' expr ')' _nfork_ statement ELSE _ngoto_ {
        generateInternalLabelReference($5.d, UsageDefined);
    }   statement                               {
        generateInternalLabelReference($8.d, UsageDefined);
    }
    | SWITCH '(' expr ')' /*5*/ _ncounter_  {/*6*/
        $<symbol>$ = addContinueBreakLabelSymbol(1000*$5.d, SWITCH_LABEL_NAME);
    } {/*7*/
        $<symbol>$ = addContinueBreakLabelSymbol($5.d, BREAK_LABEL_NAME);
        generateInternalLabelReference($5.d, UsageFork);
    } statement                 {
        generateSwitchCaseFork(true);
        ExtrDeleteContBreakSym($<symbol>7);
        ExtrDeleteContBreakSym($<symbol>6);
        generateInternalLabelReference($5.d, UsageDefined);
    }
    ;

for1maybe_expr
    : maybe_expr            {s_forCompletionType=$1.d;}
    ;

iteration_statement
    : WHILE _nlabel_ '(' expr ')' /*6*/ _nfork_
    {/*7*/
        $<symbol>$ = addContinueBreakLabelSymbol($2.d, CONTINUE_LABEL_NAME);
    } {/*8*/
        $<symbol>$ = addContinueBreakLabelSymbol($6.d, BREAK_LABEL_NAME);
    } statement                 {
        ExtrDeleteContBreakSym($<symbol>8);
        ExtrDeleteContBreakSym($<symbol>7);
        generateInternalLabelReference($2.d, UsageUsed);
        generateInternalLabelReference($6.d, UsageDefined);
    }

    | DO _nlabel_ _ncounter_ _ncounter_ { /*5*/
        $<symbol>$ = addContinueBreakLabelSymbol($3.d, CONTINUE_LABEL_NAME);
    } {/*6*/
        $<symbol>$ = addContinueBreakLabelSymbol($4.d, BREAK_LABEL_NAME);
    } statement WHILE {
        ExtrDeleteContBreakSym($<symbol>6);
        ExtrDeleteContBreakSym($<symbol>5);
        generateInternalLabelReference($3.d, UsageDefined);
    } '(' expr ')' ';'          {
        generateInternalLabelReference($2.d, UsageFork);
        generateInternalLabelReference($4.d, UsageDefined);
    }

    | FOR '(' for1maybe_expr ';'
            /*5*/ _nlabel_  maybe_expr ';'  /*8*/_ngoto_
            /*9*/ _nlabel_  maybe_expr ')' /*12*/ _nfork_
        {
        /*13*/
        generateInternalLabelReference($5.d, UsageUsed);
        generateInternalLabelReference($8.d, UsageDefined);
        $<symbol>$ = addContinueBreakLabelSymbol($9.d, CONTINUE_LABEL_NAME);
        }
        {/*14*/
            $<symbol>$ = addContinueBreakLabelSymbol($12.d, BREAK_LABEL_NAME);
        }
            statement
        {
        ExtrDeleteContBreakSym($<symbol>14);
        ExtrDeleteContBreakSym($<symbol>13);
        generateInternalLabelReference($9.d, UsageUsed);
        generateInternalLabelReference($12.d, UsageDefined);
        }
    | FOR '(' for1maybe_expr ';' COMPL_FOR_SPECIAL1
    | FOR '(' for1maybe_expr ';' _nlabel_  maybe_expr ';' COMPL_FOR_SPECIAL2
    ;

jump_statement
    : GOTO label_name ';'
    | CONTINUE ';'          {
        genContinueBreakReference(CONTINUE_LABEL_NAME);
    }
    | BREAK ';'             {
        genContinueBreakReference(BREAK_LABEL_NAME);
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
        if (includeStackPointer == 0) {
            placeCachePoint(true);
        }
    }
    | cached_external_definition_list _bef_ external_definition {
        if (includeStackPointer == 0) {
            placeCachePoint(true);
        }
    }
    | error
    ;

external_definition
    : Sv_tmp declaration_specifiers ';'     {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp top_init_declarations ';'      {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp function_definition_head {
        Symbol *p;
        int i;
        assert($2.d);
        // I think that due to the following line sometimes
        // storage was not extern, see 'addNewSymbolDef'
        /*& if ($2.d->bits.storage == StorageDefault) $2.d->bits.storage = StorageExtern; &*/
        // TODO!!!, here you should check if there is previous declaration of
        // the function, if yes and is declared static, make it static!
        addNewSymbolDef($2.d, StorageExtern, s_symbolTable, UsageDefined);
        tmpWorkMemoryi = $1.d;
        stackMemoryBlockStart();
        counters.localVar = 0;
        assert($2.d->u.type && $2.d->u.type->kind == TypeFunction);
        s_cp.function = $2.d;
        generateInternalLabelReference(-1, UsageDefined);
        for(p=$2.d->u.type->u.f.args,i=1; p!=NULL; p=p->next,i++) {
            if (p->bits.symType == TypeElipsis) continue;
            if (p->u.type == NULL) p->u.type = &s_defaultIntModifier;
            addFunctionParameterToSymTable($2.d, p, i, s_symbolTable);
        }
    } compound_statement {
        stackMemoryBlockFree();
        s_cp.function = NULL;
        if (options.taskRegime == RegimeHtmlGenerate) {
            htmlAddFunctionSeparatorReference();
        }
    }
    | Sv_tmp EXTERN STRING_LITERAL  external_definition {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp EXTERN STRING_LITERAL  '{' cached_external_definition_list '}' {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp ASM_KEYWORD '(' expr ')' ';'       {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp error compound_statement       {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp error      {
        tmpWorkMemoryi = $1.d;
    }
    | Sv_tmp ';'        {  /* empty external definition */
        tmpWorkMemoryi = $1.d;
    }
    ;

top_init_declarations
    : declaration_specifiers init_declarator eq_initializer_opt         {
        $$.d = $1.d;
        addNewDeclaration($1.d, $2.d, $3.d, StorageExtern,s_symbolTable);
    }
    | init_declarator eq_initializer_opt                        {
        $$.d = & s_defaultIntDefinition;
        addNewDeclaration($$.d, $1.d, $2.d, StorageExtern,s_symbolTable);
    }
    | top_init_declarations ',' init_declarator eq_initializer_opt          {
        $$.d = $1.d;
        addNewDeclaration($1.d, $3.d, $4.d, StorageExtern,s_symbolTable);
    }
    | error                                     {
        /* $$.d = &s_errorSymbol; */
        $$.d = typeSpecifier2(&s_errorModifier);
    }
    ;

function_definition_head
    : function_head_declaration                         /*& { $$.d = $1.d; } &*/
    | function_definition_head fun_arg_declaration      {
        int r;
        assert($1.d->u.type && $1.d->u.type->kind == TypeFunction);
        r = mergeArguments($1.d->u.type->u.f.args, $2.d);
        if (r == RESULT_ERR) YYERROR;
        $$.d = $1.d;
    }
    ;

fun_arg_declaration
    : declaration_specifiers ';'                                {
        $$.d = NULL;
    }
    | declaration_specifiers fun_arg_init_declarations ';'      {
        Symbol *p;
        assert($1.d && $2.d);
        for(p=$2.d; p!=NULL; p=p->next) {
            completeDeclarator($1.d, p);
        }
        $$.d = $2.d;
    }
    ;

fun_arg_init_declarations
    : init_declarator eq_initializer_opt            {
        $$.d = $1.d;
    }
    | fun_arg_init_declarations ',' init_declarator eq_initializer_opt              {
        $$.d = $1.d;
        LIST_APPEND(Symbol, $$.d, $3.d);
    }
    | fun_arg_init_declarations ',' error                       {
        $$.d = $1.d;
    }
    ;

function_head_declaration
    : declarator                            {
        completeDeclarator(&s_defaultIntDefinition, $1.d);
        assert($1.d && $1.d->u.type);
        if ($1.d->u.type->kind != TypeFunction) YYERROR;
        $$.d = $1.d;
    }
    | declaration_specifiers declarator     {
        completeDeclarator($1.d, $2.d);
        assert($2.d && $2.d->u.type);
        if ($2.d->u.type->kind != TypeFunction) YYERROR;
        $$.d = $2.d;
    }
    ;


Start_block:    { stackMemoryBlockStart(); }
    ;

Stop_block:     { stackMemoryBlockFree(); }
    ;

identifier
    : IDENTIFIER    /*& { $$.d = $1.d; } &*/
    | TYPE_NAME     /*& { $$.d = $1.d; } &*/
    ;

%%


static S_completionFunTab spCompletionsTab[]  = {
    {COMPL_FOR_SPECIAL1,    completeForSpecial1},
    {COMPL_FOR_SPECIAL2,    completeForSpecial2},
    {COMPL_UP_FUN_PROFILE,  completeUpFunProfile},
    {0,NULL}
};

static S_completionFunTab completionsTab[]  = {
    {COMPL_TYPE_NAME,       completeTypes},
    {COMPL_STRUCT_NAME,     completeStructs},
    {COMPL_STRUCT_REC_NAME, completeRecNames},
    {COMPL_ENUM_NAME,       completeEnums},
    {COMPL_LABEL_NAME,      completeLabels},
    {COMPL_OTHER_NAME,      completeOthers},
    {0,NULL}
};


static bool exists_valid_parser_action_on(int token) {
    int yyn1, yyn2;
    bool shift_action = (yyn1 = yysindex[lastyystate]) && (yyn1 += token) >= 0 &&
        yyn1 <= YYTABLESIZE && yycheck[yyn1] == token;
    bool reduce_action = (yyn2 = yyrindex[lastyystate]) && (yyn2 += token) >= 0 &&
        yyn2 <= YYTABLESIZE && yycheck[yyn2] == token;
    bool valid = shift_action || reduce_action;

    return valid;
}

/* These are similar in the three parsers, except that we have macro
   replacement of YACC variables so that we can have multiple parsers
   linked together. Therefore it is not straight forward to refactor
   out commonalities. */
void makeCCompletions(char *s, int len, Position *pos) {
    int token, i;
    CompletionLine compLine;

    log_trace("completing \"%s\"", s);
    strncpy(s_completions.idToProcess, s, MAX_FUN_NAME_SIZE);
    s_completions.idToProcess[MAX_FUN_NAME_SIZE-1] = 0;
    initCompletions(&s_completions, len, *pos);

    /* special wizard completions */
    for (i=0; (token=spCompletionsTab[i].token) != 0; i++) {
        log_trace("trying token %d", s_tokenName[token]);
        if (exists_valid_parser_action_on(token)) {
            log_trace("completing %d==%s in state %d", i, s_tokenName[token], lastyystate);
            (*spCompletionsTab[i].fun)(&s_completions);
            if (s_completions.abortFurtherCompletions)
                return;
        }
    }

    /* If there is a wizard completion, RETURN now */
    if (s_completions.alternativeIndex != 0 && options.server_operation != OLO_SEARCH)
        return;

    /* basic language tokens */
    for (i=0; (token=completionsTab[i].token) != 0; i++) {
        log_trace("trying token %d", s_tokenName[token]);
        if (exists_valid_parser_action_on(token)) {
            log_trace("completing %d==%s in state %d", i, s_tokenName[token], lastyystate);
            (*completionsTab[i].fun)(&s_completions);
            if (s_completions.abortFurtherCompletions)
                return;
        }
    }

    /* basic language tokens */
    for (token=0; token<LAST_TOKEN; token++) {
        if (token == IDENTIFIER)
            continue;
        if (exists_valid_parser_action_on(token)) {
            if (s_tokenName[token] != NULL) {
                if (isalpha(*s_tokenName[token]) || *s_tokenName[token]=='_') {
                    fillCompletionLine(&compLine, s_tokenName[token], NULL, TypeKeyword, 0, 0, NULL, NULL);
                } else {
                    fillCompletionLine(&compLine, s_tokenName[token], NULL, TypeToken, 0, 0, NULL, NULL);
                }
                log_trace("completing %d==%s(%s) in state %d", token, s_tokenName[token], s_tokenName[token], lastyystate);
                processName(s_tokenName[token], &compLine, 0, &s_completions);
            }
        }
    }
}
