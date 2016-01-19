/* 
%expect 

../../../byacc-1.9/yacc: 7 shift/reduce conflicts, 28 reduce/reduce conflicts.

*/
%{

#define yylval cyylval

#define yyparse cyyparse
#define yylhs cyylhs
#define yylen cyylen
#define yydefred cyydefred
#define yydgoto cyydgoto
#define yysindex cyysindex
#define yyrindex cyyrindex
#define yytable cyytable
#define yycheck cyycheck
#define yyname cyyname
#define yyrule cyyrule
#define yydebug cyydebug
#define yynerrs cyynerrs
#define yyerrflag cyyerrflag
#define yychar cyychar
#define lastyystate clastyystate
#define yyssp cyyssp
#define yyval cyyval
#define yyss cyyss
#define yyvs cyyvs
#define yygindex cyygindex
#define yyvsp cyyvsp

#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/


#define YYDEBUG 0
#define yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

#define SetStrCompl1(xxx) {\
	assert(s_opt.taskRegime);\
	if (s_opt.taskRegime == RegimeEditServer) {\
		s_structRecordCompletionType = xxx;\
		assert(s_structRecordCompletionType);\
	}\
}
#define SetStrCompl2(xxx) {\
	assert(s_opt.taskRegime);\
	if (s_opt.taskRegime == RegimeEditServer) {\
		if (xxx->m==TypePointer || xxx->m==TypeArray) {\
			s_structRecordCompletionType = xxx->next;\
			assert(s_structRecordCompletionType);\
		} else s_structRecordCompletionType = &s_errorModifier;\
	}\
}

#define CrTypeModifier(xxx,ttt) {\
		xxx = crSimpleTypeMofifier(ttt);\
		xxx = StackMemAlloc(S_typeModifiers);\
		FILLF_typeModifiers(xxx, ttt,f,( NULL,NULL) ,NULL,NULL);\
}

#define AddComposedType(ddd, ttt) appendComposedType(&ddd->u.type, ttt)

%}

/*
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
         Token definition part must be the same in all grammars
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

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
%token SIZEOF
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
%token CPP_DEFINE0      /* macro with no argument */
%token CPP_TOKENS_END

%token CPP_COLLATION	/* ## in macro body */
%token CPP_DEFINED_OP	/* defined(xxx) in #if */

/* ******************************************************************** */
/* special token signalizing end of program */
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
%token IDENT_TO_COMPLETE		/* identifier under cursor */

/* c-only */
%token CPP_MAC_ARG IDENT_NO_CPP_EXPAND

/* java-only */
%token CHAR_LITERAL

/* c++-only */

/* ****************************************************************** */

%token LAST_TOKEN



/* *************************************************************** */

%union {
	int									integer;
	unsigned							unsign;
	S_symbol							*symbol;
	S_symbolList						*symbolList;
	S_typeModifiers						*typeModif;
	S_typeModifiersList					*typeModifList;
	S_freeTrail     					*trail;
	S_idIdent							*idIdent;
	S_idIdentList						*idlist;
	S_exprTokenType						exprType;
	S_intPair							intpair;
	S_whileExtractData					*whiledata;
	S_position							position;
	S_unsPositionPair					unsPositionPair;
	S_symbolPositionPair				symbolPositionPair;
	S_symbolPositionLstPair				symbolPositionLstPair;
	S_positionLst						*positionLst;
	S_typeModifiersListPositionLstPair 	typeModifiersListPositionLstPair;

	S_extRecFindStr							*erfs;

	S_bb_int								bbinteger;
	S_bb_unsigned							bbunsign;
	S_bb_symbol								bbsymbol;
	S_bb_symbolList							bbsymbolList;
	S_bb_typeModifiers						bbtypeModif;
	S_bb_typeModifiersList					bbtypeModifList;
	S_bb_freeTrail     						bbtrail;
	S_bb_idIdent							bbidIdent;
	S_bb_idIdentList						bbidlist;
	S_bb_exprTokenType						bbexprType;
	S_bb_intPair							bbintpair;
	S_bb_whileExtractData					bbwhiledata;
	S_bb_position							bbposition;
	S_bb_unsPositionPair					bbunsPositionPair;
	S_bb_symbolPositionPair					bbsymbolPositionPair;
	S_bb_symbolPositionLstPair				bbsymbolPositionLstPair;
	S_bb_positionLst						bbpositionLst;
	S_bb_typeModifiersListPositionLstPair 	bbtypeModifiersListPositionLstPair;
	S_bb_nestedConstrTokenType				bbnestedConstrTokenType;
}


%type <bbidIdent> IDENTIFIER identifier struct_identifier enum_identifier
%type <bbidIdent> str_rec_identifier STRUCT UNION struct_or_union
%type <bbidIdent> user_defined_type TYPE_NAME
%type <bbinteger> assignment_operator
%type <bbinteger> pointer CONSTANT _ncounter_ _nlabel_ _ngoto_ _nfork_
%type <bbunsign> storage_class_specifier type_specifier1
%type <bbunsign> type_modality_specifier Sv_tmp
%type <bbsymbol> init_declarator declarator declarator2 struct_declarator
%type <bbsymbol> type_specifier_list type_mod_specifier_list
%type <bbsymbol> type_specifier_list0
%type <bbsymbol> top_init_declarations
%type <bbsymbol> declaration_specifiers init_declarations 
%type <bbsymbol> declaration_modality_specifiers declaration_specifiers0
%type <bbsymbol> enumerator parameter_declaration
%type <bbsymbol> function_head_declaration function_definition_head
%type <bbsymbol> struct_declaration_list struct_declaration
%type <bbsymbol> struct_declarator_list
%type <bbsymbolList> enumerator_list enumerator_list_comma
%type <bbsymbol> fun_arg_init_declarations fun_arg_declaration
%type <bbsymbolPositionLstPair> parameter_list parameter_type_list 
%type <bbsymbolPositionLstPair> parameter_identifier_list identifier_list
%type <bbpositionLst> argument_expr_list argument_expr_list_opt

%type <bbtypeModif> type_specifier2 
%type <bbtypeModif> struct_or_union_specifier struct_or_union_define_specifier
%type <bbtypeModif> enum_specifier enum_define_specifier
%type <bbtypeModif> type_name abstract_declarator abstract_declarator2

%type <bbexprType> primary_expr postfix_expr unary_expr cast_expr
%type <bbexprType> multiplicative_expr additive_expr shift_expr
%type <bbexprType> relational_expr equality_expr and_expr exclusive_or_expr
%type <bbexprType> inclusive_or_expr logical_and_expr logical_or_expr
%type <bbexprType> conditional_expr assignment_expr expr maybe_expr

%type <bbposition> STRING_LITERAL '(' ',' ')'

%start file
%%

primary_expr
	: IDENTIFIER			{
		S_symbol *p;
		S_symbol *dd;
		p = $1.d->sd;
		if (p != NULL && p->b.symType == TypeDefault) {
			assert(p && p);
			dd = p;
			assert(dd->b.storage != StorageTypedef);
			$$.d.t = dd->u.type;
			assert(s_opt.taskRegime);
			if (CX_REGIME()) {
				$$.d.r = addCxReference(p, &$1.d->p, UsageUsed,s_noneFileIndex, s_noneFileIndex);
			} else {
				$$.d.r = NULL;
			}
		} else {
			/* implicit function declaration */
			S_typeModifiers *p;
			S_symbol *d;
			S_symbol *dd;
			CrTypeModifier(p, TypeInt);
			$$.d.t = StackMemAlloc(S_typeModifiers);
			FILLF_typeModifiers($$.d.t, TypeFunction,f,( NULL,NULL) ,NULL,p);
			d = StackMemAlloc(S_symbol);
			FILL_symbolBits(&d->b,0,0,0,0,0,TypeDefault, StorageExtern, 0);
			FILL_symbol(d,$1.d->name,$1.d->name,$1.d->p,d->b,type,$$.d.t,NULL);
			d->u.type = $$.d.t;
			dd = addNewSymbolDef(d, StorageExtern, s_symTab, UsageUsed);
			if (CX_REGIME()) {
				$$.d.r = addCxReference(dd, &$1.d->p, UsageUsed, s_noneFileIndex, s_noneFileIndex);
			} else {
				$$.d.r = NULL;
			}
		} 
	}
	| CHAR_LITERAL			{ CrTypeModifier($$.d.t, TypeInt); $$.d.r = NULL;}
	| CONSTANT				{ CrTypeModifier($$.d.t, TypeInt); $$.d.r = NULL;}
	| LONG_CONSTANT			{ CrTypeModifier($$.d.t, TypeLong); $$.d.r = NULL;}
	| FLOAT_CONSTANT		{ CrTypeModifier($$.d.t, TypeFloat); $$.d.r = NULL;}
	| DOUBLE_CONSTANT		{ CrTypeModifier($$.d.t, TypeDouble); $$.d.r = NULL;}
	| string_literales		{
		S_typeModifiers *p;
		CrTypeModifier(p, TypeChar);
		$$.d.t = StackMemAlloc(S_typeModifiers);
		FILLF_typeModifiers($$.d.t, TypePointer,f,( NULL,NULL) ,NULL,p);
		$$.d.r = NULL;
	}
	| '(' expr ')'			{
		$$.d = $2.d;
	}
	| '(' compound_statement ')'			{ 		/* GNU's shit */
		$$.d.t = &s_errorModifier;
		$$.d.r = NULL;
	}
	| COMPL_OTHER_NAME		{ assert(0); /* token never used */ }
	;

string_literales:
		STRING_LITERAL
	|	STRING_LITERAL string_literales
	;

postfix_expr
	: primary_expr								/* { $$.d = $1.d; } */
	| postfix_expr '[' expr ']'					{
		if ($1.d.t->m==TypePointer || $1.d.t->m==TypeArray) $$.d.t=$1.d.t->next;
		else if ($3.d.t->m==TypePointer || $3.d.t->m==TypeArray) $$.d.t=$3.d.t->next;
		else $$.d.t = &s_errorModifier;
		$$.d.r = NULL;
		assert($$.d.t);
	}
/*
	| postfix_expr '(' ')'						{
		if ($1.d.t->m==TypeFunction) {
			$$.d.t=$1.d.t->next;
			handleInvocationParamPositions($1.d.r, &$2.d, NULL, &$3.d, 0);
		} else {
			$$.d.t = &s_errorModifier;
		}
		$$.d.r = NULL;
		assert($$.d.t);		
	}
*/
	| postfix_expr 
			{
				$<typeModif>$ = s_upLevelFunctionCompletionType;
			 	s_upLevelFunctionCompletionType = $1.d.t;
			}
	  '(' argument_expr_list_opt ')'	{
		s_upLevelFunctionCompletionType = $<typeModif>2;
		if ($1.d.t->m==TypeFunction) {
			$$.d.t=$1.d.t->next;
			if ($4.d==NULL) {
				handleInvocationParamPositions($1.d.r, &$3.d, NULL, &$5.d, 0);
			} else {
				handleInvocationParamPositions($1.d.r, &$3.d, $4.d->next, &$5.d, 1);
			}
		} else {
			$$.d.t = &s_errorModifier;
		}
		$$.d.r = NULL;
		assert($$.d.t);
	}
	| postfix_expr {SetStrCompl1($1.d.t);} '.' str_rec_identifier		{
		S_symbol *rec=NULL;
		$$.d.r = findStrRecordFromType($1.d.t, $4.d, &rec, CLASS_TO_ANY);
		assert(rec);
		$$.d.t = rec->u.type;
		assert($$.d.t);
	}
	| postfix_expr {SetStrCompl2($1.d.t);} PTR_OP str_rec_identifier	{
		S_typeModifiers *p;
		S_symbol *rec=NULL;
		$$.d.r = NULL;
		if ($1.d.t->m==TypePointer || $1.d.t->m==TypeArray) {
			$$.d.r = findStrRecordFromType($1.d.t->next, $4.d, &rec, CLASS_TO_ANY);
			assert(rec);
			$$.d.t = rec->u.type;
		} else $$.d.t = &s_errorModifier;
		assert($$.d.t);
	}
	| postfix_expr INC_OP						{
		$$.d.t = $1.d.t;
		RESET_REFERENCE_USAGE($1.d.r, UsageAddrUsed);
	}
	| postfix_expr DEC_OP						{
		$$.d.t = $1.d.t;
		RESET_REFERENCE_USAGE($1.d.r, UsageAddrUsed);
	}
	;

str_rec_identifier
	: identifier				/* { $$.d = $1.d; } */
	| COMPL_STRUCT_REC_NAME		{ assert(0); /* token never used */ }
	;

argument_expr_list_opt:				{
			$$.d = NULL;
		}
	|	argument_expr_list			{
			XX_ALLOC($$.d, S_positionLst);
			FILL_positionLst($$.d, s_noPos, $1.d);
		}
	;

argument_expr_list
	: assignment_expr							{ 
		$$.d = NULL; 
	}
	| argument_expr_list ',' assignment_expr	{
		$$.d = $1.d;
		appendPositionToList(&$$.d, &$2.d);
	}
	| COMPL_UP_FUN_PROFILE							{/* never used */}
	| argument_expr_list ',' COMPL_UP_FUN_PROFILE	{/* never used */}
	;

unary_expr
	: postfix_expr					/* { $$.d = $1.d; } */
	| INC_OP unary_expr				{ 
		$$.d.t = $2.d.t;
		RESET_REFERENCE_USAGE($2.d.r, UsageAddrUsed);
	}
	| DEC_OP unary_expr				{
		$$.d.t = $2.d.t;
		RESET_REFERENCE_USAGE($2.d.r, UsageAddrUsed);
	}
	| unary_operator cast_expr		{ $$.d.t = $2.d.t; $$.d.r = NULL;}
	| '&' cast_expr					{
		$$.d.t = StackMemAlloc(S_typeModifiers);
		FILLF_typeModifiers($$.d.t, TypePointer,f,( NULL,NULL) ,NULL,$2.d.t);
		RESET_REFERENCE_USAGE($2.d.r, UsageAddrUsed);
		$$.d.r = NULL;
	}
	| '*' cast_expr					{
		if ($2.d.t->m==TypePointer || $2.d.t->m==TypeArray) $$.d.t=$2.d.t->next;
		else $$.d.t = &s_errorModifier;
		assert($$.d.t);
		$$.d.r = NULL;
	}
	| SIZEOF unary_expr				{ 
		CrTypeModifier($$.d.t, TypeInt);
		$$.d.r = NULL;
	}
	| SIZEOF '(' type_name ')'		{
		CrTypeModifier($$.d.t, TypeInt);
		$$.d.r = NULL;
	}
	/* yet another GCC ext. */
	| AND_OP identifier		{
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
	: unary_expr						/* { $$.d = $1.d; } */
	| '(' type_name ')' cast_expr		{ 
		$$.d.t = $2.d; 
		$$.d.r = $4.d.r;
	}
	| '(' type_name ')' '{' initializer_list '}'		{ /* GNU-extension*/
		$$.d.t = $2.d; 
		$$.d.r = NULL;
	}
	| '(' type_name ')' '{' initializer_list ',' '}'	{ /* GNU-extension*/
		$$.d.t = $2.d; 
		$$.d.r = NULL;
	}
	;

multiplicative_expr
	: cast_expr							/* { $$.d = $1.d; } */
	| multiplicative_expr '*' cast_expr	{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	| multiplicative_expr '/' cast_expr	{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	| multiplicative_expr '%' cast_expr	{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

additive_expr
	: multiplicative_expr						/* { $$.d = $1.d; } */
	| additive_expr '+' multiplicative_expr		{
		if ($3.d.t->m==TypePointer || $3.d.t->m==TypeArray) $$.d.t = $3.d.t;
		else $$.d.t = $1.d.t; 
		$$.d.r = NULL;
	}
	| additive_expr '-' multiplicative_expr		{
		if ($3.d.t->m==TypePointer || $3.d.t->m==TypeArray) $$.d.t = $3.d.t;
		else $$.d.t = $1.d.t; 
		$$.d.r = NULL;
	}
	;

shift_expr
	: additive_expr								/* { $$.d = $1.d; } */
	| shift_expr LEFT_OP additive_expr			{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	| shift_expr RIGHT_OP additive_expr			{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

relational_expr
	: shift_expr								/* { $$.d = $1.d; } */
	| relational_expr '<' shift_expr			{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	| relational_expr '>' shift_expr			{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	| relational_expr LE_OP shift_expr			{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	| relational_expr GE_OP shift_expr			{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

equality_expr
	: relational_expr							/* { $$.d = $1.d; } */
	| equality_expr EQ_OP relational_expr		{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	| equality_expr NE_OP relational_expr		{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

and_expr
	: equality_expr								/* { $$.d = $1.d; } */
	| and_expr '&' equality_expr				{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

exclusive_or_expr
	: and_expr									/* { $$.d = $1.d; } */
	| exclusive_or_expr '^' and_expr			{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

inclusive_or_expr
	: exclusive_or_expr							/* { $$.d = $1.d; } */
	| inclusive_or_expr '|' exclusive_or_expr	{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

logical_and_expr
	: inclusive_or_expr							/* { $$.d = $1.d; } */
	| logical_and_expr AND_OP inclusive_or_expr	{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

logical_or_expr
	: logical_and_expr							/* { $$.d = $1.d; } */
	| logical_or_expr OR_OP logical_and_expr	{ 
		$$.d.t = &s_defaultIntModifier;
		$$.d.r = NULL;
	}
	;

conditional_expr
	: logical_or_expr											/* { $$.d = $1.d; } */
	| logical_or_expr '?' logical_or_expr ':' conditional_expr	{ 
		$$.d.t = $3.d.t; 
		$$.d.r = NULL;
	}
/* other GCC improvement, grrr */
	| logical_or_expr '?' ':' conditional_expr	{ 
		$$.d.t = $4.d.t; 
		$$.d.r = NULL;
	}
	;

assignment_expr
	: conditional_expr									/* { $$.d = $1.d; } */
	| unary_expr assignment_operator assignment_expr	{
		if ($1.d.r != NULL && s_opt.cxrefs == OLO_EXTRACT) {
			S_reference *rr;
			rr = duplicateReference($1.d.r);
			$1.d.r->usg = s_noUsage;
			if ($2.d == '=') {
				RESET_REFERENCE_USAGE(rr, UsageLvalUsed);
			} else {
				RESET_REFERENCE_USAGE(rr, UsageAddrUsed);
			}
		} else {
			if ($2.d == '=') {
				RESET_REFERENCE_USAGE($1.d.r, UsageLvalUsed);
			} else {
				RESET_REFERENCE_USAGE($1.d.r, UsageAddrUsed);
			}
		}
		$$.d = $1.d; 	/* $$.d.r will be used for FOR completions ! */
	}
	;

assignment_operator
	: '='					{$$.d = '=';}
	| MUL_ASSIGN			{$$.d = MUL_ASSIGN;}
	| DIV_ASSIGN			{$$.d = DIV_ASSIGN;}
	| MOD_ASSIGN			{$$.d = MOD_ASSIGN;}
	| ADD_ASSIGN			{$$.d = ADD_ASSIGN;}
	| SUB_ASSIGN			{$$.d = SUB_ASSIGN;}
	| LEFT_ASSIGN			{$$.d = LEFT_ASSIGN;}
	| RIGHT_ASSIGN			{$$.d = RIGHT_ASSIGN;}
	| AND_ASSIGN			{$$.d = AND_ASSIGN;}
	| XOR_ASSIGN			{$$.d = XOR_ASSIGN;}
	| OR_ASSIGN				{$$.d = OR_ASSIGN;}
	;

expr
	: assignment_expr							/* { $$.d = $1.d; } */
	| expr ',' assignment_expr					{ 
		$$.d.t = $3.d.t; 
		$$.d.r = NULL;
	}
	;

constant_expr
	: conditional_expr
	;

Sv_tmp:
	{
		$$.d = tmpWorkMemoryi;
	}
	;

declaration
	: Sv_tmp declaration_specifiers ';'		{ tmpWorkMemoryi = $1.d; }
	| Sv_tmp init_declarations ';'			{ tmpWorkMemoryi = $1.d; }
	| error
	;

init_declarations
	: declaration_specifiers init_declarator			{		
		$$.d = $1.d;
		addNewDeclaration($1.d, $2.d, StorageAuto,s_symTab);
	} eq_initializer_opt
	| init_declarations ',' init_declarator 			{
		$$.d = $1.d;
		addNewDeclaration($1.d, $3.d, StorageAuto,s_symTab);
	} eq_initializer_opt
	| error												{
		/* $$.d = &s_errorSymbol; */
		$$.d = typeSpecifier2(&s_errorModifier);
	}
	;

declaration_specifiers
	: declaration_modality_specifiers
	| declaration_specifiers0
	;

user_defined_type
	: TYPE_NAME												{
		int usage;
		$$.d = $1.d;
		assert(s_opt.taskRegime);
		if (CX_REGIME()) {
			assert($1.d);
			assert($1.d->sd);
			if (WORK_NEST_LEVEL0()) usage = USAGE_TOP_LEVEL_USED;
			else usage = UsageUsed;
			addCxReference($1.d->sd,&$1.d->p,usage,s_noneFileIndex,s_noneFileIndex);
		}
	}
	;

declaration_specifiers0
	: user_defined_type										{ 
		assert($1.d);
		assert($1.d->sd);
		$$.d = typeSpecifier2($1.d->sd->u.type);
	}
	| type_specifier1										{
		$$.d  = typeSpecifier1($1.d);
	}
	| type_specifier2										{
		$$.d  = typeSpecifier2($1.d);
	}
	| declaration_modality_specifiers  user_defined_type	{ 
		assert($2.d);
		assert($2.d->sd);
		$$.d = $1.d;
		declTypeSpecifier2($1.d,$2.d->sd->u.type);
	}
	| declaration_modality_specifiers type_specifier1		{
		$$.d = $1.d;
		declTypeSpecifier1($1.d,$2.d);
	}
	| declaration_modality_specifiers type_specifier2		{
		$$.d = $1.d;
		declTypeSpecifier2($1.d,$2.d);
	}
	| declaration_specifiers0 type_modality_specifier		{
		$$.d = $1.d;
		declTypeSpecifier1($1.d,$2.d);
	}
	| declaration_specifiers0 type_specifier1				{
		$$.d = $1.d;
		declTypeSpecifier1($1.d,$2.d);
	}
	| declaration_specifiers0 type_specifier2				{
		$$.d = $1.d;
		declTypeSpecifier2($1.d,$2.d);
	}
	| declaration_specifiers0 storage_class_specifier		{
		$$.d = $1.d;
		$$.d->b.storage = $2.d; 
	}
	| COMPL_TYPE_NAME										{ 
		assert(0);
	}
	| declaration_modality_specifiers COMPL_TYPE_NAME		{ 
		assert(0); /* token never used */ 
	}
	;

declaration_modality_specifiers
	: storage_class_specifier								{
		$$.d  = typeSpecifier1(TypeDefault);
		$$.d->b.storage = $1.d; 
	}
	| declaration_modality_specifiers storage_class_specifier 		{
		$$.d = $1.d;
		$$.d->b.storage = $2.d; 
	}
	| type_modality_specifier								{
		$$.d  = typeSpecifier1($1.d);
	}
	| declaration_modality_specifiers type_modality_specifier		{
		declTypeSpecifier1($1.d, $2.d);
	}
	;

/*& // an experiment
declaration_specifier0:
		storage_class_specifier
	|	type_modality_specifier
	;

declaration_specifiers0:
		declaration_specifier0
	|	declaration_specifiers0 declaration_specifier0
	;

declaration_specifiers:
		user_defined_type
	|	declaration_specifiers0 user_defined_type
	|	declaration_specifiers0 type_specifier1
	|	declaration_specifiers0 type_specifier2
	|	declaration_specifiers declaration_specifier0
	|	declaration_specifiers type_specifier1
	|	declaration_specifiers type_specifier2
	;
&*/

/* a gcc extensions ? */
asm_opt:
	|	ASM_KEYWORD '(' string_literales ')'
	;

eq_initializer_opt:
	| '=' initializer
	;

init_declarator
	: declarator asm_opt /* eq_initializer_opt	 { $$.d = $1.d; } */
	;

/* the original 
	: declarator
	| declarator '=' initializer
	;
*/

storage_class_specifier
	: TYPEDEF	{ $$.d = StorageTypedef; }
	| EXTERN	{ $$.d = StorageExtern; }
	| STATIC	{ $$.d = StorageStatic; }
	| AUTO		{ $$.d = StorageAuto; }
	| REGISTER	{ $$.d = StorageAuto; }
/*
	| INLINE	{ $$.d = StorageStatic; }
*/
	;

type_modality_specifier
	: CONST			{ $$.d = TypeDefault; }
	| VOLATILE		{ $$.d = TypeDefault; }
	| ANONYME_MOD	{ $$.d = TypeDefault; }
	;

type_modality_specifier_opt:
	| type_modality_specifier
	;

type_specifier1
	: CHAR		{ $$.d = TypeChar; }
	| SHORT		{ $$.d = TmodShort; }
	| INT		{ $$.d = TypeInt; }
	| LONG		{ $$.d = TmodLong; }
	| SIGNED	{ $$.d = TmodSigned; }
	| UNSIGNED	{ $$.d = TmodUnsigned; }
	| FLOAT		{ $$.d = TypeFloat; }
	| DOUBLE	{ $$.d = TypeDouble; }
	| VOID		{ $$.d = TypeVoid; }
	;

type_specifier2
	: struct_or_union_specifier		/* { $$.d = $1.d; } */
	| enum_specifier				/* { $$.d = $1.d; } */
	;

struct_or_union_specifier
	: struct_or_union struct_identifier								{
		int usage;
		if (WORK_NEST_LEVEL0()) usage = USAGE_TOP_LEVEL_USED;
		else usage = UsageUsed;
		$$.d = simpleStrUnionSpecifier($1.d, $2.d, usage);
	}
	| struct_or_union_define_specifier '{' struct_declaration_list '}'{
		assert($1.d && $1.d->u.t);
		$$.d = $1.d;
		specializeStrUnionDef($$.d->u.t, $3.d);
	}
	| struct_or_union_define_specifier '{' '}' 						{
		$$.d = $1.d;
	}
	;

struct_or_union_define_specifier
	: struct_or_union struct_identifier								{
		$$.d = simpleStrUnionSpecifier($1.d, $2.d, UsageDefined);
	}
	| struct_or_union 												{
		$$.d = crNewAnnonymeStrUnion($1.d);
	}
	;

struct_identifier
	: identifier			/* { $$.d = $1.d; } */ 
	| COMPL_STRUCT_NAME		{ assert(0); /* token never used */ }
	;

struct_or_union
	: STRUCT		{ $$.d = $1.d; }
	| UNION			{ $$.d = $1.d; }
	;

struct_declaration_list
	: struct_declaration								/* { $$.d = $1.d; } */
	| struct_declaration_list struct_declaration		{
		if ($1.d == &s_errorSymbol || $1.d->b.symType==TypeError) {
			$$.d = $2.d;
		} else if ($2.d == &s_errorSymbol || $1.d->b.symType==TypeError)  {
			$$.d = $1.d;
		} else {
			$$.d = $1.d;
			LIST_APPEND(S_symbol, $$.d, $2.d);
		}
	}
	;

struct_declaration
	: Sv_tmp type_specifier_list struct_declarator_list ';' 	{
		S_symbol *p;
		assert($2.d && $3.d);
		for(p=$3.d; p!=NULL; p=p->next) {
			completeDeclarator($2.d, p);
		}
		$$.d = $3.d;
		tmpWorkMemoryi = $1.d;
	}
	| error												{
		/* $$.d = &s_errorSymbol; */
		XX_ALLOC($$.d, S_symbol);
		*$$.d = s_errorSymbol;
	}
	;

struct_declarator_list
	: struct_declarator									{
		$$.d = $1.d;
		assert($$.d->next == NULL);
	}
	| struct_declarator_list ',' struct_declarator		{
		$$.d = $1.d;
		assert($3.d->next == NULL);
		LIST_APPEND(S_symbol, $$.d, $3.d);
	}
	;

struct_declarator:					{ /* gcc extension allow empty field */
		$$.d = crEmptyField();	
	}
	| ':' constant_expr				{
		$$.d = crEmptyField();
	}
	| declarator					/* { $$.d = $1.d; } */
	| declarator ':' constant_expr	/* { $$.d = $1.d; } */
	;

enum_specifier
	: ENUM enum_identifier									{
		int usage;
		if (WORK_NEST_LEVEL0()) usage = USAGE_TOP_LEVEL_USED;
		else usage = UsageUsed;
		$$.d = simpleEnumSpecifier($2.d, usage);
	}
	| enum_define_specifier '{' enumerator_list_comma '}'		{
		assert($1.d && $1.d->m == TypeEnum && $1.d->u.t);
		$$.d = $1.d;
		if ($$.d->u.t->u.enums==NULL) {
			$$.d->u.t->u.enums = $3.d;
			addToTrail(setToNull, & ($$.d->u.t->u.enums) );
		}
	}
	| ENUM '{' enumerator_list_comma '}'						{
		$$.d = crNewAnnonymeEnum($3.d);
	}
	;

enum_define_specifier
	: ENUM enum_identifier									{
		$$.d = simpleEnumSpecifier($2.d, UsageDefined);
	}
	;

enum_identifier
	: identifier			/* { $$.d = $1.d; } */
	| COMPL_ENUM_NAME		{ assert(0); /* token never used */ }
	;

enumerator_list_comma
	: enumerator_list 				/* { $$.d = $1.d; } */
	| enumerator_list ',' 			/* { $$.d = $1.d; } */
	;

enumerator_list
	: enumerator							{
		$$.d = crDefinitionList($1.d);
	}
	| enumerator_list ',' enumerator		{
		$$.d = $1.d;
		LIST_APPEND(S_symbolList, $$.d, crDefinitionList($3.d));
	}
	;

enumerator
	: identifier							{
		$$.d = crSimpleDefinition(StorageConstant,TypeInt,$1.d);
		addNewSymbolDef($$.d,StorageConstant, s_symTab, UsageDefined);
	}
	| identifier '=' constant_expr			{
		$$.d = crSimpleDefinition(StorageConstant,TypeInt,$1.d);
		addNewSymbolDef($$.d,StorageConstant, s_symTab, UsageDefined);
	}
	| error									{
		/* $$.d = &s_errorSymbol; */
		XX_ALLOC($$.d, S_symbol);
		*$$.d = s_errorSymbol;
	}
	| COMPL_OTHER_NAME		{ assert(0); /* token never used */ }
	;

declarator
	: declarator2										/* { $$.d = $1.d; } */
	| pointer declarator2								{
		$$.d = $2.d;
		assert($$.d->b.npointers == 0);
		$$.d->b.npointers = $1.d;
	}
	;

declarator2
	: identifier										{ 
		$$.d = StackMemAlloc(S_symbol);
		FILL_symbolBits(&$$.d->b,0,0,0,0,0,TypeDefault,StorageDefault,0);
		FILL_symbol($$.d,$1.d->name,$1.d->name,$1.d->p,$$.d->b,type,NULL,NULL);
		$$.d->u.type = NULL;
	}
	| '(' declarator ')'								{ 
		$$.d = $2.d;
		unpackPointers($$.d);
	}
	| declarator2 '[' ']'								{ 
		assert($1.d);
		$$.d = $1.d;
		AddComposedType($$.d, TypeArray);
	}
	| declarator2 '[' constant_expr ']'					{ 
		assert($1.d);
		$$.d = $1.d; 
		AddComposedType($$.d, TypeArray);
	}
	| declarator2 '(' ')'								{ 
		S_typeModifiers *p;
		assert($1.d);
		$$.d = $1.d; 
		p = AddComposedType($$.d, TypeFunction);
		FILL_funTypeModif(&p->u.f , NULL, NULL);
		handleDeclaratorParamPositions($1.d, &$2.d, NULL, &$3.d, 0);
	}
	| declarator2 '(' parameter_type_list ')'			{ 
		S_typeModifiers *p;
		assert($1.d);
		$$.d = $1.d; 
		p = AddComposedType($$.d, TypeFunction);
		FILL_funTypeModif(&p->u.f , $3.d.s, NULL);
		handleDeclaratorParamPositions($1.d, &$2.d, $3.d.p, &$4.d, 1);
	}
	| declarator2 '(' parameter_identifier_list ')'		{ 
		S_typeModifiers *p;
		assert($1.d);
		$$.d = $1.d; 
		p = AddComposedType($$.d, TypeFunction);
		FILL_funTypeModif(&p->u.f , $3.d.s, NULL);
		handleDeclaratorParamPositions($1.d, &$2.d, $3.d.p, &$4.d, 1);
	}
	| COMPL_OTHER_NAME		{ assert(0); /* token never used */ }
	;

pointer
	: '*'									{
		$$.d = 1;
	}
	| '*' type_mod_specifier_list				{
		$$.d = 1;
	}
	| '*' pointer							{
		$$.d = $2.d+1;
	}
	| '*' type_mod_specifier_list pointer		{
		$$.d = $3.d+1;
	}
	;

type_mod_specifier_list
	: type_modality_specifier									{
		$$.d  = typeSpecifier1($1.d);
	}
	| type_mod_specifier_list type_modality_specifier			{
		declTypeSpecifier1($1.d, $2.d);
	}
	;

/*
type_specifier_list
	: type_specifier1									{
		$$.d  = typeSpecifier1($1.d);
	}
	| type_specifier2									{
		$$.d  = typeSpecifier2($1.d);
	}
	| type_specifier_list type_specifier1				{
		declTypeSpecifier1($1.d, $2.d);
	}
	| type_specifier_list type_specifier2				{
		declTypeSpecifier2($1.d, $2.d);
	}
	;
*/

type_specifier_list
	: type_mod_specifier_list						/* { $$.d = $1.d; } */
	| type_specifier_list0							/* { $$.d = $1.d; } */
	;

type_specifier_list0
	: user_defined_type										{ 
		assert($1.d);
		assert($1.d->sd);
		assert($1.d->sd);
		$$.d = typeSpecifier2($1.d->sd->u.type);
	}
	| type_specifier1										{
		$$.d  = typeSpecifier1($1.d);
	}
	| type_specifier2										{
		$$.d  = typeSpecifier2($1.d);
	}
	| type_mod_specifier_list user_defined_type				{ 
		assert($2.d);
		assert($2.d->sd);
		assert($2.d->sd);
		$$.d = $1.d;
		declTypeSpecifier2($1.d,$2.d->sd->u.type);
	}
	| type_mod_specifier_list type_specifier1		{
		$$.d = $1.d;
		declTypeSpecifier1($1.d,$2.d);
	}
	| type_mod_specifier_list type_specifier2		{
		$$.d = $1.d;
		declTypeSpecifier2($1.d,$2.d);
	}
	| type_specifier_list0 type_modality_specifier		{
		$$.d = $1.d;
		declTypeSpecifier1($1.d,$2.d);
	}
	| type_specifier_list0 type_specifier1				{
		$$.d = $1.d;
		declTypeSpecifier1($1.d,$2.d);
	}
	| type_specifier_list0 type_specifier2				{
		$$.d = $1.d;
		declTypeSpecifier2($1.d,$2.d);
	}
	| COMPL_TYPE_NAME										{ 
		assert(0);
	}
	| type_mod_specifier_list COMPL_TYPE_NAME		{ 
		assert(0); /* token never used */ 
	}
	;

parameter_identifier_list
	: identifier_list							/* { $$.d = $1.d; } */
	| identifier_list ',' ELIPSIS				{
		S_symbol *p;
		S_position pp;
		p = StackMemAlloc(S_symbol);
		FILL_position(&pp, -1, 0, 0);
		FILL_symbolBits(&p->b,0,0,0,0,0,TypeElipsis,StorageDefault,0);
		FILL_symbol(p,"","",pp,p->b,type,NULL,NULL);
		$$.d = $1.d;
		LIST_APPEND(S_symbol, $$.d.s, p);				
		appendPositionToList(&$$.d.p, &$2.d);
	}
	;

identifier_list
	: IDENTIFIER								{
		S_symbol *p;
		p = StackMemAlloc(S_symbol);
		FILL_symbolBits(&p->b,0,0,0,0,0,TypeDefault,StorageDefault,0);
		FILL_symbol(p,$1.d->name,$1.d->name,$1.d->p,p->b,type,NULL,NULL);
		p->u.type = NULL;
		$$.d.s = p;
		$$.d.p = NULL;
	}
	| identifier_list ',' identifier			{
		S_symbol 		*p;
		p = StackMemAlloc(S_symbol);
		FILL_symbolBits(&p->b,0,0,0,0,0,TypeDefault,StorageDefault,0);
		FILL_symbol(p,$3.d->name,$3.d->name,$3.d->p,p->b,type,NULL,NULL);
		p->u.type = NULL;
		$$.d = $1.d;
		LIST_APPEND(S_symbol, $$.d.s, p);
		appendPositionToList(&$$.d.p, &$2.d);
	}
	| COMPL_OTHER_NAME		{ assert(0); /* token never used */ }
	;

parameter_type_list
	: parameter_list					/* { $$.d = $1.d; } */
	| parameter_list ',' ELIPSIS				{
		S_symbol 		*p;
		S_position 		pp;
		p = StackMemAlloc(S_symbol);
		FILL_position(&pp, -1, 0, 0);
		FILL_symbolBits(&p->b,0,0,0,0,0,TypeElipsis,StorageDefault,0);
		FILL_symbol(p,"","",pp,p->b,type,NULL,NULL);
		$$.d = $1.d;
		LIST_APPEND(S_symbol, $$.d.s, p);
		appendPositionToList(&$$.d.p, &$2.d);
	}
	;

parameter_list
	: parameter_declaration							{
		$$.d.s = $1.d;
		$$.d.p = NULL;
	}
	| parameter_list ',' parameter_declaration		{
		$$.d = $1.d;
		LIST_APPEND(S_symbol, $1.d.s, $3.d);
		appendPositionToList(&$$.d.p, &$2.d);
	}
	;


parameter_declaration
	: declaration_specifiers declarator			{ 
		completeDeclarator($1.d, $2.d);
		$$.d = $2.d;
	}
	| type_name									{
		$$.d = StackMemAlloc(S_symbol);
		FILL_symbolBits(&$$.d->b,0,0,0,0,0,TypeDefault, StorageDefault,0);
		FILL_symbol($$.d, NULL, NULL, s_noPos,$$.d->b,type,$1.d,NULL);
		$$.d->u.type = $1.d;
	}
	| error										{
		/*
		   	this was commented out, because of excess of tmpWorkMemory 
			but I am putting it in, because in many cases, this helps
			to index a function with wrong typedefed parameters, like:
			void toto(Mistype arg) {}
			In case of problems rather increase the tmpWorkMemory !!!
		*/
		XX_ALLOC($$.d, S_symbol);
		*$$.d = s_errorSymbol;
	}
	;

type_name
	: declaration_specifiers						{
		$$.d = $1.d->u.type;
	}
	| declaration_specifiers abstract_declarator	{
		$$.d = $2.d;
		LIST_APPEND(S_typeModifiers, $$.d, $1.d->u.type);
	}
	;

abstract_declarator
	: pointer								{
		int i;
		CrTypeModifier($$.d,TypePointer);
		for(i=1; i<$1.d; i++) appendComposedType(&($$.d), TypePointer);
	}
	| abstract_declarator2					{ 
		$$.d = $1.d;
	}
	| pointer abstract_declarator2			{
		int i;
		$$.d = $2.d;
		for(i=0; i<$1.d; i++) appendComposedType(&($$.d), TypePointer);
	}
	;

abstract_declarator2
	: '(' abstract_declarator ')'			{
		$$.d = $2.d; 
	}
	| '[' ']'								{
		CrTypeModifier($$.d,TypeArray);		
	}
	| '[' constant_expr ']'					{
		CrTypeModifier($$.d,TypeArray);		
	}
	| abstract_declarator2 '[' ']'			{
		$$.d = $1.d;
		appendComposedType(&($$.d), TypeArray);
	}
	| abstract_declarator2 '[' constant_expr ']'	{
		$$.d = $1.d;
		appendComposedType(&($$.d), TypeArray);
	}
	| '(' ')'										{
		CrTypeModifier($$.d,TypeFunction);
		FILL_funTypeModif(&$$.d->u.f , NULL, NULL);
	}
	| '(' parameter_type_list ')'					{
		CrTypeModifier($$.d,TypeFunction);
		FILL_funTypeModif(&$$.d->u.f , $2.d.s, NULL);
	}
	| abstract_declarator2 '(' ')'					{
		S_typeModifiers *p;
		$$.d = $1.d;
		p = appendComposedType(&($$.d), TypeFunction);
		FILL_funTypeModif(&p->u.f , NULL, NULL);
	}
	| abstract_declarator2 '(' parameter_type_list ')'			{
		S_typeModifiers *p;
		$$.d = $1.d;
		p = appendComposedType(&($$.d), TypeFunction);
		// I think there should be the following, but in abstract 
		// declarator it does not matter
		//& FILL_funTypeModif(&p->u.f , $3.d.s, NULL);
		FILL_funTypeModif(&p->u.f , NULL, NULL);
	}
	;

initializer
	: assignment_expr
      /* it is enclosed because on linux kernel it overflows memory */
	| '{' initializer_list '}'
	| '{' initializer_list ',' '}'
	| error
	;

initializer_list
	: Sv_tmp Start_block initializer Stop_block	{
		tmpWorkMemoryi = $1.d;
	}
	| initializer_list ',' Sv_tmp Start_block initializer Stop_block	{
		tmpWorkMemoryi = $3.d;
	}
	;

statement
	: Sv_tmp labeled_statement		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp compound_statement		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp expression_statement		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp selection_statement		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp iteration_statement		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp jump_statement		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp asm_statement		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp error	{
		tmpWorkMemoryi = $1.d;
	}
	;

label:
		label_def_name ':'
	|	CASE constant_expr ':' {
			GenSwitchCaseFork(0);
	}
	|	DEFAULT ':' {
			GenSwitchCaseFork(0);
	}
	;

labeled_statement
	: label statement 
	;

label_def_name
	: identifier 			{
		labelReference($1.d,UsageDefined);
	}
	| COMPL_LABEL_NAME		{ assert(0); /* token never used */ }
	;

label_name
	: identifier 			{
		labelReference($1.d,UsageUsed);
	}
	| COMPL_LABEL_NAME		{ assert(0); /* token never used */ }
	;

compound_statement
	: '{' '}'
	| '{' Start_block label_decls_opt statement_list Stop_block '}'
/*&
	| '{' Start_block label_decls_opt declaration_list Stop_block '}'
	| '{' Start_block label_decls_opt declaration_list statement_list Stop_block '}'
&*/
	;

label_decls_opt:
	|	label_decls
	;

label_decls:
		LABEL identifier {
		labelReference($2.d,UsageDeclared);
	}
	|	label_decls LABEL identifier {
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
	:					{ $$.d.t = NULL; $$.d.r = NULL; }
	| expr				{ $$.d = $1.d; }
	;

expression_statement
	: maybe_expr ';'
	;


_ncounter_:	 {EXTRACT_COUNTER_SEMACT($$.d);}
	;

_nlabel_:	{EXTRACT_LABEL_SEMACT($$.d);}
	;

_ngoto_:	{EXTRACT_GOTO_SEMACT($$.d);}
	;

_nfork_:	{EXTRACT_FORK_SEMACT($$.d);}
	;

selection_statement
	: IF '(' expr ')' _nfork_ statement						{
		GenInternalLabelReference($5.d, UsageDefined);
	}
	| IF '(' expr ')' _nfork_ statement ELSE _ngoto_ {
		GenInternalLabelReference($5.d, UsageDefined);
	}	statement								{
		GenInternalLabelReference($8.d, UsageDefined);
	}
	| SWITCH '(' expr ')' /*5*/ _ncounter_ 	{/*6*/
		AddContinueBreakLabelSymbol(1000*$5.d, SWITCH_LABEL_NAME, $<symbol>$);
	} {/*7*/
		AddContinueBreakLabelSymbol($5.d, BREAK_LABEL_NAME, $<symbol>$);
		GenInternalLabelReference($5.d, UsageFork);
	} statement					{
		GenSwitchCaseFork(1);
		ExtrDeleteContBreakSym($<symbol>7);
		ExtrDeleteContBreakSym($<symbol>6);
		GenInternalLabelReference($5.d, UsageDefined);
	}
	;

for1maybe_expr:
		maybe_expr			{s_forCompletionType=$1.d;}
	;

iteration_statement
	: WHILE _nlabel_ '(' expr ')' /*6*/ _nfork_ 
	{/*7*/
		AddContinueBreakLabelSymbol($2.d, CONTINUE_LABEL_NAME, $<symbol>$);
	} {/*8*/
		AddContinueBreakLabelSymbol($6.d, BREAK_LABEL_NAME, $<symbol>$);
	} statement					{
		ExtrDeleteContBreakSym($<symbol>8);
		ExtrDeleteContBreakSym($<symbol>7);
		GenInternalLabelReference($2.d, UsageUsed);
		GenInternalLabelReference($6.d, UsageDefined);
	}

	| DO _nlabel_ _ncounter_ _ncounter_ { /*5*/
		AddContinueBreakLabelSymbol($3.d, CONTINUE_LABEL_NAME, $<symbol>$);
	} {/*6*/
		AddContinueBreakLabelSymbol($4.d, BREAK_LABEL_NAME, $<symbol>$);
	} statement WHILE {
		ExtrDeleteContBreakSym($<symbol>6);
		ExtrDeleteContBreakSym($<symbol>5);
		GenInternalLabelReference($3.d, UsageDefined);
	} '(' expr ')' ';'			{
		GenInternalLabelReference($2.d, UsageFork);
		GenInternalLabelReference($4.d, UsageDefined);
	}

	| FOR '(' for1maybe_expr ';' 
			/*5*/ _nlabel_  maybe_expr ';'  /*8*/_ngoto_
			/*9*/ _nlabel_  maybe_expr ')' /*12*/ _nfork_
		{
		/*13*/
		GenInternalLabelReference($5.d, UsageUsed);
		GenInternalLabelReference($8.d, UsageDefined);
		AddContinueBreakLabelSymbol($9.d, CONTINUE_LABEL_NAME, $<symbol>$);
		}
		{/*14*/
		AddContinueBreakLabelSymbol($12.d, BREAK_LABEL_NAME, $<symbol>$);		
		} 
			statement
		{ 
		ExtrDeleteContBreakSym($<symbol>14);
		ExtrDeleteContBreakSym($<symbol>13);
		GenInternalLabelReference($9.d, UsageUsed);
		GenInternalLabelReference($12.d, UsageDefined);
		}		
	| FOR '(' for1maybe_expr ';' COMPL_FOR_SPECIAL1
	| FOR '(' for1maybe_expr ';' _nlabel_  maybe_expr ';' COMPL_FOR_SPECIAL2
	;

jump_statement
	: GOTO label_name ';' 
	| CONTINUE ';'			{
		GenContBreakReference(CONTINUE_LABEL_NAME);
	}
	| BREAK ';'				{
		GenContBreakReference(BREAK_LABEL_NAME);
	}
	| RETURN ';'			{
		GenInternalLabelReference(-1, UsageUsed);
	}
	| RETURN expr ';'		{
		GenInternalLabelReference(-1, UsageUsed);
	}
	;

_bef_:			{
		actionsBeforeAfterExternalDefinition();
	}
	;

/* ****************** following is some gcc asm support ************ */
/* it is not exactly as in gcc, but I hope it is suf. general */

gcc_asm_item_opt:
	|	IDENTIFIER
	|	string_literales
	|	IDENTIFIER '(' expr ')'
	|	string_literales '(' expr ')'
	;

gcc_asm_item_list:
		gcc_asm_item_opt
	|	gcc_asm_item_list ',' gcc_asm_item_opt
	;

gcc_asm_oper:
		':' gcc_asm_item_list
	|	gcc_asm_oper ':' gcc_asm_item_list
	;

asm_statement:
		ASM_KEYWORD type_modality_specifier_opt '(' expr ')' ';'
	|	ASM_KEYWORD type_modality_specifier_opt '(' expr gcc_asm_oper ')' ';'
	;

/* *********************************************************************** */

file
	: _bef_
	| _bef_ cached_external_definition_list _bef_
	;

cached_external_definition_list
	: external_definition				{
		if (inStacki == 0) {
			poseCachePoint(1);
		}
	}
	| cached_external_definition_list _bef_ external_definition {
		if (inStacki == 0) {
			poseCachePoint(1);
		}
	}
	| error
	;

external_definition
	: Sv_tmp declaration_specifiers ';'		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp top_init_declarations ';'		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp function_definition_head {
		S_symbol *p,*pa,*pp;
		int i;
		assert($2.d);
		// I think that due to the following line sometimes 
		// storage was not extern, see 'addNewSymbolDef'
		//& if ($2.d->b.storage == StorageDefault) $2.d->b.storage = StorageExtern;
		// TODO!!!, here you should check if there is previous declaration of
		// the function, if yes and is declared static, make it static!
		addNewSymbolDef($2.d, StorageExtern, s_symTab, UsageDefined);
		tmpWorkMemoryi = $1.d;
		stackMemoryBlockStart();
		s_count.localVar = 0;
		assert($2.d->u.type && $2.d->u.type->m == TypeFunction);
		s_cp.function = $2.d;
		GenInternalLabelReference(-1, UsageDefined);
		for(p=$2.d->u.type->u.f.args,i=1; p!=NULL; p=p->next,i++) {
			if (p->b.symType == TypeElipsis) continue;
			if (p->u.type == NULL) p->u.type = &s_defaultIntModifier;
			addFunctionParameterToSymTable($2.d, p, i, s_symTab);
		}
	} compound_statement {
		stackMemoryBlockFree();
		s_cp.function = NULL;
		LICENSE_CHECK();
		if (s_opt.taskRegime == RegimeHtmlGenerate) {
			htmlAddFunctionSeparatorReference();
		}
	}
	| Sv_tmp EXTERN STRING_LITERAL 	external_definition	{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp EXTERN STRING_LITERAL 	'{' cached_external_definition_list	'}' {
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp ASM_KEYWORD '(' expr ')' ';' 		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp error compound_statement 		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp error 		{
		tmpWorkMemoryi = $1.d;
	}
	| Sv_tmp ';'	 	{  /* empty external definition */
		tmpWorkMemoryi = $1.d;
	}
	;

top_init_declarations
	: declaration_specifiers init_declarator eq_initializer_opt			{		
		$$.d = $1.d;
		addNewDeclaration($1.d, $2.d, StorageExtern,s_symTab);
	}
	| init_declarator eq_initializer_opt									{
		$$.d = & s_defaultIntDefinition;
		addNewDeclaration($$.d, $1.d, StorageExtern,s_symTab);
	}
	| top_init_declarations ',' init_declarator eq_initializer_opt			{
		$$.d = $1.d;
		addNewDeclaration($1.d, $3.d, StorageExtern,s_symTab);
	}
	| error												{
		/* $$.d = &s_errorSymbol; */
		$$.d = typeSpecifier2(&s_errorModifier);
	}
	;

function_definition_head
	: function_head_declaration							/* { $$.d = $1.d; } */
	| function_definition_head fun_arg_declaration		{
		int r;
		assert($1.d->u.type && $1.d->u.type->m == TypeFunction);
		r = mergeArguments($1.d->u.type->u.f.args, $2.d);
		if (r == RESULT_ERR) YYERROR;
		$$.d = $1.d;
	}
	;

fun_arg_declaration
	: declaration_specifiers ';'								{
		$$.d = NULL;
	}
	| declaration_specifiers fun_arg_init_declarations ';'		{
		S_symbol *p;
		assert($1.d && $2.d);
		for(p=$2.d; p!=NULL; p=p->next) {
			completeDeclarator($1.d, p);
		}
		$$.d = $2.d;
	}
	;

fun_arg_init_declarations
	: init_declarator eq_initializer_opt			{		
		$$.d = $1.d;
	}
	| fun_arg_init_declarations ',' init_declarator eq_initializer_opt				{
		$$.d = $1.d;
		LIST_APPEND(S_symbol, $$.d, $3.d);
	}
	| fun_arg_init_declarations ',' error						{
		$$.d = $1.d;
	}
	;

function_head_declaration
	: declarator							{
		completeDeclarator(&s_defaultIntDefinition, $1.d);
		assert($1.d && $1.d->u.type);
		if ($1.d->u.type->m != TypeFunction) YYERROR;
		$$.d = $1.d;
	}
	| declaration_specifiers declarator		{
		completeDeclarator($1.d, $2.d);
		assert($2.d && $2.d->u.type);
		if ($2.d->u.type->m != TypeFunction) YYERROR;
		$$.d = $2.d;
	}
	;


Start_block:	{ stackMemoryBlockStart(); }
	;

Stop_block:		{ stackMemoryBlockFree(); }
	;

identifier
	: IDENTIFIER 	/* { $$.d = $1.d; } */
	| TYPE_NAME		/* { $$.d = $1.d; } */
	;

%%


static S_completionFunTab spCompletionsTab[]  = {
	{COMPL_FOR_SPECIAL1,	completeForSpecial1},
	{COMPL_FOR_SPECIAL2,	completeForSpecial2},
	{COMPL_UP_FUN_PROFILE,	completeUpFunProfile},
	{0,NULL}
};

static S_completionFunTab completionsTab[]  = {
	{COMPL_TYPE_NAME,		completeTypes},
	{COMPL_STRUCT_NAME,		completeStructs},
	{COMPL_STRUCT_REC_NAME,	completeRecNames},
	{COMPL_ENUM_NAME,		completeEnums},
	{COMPL_LABEL_NAME,		completeLabels},
	{COMPL_OTHER_NAME,		completeOthers},
	{0,NULL}
};


void makeCCompletions(char *s, int len, S_position *pos) {
	int tok, yyn, i;
	S_cline compLine;
/*fprintf(stderr,"completing \"%s\"\n",s);*/
	LICENSE_CHECK();
	strncpy(s_completions.idToProcess, s, MAX_FUN_NAME_SIZE);
	s_completions.idToProcess[MAX_FUN_NAME_SIZE-1] = 0;
	FILL_completions(&s_completions, len, *pos, 0, 0, 0, 0, 0, 0);
	/* special wizard completions */
	for (i=0;(tok=spCompletionsTab[i].token)!=0; i++) {
	    if (((yyn = yysindex[lastyystate]) && (yyn += tok) >= 0 &&
				yyn <= YYTABLESIZE && yycheck[yyn] == tok) ||
			((yyn = yyrindex[lastyystate]) && (yyn += tok) >= 0 &&
		        yyn <= YYTABLESIZE && yycheck[yyn] == tok)) {
/*fprintf(stderr,"completing %d==%s v stave %d\n",i,yyname[tok],lastyystate);*/
				(*spCompletionsTab[i].fun)(&s_completions);
				if (s_completions.abortFurtherCompletions) return;
		}
	}
	/* If there is a wizard completion, RETURN now */
	if (s_completions.ai != 0 && s_opt.cxrefs != OLO_SEARCH) return;
	/* basic language tokens */
	for (i=0;(tok=completionsTab[i].token)!=0; i++) {
	    if (((yyn = yysindex[lastyystate]) && (yyn += tok) >= 0 &&
				yyn <= YYTABLESIZE && yycheck[yyn] == tok) ||
			((yyn = yyrindex[lastyystate]) && (yyn += tok) >= 0 &&
		        yyn <= YYTABLESIZE && yycheck[yyn] == tok)) {
/*&fprintf(stderr,"completing %d==%s v stave %d\n",i,yyname[tok],lastyystate);&*/
				(*completionsTab[i].fun)(&s_completions);
				if (s_completions.abortFurtherCompletions) return;
		}
	}
	/* basic language tokens */
	for (tok=0; tok<LAST_TOKEN; tok++) {
		if (tok==IDENTIFIER) continue;
	    if (((yyn = yysindex[lastyystate]) && (yyn += tok) >= 0 &&
				yyn <= YYTABLESIZE && yycheck[yyn] == tok) ||
			((yyn = yyrindex[lastyystate]) && (yyn += tok) >= 0 &&
		        yyn <= YYTABLESIZE && yycheck[yyn] == tok)) {
				if (s_tokenName[tok]!= NULL) {
					if (isalpha(*s_tokenName[tok]) || *s_tokenName[tok]=='_') {
						FILL_cline(&compLine, s_tokenName[tok], NULL, TypeKeyword,0, 0, NULL,NULL);
					} else {
						FILL_cline(&compLine, s_tokenName[tok], NULL, TypeToken,0, 0, NULL,NULL);
					}
/*&fprintf(stderr,"completing %d==%s(%s) state %d\n",tok,yyname[tok],s_tokenName[tok],lastyystate);&*/
					processName(s_tokenName[tok], &compLine, 0, &s_completions);
				}
		}
	}

	LICENSE_CHECK();
}



